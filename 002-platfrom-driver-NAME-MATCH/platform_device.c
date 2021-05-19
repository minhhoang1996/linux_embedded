#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>

#include "platform.h"

/*our platform_data*/
static struct my_platform_data gpio_data = {
	.reset_number 	= 9,
	.led_gpio 	= 20
};

static struct resource my_resource[] = {
	[0] = {
		.start 	= 0x10000000,
		.end 	= 0x20000000,
		.name	= "platform_resource0",
		.flags	= IORESOURCE_MEM
	},
	[1] = {
		.start 	= 0x30000000,
		.end 	= 0x40000000,
		.name	= "platform_resource1",
		.flags	= IORESOURCE_MEM
	},
	[2] = {
		.start 	= 0x50000000,
		.end 	= 0x60000000,
		.name	= "platform_resource3",
		.flags	= IORESOURCE_MEM
	}
};

void device_release(struct device *dev)
{
	pr_info("release was called\n");
}

/*
 * a platform device is represented in the kernel as an instance of 
 * struct platform_device
 * */
static struct platform_device pdev = {
	/*
	 * using matching name:
	 */
	.name 		= "my_platform_name",
	.num_resources 	= ARRAY_SIZE(my_resource),
	.resource	= my_resource,
	.dev		= {

		/*
		 * when we have any other data whose type is not a part of the resource in
		 * the preceding section(my_resource). We should embed that data in a struct 
		 * and put it into platform_data. 
		 * */
				.platform_data	= &gpio_data,
				.release 	= device_release
	}
};

static int __init platform_device_init(void){
	int ret;
	pr_info("register the platform device\n");
	ret = platform_device_register(&pdev);
	if(ret){
		return ret;
	}

	return 0;
}

static void __exit platform_device_exit(void){
	pr_info("Unregister the platform device\n");
	platform_device_unregister(&pdev);

}

module_init(platform_device_init);
module_exit(platform_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hoangson31096@gmail.com");





