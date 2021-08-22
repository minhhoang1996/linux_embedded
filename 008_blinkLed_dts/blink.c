#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/time.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/of.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>
#include <linux/platform_device.h>
#include<linux/slab.h>
#include<linux/mod_devicetable.h>
#include<linux/of_device.h>

#define GPIO_SETDATAOUT_OFFSET          0x194
#define GPIO_CLEARDATAOUT_OFFSET        0x190
#define GPIO_OE_OFFSET                  0x134
#define LED                             ~(1 << 14)
#define DATA_OUT			(1 << 14)

void __iomem *base_addr;
struct timer_list my_timer;
unsigned int count = 0;

static void timer_callback(struct timer_list *data){
	if ((count % 2) == 0) 
		writel_relaxed(DATA_OUT,  base_addr + GPIO_SETDATAOUT_OFFSET);
	else
		writel_relaxed(DATA_OUT, base_addr + GPIO_CLEARDATAOUT_OFFSET); 

	count++;

	mod_timer(&my_timer,jiffies + msecs_to_jiffies(1000));
}

static const struct of_device_id led_dt_match[] = {
	{ .compatible = "led_blink", },
	{}
};

int led_platform_driver_remove(struct platform_device *pdev)
{

	dev_info(&pdev->dev,"A device is removed\n");
	return 0;
}

int led_platform_driver_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res = NULL;
	uint32_t reg_data = 0;
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	
	base_addr = ioremap(res->start,  (res->end - res->start));
	if(base_addr == NULL) {
		printk("error ioremap control mode\n");
		return -1;
	}

	reg_data = readl_relaxed(base_addr + GPIO_OE_OFFSET);
	reg_data &= LED;
	writel_relaxed(reg_data, base_addr + GPIO_OE_OFFSET);
	
	
	timer_setup(&my_timer, timer_callback, 0);
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000));
	dev_info(dev,"Probe was successful\n");

	return 0;

}

struct platform_driver led_platform_driver = 
{
	.probe = led_platform_driver_probe,
	.remove = led_platform_driver_remove,
	.driver = {
		.name = "led-blink-device",
		.of_match_table = of_match_ptr(led_dt_match)
	}
};

module_platform_driver(led_platform_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Blink Led kernel module");

#if 0
int init_module(void)
{
	uint32_t reg_data = 0;

	base_addr = ioremap(GPIO_ADDR_BASE, ADDR_SIZE);

	reg_data = readl_relaxed(base_addr + GPIO_OE_OFFSET);
	reg_data &= LED;
	writel_relaxed(reg_data, base_addr + GPIO_OE_OFFSET);
	
	timer_setup(&my_timer, timer_callback, 0);
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000));

	return 0;
}

void cleanup_module(void)
{
	del_timer(&my_timer);
}
#endif