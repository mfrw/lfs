/*
 * Learning to write a filesystem for the Linux Kernel.
 * Author: Muhammad Falak R Wani (mfrw)
 * email : falakreyaz@gmail.com
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>


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
	sb->s_magic = 0x10032013; // A number that will id the fs
	
	inode = lfs_get_inode(sb, NULL, S_IFDIR, 0);
	sb->s_root = d_make_root(inode);
	if(!sb->s_root)
		return -ENOMEM;
	return 0;
}

