/*
 * Copyright (c) 1998-2015 Erez Zadok
 * Copyright (c) 2009	   Shrikar Archak
 * Copyright (c) 2003-2015 Stony Brook University
 * Copyright (c) 2003-2015 The Research Foundation of SUNY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "trfs.h"
#include <linux/module.h>
#include <linux/parser.h>

enum { tfile,
       tfile_err
     };

static const match_table_t tokens = {
	{tfile, "tfile=%s"},
	{tfile_err, NULL}
};
/*
 * There is no need to lock the trfs_super_info's rwsem as there is no
 * way anyone can have a reference to the superblock at this point in time.
 */
static int trfs_read_super(struct super_block *sb, void *raw_data, int silent)
{


	int err = 0;
	struct super_block *lower_sb;
	struct path lower_path;

	struct inode *inode;
	struct trfs_sb_info *p;
	struct file* filp;
	struct trfs_options *opt = (struct trfs_options *)raw_data;
	char *dev_name = (char *)opt->dev_name;



	if (!dev_name) {
		printk(KERN_ERR
		       "trfs: read_super: missing dev_name argument\n");
		err = -EINVAL;
		goto out;
	}

	/* parse lower path */
	err = kern_path(dev_name, LOOKUP_FOLLOW | LOOKUP_DIRECTORY,
	                &lower_path);
	if (err) {
		printk(KERN_ERR	"trfs: error accessing "
		       "lower directory '%s'\n", dev_name);
		goto out;
	}

	/* allocate superblock private data */
	sb->s_fs_info = kzalloc(sizeof(struct trfs_sb_info), GFP_KERNEL);

	p = (struct trfs_sb_info *)sb->s_fs_info;

	p->tr_info = kzalloc(sizeof(struct trace_info), GFP_KERNEL);
	p->eWq = create_singlethread_workqueue("eWorkqueue");
	mutex_init(&p->tr_info->lock);



	p->thread_st = kthread_run(thread_fn, (void *)p, "logger_thread");
	if (p->thread_st)
		printk(KERN_INFO "Thread Created successfully\n");
	else
		printk(KERN_ALERT "Thread creation failed\n");




	filp = filp_open(opt->option, O_WRONLY | O_CREAT | O_TRUNC, 0644);

	if (IS_ERR(filp)) {
		err = PTR_ERR(filp);
		printk("Error filp_open");
		goto out_free;
	}

	p->tr_info->filp = filp;
	p->tr_info->offset = 0;
	p->tr_info->buffer_offset = 0;
	p->tr_info->bitmap = 0xFFFFFFFF;
	if (!TRFS_SB(sb)) {
		printk(KERN_CRIT "trfs: read_super: out of memory\n");
		err = -ENOMEM;
		goto out_free;
	}

	/* set the lower superblock field of upper superblock */
	lower_sb = lower_path.dentry->d_sb;
	atomic_inc(&lower_sb->s_active);
	trfs_set_lower_super(sb, lower_sb);

	/* inherit maxbytes from lower file system */
	sb->s_maxbytes = lower_sb->s_maxbytes;

	/*
	 * Our c/m/atime granularity is 1 ns because we may stack on file
	 * systems whose granularity is as good.
	 */
	sb->s_time_gran = 1;

	sb->s_op = &trfs_sops;

	sb->s_export_op = &trfs_export_ops; /* adding NFS support */

	/* get a new inode and allocate our root dentry */
	inode = trfs_iget(sb, d_inode(lower_path.dentry));
	if (IS_ERR(inode)) {
		err = PTR_ERR(inode);
		goto out_sput;
	}
	sb->s_root = d_make_root(inode);
	if (!sb->s_root) {
		err = -ENOMEM;
		goto out_iput;
	}
	d_set_d_op(sb->s_root, &trfs_dops);

	/* link the upper and lower dentries */
	sb->s_root->d_fsdata = NULL;
	err = new_dentry_private_data(sb->s_root);
	if (err)
		goto out_freeroot;

	/* if get here: cannot have error */

	/* set the lower dentries for s_root */
	trfs_set_lower_path(sb->s_root, &lower_path);

	/*
	 * No need to call interpose because we already have a positive
	 * dentry, which was instantiated by d_make_root.  Just need to
	 * d_rehash it.
	 */
	d_rehash(sb->s_root);
	if (!silent)
		printk(KERN_INFO
		       "trfs: mounted on top of %s type %s\n",
		       dev_name, lower_sb->s_type->name);
	goto out; /* all is well */

	/* no longer needed: free_dentry_private_data(sb->s_root); */
out_freeroot:
	dput(sb->s_root);
out_iput:
	iput(inode);
out_sput:
	/* drop refs we took earlier */
	atomic_dec(&lower_sb->s_active);
	kfree(TRFS_SB(sb));
	sb->s_fs_info = NULL;
out_free:
	path_put(&lower_path);

out:
	return err;
}

struct dentry *trfs_mount(struct file_system_type *fs_type, int flags,
                          const char *dev_name, void *raw_data)
{
	void *lower_path_name = (void *) dev_name;
	void *pass_options = NULL;
	char *p;
	int token;
	substring_t args[MAX_OPT_ARGS];
	char *options;

	struct trfs_options *opt = (struct trfs_options *)kmalloc(sizeof(struct trfs_options), GFP_KERNEL);


	options = (char*)raw_data;

	while ((p = strsep(&options, ",")) != NULL) {
		if (!*p)
			continue;

		token = match_token(p, tokens, args);

		switch (token) {
		case tfile:


			opt->option = match_strdup(&args[0]);
			break;
		case tfile_err:
		default:
			printk(KERN_WARNING
			       "%s: trfs: unrecognized option [%s]\n",
			       __func__, p);
		}


	}

	opt->dev_name = (void *)lower_path_name;

	pass_options = (void *)opt;


	return mount_nodev(fs_type, flags, pass_options,
	                   trfs_read_super);
}

void trfs_umount(struct super_block *sb)
{

	struct trfs_sb_info *p;
	mm_segment_t oldfs;
	int err = 0;
	printk(KERN_INFO "Unmounting trfs\n");

	p = (struct trfs_sb_info *)sb->s_fs_info;


	//Stop the logger thread
	kthread_stop(p->thread_st);

	mutex_lock(&p->tr_info->lock);
	oldfs = get_fs();
	set_fs(get_ds());

	//Empty out the buffer to the trace file before closing
	err = vfs_write(TRFS_SB(sb)->tr_info->filp, &TRFS_SB(sb)->tr_info->buffer[0], TRFS_SB(sb)->tr_info->buffer_offset, &(TRFS_SB(sb)->tr_info->offset));


	set_fs(oldfs);
	//set offset back to zero
	TRFS_SB(sb)->tr_info->buffer_offset = 0;

	mutex_unlock(&p->tr_info->lock);



	//Close the file opened to trace the calls
	filp_close(p-> tr_info->filp, NULL);
	mutex_destroy(&p->tr_info->lock);

	if (p->tr_info)
	{

		kfree(p->tr_info);
	}
	if (p->eWq)
		destroy_workqueue(p->eWq);


	//Call the generic_shutdown_super after the tracing operations are cleaned up
	generic_shutdown_super(sb);
}

static struct file_system_type trfs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= TRFS_NAME,
	.mount		= trfs_mount,
	.kill_sb	=  trfs_umount,
	.fs_flags	= 0,
};
MODULE_ALIAS_FS(TRFS_NAME);

static int __init init_trfs_fs(void)
{
	int err;

	pr_info("Registering trfs " TRFS_VERSION "\n");

	err = trfs_init_inode_cache();
	if (err)
		goto out;
	err = trfs_init_dentry_cache();
	if (err)
		goto out;
	err = register_filesystem(&trfs_fs_type);
out:
	if (err) {
		trfs_destroy_inode_cache();
		trfs_destroy_dentry_cache();
	}
	return err;
}

static void __exit exit_trfs_fs(void)
{
	trfs_destroy_inode_cache();
	trfs_destroy_dentry_cache();
	unregister_filesystem(&trfs_fs_type);
	pr_info("Completed trfs module unload\n");
}

MODULE_AUTHOR("Erez Zadok, Filesystems and Storage Lab, Stony Brook University"
              " (http://www.fsl.cs.sunysb.edu/)");
MODULE_DESCRIPTION("Trfs " TRFS_VERSION
                   " (http://trfs.filesystems.org/)");
MODULE_LICENSE("GPL");

module_init(init_trfs_fs);
module_exit(exit_trfs_fs);
