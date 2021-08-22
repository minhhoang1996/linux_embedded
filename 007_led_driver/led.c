#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/ioctl.h>
#include <linux/timer.h>

#include "led.h"

struct led_driver_data led_drv = {
	.total_devices = NUMBER_OF_LED,
	.leds_data = {
		[0] = {
				.gpio_addr = 0x44E07000, 
				.mux_offset = 0x828, 
				.data_out = 26,
				.label = "led_0.26",
				.timeout = 1000,
				.count = 0
				
		},
		
		[1] = {
				.gpio_addr = 0x4804C000, 
				.mux_offset = 0x838, 
				.data_out = 14,
				.label = "led_1.14",
				.timeout = 1000,
				.count = 0
		},
		
		[2] = {
				.gpio_addr = 0x481AC000, 
				.mux_offset = 0x88C, 
				.data_out = 1,
				.label = "led_2.1",
				.timeout = 1000,
				.count = 0
		},
	}
};

ssize_t led_read(struct file *file, char __user *buf, size_t len, loff_t *f_pos){
	u32 tmp;
	char *status;
	size_t ret;
	struct led_device_data *led_data = (struct led_device_data*)file->private_data;

	tmp = readl(led_data->gpio_base+GPIO_DATAOUT);

	if(tmp>>(led_data->data_out)&1)
		status = "ON\n";
	else
		status = "OFF\n";

	len = strlen(status);
	
    ret = copy_to_user(buf, status, len);	
	*f_pos += len - ret;

	if (*f_pos > len)
	{
		return 0;
	}
	else
        return len;
}

ssize_t led_write(struct file *file, const char __user *buf, size_t len, loff_t *off){
	pr_info("Write function\n");
	return len;
}

int led_open(struct inode *inode, struct file *file){
	struct led_device_data *led_data;
	
	led_data = container_of(inode->i_cdev, struct led_device_data, led_cdev);
	file->private_data = led_data;
	return 0;
}

long led_iotcl(struct file *file, unsigned int cmd, unsigned long arg){
	struct led_device_data *led_data = (struct led_device_data*)file->private_data;

	switch(cmd) {
		case WR_VALUE:
			if( copy_from_user(&led_data->timeout ,(int32_t*) arg, sizeof(led_data->timeout)) )
			{
					pr_err("Data Write : Err!\n");
			}
			break;
		case RD_VALUE:
			if( copy_to_user((int32_t*) arg, &led_data->timeout, sizeof(led_data->timeout)) )
			{
					pr_err("Data Read : Err!\n");
			}
			break;
		default:
			pr_info("Default\n");
			break;
	}
	return 0;
}

int led_release(struct inode *inode, struct file *file){

	return 0;
}

static struct file_operations led_fops = {
	.owner 			= THIS_MODULE,
	.read  			= led_read,
	.open			= led_open,
	.write			= led_write,
	.release		= led_release,
	.unlocked_ioctl	= led_iotcl,

};

static void timer_callback(struct timer_list *data)
{
	struct led_device_data *led = from_timer(led, data,my_timer);

	if ((led->count % 2) == 0) 
		writel(1<<(led->data_out), led->gpio_base+GPIO_SETDATAOUT_OFFSET);
	else
		writel(1<<(led->data_out), led->gpio_base+GPIO_CLEARDATAOUT_OFFSET);

	led->count++;

	mod_timer(&led->my_timer,jiffies + msecs_to_jiffies(led->timeout));
}

static int configure_leds(void){
	u32 temp;
	int i;

	/* Pin mux configurations*/
	control_base = ioremap(CONTROL_MODULE_BASE, ADDR_SIZE);
	if(control_base == NULL) {
		printk("error ioremap control mode\n");
		return -1;
	}

	for(i=0;i<NUMBER_OF_LED;i++){
		/*write the muxing configuation MODE7 for LED1*/
		writel(MODE7,control_base + led_drv.leds_data[i].mux_offset);

		/* Set GPIO pin as output*/
		led_drv.leds_data[i].gpio_base = ioremap(led_drv.leds_data[i].gpio_addr, ADDR_SIZE);
		if(led_drv.leds_data[i].gpio_base == NULL) {
			printk("error ioremap control mode\n");
			return -1;
		}
		temp = readl(led_drv.leds_data[i].gpio_base+GPIO_OE_OFFSET);
		temp &= ~(1<<led_drv.leds_data[i].data_out);

		writel(temp,led_drv.leds_data[i].gpio_base+GPIO_OE_OFFSET);

	}

	return 0;
}

static int __init led_init(void)
{
	
	int ret;
	int i;
	
	/*Dynamically allocate device numbers */
	ret = alloc_chrdev_region(&led_drv.device_number,0,NUMBER_OF_LED,"led");
	if(ret < 0){
		pr_err("Alloc chrdev failed\n");
		goto out;
	}

	/*create device class under /sys/class/ */
	led_drv.class_led = class_create(THIS_MODULE,"led_class");
	if(IS_ERR(led_drv.class_led)){
		pr_err("Class creation failed\n");
		ret = PTR_ERR(led_drv.class_led);
		goto unreg_chrdev;
	}

	if(configure_leds()){
		printk("Led configuration failed\n");
		goto out;
	}

	for(i=0;i<NUMBER_OF_LED;i++){
		/* setup timer to call my_timer_callback */
		timer_setup(&led_drv.leds_data[i].my_timer, timer_callback, 0);
		mod_timer(&led_drv.leds_data[i].my_timer, jiffies + msecs_to_jiffies(led_drv.leds_data[i].timeout));
	
		
		/*Initialize the cdev structure with fops*/
		cdev_init(&led_drv.leds_data[i].led_cdev,&led_fops);

		/*  Register a device (cdev structure) with VFS */
		led_drv.leds_data[i].led_cdev.owner = THIS_MODULE;
		ret = cdev_add(&led_drv.leds_data[i].led_cdev,led_drv.device_number+i,1);
		if(ret < 0){
			pr_err("Cdev add failed\n");
			goto cdev_del;
		}

		/*populate the sysfs with device information */
		led_drv.device_led = device_create(led_drv.class_led,NULL,led_drv.device_number+i,NULL,led_drv.leds_data[i].label,NULL);
		if(IS_ERR(led_drv.device_led)){
			pr_err("Device create failed\n");
			ret = PTR_ERR(led_drv.device_led);
			goto class_del;
		}
	}


	pr_info("Module init was successful\n");
	return 0;

class_del:
	for(;i>=0;i--){
		device_destroy(led_drv.class_led,led_drv.device_number+i);
		cdev_del(&led_drv.leds_data[i].led_cdev);
	}
	class_destroy(led_drv.class_led);

cdev_del:
	for(;i>=0;i--){
		device_destroy(led_drv.class_led,led_drv.device_number+i);
		cdev_del(&led_drv.leds_data[i].led_cdev);
	}

unreg_chrdev:
	unregister_chrdev_region(led_drv.device_number,NUMBER_OF_LED);
out:
	pr_info("Module insertion failed\n");
	return ret; 

}

static void __exit led_exit(void)
{	
	int i;
	
	iounmap(control_base);	

	for(i=0;i<NUMBER_OF_LED;i++){
		iounmap(led_drv.leds_data[i].gpio_base);
		del_timer(&led_drv.leds_data[i].my_timer);
		device_destroy(led_drv.class_led,led_drv.device_number+i);
		cdev_del(&led_drv.leds_data[i].led_cdev);
	}
	class_destroy(led_drv.class_led);
	unregister_chrdev_region(led_drv.device_number,NUMBER_OF_LED);
	
	pr_info("Module exit was successfully\n");
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");

