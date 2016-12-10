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

static DEFINE_MUTEX(lfs_sb_lock);
static DEFINE_MUTEX(lfs_inode_lock);

static DEFINE_MUTEX(lfs_directory_children_update_lock);

static struct kmem_cache *lfs_inode_cachep;

void lfs_sb_sync(struct super_block *vsb)
{
	struct buffer_head *bh;
	struct lfs_super_block *sb = LFS_SB(vsb);
	bh = sb_bread(vsb, LFS_SUPER_BLOCK_NO);
	BUG_ON(!bh);

	bh->b_data = (char *)sb;
	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);
}

struct lfs_inode *lfs_inode_search(struct super_block *sb,
		struct lfs_inode *start,
		struct lfs_inode *search)
{
	unsigned int count = 0;
	while((start->inode_no != search->inode_no) && (count < LFS_SB(sb)->inodes_count)) {
		count++;
		start++;
	}
	if(start->inode_no == search->inode_no)
		return start;
	return NULL;
}

void lfs_inode_add(struct super_block *vsb, struct lfs_inode *inode)
{
	struct lfs_super_block *sb = LFS_SB(vsb);
	struct buffer_head *bh;
	struct lfs_inode *tmp;

	if(mutex_lock_interruptible(&lfs_inode_lock)) {
		lfs_trace("lfs_inode_lock");
		return;
	}
	bh = sb_bread(vsb, LFS_INODESTORE_BLOCK_NO);
	BUG_ON(!bh);
	tmp = (struct lfs_inode *)bh->b_data;

	if(mutex_lock_interruptible(&lfs_sb_lock)) {
		mutex_unloc(lfs_inode_lock);
		lfs_trace("lfs_sb_lock");
		return;
	}

	tmp += sb->inodes_count;

	memcpy(tmp, inode, sizeof(struct lfs_inode));
	sb->inodes_count++;
	mark_buffer_dirty(bh);
	lfs_sb_sync(vsb);
	brelse(bh);

	mutex_unlock(&lfs_inode_lock);
	mutex_unlock(&lfs_sb_lock);
}

int lfs_sb_get_freeblock(struct super_block *vsb, unsigned int *out)
{
	struct lfs_super_block *sb = LFS_SB(vsb);
	int i, ret = 0;
	if(mutex_lock_interruptible(&lfs_sb_lock)) {
		lfs_trace("lfs_sb_lock\n");
		ret = -EINTR;
		return ret;
	}
	for(i = 3; i < LFS_MAX_FS_OBJS; i++)
		if(sb->free_blocks & (1 << i))
			break;
	if(unlikely(i == LFS_MAX_FS_OBJS)) {
		printk(KERN_ERR "MEM FULL");
		ret = -ENOSPC;
		goto exit:
	}
	*out = i;
	sb->free_blocks &= ~(1 << i);
	lfs_sb_sync(vsb);
exit:
	mutex_unlock(&lfs_sb_lock);
	return ret;
}

static int lfs_sb_get_object_count(struct super_block *vsb, unsigned int *out)
{
	struct lfs_super_block *sb = LSF_SB(vsb);
	if(mutex_lock_interruptible(&lfs_inode_lock)) {
		lfs_trace("lfs_inode_lock\n");
		return -EINTR;
	}
	*out = sb->inodes_count;
	mutex_unlock(&lfs_inode_lock);
	return 0;
}

static int lfs_iterate(struct file *filp, struct dir_context *ctx)
{
	loff_t pos = ctx->pos;
	struct inode *inode = filp->f_path.dentry->d_inode;
	struct super_block *sb = inode->i_sb;
	struct buffer_head *bh;
	struct lfs_inode *lf_inode;
	struct lfs_dir_record *record;
	int i;
	if(pos)
		return 0;
	printk(KERN_INFO "execing iterate");
	lf_inode = LFS_INODE(inode);
	if(unlikely(!S_ISDIR(lf_inode->mode))) {
		printk(KERN_ERR "inode %u not a dir", lf_inode->inode_no);
		return -ENOTDIR;
	}
	bh = sb_bread(sb, lf_inode->data_block_no);
	BUG_ON(!bh);
	record = (struct lfs_dir_record *) bh->b_data;
	for(i = 0; i < lf_inode->dir_children_count; i++) {
		dir_emit(ctx, record->filename, LFS_FILENAME_MAXLEN, record->inode_no, DT_UNKOWN);
		ctx->pos += sizeof(struct lfs_dir_record);
		pos += sizeof(struct lfs_dir_record);
		record ++;
	}
	brelse(bh);
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

struct lfs_inode *lfs_get_inode(struct super_block *sb, unsigned int inode_no)
{
	struct lfs_super_block *lfs_sb = LFS_SB(sb);
	struct lfs_inode *lfs_inode = NULL;
	struct lfs_inode *inode_buffer = NULL;
	int i;
	struct buffer_head *bh;
	bh = sb_bread(sb, LFS_INODESTORE_BLOCK_NO);
	BUG_ON(!bh);
	lfs_inode = (struct lfs_inode *) bh->b_data;
	for(i = 0; i < lfs_sb->inodes_count; i++) {
		if(lfs_inode->inode_no == inode_no) {
			inode_buffer = kmem_cache_alloc(lfs_inode_cachep, GFP_KERNEL);
			memcpy(inode_buffer, lfs_inode, sizeof(struct lfs_inode));
			break;
		}
		lfs_inode++;
	}
	brelse(bh);
	return inode_buffer;
}

ssize_t lfs_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
	struct lfs_inode *inode = LFS_INODE(filp->f_path.dentry.d_inode);
	struct buffer_head *bh;
	char *buffer;
	int nbytes;

	if(*ppos >= inode->file_size)
		return 0;
	bh = sb_bread(filp->f_path.dentry->d_inode->i_sb, inode->data_block_no);
	if(!bh) {
		printk(KERN_ERR "Read blockno [%;;u] failed\n", inode->data_block_no);
		return 0;
	}
	buffer = (char *)bh-b_data;
	nbytes = min((size_t)inode->file_size, len);
	if(copy_to_user(buf, buffer, nbytes)) {
		brelse(bh);
		printk(KERN_ERR "Could not copy contents to userspace\n");
		return -EFAULT;
	}
	brelse(bh);
	*ppos += nbytes;
	return nbytes;
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

	root_inode->i_private = lfs_get_inode(sb, LFS_ROOTDIR_INODE_NO);
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

