#define LFS_MAGIC  0x10032013
#define LFS_DEFAULT_BS (4 * 1024)

struct lfs_super_block {
	unsigned int version;
	unsigned int magic;
	unsigned int block_size;
	unsigned int free_blocks;

	char padding[ LFS_DEFAULT_BS - (4 * sizeof(unsigned int))];
};
