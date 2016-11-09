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

int thread_fn(void *ptr)
{
	struct trfs_sb_info *sb_private = (struct trfs_sb_info *)ptr;
	struct record *r = NULL;

	if (sb_private == NULL)
	{
		printk(KERN_ALERT "Super block private data is NULL\n");
		return 0;
	}
	r  = (struct record *)kzalloc(sizeof(struct record), GFP_KERNEL);
	allow_signal(SIGKILL);
	while (!kthread_should_stop())
	{
		//Non empty buffer - flush it to  trace file
		if (sb_private->tr_info->buffer_offset > 0)
		{
			r->flush_buffer_sig = 1;

			log_record(sb_private, r);
		}

		ssleep(1);
	}

	if (r)
	{
		kfree(r);
	}
	do_exit(0);
	return 0;
}

int file_write(struct file* file, unsigned char* data, unsigned int size, unsigned long long* offset) {
	mm_segment_t oldfs;
	int ret;

	oldfs = get_fs();
	set_fs(get_ds());

	ret = vfs_write(file, data, size, offset);

	set_fs(oldfs);
	return ret;
}
void log_record_common(struct trfs_sb_info *sb_private, struct record *r)
{

	//write id
	memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->id, sizeof(r->id));
	sb_private->tr_info->buffer_offset += sizeof(r->id);

	//write record type
	memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->type, sizeof(r->type));
	sb_private->tr_info->buffer_offset += sizeof(r->type);

	//write return value
	memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->return_value, sizeof(r->return_value));
	sb_private->tr_info->buffer_offset += sizeof(r->return_value);

	//write file address
	memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->file_address, sizeof(r->file_address));
	sb_private->tr_info->buffer_offset += sizeof(r->file_address);

}
int sync_log_record(struct trfs_sb_info *sb_private, struct record *r)
{
	int err = 0;

	//Flush buffer to trace file if kernel thread gives the signal
	if (r->flush_buffer_sig == 1)
	{
		err = file_write(sb_private->tr_info->filp, &sb_private->tr_info->buffer[0], sb_private->tr_info->buffer_offset, &(sb_private->tr_info->offset));

		//set offset back to zero
		sb_private->tr_info->buffer_offset = 0;
		return 0;
	}


	if (sb_private->tr_info->buffer_offset + r->record_size >= 2 * PAGE_SIZE)
	{

		//Write the buffer to trace file
		err = file_write(sb_private->tr_info->filp, &sb_private->tr_info->buffer[0], sb_private->tr_info->buffer_offset, &(sb_private->tr_info->offset));

		//set offset back to zero
		sb_private->tr_info->buffer_offset = 0;

	}
	//log the values which all the records have
	log_record_common(sb_private, r);

	//if log_rw_large is set to 1, we only need to write common values and data length.
	//The actual large data buffer will be then directly written from read/write function.
	if ((r->type == read || r->type == write) && r->log_rw_large == 1)
	{
		//length of data read from file
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->data_length, sizeof(r->data_length));
		sb_private->tr_info->buffer_offset += sizeof(r->data_length);

		return err;
	}

	switch (r->type)
	{
	case readdir:

		//write path length
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->path_length, sizeof(r->path_length));
		sb_private->tr_info->buffer_offset += sizeof(r->path_length);

		//write path name
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], r->path, r->path_length);
		sb_private->tr_info->buffer_offset += r->path_length;

		break;
	case open:

		//write path length
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->path_length, sizeof(r->path_length));
		sb_private->tr_info->buffer_offset += sizeof(r->path_length);

		//write path name
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], r->path, r->path_length);
		sb_private->tr_info->buffer_offset += r->path_length;

		//write open flags
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->open_flags, sizeof(r->open_flags));
		sb_private->tr_info->buffer_offset += sizeof(r->open_flags);

		//write permission mode
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->perm_mode, sizeof(r->perm_mode));
		sb_private->tr_info->buffer_offset += sizeof(r->perm_mode);

		break;

	case read:
	case write:

		//length of data read from file
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->data_length, sizeof(r->data_length));
		sb_private->tr_info->buffer_offset += sizeof(r->data_length);

		//log data read from file
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], r->data, r->data_length);
		sb_private->tr_info->buffer_offset += r->data_length;
		break;
	case close:

		break;
	case mkdir:

		//write path length
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->path_length, sizeof(r->path_length));
		sb_private->tr_info->buffer_offset += sizeof(r->path_length);

		//write path name
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], r->path, r->path_length);
		sb_private->tr_info->buffer_offset += r->path_length;
		//write permission mode
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->perm_mode, sizeof(r->perm_mode));
		sb_private->tr_info->buffer_offset += sizeof(r->perm_mode);
		break;
	case unlink:
		//write path length
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->path_length, sizeof(r->path_length));
		sb_private->tr_info->buffer_offset += sizeof(r->path_length);

		//write path name
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], r->path, r->path_length);
		sb_private->tr_info->buffer_offset += r->path_length;
		break;
	case link:
		//write path length
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->path_length, sizeof(r->path_length));
		sb_private->tr_info->buffer_offset += sizeof(r->path_length);

		//write path name
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], r->path, r->path_length);
		sb_private->tr_info->buffer_offset += r->path_length;

		//write new path length
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->new_path_length, sizeof(r->new_path_length));
		sb_private->tr_info->buffer_offset += sizeof(r->new_path_length);

		//write new path name
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], r->new_path, r->new_path_length);
		sb_private->tr_info->buffer_offset += r->new_path_length;
		break;
	case rename:
		//write path length
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->path_length, sizeof(r->path_length));
		sb_private->tr_info->buffer_offset += sizeof(r->path_length);

		//write path name
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], r->path, r->path_length);
		sb_private->tr_info->buffer_offset += r->path_length;

		//write new path length
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->new_path_length, sizeof(r->new_path_length));
		sb_private->tr_info->buffer_offset += sizeof(r->new_path_length);

		//write new path name
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], r->new_path, r->new_path_length);
		sb_private->tr_info->buffer_offset += r->new_path_length;
		break;
	case rmdir:

		//write path length
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], &r->path_length, sizeof(r->path_length));
		sb_private->tr_info->buffer_offset += sizeof(r->path_length);

		//write path name
		memcpy(&sb_private->tr_info->buffer[sb_private->tr_info->buffer_offset], r->path, r->path_length);
		sb_private->tr_info->buffer_offset += r->path_length;
		break;
	default:
		printk(KERN_ALERT "Error.Unknown option to log : %d\n", r->type);
		return -EINVAL;
	}



//	print_status(sb);
	return err;
}

// Function executed by kernel thread
int log_record(struct trfs_sb_info *sb_private, struct record *r)
{

	if (sb_private == NULL)
	{

		printk(KERN_ALERT "SB is NULL\n");
		return -1;
	}

	/* trace only if bit is set for operation in bitmap */
	if (r->flush_buffer_sig != 1 && !CHECK_BIT(sb_private, r->type))
	{
		return -1;
	}
	mutex_lock(&sb_private->tr_info->lock);

	//if flush_buffer_sig is sent from logger thread to periodically
	// flush the buffer, then donot increment the id
	if (r->flush_buffer_sig != 1)
	{
		//Assign and Increment the record ID
		r->id = sb_private->last_id;
		sb_private->last_id++;
	}


	sync_log_record(sb_private, r);

	mutex_unlock(&sb_private->tr_info->lock);

	return 0;
}
static void ework_handler(struct work_struct *pwork)
{
	my_work_t *temp;
	void *data = NULL;
	struct record *rptr = NULL;
	/** pwork is the pointer of my_work **/
	temp = container_of(pwork, my_work_t, my_work);

	data = temp->ptr;


	rptr = (struct record*)data;


	log_record(rptr->sb_private, rptr);

	if (rptr->path)
	{
		kfree(rptr->path);
	}
	if (rptr)
	{
		kfree(rptr);
	}
	if (rptr->data)
	{
		kfree(rptr->data);
	}
}

int eworkqueue_init(void* arg)
{

	my_work_t *work;

	struct record *rptr = (struct record *)arg;


	work = (my_work_t *) kmalloc(sizeof(my_work_t), GFP_KERNEL);

	/** Init the work struct with the work handler **/
	INIT_WORK( &(work->my_work), ework_handler );
	work->ptr = arg;
	if (!rptr->sb_private->eWq)
	{

		rptr->sb_private->eWq = create_singlethread_workqueue("eWorkqueue");

	}
	if (rptr->sb_private->eWq)
	{
		queue_work(rptr->sb_private->eWq, &(work->my_work) );
	}
// * This, if you want to use the default event thread -- no need to create eWq here
// schedule_work(&eWorkqueue);

	return 0;
}

static ssize_t trfs_read(struct file *file, char __user *buf,
                         size_t count, loff_t *ppos)
{
	int err;
	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;

	struct record *r = NULL;
	int res = -1;
	struct trfs_sb_info *p = NULL;
	char* copy_buffer = NULL;

	lower_file = trfs_lower_file(file);
	err = vfs_read(lower_file, buf, count, ppos);
	/* update our inode atime upon a successful lower read */
	if (err >= 0)
		fsstack_copy_attr_atime(d_inode(dentry),
		                        file_inode(lower_file));
	else
	{
		goto out;
	}

	p = (struct trfs_sb_info *)file->f_inode->i_sb->s_fs_info;
	r = (struct record *)kzalloc(sizeof(struct record), GFP_KERNEL);
	if (r == NULL)
	{
		err = -ENOMEM;
		goto out;
	}

	//set the record type
	r->type = read;

	//Store file address - a unique ID to distinguish file
	r->file_address = (unsigned long)file;
	r->return_value = err;
	r->data_length = err;

	r->record_size = sizeof(r->id) + sizeof(r->type) + sizeof(r->return_value) + sizeof(r->file_address) + sizeof(r->data_length) + r->data_length;


	r->sb_private = (struct trfs_sb_info *)file->f_inode->i_sb->s_fs_info;



	//If record size is greater than the buffer capacity (that is two page size), then flush the buffer to file
	//and then directly write the record to file
	if (r->record_size  >= 2 * PAGE_SIZE)
	{

		if (!CHECK_BIT(r->sb_private, r->type))
		{
			//If bit is not set in bitmap,
			//do not copy to buffer
			goto out;
		}
		//write directly to file, vecause large record
		r->log_rw_large = 1;
		//log_record(p, r);
		eworkqueue_init((void*)r);
		flush_workqueue(r->sb_private->eWq);

		//LOCK HERE
		mutex_lock(&p->tr_info->lock);

		//flush the contents of buffer to file
		res = file_write(p->tr_info->filp, &p->tr_info->buffer[0], p->tr_info->buffer_offset, &(p->tr_info->offset));
		p->tr_info->buffer_offset = 0;

		//write the contents of the data buffer directly to the trace file
		//no need to switch mm_segment as it is writing user buffer directly
		res = vfs_write(p->tr_info->filp, buf, r->data_length, &(p->tr_info->offset));

		//UNLOCK HERE
		mutex_unlock(&p->tr_info->lock);

		goto out;
	}


	//else record size is smaller than buffer capacity, hence just write the record to buffer
	r->data = (char * )kzalloc(r->data_length, GFP_KERNEL);
	if (r->data == NULL)
	{
		err = -ENOMEM;
		goto out;
	}
	copy_buffer = (char * )kzalloc(r->data_length, GFP_KERNEL);
	if (copy_buffer == NULL)
	{
		err = -ENOMEM;
		goto out;
	}

	if (copy_from_user(copy_buffer, buf, r->data_length))
	{
		err = -EFAULT;
		goto out;
	}

	memcpy(r->data, copy_buffer, r->data_length);


	eworkqueue_init((void*)r);



out:
	if (copy_buffer)
	{
		kfree(copy_buffer);
	}
	return err;
}

static ssize_t trfs_write(struct file *file, const char __user *buf,
                          size_t count, loff_t *ppos)
{
	int err;

	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;
	struct record *r = NULL;
	int res = -1;
	struct trfs_sb_info *p = NULL;
	char* copy_buffer = NULL;


	lower_file = trfs_lower_file(file);
	err = vfs_write(lower_file, buf, count, ppos);
	/* update our inode times+sizes upon a successful lower write */
	if (err >= 0) {
		fsstack_copy_inode_size(d_inode(dentry),
		                        file_inode(lower_file));
		fsstack_copy_attr_times(d_inode(dentry),
		                        file_inode(lower_file));
	}
	else
	{
		goto out;
	}

	p = (struct trfs_sb_info *)file->f_inode->i_sb->s_fs_info;
	r = (struct record *)kzalloc(sizeof(struct record), GFP_KERNEL);

	if (r == NULL)
	{
		err = -ENOMEM;
		goto out;
	}


	//set the record type
	r->type = write;

	//Store file address - a unique ID to distinguish file
	r->file_address = (unsigned long)file;

	r->return_value = err;
	r->data_length = err;
	r->log_rw_large = 0;
	r->record_size = sizeof(r->id) + sizeof(r->type) + sizeof(r->return_value) + sizeof(r->file_address) + sizeof(r->data_length) + r->data_length;
	r->sb_private = (struct trfs_sb_info *)file->f_inode->i_sb->s_fs_info;


	//If record size is greater than the buffer capacity (that is two page size), then flush the buffer to file
	//and then directly write the record to file
	if (r->record_size  >= 2 * PAGE_SIZE)
	{

		if (!CHECK_BIT(r->sb_private, r->type))
		{
			goto out;
		}

		r->log_rw_large = 1;

		eworkqueue_init((void*)r);
		flush_workqueue(r->sb_private->eWq);

		//LOCK HERE
		mutex_lock(&p->tr_info->lock);

		//flush the contents of buffer to file
		res = file_write(p->tr_info->filp, &p->tr_info->buffer[0], p->tr_info->buffer_offset, &(p->tr_info->offset));
		p->tr_info->buffer_offset = 0;

		//write the contents of the data buffer directly to the trace file
		//no need to switch mm_segment as it is writing user buffer directly
		res = vfs_write(p->tr_info->filp, buf, r->data_length, &(p->tr_info->offset));
		//UNLOCK HERE
		mutex_unlock(&p->tr_info->lock);
		goto out;
	}



	//else record size is smaller than buffer capacity, hence just write the record to buffer
	r->data = (char * )kzalloc(r->data_length, GFP_KERNEL);
	if (r->data == NULL)
	{
		err = -ENOMEM;
		goto out;
	}
	copy_buffer = (char * )kzalloc(r->data_length, GFP_KERNEL);
	if (copy_buffer == NULL)
	{
		err = -ENOMEM;
		goto out;
	}

	if (copy_from_user(copy_buffer, buf, r->data_length))
	{
		err = -EFAULT;
		goto out;
	}

	memcpy(r->data, copy_buffer, r->data_length);
//	res = log_record(p, r);
	eworkqueue_init((void*)r);



out:

	if (copy_buffer)
	{
		kfree(copy_buffer);
	}

	return err;
}

static int trfs_readdir(struct file *file, struct dir_context *ctx)
{

	int err = 0;
	struct trfs_sb_info *p;
	struct record *r = NULL;
	int res = -1;
	char * f_p;
	char tmp[1024];


	struct file *lower_file = NULL;
	struct dentry *dentry = file->f_path.dentry;

	lower_file = trfs_lower_file(file);
	err = iterate_dir(lower_file, ctx);
	file->f_pos = lower_file->f_pos;
	if (err >= 0)		/* copy the atime */
		fsstack_copy_attr_atime(d_inode(dentry),
		                        file_inode(lower_file));


	p = (struct trfs_sb_info *)file->f_inode->i_sb->s_fs_info;

	r = (struct record *)kzalloc(sizeof(struct record), GFP_KERNEL);

	if (r == NULL)
	{
		err = -ENOMEM;
		goto out;
	}
	f_p = dentry_path_raw(file->f_path.dentry, tmp, 1024);
	//set the record type
	r->type = readdir;

	//Store file address - a unique ID to distinguish file
	r->file_address = (unsigned long long)file;

	r->path_length = strlen(f_p);
	r->path = kzalloc(r->path_length, GFP_KERNEL);
	if (r->path == NULL)
	{
		err = -ENOMEM;
		goto out;
	}

	memcpy(r->path, f_p, r->path_length);

	r->return_value = err;
	r->record_size = sizeof(r->id) + sizeof(r->type) + sizeof(r->return_value) + sizeof(r->file_address) + sizeof(r->path_length) + r->path_length;
	res = log_record(p, r);





out:
	if (r->path)
	{
		kfree(r->path);
	}

	if (r)
	{
		kfree(r);
	}
	return err;
}

static long trfs_unlocked_ioctl(struct file *file, unsigned int cmd,
                                unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;
	struct trfs_sb_info *sbi;


	lower_file = trfs_lower_file(file);



	switch (cmd)
	{
	case TRFS_IOCSETALL:


		sbi = (struct trfs_sb_info *)file->f_path.dentry->d_sb->s_fs_info;

		sbi->tr_info->bitmap = 0xFFFFFFFF;
		break;
	case TRFS_IOCSETOPT:


		sbi = (struct trfs_sb_info *)file->f_path.dentry->d_sb->s_fs_info;

		sbi->tr_info->bitmap = arg;
		break;
	case TRFS_IOCGETOPT:
		sbi = (struct trfs_sb_info *)file->f_path.dentry->d_sb->s_fs_info;
		if (copy_to_user((void*)arg, &sbi->tr_info->bitmap, sizeof(sbi->tr_info->bitmap)))
		{
			printk("Copy to user failed.Error\n");
			return - EFAULT;
		}


		break;
	default:
		printk("Unknown ioctl\n");
		break;
	}



	/* XXX: use vfs_ioctl if/when VFS exports it */
	if (!lower_file || !lower_file->f_op)
		goto out;
	if (lower_file->f_op->unlocked_ioctl)
		err = lower_file->f_op->unlocked_ioctl(lower_file, cmd, arg);

	/* some ioctls can change inode attributes (EXT2_IOC_SETFLAGS) */
	if (!err)
		fsstack_copy_attr_all(file_inode(file),
		                      file_inode(lower_file));
out:
	return err;
}

#ifdef CONFIG_COMPAT
static long trfs_compat_ioctl(struct file *file, unsigned int cmd,
                              unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;

	lower_file = trfs_lower_file(file);

	/* XXX: use vfs_ioctl if/when VFS exports it */
	if (!lower_file || !lower_file->f_op)
		goto out;
	if (lower_file->f_op->compat_ioctl)
		err = lower_file->f_op->compat_ioctl(lower_file, cmd, arg);

out:
	return err;
}
#endif

static int trfs_mmap(struct file *file, struct vm_area_struct *vma)
{
	int err = 0;
	bool willwrite;
	struct file *lower_file;
	const struct vm_operations_struct *saved_vm_ops = NULL;

	/* this might be deferred to mmap's writepage */
	willwrite = ((vma->vm_flags | VM_SHARED | VM_WRITE) == vma->vm_flags);

	/*
	 * File systems which do not implement ->writepage may use
	 * generic_file_readonly_mmap as their ->mmap op.  If you call
	 * generic_file_readonly_mmap with VM_WRITE, you'd get an -EINVAL.
	 * But we cannot call the lower ->mmap op, so we can't tell that
	 * writeable mappings won't work.  Therefore, our only choice is to
	 * check if the lower file system supports the ->writepage, and if
	 * not, return EINVAL (the same error that
	 * generic_file_readonly_mmap returns in that case).
	 */
	lower_file = trfs_lower_file(file);
	if (willwrite && !lower_file->f_mapping->a_ops->writepage) {
		err = -EINVAL;
		printk(KERN_ERR "trfs: lower file system does not "
		       "support writeable mmap\n");
		goto out;
	}

	/*
	 * find and save lower vm_ops.
	 *
	 * XXX: the VFS should have a cleaner way of finding the lower vm_ops
	 */
	if (!TRFS_F(file)->lower_vm_ops) {
		err = lower_file->f_op->mmap(lower_file, vma);
		if (err) {
			printk(KERN_ERR "trfs: lower mmap failed %d\n", err);
			goto out;
		}
		saved_vm_ops = vma->vm_ops; /* save: came from lower ->mmap */
	}

	/*
	 * Next 3 lines are all I need from generic_file_mmap.  I definitely
	 * don't want its test for ->readpage which returns -ENOEXEC.
	 */
	file_accessed(file);
	vma->vm_ops = &trfs_vm_ops;

	file->f_mapping->a_ops = &trfs_aops; /* set our aops */
	if (!TRFS_F(file)->lower_vm_ops) /* save for our ->fault */
		TRFS_F(file)->lower_vm_ops = saved_vm_ops;

out:
	return err;
}

static int trfs_open(struct inode *inode, struct file *file)
{
	int err = 0;
	struct file *lower_file = NULL;
	struct path lower_path;
	struct record *r = NULL;
	char * f_p;
	char tmp[1024];


	/* don't open unhashed/deleted files */

	if (d_unhashed(file->f_path.dentry)) {
		err = -ENOENT;
		goto out_err;
	}

	file->private_data =
	    kzalloc(sizeof(struct trfs_file_info), GFP_KERNEL);
	if (!TRFS_F(file)) {
		err = -ENOMEM;
		goto out_err;
	}

	/* open lower object and link trfs's file struct to lower's */
	trfs_get_lower_path(file->f_path.dentry, &lower_path);
	lower_file = dentry_open(&lower_path, file->f_flags, current_cred());
	path_put(&lower_path);
	if (IS_ERR(lower_file)) {
		err = PTR_ERR(lower_file);
		lower_file = trfs_lower_file(file);
		if (lower_file) {
			trfs_set_lower_file(file, NULL);
			fput(lower_file); /* fput calls dput for lower_dentry */
		}
	} else {
		trfs_set_lower_file(file, lower_file);
	}

	if (err)
		kfree(TRFS_F(file));
	else
		fsstack_copy_attr_all(inode, trfs_lower_inode(inode));

	/* logging open */
	r  = (struct record *)kzalloc(sizeof(struct record), GFP_KERNEL);

	if (r == NULL)
	{
		err = -ENOMEM;
		goto out_err;
	}

	f_p = dentry_path_raw(file->f_path.dentry, tmp, 1024);
	//f_p = d_path(&file->f_path, tmp, 1024);

	//set the record type
	r->id = 0;
	r->type = open;
	r->open_flags = file->f_flags;
	r->perm_mode =  file->f_mode;
	r->file_address = (unsigned long)file;

	r->path_length = strlen(f_p);
	r->path = kzalloc(r->path_length, GFP_KERNEL);
	if (r->path == NULL)
	{
		err = -ENOMEM;
		goto out_err;
	}
	memcpy(r->path, f_p, r->path_length);

out_err:

	r->return_value = err;
	r->record_size = sizeof(r->id) + sizeof(r->type) + sizeof(r->return_value) + sizeof(r->file_address) + sizeof(r->open_flags) + sizeof(r->perm_mode) + sizeof(r->path_length) + r->path_length;

	r->sb_private = (struct trfs_sb_info *)file->f_inode->i_sb->s_fs_info;
	eworkqueue_init((void*)r);


	return err;
}

static int trfs_flush(struct file *file, fl_owner_t id)
{
	struct record *r = NULL;
	int err = 0;
	struct file *lower_file = NULL;
	struct trfs_sb_info *p = NULL;

	lower_file = trfs_lower_file(file);
	if (lower_file && lower_file->f_op && lower_file->f_op->flush) {
		filemap_write_and_wait(file->f_mapping);
		err = lower_file->f_op->flush(lower_file, id);
	}

	p = (struct trfs_sb_info *)file->f_inode->i_sb->s_fs_info;
	/* logging close */
	r  = (struct record *)kzalloc(sizeof(struct record), GFP_KERNEL);

	if (r == NULL)
	{
		err = -ENOMEM;
		goto out_err;
	}
	r->type = close;
	r->file_address = (unsigned long)file;
	r->return_value = err;
	r->record_size = sizeof(r->id) + sizeof(r->type) + sizeof(r->return_value) + sizeof(r->file_address);


	r->sb_private = (struct trfs_sb_info *)file->f_inode->i_sb->s_fs_info;
	eworkqueue_init((void*)r);
	//log_record(p, r);
out_err:
	return err;
}

/* release all lower object references & free the file info structure */
static int trfs_file_release(struct inode *inode, struct file *file)
{
	struct file *lower_file;

	lower_file = trfs_lower_file(file);
	if (lower_file) {
		trfs_set_lower_file(file, NULL);
		fput(lower_file);
	}

	kfree(TRFS_F(file));
	return 0;
}

static int trfs_fsync(struct file *file, loff_t start, loff_t end,
                      int datasync)
{
	int err;
	struct file *lower_file;
	struct path lower_path;
	struct dentry *dentry = file->f_path.dentry;

	err = __generic_file_fsync(file, start, end, datasync);
	if (err)
		goto out;
	lower_file = trfs_lower_file(file);
	trfs_get_lower_path(dentry, &lower_path);
	err = vfs_fsync_range(lower_file, start, end, datasync);
	trfs_put_lower_path(dentry, &lower_path);
out:
	return err;
}

static int trfs_fasync(int fd, struct file *file, int flag)
{
	int err = 0;
	struct file *lower_file = NULL;

	lower_file = trfs_lower_file(file);
	if (lower_file->f_op && lower_file->f_op->fasync)
		err = lower_file->f_op->fasync(fd, lower_file, flag);

	return err;
}

/*
 * Wrapfs cannot use generic_file_llseek as ->llseek, because it would
 * only set the offset of the upper file.  So we have to implement our
 * own method to set both the upper and lower file offsets
 * consistently.
 */
static loff_t trfs_file_llseek(struct file *file, loff_t offset, int whence)
{
	int err;
	struct file *lower_file;

	err = generic_file_llseek(file, offset, whence);
	if (err < 0)
		goto out;

	lower_file = trfs_lower_file(file);
	err = generic_file_llseek(lower_file, offset, whence);

out:
	return err;
}

/*
 * Wrapfs read_iter, redirect modified iocb to lower read_iter
 */
ssize_t
trfs_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	int err;
	struct file *file = iocb->ki_filp, *lower_file;

	lower_file = trfs_lower_file(file);
	if (!lower_file->f_op->read_iter) {
		err = -EINVAL;
		goto out;
	}

	get_file(lower_file); /* prevent lower_file from being released */
	iocb->ki_filp = lower_file;
	err = lower_file->f_op->read_iter(iocb, iter);
	iocb->ki_filp = file;
	fput(lower_file);
	/* update upper inode atime as needed */
	if (err >= 0 || err == -EIOCBQUEUED)
		fsstack_copy_attr_atime(d_inode(file->f_path.dentry),
		                        file_inode(lower_file));
out:
	return err;
}

/*
 * Wrapfs write_iter, redirect modified iocb to lower write_iter
 */
ssize_t
trfs_write_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	int err;
	struct file *file = iocb->ki_filp, *lower_file;

	lower_file = trfs_lower_file(file);
	if (!lower_file->f_op->write_iter) {
		err = -EINVAL;
		goto out;
	}

	get_file(lower_file); /* prevent lower_file from being released */
	iocb->ki_filp = lower_file;
	err = lower_file->f_op->write_iter(iocb, iter);
	iocb->ki_filp = file;
	fput(lower_file);
	/* update upper inode times/sizes as needed */
	if (err >= 0 || err == -EIOCBQUEUED) {
		fsstack_copy_inode_size(d_inode(file->f_path.dentry),
		                        file_inode(lower_file));
		fsstack_copy_attr_times(d_inode(file->f_path.dentry),
		                        file_inode(lower_file));
	}
out:
	return err;
}

const struct file_operations trfs_main_fops = {
	.llseek		= generic_file_llseek,
	.read		= trfs_read,
	.write		= trfs_write,
	.unlocked_ioctl	= trfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= trfs_compat_ioctl,
#endif
	.mmap		= trfs_mmap,
	.open		= trfs_open,
	.flush		= trfs_flush,
	.release	= trfs_file_release,
	.fsync		= trfs_fsync,
	.fasync		= trfs_fasync,
	.read_iter	= trfs_read_iter,
	.write_iter	= trfs_write_iter,
};

/* trimmed directory options */
const struct file_operations trfs_dir_fops = {
	.llseek		= trfs_file_llseek,
	.read		= generic_read_dir,
	.iterate	= trfs_readdir,
	.unlocked_ioctl	= trfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= trfs_compat_ioctl,
#endif
	.open		= trfs_open,

	.release	= trfs_file_release,

	.flush		= trfs_flush,
	.fsync		= trfs_fsync,

	.fasync		= trfs_fasync,
};

