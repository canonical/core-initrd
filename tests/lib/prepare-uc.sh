#!/bin/bash

set -e
set -x 

# include auxiliary functions from this script
. "$TESTSLIB/prepare-utils.sh"

# install dependencies
install_core_initrd_deps "$PROJECT_PATH"

# TODO: is this still necessary for core-initrd? (this was copied from snapd)
# create test user for spread to use
groupadd --gid 12345 test
adduser --uid 12345 --gid 12345 --disabled-password --gecos '' test

if getent group systemd-journal >/dev/null; then
    usermod -G systemd-journal -a test
    id test | MATCH systemd-journal
fi

echo 'test ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

download_core_initrd_snaps "$SNAP_BRANCH"

# skip the build process if the uc-initramfs was provided
if [ ! -f "uc-initramfs.deb" ]; then
    build_core_initrd "$PROJECT_PATH"
    cp ./ubuntu-core-initramfs*.deb uc-initramfs.deb
fi

# install ubuntu-core-initramfs here so the repack-kernel uses the version of
# ubuntu-core-initramfs we built here
apt install -yqq ./uc-initramfs.deb

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
add_core_initrd_ssh_users

# next re-pack the kernel snap with this version of ubuntu-core-initramfs
inject_initramfs

# penultimately, re-pack the gadget snap with snakeoil signed shim
repack_and_sign_gadget

# finally build the uc22 image
build_core22_image

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
mv pc.img "$PROJECT_PATH/pc.img"

