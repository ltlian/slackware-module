#include <linux/module.h>	// included for all kernel modules
#include <linux/kernel.h>	// included for KERN_INFO
#include <linux/init.h>		// included for __init and __exit macros
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "AirFlowSensor.h"

#define DEV_MAJOR 0		//Means to use a dynamic
#define DEV_NAME "airtune"
#define BUF_SIZE 128

// Enums consisting of a character's ascii value for interpreting input types
#define DELIMITER 61	// Equals ("=") sign
#define G 71			// Gain
#define O 79			// Offset
#define R 82			// Reference value

#define SUCCESS 0

static int major_number;

struct device
{
	char mem[BUF_SIZE];
	unsigned int count;
} char_dev;

// open function - called when the "file" /dev/ is opened in userspace
static int dev_open (struct inode *inode, struct file *file)
{
	// printk("%s:  device driver open\n", DEV_NAME);
	return 0;
}


// close function - called when the "file" /dev/airtune is closed in userspace
static int dev_release (struct inode *inode, struct file *file)
{
	printk("%s:  device driver closed\n", DEV_NAME);
	return 0;
}

static ssize_t dev_read (struct file *file, char *buf, size_t count, loff_t *ppos)
{
	char readBuf[BUF_SIZE];
	int readSize;

	// The driver is expected to return 0 to mark EOF
	if (*ppos > 0) {
		return 0;
	}

	sprintf(readBuf, "%d\n", ReadSensor());
	readSize = strlen(readBuf);

	if (copy_to_user(
		buf,				// Destination (user space)
		readBuf,			// Source (kernel space)
		readSize))			// n bytes to copy
	{
		printk("[airtune] copy_to_user failed\n");
		return -EFAULT;
	}

	*ppos += readSize;

	return readSize;
}

static ssize_t dev_write (struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	unsigned long argo;
	int readSize = sizeof char_dev.mem;
	char* value;
	char *ptr;
	long valInt;

	// TODO necessary to reset memory before read?
	memset(char_dev.mem, 0, readSize);
	argo = copy_from_user(
		char_dev.mem,	// destination	(kernel space)
		buf,			// source		(user space)
		count			// n bytes to copy
	);

	// point 'value' at the 3rd character of the string input and interpret it
	// as the start of a substring to be placed in valInt as a means of fetching
	// the option argument
	value = char_dev.mem + 2;
	valInt = simple_strtol(value, &ptr, 10);

	// valInt == 0						=> string could not be converted to number.
	// char_dev.mem[1] != DELIMITER		=> string is not in the right format
	if ((valInt == 0 && ptr == value) || (char_dev.mem[1] != DELIMITER))
	{
		printk("[136] >> Ignoring command; could not parse %s\n", char_dev.mem);
		return readSize;
	}

	switch(char_dev.mem[0]) {
		case G:
			printk("Setting Gain to %ld\n", valInt);
			SetSensorGain(valInt);
			break;

		case O:
			printk("Setting Offset to %ld\n", valInt);
			SetSensorOffset(valInt);
			break;

		case R:
			printk("Setting Reference to %ld\n", valInt);
			SetOutputRefValue(valInt);
			break;

		default:
			printk("Invalid input header\n");
		}

	return readSize;
}

// define which file operations are supported
struct file_operations dev_fops =
{
	.owner	=	THIS_MODULE,
	.llseek	=	NULL,
	.read		=	dev_read,
	.write	=	dev_write,
	.readdir	=	NULL,
	.poll		=	NULL,
	.ioctl	=	NULL,
	.mmap		=	NULL,
	.open		=	dev_open,
	.flush	=	NULL,
	.release	=	dev_release,
	.fsync	=	NULL,
	.fasync	=	NULL,
	.lock		=	NULL,
	//	.readv	=	NULL,
	//	.writev	=	NULL,
};



// initialize module
static int __init dev_init_module (void)
{
	major_number = register_chrdev( 0, DEV_NAME, &dev_fops);

	if (major_number < 0)
	{
	  printk(KERN_ALERT "Registering char device %s failed with %d\n", DEV_NAME, major_number);
	  return major_number;
	}

	printk(KERN_INFO "%s was assigned major number %d. To talk to\n", DEV_NAME, major_number);
	printk(KERN_INFO "the driver, create a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEV_NAME, major_number);

	return SUCCESS;
}

// close and cleanup module
static void __exit dev_cleanup_module (void)
{
	unregister_chrdev(major_number, DEV_NAME);
	printk("%s: dev_cleanup_module Device Driver Removed\n", DEV_NAME);
}

module_init(dev_init_module);
module_exit(dev_cleanup_module);
MODULE_AUTHOR("Morten Mossige, University of Stavanger. Modified by Lars Tomas Lian");
MODULE_DESCRIPTION("Linux devicedriver for tuning air sensors");
MODULE_LICENSE("GPL");
