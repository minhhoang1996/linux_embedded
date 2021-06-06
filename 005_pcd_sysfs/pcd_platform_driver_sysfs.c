#include "pcd_platform_driver_sysfs.h"


/*configuration data of the driver for devices */
struct device_config pcdev_config[] = 
{
	[PCDEVA1X] = {.config_item1 = 60, .config_item2 = 21},
	[PCDEVB1X] = {.config_item1 = 50, .config_item2 = 22},
	[PCDEVC1X] = {.config_item1 = 40, .config_item2 = 23},
	[PCDEVD1X] = {.config_item1 = 30, .config_item2 = 24}
	
};


/*Driver's private data */
struct pcdrv_private_data pcdrv_data;



/* file operations of the driver */
struct file_operations pcd_fops=
{
	.open = pcd_open,
	.release = pcd_release,
	.read = pcd_read,
	.write = pcd_write,
	.llseek = pcd_lseek,
	.owner = THIS_MODULE
};

ssize_t max_size_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	/* Get access to the private data*/
	struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);
	return sprintf(buf, "%d\n", dev_data->pdata.size);
}

ssize_t serial_num_show(struct device *dev, struct device_attribute *attr, char *buf)
{ 
	/* Get access to the private data*/
	struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);
	return sprintf(buf, "%s\n", dev_data->pdata.serial_number);
}

ssize_t max_size_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	long result;
	int ret;
	struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);

	ret = kstrtol(buf, 10, &result);
	if(ret)
		return ret;

	dev_data->pdata.size = result;

	dev_data->buffer = krealloc(dev_data->buffer, dev_data->pdata.size, GFP_KERNEL);

	return count;
}


/* create 2 variables of struct devie_attribute */
static DEVICE_ATTR(max_size, S_IRUGO|S_IWUSR, max_size_show, max_size_store);
static DEVICE_ATTR(serial_num, S_IRUGO, serial_num_show, NULL);

int pcd_sysfs_create_files(struct device *pcd_dev)
{
	int ret;
	ret = sysfs_create_file(&pcd_dev->kobj, &dev_attr_max_size.attr);
	if(ret)
		return ret;
	return sysfs_create_file(&pcd_dev->kobj, &dev_attr_serial_num.attr);
}

/*Called when the device is removed from the system */
int pcd_platform_driver_remove(struct platform_device *pdev)
{
	struct pcdev_private_data  *dev_data = dev_get_drvdata(&pdev->dev);

	/*1. Remove a device that was created with device_create() */
	device_destroy(pcdrv_data.class_pcd,dev_data->dev_num);
	
	/*2. Remove a cdev entry from the system*/
	cdev_del(&dev_data->cdev);

	pcdrv_data.total_devices--;

	dev_info(&pdev->dev,"A device is removed\n");
	return 0;
}

struct pcdev_platform_data* pcdev_get_platdata_from_dt(struct device *dev)
{
	struct device_node *dev_node = dev->of_node;
	struct pcdev_platform_data *pdata;
	if(!dev_node)
		return NULL;
	

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if(!pdata){
		dev_info(dev,"Cannot allocate memory\n");
		return ERR_PTR(-ENOMEM);
	}	
	if(of_property_read_string(dev_node, "org,device-serial-num", &pdata->serial_number)){
		dev_info(dev,"Missing serial number property\n");
		return ERR_PTR(-EINVAL);		
	}
	
	if(of_property_read_u32(dev_node, "org,size", &pdata->size)){
		dev_info(dev,"Missing size property\n");
		return ERR_PTR(-EINVAL);		
	}
	
	if(of_property_read_u32(dev_node, "org,perm", &pdata->perm)){
		dev_info(dev,"Missing permission property\n");
		return ERR_PTR(-EINVAL);		
	}

	return pdata;
};

/*Called when matched platform device is found */
int pcd_platform_driver_probe(struct platform_device *pdev)
{
	int ret;	


	struct pcdev_private_data *dev_data;

	struct pcdev_platform_data *pdata;

	int driver_data;

	dev_info(&pdev->dev,"A device is detected\n");

	pdata = pcdev_get_platdata_from_dt(&pdev->dev);
	
	if(IS_ERR(pdata)){
		return PTR_ERR(pdata);
	}
	
	if(!pdata)
	{
		/*From platform device setup code*/
		pdata = (struct pcdev_platform_data*)dev_get_platdata(&pdev->dev);
		if(!pdata){
			pr_info("No platform data available\n");
			return -EINVAL;
		}
		driver_data = pdev->id_entry->driver_data;
	}else{
		/*From device tree*/
		//match = of_match_device(pdev->dev.driver->of_match_table, &pdev->dev);
		//driver_data = (int)match->data;

		driver_data = (long)of_device_get_match_data(&pdev->dev);
	}

	/*Dynamically allocate memory for the device private data  */
	dev_data = devm_kzalloc(&pdev->dev, sizeof(*dev_data),GFP_KERNEL);
	if(!dev_data){
		pr_info("Cannot allocate memory \n");
		return -ENOMEM;
	}

	/*save the device private data pointer in platform device structure */
	dev_set_drvdata(&pdev->dev,dev_data);

	dev_data->pdata.size = pdata->size;
	dev_data->pdata.perm = pdata->perm;
	dev_data->pdata.serial_number = pdata->serial_number;

	pr_info("Device serial number = %s\n",dev_data->pdata.serial_number);
	pr_info("Device size = %d\n", dev_data->pdata.size);
	pr_info("Device permission = %d\n",dev_data->pdata.perm);

	pr_info("Config item 1 = %d\n",pcdev_config[driver_data].config_item1 );
	pr_info("Config item 2 = %d\n",pcdev_config[driver_data].config_item2 );


	/*Dynamically allocate memory for the device buffer using size 
	information from the platform data */
	dev_data->buffer = devm_kzalloc(&pdev->dev,dev_data->pdata.size,GFP_KERNEL);
	if(!dev_data->buffer){
		pr_info("Cannot allocate memory \n");
		return -ENOMEM;
	}

	/* Get the device number */
	dev_data->dev_num = pcdrv_data.device_num_base + pcdrv_data.total_devices;

	/*Do cdev init and cdev add */
	cdev_init(&dev_data->cdev,&pcd_fops);
	
	dev_data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev_data->cdev,dev_data->dev_num,1);
	if(ret < 0){
		pr_err("Cdev add failed\n");
		return ret;
	}

	/*Create device file for the detected platform device */
	pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd,&pdev->dev,\
			dev_data->dev_num,NULL,"pcdev-%d",pcdrv_data.total_devices);
	if(IS_ERR(pcdrv_data.device_pcd)){
		pr_err("Device create failed\n");
		ret = PTR_ERR(pcdrv_data.device_pcd);
		cdev_del(&dev_data->cdev);
		return ret;
		
	}

	pcdrv_data.total_devices++;

	ret = pcd_sysfs_create_files(pcdrv_data.device_pcd);
	if(ret<0){
		device_destroy(pcdrv_data.class_pcd,dev_data->dev_num);
		return ret;
	}
	
	return 0;

}

struct platform_device_id pcdevs_ids[] = 
{
	[0] = {.name = "pcdev-A1x",.driver_data = PCDEVA1X},
	[1] = {.name = "pcdev-B1x",.driver_data = PCDEVB1X},
	[2] = {.name = "pcdev-C1x",.driver_data = PCDEVC1X},
	[3] = {.name = "pcdev-D1x",.driver_data = PCDEVD1X},
	{ } /*Null termination */
};

struct of_device_id org_pcdev_dt_match[] = {
	{.compatible = "pcdev-A1x", .data = (void*)PCDEVA1X},	
	{.compatible = "pcdev-B1x", .data = (void*)PCDEVB1X},
	{.compatible = "pcdev-C1x", .data = (void*)PCDEVC1X},
	{.compatible = "pcdev-D1x", .data = (void*)PCDEVD1X},
};

struct platform_driver pcd_platform_driver = 
{
	.probe = pcd_platform_driver_probe,
	.remove = pcd_platform_driver_remove,
	.id_table = pcdevs_ids,
	.driver = {
		.name = "pseudo-char-device",
		.of_match_table = org_pcdev_dt_match
	}

};



static int __init pcd_platform_driver_init(void)
{
	int ret;

	/*Dynamically allocate a device number for MAX_DEVICES */
	ret = alloc_chrdev_region(&pcdrv_data.device_num_base,0,MAX_DEVICES,"pcdevs");
	if(ret < 0){
		pr_err("Alloc chrdev failed\n");
		return ret;
	}

	/*Create device class under /sys/class */
	pcdrv_data.class_pcd = class_create(THIS_MODULE,"pcd_class");
	if(IS_ERR(pcdrv_data.class_pcd)){
		pr_err("Class creation failed\n");
		ret = PTR_ERR(pcdrv_data.class_pcd);
		unregister_chrdev_region(pcdrv_data.device_num_base,MAX_DEVICES);
		return ret;
	}

	/*Register a platform driver */
	platform_driver_register(&pcd_platform_driver);
	
	pr_info("pcd platform driver loaded\n");
	
	return 0;

}



static void __exit pcd_platform_driver_cleanup(void)
{
	/*Unregister the platform driver */
	platform_driver_unregister(&pcd_platform_driver);

	/*Class destroy */
	class_destroy(pcdrv_data.class_pcd);

	/*Unregister device numbers for MAX_DEVICES */
	unregister_chrdev_region(pcdrv_data.device_num_base,MAX_DEVICES);
	
	pr_info("pcd platform driver unloaded\n");

}



module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_cleanup);

//module_platform_driver(pcd_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hoang Nguyen");
MODULE_DESCRIPTION("A pseudo character platform driver which handles n platform pcdevs");
