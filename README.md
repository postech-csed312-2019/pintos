# Pintos

Pintos, a simple operating system framework for the 80x86 architecture created at Stanford University

## Environment

```sh
$ lsb_release -a
No LSB modules are available.
Distributor ID:	Ubuntu
Description:	Ubuntu 20.04.1 LTS
Release:	20.04
Codename:	focal
$ uname -a
Linux ubuntu 5.4.0-45-generic #49-Ubuntu SMP Wed Aug 26 13:38:52 UTC 2020 x86_64 x86_64 x86_64 GNU/Linux
```

## Prerequisites

```sh
$ sudo apt-get update
$ sudo apt-get install binutils pkg-config zlib1g-dev libglib2.0-dev gcc libc6-dev autoconf libtool libsdl1.2-dev g++ libx11-dev libxrandr-dev libxi-dev perl libc6-dbg gdb make git qemu ctags
```

## Download Pintos 

Download the Pintos source code from [here](http://pintos-os.org/cgi-bin/gitweb.cgi?p=pintos-anon;a=summary).

```sh
$ cd ~
$ git clone git://pintos-os.org/pintos-anon pintos
```
## Run Pintos in QEMU

Refer to [here](https://github.com/ivogeorg/os-playground/blob/master/pintos-with-qemu.md).
