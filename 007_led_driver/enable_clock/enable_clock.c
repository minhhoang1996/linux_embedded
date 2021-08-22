#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <net/tcp.h>

#define CM_PER_ADDR 0x44E00000
#define CM_PER_SIZE 0x3FF
#define CM_PER_GPIO1_ADDR   0xAC
#define CM_PER_GPIO2_ADDR   0xB0
#define CM_PER_GPIO3_ADDR   0xB4

#define GPIO_COUNT 3


//A couple of standard descriptions
MODULE_LICENSE("GPL");

static int hello_init(void)
{
    static volatile void* cm_per;
    static volatile unsigned int* cm_per_gpio[GPIO_COUNT];

    static volatile int cm_per_addr[GPIO_COUNT] = {CM_PER_GPIO1_ADDR, CM_PER_GPIO2_ADDR, CM_PER_GPIO3_ADDR};

    static int i = 0;

    printk(KERN_NOTICE "Module2: Initializing module\n");

    cm_per = ioremap(CM_PER_ADDR, CM_PER_SIZE);
        if(!cm_per){
            printk (KERN_ERR "Error: Failed to map GM_PER.\n");
            return -1;  //Break to avoid segfault
        }

    for(i = 0; i < GPIO_COUNT; i++){
        cm_per_gpio[i] = cm_per + cm_per_addr[i];

        //Check if clock is disabled
        if(*cm_per_gpio[i] != 0x2){
        printk(KERN_NOTICE "Enabling clock on GPIO[%d] bank...\n", (i+1));
            *cm_per_gpio[i] = 0x2;  //Enable clock
            //Wait for enabled clock to be set
            while(*cm_per_gpio[i] != 0x2){}
        }

        //Print hex value of clock
        printk(KERN_NOTICE "cm_per_gpio[%d]: %04x\n", (i+1), *(cm_per_gpio[i]));
    }


    return 0;
}

static void hello_exit(void)
{
    printk(KERN_INFO "Module: Exit module.\n"); //Print exit notice and exit without exploding anythin
}

module_init(hello_init);
module_exit(hello_exit);
