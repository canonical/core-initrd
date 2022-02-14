#!/bin/bash

set -e
set -x 

apt update -yqq

# needed for dracut which is a build-dep of ubuntu-core-initramfs
# and for the version of snapd here which we want to use to pull snap-bootstrap
# from when we build the debian package
add-apt-repository ppa:snappy-dev/image -y
apt update
apt upgrade -yqq

# these are already installed in the lxd image which speeds things up, but they
# are missing in qemu and google images.
apt install initramfs-tools-core psmisc fdisk snapd mtools ovmf qemu-system-x86 sshpass whois openssh-server -yqq

# use the snapd snap explicitly
# TODO: since ubuntu-image ships it's own version of `snap prepare-image`, 
# should we instead install beta/edge snapd here and point ubuntu-image to this
# version of snapd?
# TODO: https://bugs.launchpad.net/snapd/+bug/1712808
# There is a bug in snapd that prevents udev rules from reloading in privileged containers
# with the following error message: 'cannot reload udev rules: exit status 1' when installing
# snaps. However it seems that retrying the installation fixes it
if ! snap install snapd; then
    echo "FIXME: snapd install failed, retrying"
    snap install snapd
fi
snap install ubuntu-image --classic

# install build-deps for ubuntu-core-initramfs
(
    cd "$SETUPDIR"
    apt -y build-dep ./
)

# TODO: is this still necessary for core-initrd? (this was copied from snapd)
# create test user for spread to use
groupadd --gid 12345 test
adduser --uid 12345 --gid 12345 --disabled-password --gecos '' test

if getent group systemd-journal >/dev/null; then
    usermod -G systemd-journal -a test
    id test | MATCH systemd-journal
fi

echo 'test ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# get the model
# TODO: should this be a variant to test multiple grades i.e. signed and secured?
curl -o ubuntu-core-22-amd64-dangerous.model https://raw.githubusercontent.com/snapcore/models/master/ubuntu-core-22-amd64-dangerous.model

# build the debian package here of ubuntu-core-initramfs
(
    cd "$SETUPDIR"
    DEB_BUILD_OPTIONS='nocheck testkeys' dpkg-buildpackage -tc -b -Zgzip

    # save our debs somewhere safe
    cp ../*.deb "$SETUPDIR"
)

# install ubuntu-core-initramfs here so the repack-kernel uses the version of
# ubuntu-core-initramfs we built here
apt install -yqq "$SETUPDIR"/ubuntu-core-initramfs*.deb

# now download snap dependencies 
# TODO:UC22: when should some of these things start tracking stable ?
snap download pc-kernel --channel=22/$SNAP_BRANCH --basename=upstream-pc-kernel
snap download pc --channel=22/$SNAP_BRANCH --basename=upstream-pc-gadget
snap download core22 --channel=$SNAP_BRANCH --basename=upstream-core22

# note that we currently use snap-bootstrap from the snapd deb package, but we 
# could instead copy a potentially more up-to-date version out of the snapd snap
# but we don't currently do that as it doesn't represent what 
# ubuntu-core-initramfs would actually used if that package was built from LP
snap download snapd --channel=$SNAP_BRANCH --basename=upstream-snapd

# next repack / modify the snaps we use in the image, we do this for a few 
# reasons:
# 1. we need an automated way to get a user on the system to ssh into regardless
#    of grade, so we add a systemd service to the snapd snap which will create 
#    our user we can ssh into the VM with (if we only wanted to test dangerous,
#    we could use cloud-init).
# 2. we need to modify the initramfs in the kernel snap to use our initramfs, 
#    which requires re-signing of the kernel.efi
# 3. since we re-sign the kernel.efi of the kernel snap, for successful secure
#    boot, we need to also re-sign the gadget shim too

# first re-pack snapd snap with special systemd service which runs during run 
# mode to create a user for us to inspect the system state
# we also extract the snap-bootstrap binary for insertion into the kernel snap's
# initramfs too

snapddir=/tmp/snapd-workdir
unsquashfs -d $snapddir upstream-snapd.snap

# inject systemd service to setup users and other tweaks for us
# these are copied from upstream snapd prepare.sh, slightly modified to not 
# extract snapd spread data from ubuntu-seed as we don't need all that here
cat > "$snapddir/lib/systemd/system/snapd.spread-tests-run-mode-tweaks.service" <<'EOF'
[Unit]
Description=Tweaks to run mode for spread tests
Before=snapd.service
Documentation=man:snap(1)

[Service]
Type=oneshot
ExecStart=/usr/lib/snapd/snapd.spread-tests-run-mode-tweaks.sh
RemainAfterExit=true

[Install]
WantedBy=multi-user.target
EOF

cat > "$snapddir/usr/lib/snapd/snapd.spread-tests-run-mode-tweaks.sh" <<'EOF'
#!/bin/sh
set -e
# Print to kmsg and console
# $1: string to print
print_system()
{
    printf "%s spread-tests-run-mode-tweaks.sh: %s\n" "$(date -Iseconds --utc)" "$1" |
        tee -a /dev/kmsg /dev/console /run/mnt/ubuntu-seed/spread-tests-run-mode-tweaks-log.txt || true
}

# ensure we don't enable ssh in install mode or spread will get confused
if ! grep 'snapd_recovery_mode=run' /proc/cmdline; then
    print_system "not in run mode - script not running"
    exit 0
fi
if [ -e /root/spread-setup-done ]; then
    print_system "already ran, not running again"
    exit 0
fi

print_system "in run mode, not run yet, extracting overlay data"

# extract data from previous stage
(cd / && tar xf /run/mnt/ubuntu-seed/run-mode-overlay-data.tar.gz)
cp -r /root/test-var/lib/extrausers /var/lib

# user db - it's complicated
for f in group gshadow passwd shadow; do
    # now bind mount read-only those passwd files on boot
    cat >/etc/systemd/system/etc-"$f".mount <<EOF2
[Unit]
Description=Mount root/test-etc/$f over system etc/$f
Before=ssh.service

[Mount]
What=/root/test-etc/$f
Where=/etc/$f
Type=none
Options=bind,ro

[Install]
WantedBy=multi-user.target
EOF2
    systemctl enable etc-"$f".mount
    systemctl start etc-"$f".mount
done

mkdir -p /home/test
chown 12345:12345 /home/test
mkdir -p /home/ubuntu
chown 1000:1000 /home/ubuntu
mkdir -p /etc/sudoers.d/
echo 'test ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers.d/99-test-user
echo 'ubuntu ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers.d/99-ubuntu-user
# TODO: do we need this for our nested VM? We don't login as root to the nested
#       VM...
sed -i 's/\#\?\(PermitRootLogin\|PasswordAuthentication\)\>.*/\1 yes/' /etc/ssh/sshd_config
echo "MaxAuthTries 120" >> /etc/ssh/sshd_config
grep '^PermitRootLogin yes' /etc/ssh/sshd_config
systemctl reload ssh

print_system "done setting up ssh for spread test user"

touch /root/spread-setup-done
EOF
chmod 0755 "$snapddir/usr/lib/snapd/snapd.spread-tests-run-mode-tweaks.sh"

snap pack --filename=snapd.snap "$snapddir"
rm -r $snapddir

# next re-pack the kernel snap with this version of ubuntu-core-initramfs

# extract the kernel snap, including extracting the initrd from the kernel.efi
kerneldir="$(mktemp --tmpdir -d kernel-workdirXXXXXXXXXX)"
trap 'rm -rf "${kerneldir}"' EXIT

unsquashfs -f -d "${kerneldir}" upstream-pc-kernel.snap
(
    cd "${kerneldir}"
    config="$(echo config-*)"
    kernelver="${config#config-}"
    objcopy -O binary -j .linux kernel.efi kernel.img-"${kernelver}"
    ubuntu-core-initramfs create-initrd --kerneldir modules/"${kernelver}" --kernelver "${kernelver}" --firmwaredir firmware --output ubuntu-core-initramfs.img
    ubuntu-core-initramfs create-efi --initrd ubuntu-core-initramfs.img --kernel kernel.img --output kernel.efi --kernelver "${kernelver}"
    mv "kernel.efi-${kernelver}" kernel.efi
    rm kernel.img-"${kernelver}"
    rm ubuntu-core-initramfs.img-"${kernelver}"
)

snap pack --filename=pc-kernel.snap "${kerneldir}"

# penultimately, re-pack the gadget snap with snakeoil signed shim

gadgetdir=/tmp/gadget-workdir
unsquashfs -d "$gadgetdir" upstream-pc-gadget.snap

sbattach --remove "$gadgetdir/shim.efi.signed"
SNAKE_OIL_DIR=/usr/lib/ubuntu-core-initramfs/snakeoil
sbsign --key "$SNAKE_OIL_DIR/PkKek-1-snakeoil.key" --cert "$SNAKE_OIL_DIR/PkKek-1-snakeoil.pem" --output "$gadgetdir/shim.efi.signed" "$gadgetdir/shim.efi.signed"

snap pack --filename=pc-gadget.snap "$gadgetdir"
rm -rf "$gadgetdir"

# finally build the uc22 image
ubuntu-image snap \
    -i 8G \
    --snap upstream-core22.snap \
    --snap snapd.snap \
    --snap pc-kernel.snap \
    --snap pc-gadget.snap \
    ubuntu-core-22-amd64-dangerous.model

# setup some data we will inject into ubuntu-seed partition of the image above
# that snapd.spread-tests-run-mode-tweaks.service will ingest

# this sets up some /etc/passwd and group magic that ensures the test and ubuntu
# users are working, mostly copied from snapd spread magic
mkdir -p /root/test-etc
# NOTE that we don't use the real extrausers db on the host VM here because that
# could be used to actually login to the L1 VM, which we don't want to allow, so
# put it in a fake dir that login() doesn't actually look at for the host L1 VM.
mkdir -p /root/test-var/lib/extrausers
touch /root/test-var/lib/extrausers/sub{uid,gid}
for f in group gshadow passwd shadow; do
    # don't include the ubuntu user here, we manually add that later on
    grep -v "^root:" /etc/"$f" | grep -v "^ubuntu:" /etc/"$f" > /root/test-etc/"$f"
    grep "^root:" /etc/"$f" >> /root/test-etc/"$f"
    chgrp --reference /etc/"$f" /root/test-etc/"$f"
    # append test user for testing
    grep "^test:" /etc/"$f" >> /root/test-var/lib/extrausers/"$f"
    # check test was copied
    MATCH "^test:" < /root/test-var/lib/extrausers/"$f"
done

# TODO: could we just do this in the script above with adduser --extrausers and
# echo ubuntu:ubuntu | chpasswd ?
# dynamically create the ubuntu user in our fake extrausers with password of 
# ubuntu
#shellcheck disable=SC2016
echo 'ubuntu:$6$5jPdGxhc$8DgCHDdjj9IQxefS9atknQq4JVVYqy6KiPV/p4fDf5NUI6dqKTAf0vUZNx8FUru/pNgOQMwSMzS5pFj3hp4pw.:18492:0:99999:7:::' >> /root/test-var/lib/extrausers/shadow
#shellcheck disable=SC2016
echo 'ubuntu:$6$5jPdGxhc$8DgCHDdjj9IQxefS9atknQq4JVVYqy6KiPV/p4fDf5NUI6dqKTAf0vUZNx8FUru/pNgOQMwSMzS5pFj3hp4pw.:18492:0:99999:7:::' >> /root/test-etc/shadow
echo 'ubuntu:!::' >> /root/test-var/lib/extrausers/gshadow
# use gid of 1001 in case sometimes the lxd group sneaks into the extrausers image somehow...
echo "ubuntu:x:1000:1001:Ubuntu:/home/ubuntu:/bin/bash" >> /root/test-var/lib/extrausers/passwd
echo "ubuntu:x:1000:1001:Ubuntu:/home/ubuntu:/bin/bash" >> /root/test-etc/passwd
echo "ubuntu:x:1001:" >> /root/test-var/lib/extrausers/group

# add the test user to the systemd-journal group if it isn't already
sed -r -i -e 's/^systemd-journal:x:([0-9]+):$/systemd-journal:x:\1:test/' /root/test-etc/group

# tar the runmode tweaks and copy them to the image
tar -c -z -f run-mode-overlay-data.tar.gz \
    /root/test-etc /root/test-var/lib/extrausers
partoffset=$(fdisk -lu pc.img | awk '/EFI System$/ {print $2}')
mcopy -i pc.img@@$(($partoffset * 512)) run-mode-overlay-data.tar.gz ::run-mode-overlay-data.tar.gz

# the image is now ready to be booted
