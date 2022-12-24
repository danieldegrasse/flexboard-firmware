/*
 * Copyright 2022 Daniel DeGrasse <daniel@degrasse.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led.h>

#include "led_matrix.h"

#define HW_ROW_COUNT 12
#define HW_COL_COUNT 16
#define SW_ROW_COUNT 8
#define SW_COL_COUNT 6


struct led_matrix mat1 = {
	.led_dev = DEVICE_DT_GET(DT_NODELABEL(led_ctrl1)),
	.hw_col = HW_COL_COUNT,
	.hw_row = HW_ROW_COUNT,
	.sw_col = SW_COL_COUNT,
	.sw_row = SW_ROW_COUNT,
};

struct led_matrix mat2 = {
	.led_dev = DEVICE_DT_GET(DT_NODELABEL(led_ctrl2)),
	.hw_col = HW_COL_COUNT,
	.hw_row = HW_ROW_COUNT,
	.sw_col = SW_COL_COUNT,
	.sw_row = SW_ROW_COUNT,
};

int led_on_off(struct led_matrix *inst)
{
	int ret;
	uint8_t row, col;

	/* Phase 1: turn on all LEDs in sequence */
	printk("%s: Enable LEDs in sequence\n", __func__);
	for (row = 0; row < SW_ROW_COUNT; row++) {
		for (col = 0; col < SW_COL_COUNT; col++) {
			ret = led_matrix_on(inst, row, col);
			if (ret< 0) {
				printk("Error: could not enable led "
					"at [%d, %d]: (%d)\n",
					row, col, ret);
				return ret;
			}
			k_msleep(100);
		}
	}
	k_msleep(500);
	printk("%s: Disable LEDs in sequence\n", __func__);
	/* Phase 2: turn off all LEDs in sequence */
	for (row = 0; row < SW_ROW_COUNT; row++) {
		for (col = 0; col < SW_COL_COUNT; col++) {
			ret = led_matrix_off(inst, row, col);
			if (ret< 0) {
				printk("Error: could not disable led "
					"at [%d, %d]: (%d)\n",
					row, col, ret);
				return ret;
			}
			k_msleep(100);
		}
	}
	k_msleep(500);
	printk("%s: Toggle each led\n", __func__);
	/* Phase 3: turn on each led for a short duration */
	for (row = 0; row < SW_ROW_COUNT; row++) {
		for (col = 0; col < SW_COL_COUNT; col++) {
			ret = led_matrix_on(inst, row, col);
			if (ret< 0) {
				printk("Error: could not enable led "
					"at [%d, %d]: (%d)\n",
					row, col, ret);
				return ret;
			}
			k_msleep(100);
			ret = led_matrix_off(inst, row, col);
			if (ret< 0) {
				printk("Error: could not disable led "
					"at [%d, %d]: (%d)\n",
					row, col, ret);
				return ret;
			}
		}
	}
	k_msleep(500);
	return 0;
}

int led_brightness(struct led_matrix *inst)
{
	int ret;
	uint8_t row, col;

	/* Phase 1: Set LED brightness to low value sequentially */
	printk("%s: Dim LEDs in sequence\n", __func__);
	for (row = 0; row < SW_ROW_COUNT; row++) {
		for (col = 0; col < SW_COL_COUNT; col++) {
			ret = led_matrix_set_brightness(inst, row, col, 0x10);
			if (ret< 0) {
				printk("Error: could not enable led "
					"at [%d, %d]: (%d)\n",
					row, col, ret);
				return ret;
			}
			k_msleep(100);
		}
	}
	/* Phase 2: Raise LED brightness sequentially */
	printk("%s: Restore LED brightness in sequence\n", __func__);
	for (row = 0; row < SW_ROW_COUNT; row++) {
		for (col = 0; col < SW_COL_COUNT; col++) {
			ret = led_matrix_set_brightness(inst, row, col, 0xFF);
			if (ret< 0) {
				printk("Error: could not enable led "
					"at [%d, %d]: (%d)\n",
					row, col, ret);
				return ret;
			}
			k_msleep(100);
		}
	}
	/* Phase 3: Clear LED brightness sequentially */
	printk("%s: Clear LED brightness in sequence\n", __func__);
	for (row = 0; row < SW_ROW_COUNT; row++) {
		for (col = 0; col < SW_COL_COUNT; col++) {
			ret = led_matrix_set_brightness(inst, row, col, 0x0);
			if (ret< 0) {
				printk("Error: could not enable led "
					"at [%d, %d]: (%d)\n",
					row, col, ret);
				return ret;
			}
			k_msleep(100);
		}
	}
	return 0;
}

uint8_t led_state[HW_COL_COUNT * HW_ROW_COUNT];

int led_channel_write(struct led_matrix *inst)
{
	int ret;
	uint32_t led_idx;

	/* Phase 1: set all LEDs to full brightness */
	printk("%s: Set all LEDs to full brightness\n", __func__);
	memset(led_state, 0, sizeof(led_state));
	for (uint8_t row = 0; row < SW_ROW_COUNT; row++) {
		for (uint8_t col = 0; col < SW_COL_COUNT; col++) {
			ret = led_matrix_hw_index(inst, row, col, &led_idx);
			if (ret) {
				printk("Error getting LED HW index: (%d)\n", ret);
				return ret;
			}
			led_state[led_idx] = 0xFF;
		}
	}
	ret = led_write_channels(inst->led_dev, 0, sizeof(led_state),led_state);
	if (ret) {
		printk("Error: could not write LED channels (%d)\n", ret);
		return ret;
	}
	k_msleep(1000);
	/* Phase 2: Dim every other LED */
	printk("%s: Dim alternating LEDs\n", __func__);
	for (uint8_t row = 0; row < SW_ROW_COUNT; row++) {
		for (uint8_t col = 0; col < SW_COL_COUNT; col++) {
			if (col & 0x1) {
				continue;
			}
			ret = led_matrix_hw_index(inst, row, col, &led_idx);
			if (ret) {
				printk("Error getting LED HW index: (%d)\n", ret);
				return ret;
			}
			led_state[led_idx] = 0x60;
		}
	}
	ret = led_write_channels(inst->led_dev, 0, sizeof(led_state),led_state);
	if (ret) {
		printk("Error: could not write LED channels (%d)\n", ret);
		return ret;
	}
	k_msleep(1000);
	/* Phase 3: Disable quadrant of LED display */
	printk("%s: Disable LED quadrant\n", __func__);
	for (uint8_t row = 0; row < SW_ROW_COUNT / 2; row++) {
		for (uint8_t col = 0; col < SW_COL_COUNT / 2; col++) {
			ret = led_matrix_hw_index(inst, row, col, &led_idx);
			if (ret) {
				printk("Error getting LED HW index: (%d)\n", ret);
				return ret;
			}
			led_state[led_idx] = 0x0;
		}
	}
	ret = led_write_channels(inst->led_dev, 0,
		((SW_ROW_COUNT / 2) * HW_COL_COUNT) ,led_state);
	if (ret) {
		printk("Error: could not write LED channels (%d)\n", ret);
		return ret;
	}
	k_msleep(1000);
	return 0;
}

static int led_blink_test(struct led_matrix *inst)
{
	int ret;
	uint64_t delta = k_uptime_get();

	/* Phase 1: blink LEDs with low delay */
	printk("%s: Blink LEDs with low delay \n", __func__);
	for (uint8_t row = 0; row < SW_ROW_COUNT; row++) {
		for (uint8_t col = 0; col < SW_COL_COUNT; col++) {
			ret = led_matrix_blink(inst, row, col, 560, 560);
			if (ret < 0) {
				printk("Error: could not set blink mode: (%d)\n", ret);
				return ret;
			}
		}
	}
	printk("LED blink routine took %llu ms\n", k_uptime_delta(&delta));
	k_msleep(5000);
	return 0;
}

void main(void)
{
	int ret;
	if (!device_is_ready(mat1.led_dev)) {
		printk("LED controller 1 is not ready\n");
		return;
	}
	if (!device_is_ready(mat2.led_dev)) {
		printk("LED controller 2 is not ready\n");
		return;
	}
	while (1) {
		ret = led_blink_test(&mat1);
		if (ret < 0) {
			return;
		}
		ret = led_channel_write(&mat1);
		if (ret < 0) {
			return;
		}
		ret = led_brightness(&mat1);
		if (ret < 0) {
			return;
		}
		ret = led_on_off(&mat1);
		if (ret < 0) {
			return;
		}
		ret = led_blink_test(&mat2);
		if (ret < 0) {
			return;
		}
		ret = led_channel_write(&mat2);
		if (ret < 0) {
			return;
		}
		ret = led_brightness(&mat2);
		if (ret < 0) {
			return;
		}
		ret = led_on_off(&mat2);
		if (ret < 0) {
			return;
		}
	}
}
