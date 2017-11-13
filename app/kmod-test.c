#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/ioctl.h>

#include "../kmod_user.h"

int main(void)
{
	int fd;
	char *mem = NULL;
	struct shared_struct s;

	fd = open("/dev/kmod", O_RDWR);
	if (fd < 0) {
		printf("failed to open the character device\n");
		return -1;
	}

	mem = mmap(0, 4096, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	printf("mmap() is done\n");
	printf("Message from kernel on shared memory : %s\n", mem);

	snprintf(mem, 4096, "Application overwrites shared memory.");
	memset(&s, 0, sizeof(struct shared_struct));
	s.len = strlen(mem);

	ioctl(fd, IOCPRINTK, &s);

	close(fd);

	return 0;
}
