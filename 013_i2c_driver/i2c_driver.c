#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/kernel.h>

#define I2C_BUS_AVAILABLE   (          1 )              // I2C Bus available by modify the device-tree
#define SLAVE_DEVICE_NAME   ( "ETX_OLED" )              // Device and Driver Name
#define SSD1306_SLAVE_ADDR  (       0x3C )              // SSD1306 OLED Slave Address

struct ssd1306_data {
	struct i2c_client *ssd1306_client;
	
};

static int I2C_Write(struct i2c_client *client, unsigned char *buf, unsigned int len)
{
    /*
    ** Sending Start condition, Slave address with R/W bit, 
    ** ACK/NACK and Stop condtions will be handled internally.
    */ 
    int ret = i2c_master_send(client, buf, len);
    
    return ret;
}


static int I2C_Read(struct i2c_client *client, unsigned char *out_buf, unsigned int len)
{
    /*
    ** Sending Start condition, Slave address with R/W bit, 
    ** ACK/NACK and Stop condtions will be handled internally.
    */ 
    int ret = i2c_master_recv(client, out_buf, len);
    
    return ret;
}


static void SSD1306_Write(struct i2c_client *client, bool is_cmd, unsigned char data)
{
    unsigned char buf[2] = {0};
    int ret;
    
    if( is_cmd == true )
    {
        buf[0] = 0x00; /*next data byte is command*/
    }
    else
    {
        buf[0] = 0x40; /*next data byte is data*/
    }
    
    buf[1] = data;
    
    ret = I2C_Write(client, buf, 2);
}


static int SSD1306_DisplayInit(struct i2c_client *client)
{
    msleep(100);               // delay

    /*
    ** Commands to initialize the SSD_1306 OLED Display
    */
    SSD1306_Write(client, true, 0xAE); // Entire Display OFF
    SSD1306_Write(client, true, 0xD5); // Set Display Clock Divide Ratio and Oscillator Frequency
    SSD1306_Write(client, true, 0x80); // Default Setting for Display Clock Divide Ratio and Oscillator Frequency that is recommended
    SSD1306_Write(client, true, 0xA8); // Set Multiplex Ratio
    SSD1306_Write(client, true, 0x3F); // 64 COM lines
    SSD1306_Write(client, true, 0xD3); // Set display offset
    SSD1306_Write(client, true, 0x00); // 0 offset
    SSD1306_Write(client, true, 0x40); // Set first line as the start line of the display
    SSD1306_Write(client, true, 0x8D); // Charge pump
    SSD1306_Write(client, true, 0x14); // Enable charge dump during display on
    SSD1306_Write(client, true, 0x20); // Set memory addressing mode
    SSD1306_Write(client, true, 0x00); // Horizontal addressing mode
    SSD1306_Write(client, true, 0xA1); // Set segment remap with column address 127 mapped to segment 0
    SSD1306_Write(client, true, 0xC8); // Set com output scan direction, scan from com63 to com 0
    SSD1306_Write(client, true, 0xDA); // Set com pins hardware configuration
    SSD1306_Write(client, true, 0x12); // Alternative com pin configuration, disable com left/right remap
    SSD1306_Write(client, true, 0x81); // Set contrast control
    SSD1306_Write(client, true, 0x80); // Set Contrast to 128
    SSD1306_Write(client, true, 0xD9); // Set pre-charge period
    SSD1306_Write(client, true, 0xF1); // Phase 1 period of 15 DCLK, Phase 2 period of 1 DCLK
    SSD1306_Write(client, true, 0xDB); // Set Vcomh deselect level
    SSD1306_Write(client, true, 0x20); // Vcomh deselect level ~ 0.77 Vcc
    SSD1306_Write(client, true, 0xA4); // Entire display ON, resume to RAM content display
    SSD1306_Write(client, true, 0xA6); // Set Display in Normal Mode, 1 = ON, 0 = OFF
    SSD1306_Write(client, true, 0x2E); // Deactivate scroll
    SSD1306_Write(client, true, 0xAF); // Display ON in normal mode
    
    return 0;
}

static void SSD1306_Fill(struct i2c_client *client, unsigned char data)
{
    unsigned int total  = 128 * 8;  // 8 pages x 128 segments x 8 bits of data
    unsigned int i      = 0;
    
    //Fill the Display
    for(i = 0; i < total; i++)
    {
        SSD1306_Write(client, false, data);
    }
}

static int ssd1306_oled_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
    struct ssd1306_data *ssd1306;
	int err;
    
	
	if (!i2c_check_functionality(client->adapter,
		I2C_FUNC_SMBUS_WRITE_BYTE_DATA |
		I2C_FUNC_SMBUS_READ_BYTE_DATA))
		return -EIO;
	
	ssd1306 = kzalloc(sizeof(struct ssd1306_data), GFP_KERNEL);
	if (!ssd1306) {
		err = -ENOMEM;
		goto fail1;
	}
	ssd1306->ssd1306_client = client;
	
	pr_info("Chip found @ 0x%X (%s)\n", client->addr, client->adapter->name);
	
	//fill the OLED with this data
	SSD1306_DisplayInit(client);
    SSD1306_Fill(client, 0xFF);
	
	
    pr_info("OLED Probed!!!\n");
    
	i2c_set_clientdata(client, ssd1306);
    return 0;
	
fail1:
	return err;
}


static int ssd1306_oled_remove(struct i2c_client *client)
{   
	
	struct ssd1306_data *ssd1306 = i2c_get_clientdata(client);
	
	pr_info("Chip found @ 0x%X (%s)\n", ssd1306->ssd1306_client->addr, ssd1306->ssd1306_client->adapter->name);
	
	//fill the OLED with this data
    SSD1306_Fill(ssd1306->ssd1306_client, 0x00);
	
    pr_info("OLED Removed!!!\n");
    return 0;
}


static const struct i2c_device_id ssd1306_oled_id[] = {
        { SLAVE_DEVICE_NAME, 0 },
        { }
};
MODULE_DEVICE_TABLE(i2c, ssd1306_oled_id);

static const struct of_device_id ssd1306_of_table[] = {
	{ .compatible = "solomon,ssd1306" },
	{ }
};
MODULE_DEVICE_TABLE(of, ssd1306_of_table);

static struct i2c_driver ssd1306_oled_driver = {
        .driver = {
            .name   = SLAVE_DEVICE_NAME,
            .owner  = THIS_MODULE,
			.of_match_table = of_match_ptr(ssd1306_of_table),
        },
        .probe          = ssd1306_oled_probe,
        .remove         = ssd1306_oled_remove,
        .id_table       = ssd1306_oled_id,
};

module_i2c_driver(ssd1306_oled_driver);

MODULE_AUTHOR("Hoang Nguyen <hoangson31096@gmail.com>");
MODULE_LICENSE("GPL");
