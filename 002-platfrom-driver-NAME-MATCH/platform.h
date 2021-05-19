/*
 * Other data than IRQ or memory must be embedded in a structure and
 * passed to "platform_device.device.platform_data"
 */

struct my_platform_data {
        int reset_number;
        int led_gpio;
};

