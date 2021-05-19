#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/mod_devicetable.h>

#include "platform.h"

static struct my_driver_data ID_table_match_data[] = {
	[0] = {
		.month  = 4,
		.year	= 2020
	},
	[1] = {
		.month 	= 5,
		.year	= 2021
	}
};

static int my_pdrv_probe_pdata(struct my_driver_data *data, struct platform_device *pdev){
	data = (struct my_driver_data *) pdev->id_entry->driver_data;
	/*Extract data*/
	pr_info("Extract data from matched device: month = %d\n", data->month);
	pr_info("Extract data from matched device: year  = %d\n", data->year);
	return 0;
}

static int my_pdrv_probe(struct platform_device *pdev){
	struct resource *ret;
	struct my_driver_data *data;
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
	
	/* The platform data is converted back into the original data structure */
	my_pdrv_probe_pdata(data, pdev);
	return 0;
}

static int my_pdrv_remove(struct platform_device *pdev){
	pr_info("The remove function was called\n");
	return 0;
}

/*
 * Matching ID table 
 */
static struct platform_device_id my_device_id[] = {
	[0] = {
		.name 		= "device1",
		.driver_data	= (kernel_ulong_t) &ID_table_match_data[0]
	},

	[1] = {
		.name 		= "device2",
		.driver_data	= (kernel_ulong_t) &ID_table_match_data[1]
	},

	[2] = { 
		.name 		= "device3",
	}
};

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
	.id_table = my_device_id

};

			


module_platform_driver(pdrv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hoangson31096@gmail.com");





