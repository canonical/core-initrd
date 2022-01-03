# Initrd for Ubuntu Core

This repository contains source code to build initial ramdisk used in Ubuntu Core Embedded Linux OS.

# Purpose

See: https://en.wikipedia.org/wiki/Initial_ramdisk

# Architecture

In Ubuntu Core, initrd.img file is part of Kernel Snap as a binary. This file is brought to Kernel snap from a PPA. See [integration](#integrating-with-kernel-snap) below.

In UC20, initrd is migrated from script based implementation (UC16/18) to **systemd based**.
See the [architecture document](ARCHITECTURE.md) for more details.

# Building initrd for Ubuntu Core

TODO: Write documentation for how to build initrd locally

## Prequisities

## Preparation

## Building

# Testing & Debugging

See [Hacking](HACKING.md)

# Releasing

The UC initrd is included in kernel snaps. However, the process to get
it inside the kernel snap is not immediate and consists of a few
steps. First, we need to build the `ubuntu-core-initramfs`
debian package in the [snappy-dev/image
PPA](https://launchpad.net/~snappy-dev/+archive/ubuntu/image) by
following these steps:

1. Update the changelog with latest changes since last release (use `dch -i` for this)
1. Commit using `debcommit --release -a` which will commit the changelog update & tag the release
1. Propose a PR to the repo with the new changelog, get it reviewed and merged
1. Push the tag to the repository with the new version (GitHub pull requests do not update tags)
1. Build the source package by running (note that the clean command
   removes all untracked files, including subtrees with .git folders)

        git clean -ffdx
        gbp buildpackage -S -sa -d --git-ignore-branch

1. Compare with the latest package that was uploaded to the snappy-dev
PPA to make sure that the changes are correct.  For this, you can
download the .dsc file and the tarball from the PPA, then run debdiff
to find out the differences:

        dget https://launchpad.net/~snappy-dev/+archive/ubuntu/image/+sourcefiles/ubuntu-core-initramfs/<old_version>/ubuntu-core-initramfs_<old_version>.dsc
        debdiff ubuntu-core-initramfs_<old_version>.dsc ubuntu-core-initramfs_<new_version>.dsc > diff.txt

1. Upload, or request sponsorship, to the snappy-dev PPA

        dput ppa:snappy-dev/image ubuntu-core-initramfs_<new_version>_source.changes

1. Make sure that the package has been built correctly. If not, make
   changes appropriately and repeat these steps, including creating a
   new changelog entry.

Note that `ubuntu-core-initramfs` gets some files from its build
dependencies while being built, including for instance
`snap-bootstrap`, so we need to make sure that the snappy-dev PPA
already contains the desired version of the snapd deb package (or
others) when we upload the package.

After this, the initrd changes will be included in future kernel snaps
releases automatically, following the usual 3 weeks cadence, as the
snappy-dev PPA is included when these build happen.

# Bootchart

It is possible to enable bootcharts by adding `core.bootchart` to the
kernel command line. The sample collector will run until the systemd
switches root, and the chart will be saved in `/run/log`. If
bootcharts are also enabled for the core snap, that file will be
eventually moved to the `ubuntu-save` partition (see Core snap
documentation).
