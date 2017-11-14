#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include "../kmod_user.h"

int main(void)
{
	int fd;
	char *mem = NULL;
	struct shared_struct s;
	struct pollfd pfd;

	fd = open("/dev/kmod", O_RDWR);
	if (fd < 0) {
		printf("failed to open the character device\n");
		return -1;
	}

	memset(&s, 0, sizeof(struct shared_struct));
	s.len = 10000;
	if (ioctl(fd, IOCREGMEM, &s) != 0) {
		close(fd);
		return -1;
	}

	mem = mmap(0, 10000, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (mem == NULL) {
		close(fd);
		return -1;
	}

	snprintf(&mem[9000], 1000, "Hello world!");
	memset(&s, 0, sizeof(struct shared_struct));
	s.len = strlen("Hello world!");
	s.off = 9000;
	ioctl(fd, IOCPRINTK, &s);

	pfd.fd = fd;
	pfd.events = POLLIN;

	printf("Start poll, wait 2 sec\n");
	poll(&pfd, 1, 2000);
	printf("poll done\n");

	close(fd);

	return 0;
}
