#ifndef _SUPER_H
#include "lfs.h"

static inline struct lfs_super_block *LFS_SB(struct super_block *sb)
{
	return sb->s_fs_info;
}

#endif
