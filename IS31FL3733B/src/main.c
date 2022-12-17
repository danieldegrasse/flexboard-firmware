/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>

const struct gpio_dt_spec pte4 = {
		.port = DEVICE_DT_GET(DT_NODELABEL(gpioe)),
		.pin = 4,
		.dt_flags = 0,
};
const struct i2c_dt_spec i2c = {
	.bus = DEVICE_DT_GET(DT_NODELABEL(i2c2)),
	.addr = 0x50,
};

int select_page(uint8_t page)
{
	int ret;
	uint8_t page_sel[2];
	/* Unlock page selection register */
	page_sel[0] = 0xFE;
	page_sel[1] = 0xC5;
	ret = i2c_write_dt(&i2c, page_sel, 2);
	if (ret < 0) {
		printk("Error, could not write to function select unlock\n");
		return ret;
	}
	page_sel[1] = 0x0;
	/* Read page selection unlock to confirm it was set */
	ret = i2c_write_read_dt(&i2c, &page_sel[0], 1, &page_sel[1], 1);
	if (ret < 0) {
		printk("Error, could not read function select unlock\n");
		return ret;
	}
	if (page_sel[1] != 0xC5) {
		printk("Error, function select unlock did not accept write\n");
		return -ENOMSG;
	}
	/* Write to function select */
	page_sel[0] = 0xFD;
	page_sel[1] = page;
	ret = i2c_write_dt(&i2c, page_sel, 2);
	if (ret) {
		printk("Error, could not write to function select register\n");
		return ret;
	}
	printk("Selected page %d\n", page);
	return ret;
}

void is31fl3733_init(void)
{
	int ret;
	k_msleep(500);

	if (!device_is_ready(pte4.port)) {
		printk("Error, PTE is not ready\n");
		return;
	}
	if (!device_is_ready(i2c.bus)) {
		printk("Error, I2C bus is not ready\n");
		return;
	}

	ret = gpio_pin_configure_dt(&pte4, GPIO_OUTPUT);
	if (ret < 0) {
		printk("Couldn't configure PTE4 (%d)\n", ret);
		return;
	}

	ret = gpio_pin_set_dt(&pte4, 1);
	if (ret < 0) {
		printk("Couldn't set PTE4 high (%d)\n", ret);
		return;
	}
	printk("Set PTE4 (SDB) to logic 1\n");

	ret = select_page(3);
	if (ret) {
		printk("Could not select page 3\n");
		return;
	}
	/* Set chip to normal operation mode, exit software shutdown */
	ret = i2c_reg_write_byte_dt(&i2c, 0x0, 0x1);
	if (ret) {
		printk("Could not set to normal operation mode (%d)\n", ret);
	}
	printk("Set normal operation mode\n");
	/* Set global current control to max value */
	ret = i2c_reg_write_byte_dt(&i2c, 0x1, 0xFF);
	if (ret) {
		printk("Could not set global current control register (%d)\n", ret);
	}
	printk("Set global current control register\n");
	/* Set PWM for CS1, SW1 to max value */
	ret = select_page(1);
	if (ret) {
		printk("Could not select page 1 (%d)\n", ret);
		return;
	}
	ret = i2c_reg_write_byte_dt(&i2c, 0x0, 0xFF);
	if (ret) {
		printk("Could not set PWM for SW1 CS1 (%d)\n", ret);
		return;
	}
	printk("Set PWM for SW1 CS1 to max\n");
	/* Set PWM for CS2, SW1; CS2, SW2, and CS1,SW2 to reduced values */
	i2c_reg_write_byte_dt(&i2c, 0x1, 0x03); /* CS2, SW1 */
	i2c_reg_write_byte_dt(&i2c, 0x10, 0x08); /* CS1, SW2 */
	i2c_reg_write_byte_dt(&i2c, 0x11, 0x40); /* CS2, SW2 */
	ret = select_page(0);
	if (ret) {
		printk("Could not select page 0 (%d)\n", ret);
		return;
	}
	while (1) {
		/* Enable CS1, SW1 led */
		ret = i2c_reg_write_byte_dt(&i2c, 0x0, 0x1);
		if (ret) {
			printk("Could not enable SW1 CS1 (%d)\n", ret);
		}
		printk("Set SW1 CS1\n");
		k_msleep(2000);
		/* Enable all leds */
		i2c_reg_write_byte_dt(&i2c, 0x0, 0x3); /* CS2, SW1; CS1, SW1 */
		i2c_reg_write_byte_dt(&i2c, 0x2, 0x3); /* CS1, SW2; CS2, SW2 */
		printk("Enabled all leds\n");
		k_msleep(2000);
		/* Disable CS1, SW2; CS2, SW2 */
		i2c_reg_write_byte_dt(&i2c, 0x2, 0x0);
	}
}
const struct device *led_dev1 = DEVICE_DT_GET(DT_NODELABEL(led_ctrl1));
const struct device *led_dev2 = DEVICE_DT_GET(DT_NODELABEL(led_ctrl2));

int led_on_off_test() {
	int ret = 0;
	/* Phase 1- enable all LEDs in sequence */
	for (uint8_t i = 0; i < 48; i++) {
		ret = led_on(led_dev1, i);
		if (ret) {
			printk("Error: could not enable LED%u: (%d)\n",
				i, ret);
			return ret;

		}
		ret = led_on(led_dev2, i);
		if (ret) {
			printk("Error: could not enable LED%u: (%d)\n",
				i, ret);
			return ret;
		}
		k_msleep(100);
	}
	k_msleep(500);
	/* Phase 2- disable all LEDs in sequence */
	for (uint8_t i = 0; i < 48; i++) {
		ret = led_off(led_dev1, i);
		if (ret) {
			printk("Error: could not disable LED%u: (%d)\n",
				i, ret);
			return ret;
		}
		ret = led_off(led_dev2, i);
		if (ret) {
			printk("Error: could not disable LED%u: (%d)\n",
				i, ret);
			return ret;
		}
		k_msleep(100);
	}
	/* Phase 3- enable each LED individually */
	for (uint8_t i = 0; i < 48; i++) {
		ret = led_on(led_dev1, i);
		if (ret) {
			printk("Error: could not enable LED%u: (%d)\n",
				i, ret);
			return ret;
		}
		ret = led_on(led_dev2, i);
		if (ret) {
			printk("Error: could not enable LED%u: (%d)\n",
				i, ret);
			return ret;
		}
		k_msleep(100);
		ret = led_off(led_dev1, i);
		if (ret) {
			printk("Error: could not disable LED%u: (%d)\n",
				i, ret);
			return ret;
		}
		ret = led_off(led_dev2, i);
		if (ret) {
			printk("Error: could not disable LED%u: (%d)\n",
				i, ret);
			return ret;
		}
	}
	k_msleep(500);
	return ret;
}

int led_set_channels_test(void)
{
	int ret;
	uint8_t channels[48];
	uint8_t i;

	/* Start by setting PWM for all channels to half */
	for (i = 0; i < 48; i++) {
		channels[i] = 0xFF;
	}
	ret = led_write_channels(led_dev1, 0, 48, channels);
	if (ret) {
		printk("Error: could not set LED channels (%d)\n", ret);
		return ret;
	}
	/* Verify that out of bound writes fail */
	ret = led_write_channels(led_dev1, 10, 48, channels);
	if (ret == 0) {
		printk("Error: LED channel setup should fail with OOB write\n");
	} else {
		printk("Fail: LED write channels OOB: (%d)\n", ret);
		ret = 0;
	}
	k_msleep(500);
	/* Try turning every other LED off */
	for (i = 0; i < 48; i++) {
		if (i & 0x1) {
			continue;
		}
		channels[i] = 0x00;
	}
	ret = led_write_channels(led_dev1, 0, 48, channels);
	if (ret < 0) {
		printk("Error: Could not write to led channels: (%d)\n", ret);
		return ret;
	}
	k_msleep(500);
	/* Try enabling a subset of all LEDs */
	for (i = 0; i < 48; i++) {
		channels[i] = 0xFF;
	}
	ret = led_write_channels(led_dev1, 24, 24, channels);
	if (ret < 0) {
		printk("Error: could not write to led channels: (%d)\n", ret);
	}
	k_msleep(500);
	/* Enable all LED channels */
	ret = led_write_channels(led_dev1, 0, 48, channels);
	if (ret < 0) {
		printk("Error: could not write to led channels: (%d)\n", ret);
	}
	k_msleep(500);
	return ret;
}

void main(void)
{
	int ret;

	if (!device_is_ready(led_dev1) || !device_is_ready(led_dev2)) {
		printk("Error: Led device not ready\n");
		return;
	}

	while (1) {
		ret = led_set_channels_test();
		if (ret < 0) {
			return;
		}
		ret = led_on_off_test();
		if (ret < 0) {
			return;
		}
	}

}
