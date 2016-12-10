#include "lfs.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


int main(int argc, char *argv[])
{
	int fd;
	ssize_t ret;
	struct lfs_super_block sb;
	if(argc != 2) {
		fprintf(stderr, "usage: mkfs-lfs <device>\n");
		return -1;
	}

	fd = open(argv[1], O_RDWR);
	if(fd < 0) {
		perror("error: opening device");
		return -1;
	}
	sb.version = 1;
	sb.magic = LFS_MAGIC;
	sb.block_size = LFS_DEFAULT_BS;
	sb.free_blocks = ~0;
	ret = write(fd, (char *)&sb, sizeof(sb));
	if(ret != LFS_DEFAULT_BS)
		fprintf(stderr, "write [%d] unequal to def BS\n", (int)ret);
	else
		printf("superblock written successfully\n");
	close(fd);
	return 0;
}
