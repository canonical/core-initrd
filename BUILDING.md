# Building initrd for Ubuntu Core

## Prequisities

- An ubuntu desktop machine (20.04 amd64 recommended) or a VM
- Install LXD and auto-configure it
  ```sudo snap install lxd && lxd init --auto```
  - Configure lxd to support storage device (a directory) mount in R/W mode
- git
- 2Gbyte free space in your disk

## Preparation 

- Fork this repository
- Install git: 
    ```
    sudo apt install git
    ```
- Clone the fork of this repository to your host machine:
  
  ```
  git clone git@github.com:stulluk/core-initrd.git
  ```

- Create a Ubuntu 20.004 ( Focal ) lxd container

    ```
     $  lxc launch ubuntu:20.04 myfocal
    Creating myfocal
    Starting myfocal                            
     $
    ```

- map your user/group id to allow R/W access to shared directory:
  
    ``` 
    lxc config set myfocal raw.idmap "both 1000 1000" 
    lxc restart myfocal
    ```

- Add a shared R/W directory with your container

    ```
     $  lxc config device add myfocal shared-initrd disk source=/home/stulluk/core-initrd/ path=/src/
    Device shared-initrd added to myfocal
     $
    ```
- Start a shell in your container

```
lxc shell myfocal
```

- Install snappy-dev PPA
```
add-apt-repository -y ppa:snappy-dev/image
```

- Install required packages
```
apt -y update && apt-y upgrade && apt -y install devscripts equivs
```



## Building

- Create dependency meta package:

```
root@myfocal:/src# mk-build-deps
dh_testdir
...
dh_builddeb
dpkg-deb: building package 'ubuntu-core-initramfs-build-deps' in '../ubuntu-core-initramfs-build-deps_49_amd64.deb'.

The package has been created.
Attention, the package has been created in the current directory,
not in ".." as indicated by the message above!
root@myfocal:/src#
```

- Now we can install **build dependencies**:
```
apt install ./ubuntu-core-initramfs-build-deps_49_amd64.deb
```
This will install a ton of package. Please be patient.

After all these, now we are ready to build core-initrd:

```
debuild -us -uc
```

This creates a debian package of the initrd as well.

- Now we need to install this **deb package** to our container

```
root@myfocal:/src# apt install ../ubuntu-core-initramfs_49_amd64.deb 
Reading package lists... Done
...
Setting up ubuntu-core-initramfs (49) ...
root@myfocal:/src#
```

- Add **kernel modules** to the container
```
apt -y install linux-modules-$(uname -r)
```


- Lets create **initrd.img**

```
root@myfocal:/src# bin/ubuntu-core-initramfs create-initrd
root@myfocal:/src# ll /boot/
total 77996
...
-rw-r--r--  1 root root 37485460 Sep  6 15:41 initrd.img-5.11.0-27-generic
...
root@myfocal:/src#
```

- finally, create **kernel.efi**

```
root@myfocal:/src# bin/ubuntu-core-initramfs create-efi
warning: data remaining[36321280 vs 36330892]: gaps between PE/COFF sections?
warning: data remaining[36321280 vs 36330896]: gaps between PE/COFF sections?
Signing Unsigned original image
root@myfocal:/src# ll /boot/
total 113480
...
-rw-r--r--  1 root root 37485460 Sep  6 15:41 initrd.img-5.11.0-27-generic
-rw-------  1 root root 36332440 Sep  6 15:42 kernel.efi-5.11.0-27-generic
-rw-r--r--  1 root root 26148697 Sep  6 15:42 ubuntu-core-initramfs.img-5.11.0-27-generic
...
root@myfocal:/src# 
```


# Integrating with Kernel Snap

When your initrd DEB package is built, you need to perform following steps to make it a part of kernel snap:

TODO: Write documentation for how to integrate initrd.img to your (custom or canonical) kernel 


# Testing & Debugging

Run:

```
spread qemu-nested
```

TODO: Write this [Testing](TESTING.md)