#ifndef _LFS_H
#define _LFS_H


#define LFS_MAGIC  0x10032013
#define LFS_DEFAULT_BS (4 * 1024)
#define LFS_FILENAME_MAXLEN 255

#define LFS_ROOTDIR_INODE_NO 1
#define LFS_ROOT_INODE_NO 1
#define LFS_ROOTDIR_DATABLOCK_NO  2
#define LFS_SUPER_BLOCK_NO 0
#define LFS_INODESTORE_BLOCK_NO 1


struct lfs_dir_record {
	char filename[LFS_FILENAME_MAXLEN];
	unsigned int inode_no;
};

struct lfs_inode {
	mode_t mode;
	unsigned int inode_no;
	unsigned int data_block_no;
	union {
		unsigned int file_size;
		unsigned int dir_children_count;
	};
};

struct lfs_dir_contents {
	unsigned int children_count;
	struct lfs_dir_record records[];
};

struct lfs_super_block {
	unsigned int version;
	unsigned int magic;
	unsigned int block_size;
	unsigned int free_blocks;
	unsigned int inodes_count;

	struct lfs_inode root_inode;
	char padding[ LFS_DEFAULT_BS - (4 * sizeof(unsigned int))];
};


#endif
