#ifndef _KMOD_USER_H
#define _KMOD_USER_H

#define IOCREGMEM _IO('i', 1)
#define IOCPRINTK _IO('i', 2)

struct shared_struct {
	unsigned long len;
	unsigned long off;
};

#endif
