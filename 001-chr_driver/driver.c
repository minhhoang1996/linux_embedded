
/*1. Reserve a major and arange of minor with alloc_chrdev_region()*/
 

/*2. Create class for your devices with class_create*/

/*3. Set up a struct file_operations(to be given to cdev_init), and for each device you need to create, call cdev_init() and cdev_add() to register the device */


/*4. Create a device_create() for each device, with proper name. It will result in your device being created in the /dev direction*/


#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "eep_ioctl.h"

#define EEP_NBANK 8
#define EEP_DEVICE_NAME "eep-mem"
#define EEP_CLASS "eep-class"
#define BUF_SIZE 1024

struct class *eep_class;
struct device *eep_device;
dev_t dev_num;
char device_buff[BUF_SIZE];


struct ioctl_data kernel_data;

/*device-specific data*/
struct device_data {
	struct cdev eep_cdev;
	char *specific_data;
};

struct device_data eep_data[] = {	
	[0] = {.specific_data="data of device 1."},
	[1] = {.specific_data="data of device 2."},
	[2] = {.specific_data="data of device 3."},
	[3] = {.specific_data="data of device 4."},
	[4] = {.specific_data="data of device 5."},
	[5] = {.specific_data="data of device 6."},
	[6] = {.specific_data="data of device 7."},
	[7] = {.specific_data="data of device 8."}
};


ssize_t eep_read(struct file *file, char __user *buf, size_t size, loff_t *pos){
	pr_info("Request for read %ld bytes from position: %lld\n", size, *pos);

	if(*pos > BUF_SIZE)
		return 0; /*End of file*/
	if( (size + *pos) > BUF_SIZE)
		size = BUF_SIZE - *pos;	

	if(copy_to_user(buf, &device_buff[*pos], size))
		return -EFAULT;

	*pos += size;
	return size;
}

ssize_t eep_write(struct file *file, const char __user *buf, size_t size, loff_t *pos)
{
	pr_info("Request for write %ld bytes to %lld position\n", size, *pos);
	
	if(*pos > size)
		return -EINVAL;
	if( (size + *pos) > BUF_SIZE)
		size = BUF_SIZE - *pos;
	
	if(size == 0)
		return -ENOMEM;

	if(copy_from_user(&device_buff[*pos], buf, size))
		return -EFAULT;

	*pos += size;
	return size;
}

int eep_open(struct inode *inode, struct file *file){
	struct device_data *eep;

	pr_info("Open device with the MIJOR:MINOR %d:%d\n",MAJOR(inode->i_rdev), \
			MINOR(inode->i_rdev));
	eep = kmalloc(sizeof(struct device_data), GFP_KERNEL);

	eep = container_of(inode->i_cdev, struct device_data, eep_cdev);
	pr_info("specific of data: %s\n", eep->specific_data);
	return 0;
}

int eep_release(struct inode *inode, struct file *file){
	pr_info("release called\n");
	return 0;
}

loff_t eep_lseek(struct file *file, loff_t offset, int whence){
	loff_t newpos;
	pr_info("lseek called\n");
	switch(whence){
		case SEEK_SET:
			newpos = offset;
			if(newpos > BUF_SIZE || newpos < 0)
				return -EINVAL;
			break;
		case SEEK_CUR:
			newpos = file->f_pos + offset;
			if(newpos > BUF_SIZE || newpos < 0)
				return -EINVAL;
			break;
		case SEEK_END:
			newpos = BUF_SIZE - offset;
			if(newpos < 0 || newpos > BUF_SIZE)
				return -EINVAL;
			break;
		default:
			return -EINVAL;
	}
	file->f_pos = newpos;
	return newpos;
}

long eep_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	switch(cmd){
		case EEP_WRITE_BUF:
			pr_info("ioctl to write\n");
			if( copy_from_user(&kernel_data, (struct ioctl_data *)arg, sizeof(struct ioctl_data)) != 0){
				pr_info("ioctl write: Err\n");
				return -EFAULT;
			}	
			pr_info("write the value: %d\n", kernel_data.val);
			pr_info("Write the buffer: %s\n", kernel_data.ioctl_buff);
			break;
		case EEP_READ_BUF:
			pr_info("ioctl to read");
			if( copy_to_user((struct ioctl_data *)arg, &kernel_data, sizeof(struct ioctl_data)) != 0){
				pr_info("ioctl read: Err\n");
				return -EFAULT;
			}
			pr_info("Get the value: %d\n", kernel_data.val);
			pr_info("Get the buffer: %s\n", kernel_data.ioctl_buff);
			break;
		default:
			return -ENOTTY;

	}
	return 0;
}

struct file_operations eep_operations  = {
	.owner 		= THIS_MODULE,
	.read 		= eep_read,
	.write 		= eep_write,
	.open 		= eep_open,
	.release 	= eep_release,
	.llseek 	= eep_lseek,
	.unlocked_ioctl = eep_ioctl
};

static int __init my_init(void)
{
	int i;
	int ret;
	dev_t curr_dev;

	/*request the kernel for EEP_NBAK devices*/
	ret = alloc_chrdev_region(&dev_num, 0, EEP_NBANK, EEP_DEVICE_NAME);
	if(ret < 0){
		pr_info("request dev_num failed\n");
		goto out_err;
	}
	
	/*let's create our device's class, visible in /sys/class*/
	eep_class = class_create(THIS_MODULE, EEP_CLASS);
	if(IS_ERR(eep_class)){
		pr_info("class_create() failed for eep_class\n");
		ret = PTR_ERR(eep_class);
		goto out_err1;
	}


	/*Each eeprom bank represented as a char device (cdev)*/
	for(i=0; i<EEP_NBANK; i++)
	{
		/*Tie file_operations to the cdev*/
		cdev_init(&eep_data[i].eep_cdev, &eep_operations);
		eep_data[i].eep_cdev.owner = THIS_MODULE;
		
		
		/*Device number to use to add cdev to the core*/
		curr_dev = MKDEV(MAJOR(dev_num), MINOR(dev_num)+i);

			
		/*Now make the device five for the users to access*/
		ret = cdev_add(&eep_data[i].eep_cdev, curr_dev, 1);
		if(ret < 0){
			pr_info("cdev_add failed\n");
			goto out_err2;
		}

		/*create device node each device /dev/eep-mem...
		 * with our class used here, devices can also be viewed under 
		 * /sys/class/eep-class.
		 * */
		eep_device = device_create(eep_class, NULL, curr_dev,
				NULL, "eep-dev-%d",i);
		if(IS_ERR(eep_device)){
			pr_info("device create failed\n");
			goto out_err2;
		}
	}
	
	pr_info("Module init was successfully\n");
	return 0;

out_err2:
	for(i=0; i<EEP_NBANK;i++){
		device_destroy(eep_class, dev_num+i);
		cdev_del(&eep_data[i].eep_cdev);
	}
out_err1:
	class_destroy(eep_class);
out_err:
	return ret;
}

static void __exit my_exit(void)
{	
	int i;

	pr_info("Module exit was successfully\n");
	for(i=0; i<EEP_NBANK;i++)
	{
		device_destroy(eep_class, dev_num+i);
		cdev_del(&eep_data[i].eep_cdev);
	}
	class_destroy(eep_class);
	unregister_chrdev_region(dev_num, EEP_NBANK);
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");

