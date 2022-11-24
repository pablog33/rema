/*
 * encoders.c
 *
 *  Created on: 7 nov. 2022
 *      Author: gspc
 */

#include <stdint.h>
#include "board.h"
#include "encoders.h"
#include "gpio.h"

bool x_zs = 0;


/**
* @brief	Handle interrupt from GPIO pin or GPIO pin mapped to PININT
* @return	Nothing
*/
void GPIO3_IRQHandler(void)
{
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(3));
	x_zs = 1;
}





/**
 * @brief	Main program body
 * @return	Does not return
 */
void ZS_init(void)
{

	gpio_init_input((struct gpio_entry) {4, 3, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0), 2, 3});	//DIN3 P4_3 	PIN07  	GPIO2[3]   X_AXIS_POSITION_SENSOR


	/* Configure interrupt channel for the GPIO pin in SysCon block */
	Chip_SCU_GPIOIntPinSel(3, 2, 3); /* INT3 GPIO2[3]

	/* Configure channel interrupt as edge sensitive and falling edge interrupt */
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(3));
	Chip_PININT_SetPinModeEdge(LPC_GPIO_PIN_INT, PININTCH(3));
	Chip_PININT_EnableIntLow(LPC_GPIO_PIN_INT, PININTCH(3));

	/* Enable interrupt in the NVIC */
	NVIC_ClearPendingIRQ(PIN_INT3_IRQn);
	NVIC_EnableIRQ(PIN_INT3_IRQn);


}
