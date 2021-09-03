#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/platform_data/i2c-omap.h>
#include <linux/pm_runtime.h>
#include <linux/pinctrl/consumer.h>

#define ADAPTER_NAME     "DEMO_I2C_ADAPTER"

struct demo_i2c_bus_platform_data {
	u32		clkrate;
	u32		rev;
	u32		flags;
};

struct demo_i2c_dev {
	struct i2c_adapter	adapter;
	struct device		*dev;
	void __iomem		*base;
	u8			*regs;
	int			reg_shift;
	u32			speed;		/* Speed of bus in kHz */
};

enum {
	OMAP_I2C_REV_REG = 0,
	OMAP_I2C_IE_REG,
	OMAP_I2C_STAT_REG,
	OMAP_I2C_IV_REG,
	OMAP_I2C_WE_REG,
	OMAP_I2C_SYSS_REG,
	OMAP_I2C_BUF_REG,
	OMAP_I2C_CNT_REG,
	OMAP_I2C_DATA_REG,
	OMAP_I2C_SYSC_REG,
	OMAP_I2C_CON_REG,
	OMAP_I2C_OA_REG,
	OMAP_I2C_SA_REG,
	OMAP_I2C_PSC_REG,
	OMAP_I2C_SCLL_REG,
	OMAP_I2C_SCLH_REG,
	OMAP_I2C_SYSTEST_REG,
	OMAP_I2C_BUFSTAT_REG,
	/* only on OMAP4430 */
	OMAP_I2C_IP_V2_REVNB_LO,
	OMAP_I2C_IP_V2_REVNB_HI,
	OMAP_I2C_IP_V2_IRQSTATUS_RAW,
	OMAP_I2C_IP_V2_IRQENABLE_SET,
	OMAP_I2C_IP_V2_IRQENABLE_CLR,
};

static const u8 reg_map_ip_v2[] = {
	[OMAP_I2C_REV_REG] = 0x04,
	[OMAP_I2C_IE_REG] = 0x2c,
	[OMAP_I2C_STAT_REG] = 0x28,
	[OMAP_I2C_IV_REG] = 0x34,
	[OMAP_I2C_WE_REG] = 0x34,
	[OMAP_I2C_SYSS_REG] = 0x90,
	[OMAP_I2C_BUF_REG] = 0x94,
	[OMAP_I2C_CNT_REG] = 0x98,
	[OMAP_I2C_DATA_REG] = 0x9c,
	[OMAP_I2C_SYSC_REG] = 0x10,
	[OMAP_I2C_CON_REG] = 0xa4,
	[OMAP_I2C_OA_REG] = 0xa8,
	[OMAP_I2C_SA_REG] = 0xac,
	[OMAP_I2C_PSC_REG] = 0xb0,
	[OMAP_I2C_SCLL_REG] = 0xb4,
	[OMAP_I2C_SCLH_REG] = 0xb8,
	[OMAP_I2C_SYSTEST_REG] = 0xbC,
	[OMAP_I2C_BUFSTAT_REG] = 0xc0,
	[OMAP_I2C_IP_V2_REVNB_LO] = 0x00,
	[OMAP_I2C_IP_V2_REVNB_HI] = 0x04,
	[OMAP_I2C_IP_V2_IRQSTATUS_RAW] = 0x24,
	[OMAP_I2C_IP_V2_IRQENABLE_SET] = 0x2c,
	[OMAP_I2C_IP_V2_IRQENABLE_CLR] = 0x30,
};



static inline void demo_i2c_write_reg(struct demo_i2c_dev *demo, int reg, u16 val)
{
	writew_relaxed(val, demo->base + (demo->regs[reg] << demo->reg_shift));
}

static inline u16 demo_i2c_read_reg(struct demo_i2c_dev *demo, int reg)
{
	return readw_relaxed(demo->base + (demo->regs[reg] << demo->reg_shift));
}

static u32 demo_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C;
}

static int demo_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	int i;
    
    for(i = 0; i < num; i++)
    {
        int j;
        struct i2c_msg *msg_temp = &msgs[i];
        
        pr_info("[Count: %d] [%s]: [Addr = 0x%x] [Len = %d] [Data] = ", i, __func__, msg_temp->addr, msg_temp->len);
        
        for( j = 0; j < msg_temp->len; j++ )
        {
            pr_cont("[0x%02x] ", msg_temp->buf[j]);
        }
    }
    return 0;
}

static int demo_smbus_xfer(struct i2c_adapter *adap, u16 addr,
			  unsigned short flags, char read_write,
			  u8 command, int size, union i2c_smbus_data *data)
{
	return 0;
}

static struct i2c_algorithm demo_i2c_algorithm = {
    .smbus_xfer	    = demo_smbus_xfer,
    .master_xfer    = demo_i2c_xfer,
    .functionality  = demo_func,
};

static struct demo_i2c_bus_platform_data demo_pdata = {
	.rev = 1,
};

static const struct of_device_id demo_i2c_of_match[] = {
	{
		.compatible = "ti,demo-i2c",
		.data = &demo_pdata,
	},
	{}
};
MODULE_DEVICE_TABLE(of, demo_i2c_of_match);

static int demo_i2c_probe(struct platform_device *pdev)
{
	struct demo_i2c_dev *demo_data;
	struct i2c_adapter	*adap;
	struct resource		*mem;
	const struct of_device_id *match;
	const struct demo_i2c_bus_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct device_node	*node = pdev->dev.of_node;
	int r;
	
	demo_data = devm_kzalloc(&pdev->dev, sizeof(struct demo_i2c_dev), GFP_KERNEL);
	if (!demo_data)
		return -ENOMEM;
	
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	demo_data->base = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(demo_data->base))
		return PTR_ERR(demo_data->base);
	
	match = of_match_device(of_match_ptr(demo_i2c_of_match), &pdev->dev);
	if (match) {
		u32 freq = 100000; /* default to 100000 Hz */

		pdata = match->data; /* Get the data from attached of device id structure*/

		of_property_read_u32(node, "clock-frequency", &freq);
		/* convert DT freq value in Hz into kHz for speed */
		demo_data->speed = freq / 1000;
	}
	pr_info("speed of bus = %dHZ\n", demo_data->speed);
	
	demo_data->regs = (u8 *)reg_map_ip_v2;
	
	
	
	
	demo_data->dev = &pdev->dev;
	platform_set_drvdata(pdev, demo_data);
	
	adap = &demo_data->adapter;
	i2c_set_adapdata(adap, demo_data);
	
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_DEPRECATED;
	strlcpy(adap->name, ADAPTER_NAME, sizeof(adap->name));
	adap->algo = &demo_i2c_algorithm;
	adap->nr = pdata->rev; /*i2c-1*/
	adap->dev.parent = &pdev->dev;
	adap->dev.of_node = pdev->dev.of_node;
	pr_info("i2c-%d has been registered\n", adap->nr);
	
	r = i2c_add_numbered_adapter(adap);
	if (r)
		goto err_unuse_clocks;

		
	return 0;
	
err_unuse_clocks:
	return r;
}

static int demo_i2c_remove(struct platform_device *pdev)
{
	struct demo_i2c_dev	*demo_data = platform_get_drvdata(pdev);
	pr_info("i2c-%d has been removed\n", demo_data->adapter.nr);
	i2c_del_adapter(&demo_data->adapter);
	
	return 0;
}

static struct platform_driver demo_i2c_driver = {
	.probe		= demo_i2c_probe,
	.remove		= demo_i2c_remove,
	.driver		= {
		.name	= "demo_i2c",
		.of_match_table = of_match_ptr(demo_i2c_of_match),
	},
};

module_platform_driver(demo_i2c_driver);

MODULE_AUTHOR("Hoang Nguyen <hoangson31096@gmail.com>");
MODULE_DESCRIPTION("I2C bus adapter");
MODULE_LICENSE("GPL");