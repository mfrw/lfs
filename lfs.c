/*
 * Learning to write a filesystem for the Linux Kernel.
 * Author: Muhammad Falak R Wani (mfrw)
 * email : falakreyaz@gmail.com
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/version.h>
#include <linux/jbd2.h>
#include <linux/parser.h>
#include <linux/blkdev.h>
#include <linux/types.h>

#include "super.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Muhammad Falak R Wani (mfrw)");

static int lfs_iterate(struct file *filp, void *dirent, filldir_t filldir)
{
	loff_t pos = filp->f_pos;
	struct inode *inode = filp->f_path.dentry->d_inode;
	struct super_block *sb = inode->i_sb;
	struct buffer_head *bh;
	struct lfs_inode *lf_inode;
	struct lfs_dir_record *record;
	int i;
	printk(KERN_INFO "execing iterate");
	lf_inode = inode->i_private;
	if(unlikely(!S_ISDIR(lf_inode->mode))) {
		printk(KERN_ERR "inode %u not a dir", lf_inode->inode_no);
		return -ENOTDIR;
	}
	bh = sb_bread(sb, lf_inode->data_block_no);
	record = (struct lfs_dir_record *) bh->b_data;
	for(i = 0; i < lf_inode->dir_children_count; i++) {
		filldir(dirent, record->filename, LFS_FILENAME_MAXLEN, pos, record->inode_no, DT_UNKNOWN);
		pos += sizeof(struct lfs_dir_record);
		record ++;
	}
	return 1;
}

const struct file_operations lfs_dir_ops = {
	.owner = THIS_MODULE,
	.iterate = lfs_iterate,
};

struct dentry *lfs_lookup(struct inode *parent_inode, struct dentry *child_dentry,
		unsigned int flags)
{
	// A dummy call back again
	return NULL;
}

static struct inode_operations lfs_inode_ops = {
	.lookup = lfs_lookup,
};

struct inode *lfs_get_inode(struct super_block *sb,
			const struct inode *dir, umode_t mode,
			dev_t dev)
{
	struct inode *inode = new_inode(sb);
	if(inode) {
		inode->i_ino = get_next_ino();
		inode_init_owner(inode, dir, mode);
		inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;

		switch(mode & S_IFMT) {
			case S_IFDIR:
				inc_nlink(inode);
				break;
			case S_IFREG:
			case S_IFLNK:
			default:
				printk(KERN_ERR "Only root can have an inode\n");
				return NULL;
				break;
		}
	}
	return inode;
}

int lfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *root_inode;
	struct buffer_head *bh;
	struct lfs_super_block *sb_disk;
	bh = sb_bread(sb, 0);
	sb_disk = (struct lfs_super_block *)bh->b_data;
	printk(KERN_INFO "Magic num read :[%d]\n", sb_disk->magic);
	if(unlikely(sb_disk->magic != LFS_MAGIC)) {
		printk(KERN_ERR "BAD FS: Magic nums dont match\n");
		return -EPERM;
	}
	if(unlikely(sb_disk->block_size != LFS_DEFAULT_BS)) {
		printk(KERN_ERR "BAD FS: Block size is not default\n");
		return -EPERM;
	}
	printk(KERN_INFO "All good to go\n");
	sb->s_magic = LFS_MAGIC; // A number that will id the fs
	
	sb->s_fs_info = sb_disk;

	root_inode = new_inode(sb);
	root_inode->i_ino = LFS_ROOT_INODE_NO;
	inode_init_owner(root_inode, NULL, S_IFDIR);
	root_inode->i_sb = sb;
	root_inode->i_op = &lfs_inode_ops;
	root_inode->i_fop = &lfs_dir_ops;
	root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = CURRENT_TIME;
	root_inode->i_private = &(sb_disk->root_inode);

	sb->s_root = d_make_root(root_inode);
	if(!sb->s_root)
		return -ENOMEM;
	return 0;
}

static struct dentry *lfs_mount(struct file_system_type *fs_type,
		int flags, const char *dev_name,
		void *data)
{
	struct dentry *ret;
	ret = mount_bdev(fs_type, flags, dev_name, data, lfs_fill_super);
	if(unlikely(IS_ERR(ret)))
		printk(KERN_ERR "Error mounting");
	else
		printk(KERN_INFO "lfs mounted on [%s]\n", dev_name);
	return ret;
}

static void lfs_kill_sb(struct super_block *sb) {
	printk(KERN_INFO "Will add details latter");
	return;
}

struct file_system_type lfs_fs_type = {
	.owner = THIS_MODULE,
	.name = "lfs",
	.mount = lfs_mount,
	.kill_sb = lfs_kill_sb,
};

static int lfs_init(void)
{
	int ret;
	ret = register_filesystem(&lfs_fs_type);
	if(likely(ret == 0))
		printk(KERN_INFO "register: success\n");
	else
		printk(KERN_ERR "register: failure\n");
	return ret;
}

static void lfs_exit(void)
{
	int ret;
	ret = unregister_filesystem(&lfs_fs_type);
	if(likely(ret == 0))
		printk(KERN_INFO "unregister: success\n");
	else
		printk(KERN_ERR "unregister: failure\n");
}
module_init(lfs_init);
module_exit(lfs_exit);

