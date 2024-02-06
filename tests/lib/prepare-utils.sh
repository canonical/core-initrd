#!/bin/bash

set -e
set -x 

: ${SSH_PORT:=8022}
: ${MON_PORT:=8888}
: ${UBUNTU_IMAGE_CHANNEL:=latest/edge}

execute_remote(){
    sshpass -p ubuntu ssh -p "$SSH_PORT" -o ConnectTimeout=10 -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no test@localhost "$*"
}

wait_for_ssh(){
    local service_name="$1"
    retry=400
    wait=1
    while ! execute_remote true; do
        if ! systemctl is-active "$service_name"; then
            echo "Service no longer active"
            systemctl status "${service_name}" || true
            return 1
        fi

        retry=$(( retry - 1 ))
        if [ $retry -le 0 ]; then
            echo "Timed out waiting for ssh. Aborting!"
            return 1
        fi
        sleep "$wait"
    done
}

nested_wait_for_snap_command(){
  retry=800
  wait=1
  while ! execute_remote command -v snap; do
      retry=$(( retry - 1 ))
      if [ $retry -le 0 ]; then
          echo "Timed out waiting for snap command to be available. Aborting!"
          exit 1
      fi
      sleep "$wait"
  done
}

cleanup_snapd_core_vm(){
    # stop the VM if it is running
    systemctl stop nested-vm
}

start_snapd_core_vm() {
    local work_dir="$1"

    # Do not enable SMP on GCE as it will cause boot issues. There is most likely
    # a bug in the combination of the kernel version used in GCE images, combined with
    # a new qemu version (v6) and OVMF
    # TODO try again to enable more cores in the future to see if it is fixed
    PARAM_MEM="-m 4096"
    PARAM_SMP="-smp 1"
    PARAM_DISPLAY="-nographic"
    PARAM_NETWORK="-net nic,model=virtio -net user,hostfwd=tcp::${SSH_PORT}-:22"
    # TODO: do we need monitor port still?
    PARAM_MONITOR="-monitor tcp:127.0.0.1:${MON_PORT},server,nowait"
    PARAM_RANDOM="-object rng-random,id=rng0,filename=/dev/urandom -device virtio-rng-pci,rng=rng0"
    PARAM_CPU=""
    PARAM_TRACE="-d cpu_reset"
    PARAM_LOG="-D ${work_dir}/qemu.log"
    PARAM_SERIAL="-serial file:${work_dir}/serial.log"
    PARAM_TPM=""

    ATTR_KVM=""
    if [ "$ENABLE_KVM" = "true" ]; then
        ATTR_KVM=",accel=kvm"
        # CPU can be defined just when kvm is enabled
        PARAM_CPU="-cpu host"
    fi

    mkdir -p "${work_dir}/image/"
    cp -f "/usr/share/OVMF/OVMF_VARS_4M.fd" "${work_dir}/image/OVMF_VARS_4M.fd"
    PARAM_BIOS="-drive file=/usr/share/OVMF/OVMF_CODE_4M.fd,if=pflash,format=raw,unit=0,readonly=on -drive file=${work_dir}/image/OVMF_VARS_4M.fd,if=pflash,format=raw"
    PARAM_MACHINE="-machine q35${ATTR_KVM} -global ICH9-LPC.disable_s3=1"
    PARAM_IMAGE="-drive file=${work_dir}/pc.img,cache=none,format=raw,id=disk1,if=none -device virtio-blk-pci,drive=disk1,bootindex=1"

    SVC_NAME="nested-vm"
    if ! sudo systemd-run --service-type=simple --unit="${SVC_NAME}" -- \
                qemu-system-x86_64 \
                ${PARAM_SMP} \
                ${PARAM_CPU} \
                ${PARAM_MEM} \
                ${PARAM_TRACE} \
                ${PARAM_LOG} \
                ${PARAM_MACHINE} \
                ${PARAM_DISPLAY} \
                ${PARAM_NETWORK} \
                ${PARAM_BIOS} \
                ${PARAM_RANDOM} \
                ${PARAM_IMAGE} \
                ${PARAM_SERIAL} \
                ${PARAM_MONITOR}; then
        echo "Failed to start ${SVC_NAME}" 1>&2
        systemctl status "${SVC_NAME}" || true
        return 1
    fi

    # Wait until ssh is ready
    if ! wait_for_ssh "${SVC_NAME}"; then
        echo "===== SERIAL PORT OUTPUT ======" 1>&2
        cat "${work_dir}/serial.log" 1>&2
        echo "===== END OF SERIAL PORT OUTPUT ======" 1>&2
        return 1
    fi

    # Wait for the snap command to become ready
    nested_wait_for_snap_command
}

install_core_initrd_deps() {
    local project_dir="$1"

    # needed for dracut which is a build-dep of ubuntu-core-initramfs
    # and for the version of snapd here which we want to use to pull snap-bootstrap
    # from when we build the debian package
    sudo add-apt-repository ppa:snappy-dev/image -y
    sudo DEBIAN_FRONTEND=noninteractive apt update -yqq
    sudo apt-mark hold grub-efi-amd64-signed
    sudo DEBIAN_FRONTEND=noninteractive apt upgrade -o Dpkg::Options::="--force-confnew" -yqq

    # these are already installed in the lxd image which speeds things up, but they
    # are missing in qemu and google images.
    sudo DEBIAN_FRONTEND=noninteractive apt install --no-install-recommends -o Dpkg::Options::="--force-confnew" psmisc fdisk snapd mtools ovmf qemu-system-x86 sshpass whois openssh-server -yqq

    # use the snapd snap explicitly
    # TODO: since ubuntu-image ships it's own version of `snap prepare-image`, 
    # should we instead install beta/edge snapd here and point ubuntu-image to this
    # version of snapd?
    # TODO: https://bugs.launchpad.net/snapd/+bug/1712808
    # There is a bug in snapd that prevents udev rules from reloading in privileged containers
    # with the following error message: 'cannot reload udev rules: exit status 1' when installing
    # snaps. However it seems that retrying the installation fixes it
    if ! sudo snap install snapd; then
        echo "FIXME: snapd install failed, retrying"
        sudo snap install snapd
    fi
    sudo snap install ubuntu-image --classic --channel="${UBUNTU_IMAGE_CHANNEL}"
}

download_core_initrd_snaps() {
    local snap_branch="$1"

    # get the model
    curl -o ubuntu-core-amd64-dangerous.model https://raw.githubusercontent.com/snapcore/models/master/ubuntu-core-24-amd64-dangerous.model

    # download neccessary images
    snap download pc-kernel --channel=24/${snap_branch} --basename=upstream-pc-kernel
    snap download pc --channel=24/${snap_branch} --basename=upstream-pc-gadget
    snap download snapd --channel=${snap_branch} --basename=upstream-snapd
    snap download core24 --channel=${snap_branch} --basename=upstream-core24
}

build_core_initrd() {
    local project_dir="$1"
    local current_dir="$(pwd)"
    
    # build the debian package of ubuntu-core-initramfs
    (
        cd "$project_dir"
        sudo apt -y build-dep ./

        DEB_BUILD_OPTIONS='nocheck testkeys' dpkg-buildpackage -tc -b -Zgzip

        # save our debs somewhere safe
        cp ../*.deb "$current_dir"
    )
}

inject_initramfs_into_kernel() {
    local kernel_snap="$1"
    local kernel_name=$(basename "$1")

    # extract the kernel snap, including extracting the initrd from the kernel.efi
    kerneldir="$(mktemp --tmpdir -d kernel-workdirXXXXXXXXXX)"
    trap 'rm -rf "${kerneldir}"' EXIT

    unsquashfs -f -d "${kerneldir}" "$kernel_snap"
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

    rm "$kernel_snap"
    snap pack --filename=$kernel_name "$kerneldir"
}

# create two new users that used during testing when executing
# snapd tests, external for the external backend, and 'test'
# for the snapd test setup
create_cloud_init_cdimage_config() {
    local CONFIG_PATH=$1
    cat << 'EOF' > "$CONFIG_PATH"
#cloud-config
datasource_list: [NoCloud]
users:
  - name: external
    sudo: "ALL=(ALL) NOPASSWD:ALL"
    lock_passwd: false
    plain_text_passwd: 'ubuntu'
  - name: test
    sudo: "ALL=(ALL) NOPASSWD:ALL"
    lock_passwd: false
    plain_text_passwd: 'ubuntu'
    uid: "12345"

EOF
}

repack_and_sign_gadget() {
    local snakeoil_dir=/usr/lib/ubuntu-core-initramfs/snakeoil
    local gadget_dir=/tmp/gadget-workdir
    local gadget_snap="$1"
    local gadget_name=$(basename "$1")
    local use_cloudinit=${2:-}

    unsquashfs -d "$gadget_dir" "$gadget_snap"

    # add the cloud.conf to the gadget, this is only used in non-nested
    # contexts where we are not as easily able to inject the same users
    if [ -n "$use_cloudinit" ]; then
        create_cloud_init_cdimage_config "${gadget_dir}/cloud.conf"
    fi

    sbattach --remove "$gadget_dir/shim.efi.signed"
    sbsign --key "$snakeoil_dir/PkKek-1-snakeoil.key" --cert "$snakeoil_dir/PkKek-1-snakeoil.pem" --output "$gadget_dir/shim.efi.signed" "$gadget_dir/shim.efi.signed"

    rm "$gadget_snap"
    snap pack --filename=$gadget_name "$gadget_dir"
    rm -rf "$gadget_dir"
}

add_core_initrd_ssh_users() {
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
# ssh might not be active yet so the command might fail
systemctl reload ssh || true

print_system "done setting up ssh for spread test user"

touch /root/spread-setup-done
EOF
    chmod 0755 "$snapddir/usr/lib/snapd/snapd.spread-tests-run-mode-tweaks.sh"

    rm upstream-snapd.snap
    snap pack --filename=upstream-snapd.snap "$snapddir"
    rm -r $snapddir
}

build_core24_image() {
    ubuntu-image snap \
        -i 8G \
        --snap upstream-core24.snap \
        --snap upstream-snapd.snap \
        --snap upstream-pc-kernel.snap \
        --snap upstream-pc-gadget.snap \
        ubuntu-core-amd64-dangerous.model
}
