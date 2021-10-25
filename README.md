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

# Integrating with Kernel Snap

When your initrd DEB package is built, you need to perform following steps to make it a part of kernel snap:

TODO: Write documentation for how to integrate initrd.img to your (custom or canonical) kernel 

# Bootchart

It is possible to enable bootcharts by adding `core.bootchart` to the
kernel command line. The sample collector will run until the systemd
switches root, and the chart will be saved in `/run/log`. If
bootcharts are also enabled for the core snap, that file will be
eventually moved to the `ubuntu-save` partition (see Core snap
documentation).
