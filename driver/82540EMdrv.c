/*
 * Derek Qian
 * 02/22/2013
 * CS 510 - Software Development and Testing
 *
 * Project 4: Driver to read register 0 from 82540EM.
 */

/*
 * need to rmmod e1000 first.
 * modprobe e1000 to make the network work again.
 * command needed:
 *     mknod /dev/82540EMdrv c 250 0
 *     chown derek.derek /dev/82540EMdrv
 */
#include <linux/module.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/usb.h>
#include <linux/slab.h>


#define DEVCNT 1
#define DEVNAME "82540EM"

static int i82540EMdrv_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent);
static void i82540EMdrv_pci_remove(struct pci_dev *pdev);
static int i82540EMdrv_open(struct inode *inode, struct file *file);
static ssize_t i82540EMdrv_read(struct file *file, char __user *buf, size_t len, loff_t *offset);
static ssize_t i82540EMdrv_write(struct file *file, const char __user *buf, size_t len, loff_t *offset);

/* lazy global variable... */
static u8 __iomem *hw_addr;

/* device ID table */
static struct pci_device_id i82540EMdrv_pci_table[] = {
	{ PCI_DEVICE(0x8086, 0x100E) },
	{ } /* terminating entry - this is required */
};
MODULE_DEVICE_TABLE(pci, i82540EMdrv_pci_table);

static struct pci_driver i82540EMdrv_pci_driver = {
	.name = "82540EM",
	.id_table = i82540EMdrv_pci_table,
	.probe = i82540EMdrv_pci_probe,
	.remove = i82540EMdrv_pci_remove,
};


static struct my_dev {
	struct cdev cdev;
	/* more stuff will go in here later... */
	int read_count;
} i82540EMdrv_dev;

/* File operations for our device */
static struct file_operations i82540EMdrv_fops = {
	.owner = THIS_MODULE,
	.open = i82540EMdrv_open,
	.read = i82540EMdrv_read,
	.write = i82540EMdrv_write,
};

static dev_t i82540EMdrv_node;


static int __devinit i82540EMdrv_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	printk(KERN_INFO "i82540EMdrv_pci_probe!\n");
	pci_request_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM), "i82540EMdrv_pci");
	hw_addr = ioremap(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
	//printk(KERN_INFO "i82540EMdrv_pci_probe: 0x%08x\n", readl(hw_addr));

	if (alloc_chrdev_region(&i82540EMdrv_node, 0, DEVCNT, DEVNAME)) {
		printk(KERN_ERR "alloc_chrdev_region() failed!\n");
		return -1;
	}

	printk(KERN_INFO "Allocated %d devices at major: %d\n", DEVCNT, MAJOR(i82540EMdrv_node));

	/* Initialize the character device and add it to the kernel */
	cdev_init(&i82540EMdrv_dev.cdev, &i82540EMdrv_fops);
	i82540EMdrv_dev.cdev.owner = THIS_MODULE;

	if (cdev_add(&i82540EMdrv_dev.cdev, i82540EMdrv_node, DEVCNT)) {
		printk(KERN_ERR "cdev_add() failed!\n");
		/* clean up chrdev allocation */
		unregister_chrdev_region(i82540EMdrv_node, DEVCNT);

		return -1;
	}

	return 0;
}

static void __devexit i82540EMdrv_pci_remove(struct pci_dev *pdev)
{
	/* destroy the cdev */
	cdev_del(&i82540EMdrv_dev.cdev);

	/* clean up the devices */
	unregister_chrdev_region(i82540EMdrv_node, DEVCNT);

	iounmap(hw_addr);
	hw_addr = NULL;
	pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
	printk(KERN_INFO "i82540EMdrv_pci_remove!\n");
	return;
}

static int i82540EMdrv_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "i82540EMdrv_open!\n");
	i82540EMdrv_dev.read_count = 0;
	return 0;
}

static ssize_t i82540EMdrv_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
	/* Get a local kernel buffer set aside */
	int ret;
	unsigned int temp[2] = {0};

	/* Make sure our user wasn't bad... */
	if (!buf || len<sizeof(int)*2) {
		ret = -EINVAL;
		goto out;
	}

	temp[0] = i82540EMdrv_dev.read_count;
	temp[1] = readl(hw_addr + 0x0000);
	if (copy_to_user(buf, temp, sizeof(int)*2)) {
		ret = -EFAULT;
		goto out;
	}
	ret = sizeof(int)*2;

	/* Good to go, so printk the thingy */
	printk(KERN_INFO "i82540EMdrv_read: User got from us %d - 0x%08x\n", temp[0], temp[1]);

out:
	i82540EMdrv_dev.read_count++;
	return ret;
}

static ssize_t i82540EMdrv_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
	/* Have local kernel memory ready */
	char *kern_buf;
	int ret;

	/* Make sure our user isn't bad... */
	if (!buf) {
		ret = -EINVAL;
		goto out;
	}

	/* Get some memory to copy into... */
	kern_buf = kmalloc(len, GFP_KERNEL);

	/* ...and make sure it's good to go */
	if (!kern_buf) {
		ret = -ENOMEM;
		goto out;
	}

	/* Copy from the user-provided buffer */
	if (copy_from_user(kern_buf, buf, len)) {
		/* uh-oh... */
		ret = -EFAULT;
		goto mem_out;
	}

	ret = len;

	/* print what userspace gave us */
	printk(KERN_INFO "i82540EMdrv_write: Userspace wrote \"%s\"!\n", kern_buf);

mem_out:
	kfree(kern_buf);
out:
	return ret;
}

static int __init i82540EMdrv_init(void)
{
	int ret;
	printk(KERN_INFO "i82540EMdrv_init!\n");
	ret = pci_register_driver(&i82540EMdrv_pci_driver);
	return ret;
}

static void __exit i82540EMdrv_exit(void)
{
	pci_unregister_driver(&i82540EMdrv_pci_driver);
	printk(KERN_INFO "i82540EMdrv_exit!\n");
}

MODULE_AUTHOR("Derek Qian");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
module_init(i82540EMdrv_init);
module_exit(i82540EMdrv_exit);
