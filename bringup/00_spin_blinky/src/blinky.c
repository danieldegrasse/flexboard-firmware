/*
 * Copyright 2022, Daniel DeGrasse
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/clock/kinetis_sim.h>
#include <fsl_port.h>


#define DELAY_CYCLES0 10000
#define DELAY_CYCLES1 1000

/*
 * Since it's too early to use Zephyr GPIO APIs, we must extract base addresses
 * for the GPIO and PORT from devicetree directly, and then use NXP HAL APIs.
*/
#define LED_DEV_NODE DT_GPIO_CTLR(DT_ALIAS(led0), gpios)
#define LED_DEV_PORT DT_PHANDLE(LED_DEV_NODE, nxp_kinetis_port)

extern void _debug_loop(void); /* Assembly function  to loop at boot */

/*
 * This function will be called at platform boot. We will use it
 * to blink the LED in a loop.
 */
void z_arm_platform_init(void)
{
	uint32_t pcr;
	volatile uint32_t i, j;

	_debug_loop();

	GPIO_Type *gpio_base = 
		(GPIO_Type *)DT_REG_ADDR(LED_DEV_NODE);
	const uint8_t gpio_pin = DT_GPIO_PIN(DT_ALIAS(led0), gpios);
	PORT_Type *port_base =
		(PORT_Type *)DT_REG_ADDR(LED_DEV_PORT);

	/* Enable PORT clocks */
	CLOCK_EnableClock(CLK_GATE_DEFINE(DT_CLOCKS_CELL(LED_DEV_PORT, offset),
			DT_CLOCKS_CELL(LED_DEV_PORT, bits)));

	/* Set pin mux to GPIO */
	pcr = port_base->PCR[gpio_pin];
	pcr &= ~PORT_PCR_MUX_MASK;
	pcr |= PORT_PCR_MUX(kPORT_MuxAsGpio);

	port_base->PCR[gpio_pin] = pcr;

	/* Set pin to output */
	gpio_base->PDDR |= BIT(gpio_pin);
	/* Toggle GPIO pin in a loop. We will use a busy wait to create a delay */

	while (1) {
		/* Toggle GPIO pin */
		gpio_base->PTOR = BIT(gpio_pin);
		for (i = 0; i < DELAY_CYCLES0; i++) {
			i++;
			for (j = 0; j < DELAY_CYCLES1; j++) {
				j++;
			}
		}
	}
}
