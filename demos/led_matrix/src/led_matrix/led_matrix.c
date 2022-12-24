/*
 * Copyright 2022 Daniel DeGrasse <daniel@degrasse.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/led.h>

#define LOG_LEVEL CONFIG_LED_MATRIX_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_matrix);

#include "led_matrix.h"


/* Helper function to toggle LED in matrix */
static int led_matrix_on_off(struct led_matrix *inst,
        uint8_t row, uint8_t col, bool on)
{
        uint32_t led_idx;
        int ret;

        ret = led_matrix_hw_index(inst, row, col, &led_idx);
        if (ret < 0) {
                return ret;
        }

        if (on) {
                return led_on(inst->led_dev, led_idx);
        } else {
                return led_off(inst->led_dev, led_idx);
        }
}

/**
 * Gets hardware index of a given LED. Can be used for direct interaction
 * with LED API.
 * @param inst: led matrix framework instance
 * @param row: led row index
 * @param col: led column index
 * @param idx: filled with LED index value
 * @return 0 on success, or negative value on error
*/
int led_matrix_hw_index(struct led_matrix *inst, uint8_t row,
        uint8_t col, uint32_t *idx)
{
        /* Check arguments */
        if (inst == NULL || idx == NULL ||
                row >= inst->sw_row ||
                col >= inst->sw_col) {
                return -EINVAL;
        }
        /* Calculate hardware index of row and column */
        *idx = (row * inst->hw_col) + col;
        if (*idx > (inst->hw_col * inst->hw_row)) {
                return -EINVAL;
        }
        LOG_DBG("LED at [%d,%d] converted to HW idx %d", row, col, *idx);
        return 0;
}



/**
 * Enables LED in matrix.
 * @param inst: led matrix framework instance
 * @param row: led row index
 * @param col: led column index
 * @return 0 on success, or negative value on error
*/
int led_matrix_on(struct led_matrix *inst, uint8_t row, uint8_t col)
{
        /* Check arguments */
        if (inst == NULL ||
                row >= inst->sw_row ||
                col >= inst->sw_col) {
                return -EINVAL;
        }
        return led_matrix_on_off(inst, row, col, true);
}

/**
 * Disables LED in matrix.
 * @param inst: led matrix framework instance
 * @param row: led row index
 * @param col: led column index
 * @return 0 on success, or negative value on error
*/
int led_matrix_off(struct led_matrix *inst, uint8_t row, uint8_t col)
{
        /* Check arguments */
        if (inst == NULL ||
                row >= inst->sw_row ||
                col >= inst->sw_col) {
                return -EINVAL;
        }
        return led_matrix_on_off(inst, row, col, false);
}

/**
 * Blinks an LED in the matrix
 * @param inst: led matrix framework instance
 * @param row: led row index
 * @param col: led column index
 * @param delay_on: on delay in milliseconds
 * @param delay_off: off delay in milliseconds
 * @return 0 on success, or negative value on error
*/
int led_matrix_blink(struct led_matrix *inst, uint8_t row, uint8_t col,
        uint32_t delay_on, uint32_t delay_off)
{
        int ret;
        uint32_t led_idx;

        /* Check arguments */
        if (inst == NULL ||
                row >= inst->sw_row ||
                col >= inst->sw_col) {
                return -EINVAL;
        }

        ret = led_matrix_hw_index(inst, row, col, &led_idx);
        if (ret < 0) {
                return ret;
        }
        return led_blink(inst->led_dev, led_idx, delay_on, delay_off);
}

/**
 * Sets brightness of LED in matrix
 * @param inst: led matrix framework instance
 * @param row: led row index
 * @param col: led column index
 * @param value: led brightness, 0-255
 * @return 0 on success, or negative value on error
*/
int led_matrix_set_brightness(struct led_matrix *inst, uint8_t row,
        uint8_t col, uint8_t value)
{
        uint32_t led_idx;
        int ret;

        /* Check arguments */
        if (inst == NULL ||
                row >= inst->sw_row ||
                col >= inst->sw_col) {
                return -EINVAL;
        }

        ret = led_matrix_hw_index(inst, row, col, &led_idx);
        if (ret < 0) {
                return ret;
        }
        return led_set_brightness(inst->led_dev, led_idx, value);
}
