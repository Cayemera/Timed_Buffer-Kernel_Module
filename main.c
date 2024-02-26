/*
 * main.c --  template file given for TimeoutBuffer device
 *            assignment for CSCI 5103 
 * This code is based on the scullpipe code from LDD book.
 *         CSCI 5103 March 2023  -- Anand Tripathi  
 *
 * This module will create only one device /dev/timedbufer
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 */

#include <linux/sched.h>

#include <linux/configfs.h>
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
#include <linux/sched/signal.h>
#include "timedbuffer.h"	/* local definitions */

/*
 * Our parameters which can be set at load time.
 */

struct scull_buffer {
        wait_queue_head_t inq, outq;       /* read and write queues */
        char *buffer, *end;                /* begin of buf, end of buf */
        int buffersize;                    /* used in pointer arithmetic */
        char *rp, *wp;                     /* where to read, where to write */
	int  itemcount;			   /* Number of items in the buffer */
        int nreaders, nwriters;            /* number of openings for r/w */
        struct semaphore sem;              /* mutual exclusion semaphore */
        struct cdev cdev;                  /* Char device structure */
};

dev_t scull_b_devno;			/* Our first device number */

static struct scull_buffer *scull_b_devices;


#define init_MUTEX(_m) sema_init(_m, 1);

int scull_major =   SCULL_MAJOR;
int scull_minor =   0;
int NITEMS 	=   20;
int itemsize = ITEM_SIZE;

module_param(scull_major, int, S_IRUGO);
module_param(scull_minor, int, S_IRUGO);
module_param(NITEMS, int, 0);

MODULE_AUTHOR("Student CSCI 5103-Spring-2023 - adding code to the frameowrk by Anand Tripathi");
MODULE_LICENSE("Dual BSD/GPL");

/*
 * Open and close
 */
static int scull_b_open(struct inode *inode, struct file *filp)
{
	struct scull_buffer *dev;
	dev = container_of(inode->i_cdev, struct scull_buffer, cdev);
	filp->private_data = dev;

        // FILL IN YOUR CODE HERE

	return nonseekable_open(inode, filp);
}

static int scull_b_release(struct inode *inode, struct file *filp)
{
	struct scull_buffer *dev = filp->private_data;
	//
	// FILL IN YOUR CODE HERE

	return 0;   // just a dummy return statement, YOU MUST WRITE CORRECT code here
}

/*
 * Data management: read and write
*/
static ssize_t scull_b_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_buffer *dev = filp->private_data;

	// FILL IN YOUR CODE HERE

	return 0;   // just a dummy return statement, YOU MUST WRITE CORRECT code here
}


static ssize_t scull_b_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_buffer *dev = filp->private_data;

	// FILL IN YOUR CODE HERE
	
	return 0;   // just a dummy return statement WHICH YOU MUST WRITE CORRECTLY
}

/*
 * The file operations for the buffer device
 * (some are overlayed with bare scull)
 */
struct file_operations scull_buffer_fops = {
	.owner =	THIS_MODULE,
	.llseek =	no_llseek,
	.read =		scull_b_read,
	.write =	scull_b_write,
	.open =		scull_b_open,
	.release =	scull_b_release,
};

/*
 * Set up a cdev entry.
 */
static void scull_b_setup_cdev(struct scull_buffer *dev, int index)
{
	int err, devno = scull_b_devno + index;

	cdev_init(&dev->cdev, &scull_buffer_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add (&dev->cdev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
		printk(KERN_NOTICE "Error %d adding timedbuffer%d", err, index);
}

/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
void scull_b_cleanup_module(void)
{
	if (!scull_b_devices)
		return; /* nothing else to release */

	cdev_del(&scull_b_devices[0].cdev);
	kfree(scull_b_devices[0].buffer);

	kfree(scull_b_devices);
	unregister_chrdev_region(scull_b_devno, 1); // only one device to be unregistered
}

int scull_b_init_module(void)
{
	int result;
	dev_t dev = 0;
/*
 * Get a range of minor numbers to work with, asking for a dynamic
 * major unless directed otherwise at load time.
 */
	if (scull_major) {
		dev = MKDEV(scull_major, scull_minor);
		result = register_chrdev_region(dev, 1, "timedbuffer");
	} else {
		result = alloc_chrdev_region(&dev, scull_minor, 1, "timedbuffer");
		scull_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "scull_b_init: can't get major %d\n", scull_major);
		return 0;
	}

	scull_b_devno = dev;
	scull_b_devices = kmalloc(sizeof(struct scull_buffer), GFP_KERNEL); // only one device to be created
	if (scull_b_devices == NULL) {
		unregister_chrdev_region(dev, 1);
		return 0;
	}
	memset(scull_b_devices, 0, sizeof(struct scull_buffer));
        init_waitqueue_head(&(scull_b_devices[0].inq));
	init_waitqueue_head(&(scull_b_devices[0].outq));
	init_MUTEX(&scull_b_devices[0].sem);
	scull_b_setup_cdev(scull_b_devices, 0);

	return 1;
}

module_init(scull_b_init_module);
module_exit(scull_b_cleanup_module);
