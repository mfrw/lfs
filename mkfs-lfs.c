#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "lfs.h"


int main(int argc, char *argv[])
{
	int fd;
	ssize_t ret;
	struct lfs_super_block sb;
	struct lfs_inode root_inode;
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
	sb.inodes_count = 1;
	sb.free_blocks = ~0;
	ret = write(fd, (char *)&sb, sizeof(sb));
	if(ret != LFS_DEFAULT_BS) {
		fprintf(stderr, "write [%d] unequal to def BS\n", (int)ret);
		ret = -1;
		goto exit;
	}
	printf("superblock written successfully\n");

	root_inode.mode = S_IFDIR;
	root_inode.inode_no = LFS_ROOTDIR_INODE_NO;
	root_inode.data_block_no = LFS_ROOTDIR_DATABLOCK_NO;
	root_inode.dir_children_count = 0;

	ret = write(fd, (char *)&root_inode, sizeof(root_inode));
	if(ret != sizeof(root_inode)) {
		fprintf(stderr, "The inode store was not written properly\n");
		ret = -1;
		goto exit;
	}
	printf("inode store written succesfully\n");
	ret = 0;
exit:
	close(fd);
	return 0;
}
