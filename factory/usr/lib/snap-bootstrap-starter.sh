#!/bin/sh
set -e

# Run snap-bootstrap
/usr/lib/snapd/snap-bootstrap initramfs-mounts

# TODO:UC20: move this code into snap-bootstrap proper so this script _just_ 
# calls snap-bootstrap
mkdir -p /run/systemd/system/initrd-fs.target.d
cat >/run/systemd/system/initrd-fs.target.d/core.conf <<EOF
[Unit]
Requires=populate-writable.service
After=populate-writable.service
EOF
cp -r /run/systemd/system/initrd-fs.target.d /run/systemd/system/initrd.target.d
cp -r /run/systemd/system/initrd-fs.target.d /run/systemd/system/initrd-switch-root.target.d

