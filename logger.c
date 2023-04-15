/*
 * main.c -- the bare logger char module
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>

#include <linux/uaccess.h>	/* copy_*_user */

#include "logger.h"		/* local definitions */

/*
 * Our parameters which can be set at load time.
 */

int logger_major =   LOGGER_MAJOR;
int logger_minor =   0;
int logger_nr_devs = LOGGER_NR_DEVS;	/* number of bare logger devices */

module_param(logger_major, int, S_IRUGO);
module_param(logger_minor, int, S_IRUGO);
module_param(logger_nr_devs, int, S_IRUGO);

MODULE_AUTHOR("Junliang");
MODULE_LICENSE("Dual BSD/GPL");

struct logger_dev *logger_devices;	/* allocated in logger_init_module */

static __u32 get_entry_len(struct logger_dev *log, size_t off)
{

    __u16 val;
    switch (log->size - off) {
        case 1:
            memcpy(&val, log->buffer + off, 1);
            memcpy(((char *)&val) + 1, log->buffer, 1);
            break;
        default:
            memcpy(&val, log->buffer + off, 2);
    }

    return sizeof(struct logger_entry) + val;
}

static void do_write_log(struct logger_dev *log, const void *buf, size_t count)
{
    size_t len;
    len = min(count, log->size - log->w_off);
    memcpy(log->buffer + log->w_off, buf, len);
    if (count != len)
        memcpy(log->buffer, buf + len, count - len);

    log->w_off = logger_offset(log->w_off + count);
}


static ssize_t do_write_log_from_user(struct logger_dev *log, const void __user *buf, size_t count)
{
    size_t len;

    len = min(count, log->size - log->w_off);
    if (len && copy_from_user(log->buffer + log->w_off, buf, len))
        return -EFAULT;

    if (count != len)
        if (copy_from_user(log->buffer, buf + len, count - len))
            return -EFAULT;

    log->w_off = logger_offset(log->w_off + count);

    return count;
}

/*
 * reads exactly 'count' bytes from 'log' into the user-space buffer 'buf'.
 *
 */
static ssize_t do_read_log_to_user(struct logger_dev *log,
                                    char __user *buf,
                                    size_t count)
{
    size_t len;
    len = min(count, log->size - log->r_off);
    if (copy_to_user(buf, log->buffer + log->r_off, len))
        return -EFAULT;

    if (count != len)
        if (copy_to_user(buf + len, log->buffer, count - len))
            return -EFAULT;
    log->r_off = logger_offset(log->r_off + count);
    return count;
}


/*
 * Empty out the logger device; must be called with the device
 * semaphore held.
 */
int logger_trim(struct logger_dev *dev)
{

	kfree(dev->buffer);
	dev->size = 0;
	return 0;
}

/*
 * Open and close
 */

int logger_open(struct inode *inode, struct file *filp)
{
	struct logger_dev *dev; /* device information */

	dev = container_of(inode->i_cdev, struct logger_dev, cdev);
	filp->private_data = dev; /* for other methods */

	return 0;          /* success */
}

int logger_release(struct inode *inode, struct file *filp)
{
	return 0;
}
ssize_t logger_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct logger_dev *log = filp->private_data;

    struct logger_entry header;
    ssize_t ret = 0;

    header.pid = current->tgid;
    header.tid = current->pid;
    header.len = min_t(size_t, count, LOGGER_ENTRY_MAX_PAYLOAD);

    if (unlikely(!header.len))
        return 0;
    do_write_log(log, &header, sizeof(struct logger_entry));

    ret = do_write_log_from_user(log, buf, count);


    return ret;


}

ssize_t logger_read(struct file *file, char __user *buf,
                        size_t count, loff_t *pos)
{

    struct logger_dev *log = file->private_data;
    ssize_t ret;

    ret = (log->w_off == log->r_off);

    if (ret) return 0; // empty

    ret = get_entry_len(log, log->r_off);
    if (count < ret) {
        ret = -EINVAL;
        goto out;
    }

    ret = do_read_log_to_user(log, buf, ret);

out:
    return ret;
}


struct file_operations logger_fops = {
	.owner =    THIS_MODULE,
	.read =     logger_read,
	.write =    logger_write,
	.open =     logger_open,
	.release =  logger_release,
};

/*
 * Finally, the module stuff
 */

/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
void logger_cleanup_module(void)
{
	int i;
	dev_t devno = MKDEV(logger_major, logger_minor);

	/* Get rid of our char dev entries */
	if (logger_devices) {
		for (i = 0; i < logger_nr_devs; i++) {
			logger_trim(logger_devices + i);
			cdev_del(&logger_devices[i].cdev);
		}
		kfree(logger_devices);
	}

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, logger_nr_devs);
}


/*
 * Set up the char_dev structure for this device.
 */
static void logger_setup_cdev(struct logger_dev *dev, int index)
{
	int err, devno = MKDEV(logger_major, logger_minor + index);
    
	cdev_init(&dev->cdev, &logger_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add (&dev->cdev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
		printk(KERN_NOTICE "Error %d adding logger%d", err, index);
}


int logger_init_module(void)
{
	int result, i;
	dev_t dev = 0;

	/*
	 * Get a range of minor numbers to work with, asking for a dynamic
	 * major unless directed otherwise at load time.
	 */
	if (logger_major) {
		dev = MKDEV(logger_major, logger_minor);
		result = register_chrdev_region(dev, logger_nr_devs, "logger");
	} else {
		result = alloc_chrdev_region(&dev, logger_minor, logger_nr_devs,
				"logger");
		logger_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "logger: can't get major %d\n", logger_major);
		return result;
	}

	/* 
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
	logger_devices = kmalloc(logger_nr_devs * sizeof(struct logger_dev), GFP_KERNEL);
	if (!logger_devices) {
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(logger_devices, 0, logger_nr_devs * sizeof(struct logger_dev));

        /* Initialize each device. */
	for (i = 0; i < logger_nr_devs; i++) {
	    logger_devices[i].buffer = kmalloc(BUF_SIZE, GFP_KERNEL);
	    logger_devices[i].w_off = 0;
	    logger_devices[i].r_off = 0;
	    logger_devices[i].size = BUF_SIZE;
	    logger_setup_cdev(&logger_devices[i], i);
	}

	return 0; /* succeed */

  fail:
	logger_cleanup_module();
	return result;
}

module_init(logger_init_module);
module_exit(logger_cleanup_module);
