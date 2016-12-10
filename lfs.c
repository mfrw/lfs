/*
 * Learning to write a filesystem for the Linux Kernel.
 * Author: Muhammad Falak R Wani (mfrw)
 * email : falakreyaz@gmail.com
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>

#include "lfs.h"


static int lfs_iterate(struct file *filp, void *dirent, filldir_t filldir)
{
	// A dummy function for supporting ls
	return 0;
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
	struct inode *inode;
	sb->s_magic = LFS_MAGIC; // A number that will id the fs
	
	inode = lfs_get_inode(sb, NULL, S_IFDIR, 0);
	inode->i_op = &lfs_inode_ops;
	inode->i_fop = &lfs_dir_ops;
	sb->s_root = d_make_root(inode);
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

