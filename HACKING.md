# Hacking the Initrd for Ubuntu Core

This document explains how to modify & update initrd for Ubuntu Core

# Purpose

Sometimes you may need to alter the behaviour of stock initrd or you may want to add a new functionality to it. 

# Target platforms

Modifying initrd is relatively easy for arm SBC's (Like RPi) or amd64 devices ( Like Intel NUCs ) without secure boot ( No TPM or PTT or similar)

For devices with secure boot, one need to :

TODO: Explain how to modify initrd when there is Secure boot / FDE.

# Prequisities

- A Linux Computer with various development tools
  - Latest Ubuntu Desktop is recommended
  - (For a SBC like RPi) you will need UART TTL serial debug cable (Because you will not have SSH in initrd)
  - initramfs-tools
  - snapcraft
  - etc

## Getting a debug shell in initrd

Getting a debug shell in initrd is very simple:
1. Boot your UC20 image on your RPi
2. Access to it via SSH
3. Edit your kernel commandline:

First your current kernel commandline is:
```
 ~$ cat /run/mnt/ubuntu-seed/cmdline.txt 
dwc_otg.lpm_enable=0 console=serial0,115200 elevator=deadline rng_core.default_quality=700 vt.handoff=2 quiet splash 
 ~$ 
```
Now you need to add following three arguments to it:
```
rd.systemd.debug_shell=serial0 dangerous rd.systemd.unit=emergency.service 
```

So it will look like:
```
 ~$ cat /run/mnt/ubuntu-seed/cmdline.txt 
dwc_otg.lpm_enable=0 console=serial0,115200 elevator=deadline rng_core.default_quality=700 vt.handoff=2 quiet splash rd.systemd.debug_shell=serial0 dangerous rd.systemd.unit=emergency.service
 ~$
```

**Warning**: Because of a [bug](https://bugs.launchpad.net/ubuntu/+source/flash-kernel/+bug/1933093) in boot.scr, following will not happen, and your RPi will boot to main rootfs again (you can ssh to it). Leaving this warning until this bug is fixed.

Finally, after a reboot, you will get a shell like this:
```
PM_RSTS: 0x00001000
....lot of logs here
Starting start4.elf @ 0xfec00200 partition 0

U-Boot 2020.10+dfsg-1ubuntu0~20.04.2 (Jan 08 2021 - 13:03:11 +0000)

DRAM:  3.9 GiB
....lot of logs
Decompressing kernel...
Uncompressed size: 23843328 = 0x16BD200
20600685 bytes read in 1504 ms (13.1 MiB/s)
Booting Ubuntu (with booti) from mmc 0:...
## Flattened Device Tree blob at 02600000
   Booting using the fdt blob at 0x2600000
   Using Device Tree in place at 0000000002600000, end 000000000260f07f

Starting kernel ...
[    1.224420] spi-bcm2835 fe204000.spi: could not get clk: -517

BusyBox v1.30.1 (Ubuntu 1:1.30.1-4ubuntu6.3) built-in shell (ash)
Enter 'help' for a list of built-in commands.
#
```

In this state, if you want to pivot-root to main rootfs, just execute:

```
# systemctl start basic.target
```

# Hacking without re-building

Sometimes, in order for testing some new feature, rebuilding is not necessary. It is possible to add your fancy nice application to current initrd.img.

Basically:
- Download current initrd.img from your RPi to your host
- unpack it to a directory
- add your binary / systemd unit into it
- repack initrd.img
- upload to your RPi
- reboot

```
 ~ $  mkdir uc-initrd
 ~ $  cd uc-initrd/
 ~/uc-initrd $  scp pi:/run/mnt/ubuntu-boot/uboot/ubuntu/pi-kernel_292.snap/initrd.img .
initrd.img                                                            100%   20MB  34.8MB/s   00:00    
 ~/uc-initrd $  unmkinitramfs initrd.img somedir
 ~/uc-initrd $  tree -L 1 somedir/
somedir/
├── bin -> usr/bin
├── etc
├── init -> usr/lib/systemd/systemd
├── lib -> usr/lib
├── lib64 -> usr/lib64
├── sbin -> usr/bin
└── usr

5 directories, 2 files
 ~/uc-initrd $  echo "echo \"Hello world\"" > somedir/usr/bin/hello.sh
 ~/uc-initrd $  chmod +x somedir/usr/bin/hello.sh 
 ~/uc-initrd $  somedir/usr/bin/hello.sh 
Hello world
 ~/uc-initrd $  
 ~/uc-initrd $  cd somedir/
 ~/uc-initrd/somedir $  find ./ | cpio -H newc -o | lz4 -9 -l > ../initrd.img.new
103890 blocks
 ~/uc-initrd/somedir $  cd ..
 ~/uc-initrd $  file initrd.img
initrd.img: LZ4 compressed data (v0.1-v0.9)
 ~/uc-initrd $  file initrd.img.new 
initrd.img.new: LZ4 compressed data (v0.1-v0.9)
 ~/uc-initrd $  ll
total 40252
drwxrwxr-x  3       4096 Haz 21 17:53 ./
drwxr-x--- 36       4096 Haz 21 17:52 ../
-rwxr-xr-x  1   20600685 Haz 21 17:43 initrd.img*
-rw-rw-r--  1   20601051 Haz 21 17:53 initrd.img.new
drwxrwxr-x  4       4096 Haz 21 17:49 somedir/
 ~/uc-initrd $  scp initrd.img.new pi:/tmp/
initrd.img.new                                                        100%   20MB  41.6MB/s   00:00    
 ~/uc-initrd $  ssh pi
Welcome to Ubuntu 20.04.2 LTS (GNU/Linux 5.4.0-1037-raspi aarch64)
......
Last login: Mon Jun 21 14:40:10 2021 from 192.168.1.247
 ~$ sudo cp /tmp/initrd.img.new /run/mnt/ubuntu-boot/uboot/ubuntu/pi-kernel_292.snap/initrd.img 
 ~$ sync
 ~$ sudo reboot
Connection to 192.168.1.224 closed by remote host.
Connection to 192.168.1.224 closed.
 ~/uc-initrd $ 
```

And then, finally, on your serial console :
```
Starting kernel ...

[    1.223730] spi-bcm2835 fe204000.spi: could not get clk: -517


BusyBox v1.30.1 (Ubuntu 1:1.30.1-4ubuntu6.3) built-in shell (ash)
Enter 'help' for a list of built-in commands.

# hello.sh 
Hello world
# 

```


# Hacking with rebuilding



# Troubleshooting

