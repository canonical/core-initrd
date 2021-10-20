#!/bin/sh -ex

ln -s /sbin/kmod /sbin/modprobe
mv /lib/modules /lib/modules-initrd
ln -s /run/mnt/kernel/lib/modules/ /lib/

find /sys/devices/ -name boot_vga |
    while read -r boot_vga_f; do
        grep -q 1 "$boot_vga_f" || continue
        dev_d=${boot_vga_f%/boot_vga}
        modalias=$(cat "$dev_d"/modalias)
        modprobe "$modalias"
        break
    done
