# Linux Kernel Module Template

This module makes a character device.

## What does this example demonstrate?

This module shows how to use syscalls with a character device kernel module. This example implements following file operations.

- open( )
- close( )
- mmap( )
- poll( )

### What does the example application do?

1. open( ) a special file made by this kernel module.
1. ioctl( ) requests this kernel module to allocate memory in the kernel address space.
1. mmap( ) asks this kernel module to map memory that is allocated by ioctl( ) to the application's address space. (Shared memory between the application and kernel)
1. The application writes "Hello world!" to the shared memory region.
1. ioctl( ) requests to read and print "Hello world!" written on the shared memory.
1. poll( ) syscall waits until 2 second passes.

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

## Other info

This program is tested on Ubuntu 17.04 with Linux-4.10.

### Cannot you compile this module?

Do you have the header files of your Linux kernel or the kernel source itself?

The following command installing the kernel header files may be able to solve the problem.

```
sudo apt-get install linux-headers-$(uname -r)
```
