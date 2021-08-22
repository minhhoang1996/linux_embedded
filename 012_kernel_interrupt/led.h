
#define NUMBER_OF_LED 3

#define CONTROL_MODULE_BASE 0x44E10000
#define MODE7				0x27

#define ADDR_SIZE       (0x1000)
#define GPIO_SETDATAOUT_OFFSET          0x194
#define GPIO_CLEARDATAOUT_OFFSET        0x190
#define GPIO_DATAOUT            		0x13C
#define GPIO_DATAIN             		0x138
#define GPIO_OE_OFFSET                  0x134
#define mem_size        				4096 
volatile void __iomem *control_base;

#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)
#define REG_CURRENT_TASK _IOW('a','c',int32_t*)


struct led_device_data {
	volatile void __iomem *gpio_base;
	struct cdev led_cdev;
	u32 gpio_addr;
	u32 mux_offset;
	u32 data_out;
	char *label;
	struct timer_list my_timer;
	int timeout;
	int count;
};

struct led_driver_data{
	
	int total_devices;
	dev_t device_number;
	struct class *class_led;
	struct device *device_led;
	struct led_device_data leds_data[NUMBER_OF_LED];
};
