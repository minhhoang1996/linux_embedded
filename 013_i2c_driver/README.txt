1. Add the pin ctrl for i2c1 node:

       i2c1_pins: pinmux_i2c1_pins {
               pinctrl-single,pins = <
                       AM33XX_PADCONF(AM335X_PIN_SPI0_D1, PIN_INPUT_PULLUP, MUX_MODE2)
                       AM33XX_PADCONF(AM335X_PIN_SPI0_CS0, PIN_INPUT_PULLUP, MUX_MODE2)     
               >;
       };

2. Enable I2C bus node and add the i2c client device node:

&i2c1 {
       pinctrl-names = "default";
       pinctrl-0 = <&i2c1_pins>;
       status = "okay";
       clock-frequency = <100000>;
       ssd1306: ssd1306@3c {
               compatible = "solomon,ssd1306";
               reg = <0x3C>;
               #address-cells = <1>;
               #size-cells = <1>;
       };
};


