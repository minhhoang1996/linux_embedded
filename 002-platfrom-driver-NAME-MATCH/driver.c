#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>

#include "platform.h"

static int my_pdrv_probe(struct platform_device *pdev){
	struct resource *ret;
	struct my_platform_data *my_data;
	int i;

	pr_info("The probe function was called\n");

	/*extract datas from resource structure embedded in pdev structure*/
	pr_info("The number of resource: %d\n", pdev->num_resources);

	for(i=0; i<pdev->num_resources; i++){
		ret = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if(ret == NULL ){
			pr_info("Failed to get resource\n");
			return -ENODEV;
		}
		pr_info("Extract data from resource %d: \n", i);
		pr_info("name of resource: %s\n", ret->name);
		pr_info("start memory of resource: %02x\n", (unsigned int)ret->start);
		pr_info("end memory of resource:   %02x\n", (unsigned int)ret->end);
		/*
		 * Really use this IO memory, need go though the process
		 * of applying first and then mapping.
		 * 
		 * devm_request_mem_region(...);
		 * add_start = ioreadmap(...);
		 * */
	}

	/*
	 * extract datas from platform_data from platform_device.device.platform_data
	 * use pdev->dev.platform_data to get it or use the kernel-provided funtion:
	 */
	my_data = dev_get_platdata(&pdev->dev);
	pr_info("Get the rest_number from plaform_data: %d\n", my_data->reset_number);
	pr_info("Get the led_gpio from plaform_data:    %d\n", my_data->led_gpio);

	return 0;
}

static int my_pdrv_remove(struct platform_device *pdev){
	pr_info("The remove function was called\n");
	return 0;
}


/*
 * struct platform_driver: register the driver with the platform bus core with
 * dedicated functions.
 * */
struct platform_driver pdrv = {
	.probe 	= my_pdrv_probe,
	.remove	= my_pdrv_remove,
	.driver = {
			.owner 	= THIS_MODULE,
			.name	= "my_platform_name"
	},

};

module_platform_driver(pdrv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hoangson31096@gmail.com");





