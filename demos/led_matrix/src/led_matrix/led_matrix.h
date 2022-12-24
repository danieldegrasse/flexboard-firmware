/*
 * Copyright 2022 Daniel DeGrasse <daniel@degrasse.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LED_MATRIX_H_
#define LED_MATRIX_H_

#include <zephyr/kernel.h>

struct led_matrix {
        const struct device *led_dev; /* Led controller device */
        const uint8_t sw_row; /* Software row count */
        const uint8_t sw_col; /* Software column count */
        const uint8_t hw_row; /* Hardware row count */
        const uint8_t hw_col; /* Hardware column count */
};

/**
 * Enables LED in matrix.
 * @param inst: led matrix framework instance
 * @param row: led row index
 * @param col: led column index
 * @return 0 on success, or negative value on error
*/
int led_matrix_on(struct led_matrix *inst, uint8_t row, uint8_t col);


/**
 * Disables LED in matrix.
 * @param inst: led matrix framework instance
 * @param row: led row index
 * @param col: led column index
 * @return 0 on success, or negative value on error
*/
int led_matrix_off(struct led_matrix *inst, uint8_t row, uint8_t col);


/**
 * Sets brightness of LED in matrix
 * @param inst: led matrix framework instance
 * @param row: led row index
 * @param col: led column index
 * @param value: led brightness, 0-255
 * @return 0 on success, or negative value on error
*/
int led_matrix_set_brightness(struct led_matrix *inst, uint8_t row,
        uint8_t col, uint8_t value);

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
        uint32_t delay_on, uint32_t delay_off);

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
        uint8_t col, uint32_t *idx);


#endif /* LED_MATRIX_H_ */