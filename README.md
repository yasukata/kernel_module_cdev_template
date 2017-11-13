# Linux Kernel Module Template

This module makes a character device.

## How to build and test

The kernel module installation.

```
$ make
$ insmod ./kmod.ko
```

Test application.

```
$ cd app
$ make
$ ./kmod-test
```

