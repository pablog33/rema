/*
 * encoders.c
 *
 *  Created on: 7 nov. 2022
 *      Author: gspc
 */

#include <stdint.h>
#include "board.h"
#include "encoders.h"

int count_a = 0;
int count_b = 0;
int count_z = 0;
int count_isr = 0;

void GINT0_IRQHandler(void)
{
	//static int count_isr = 0;
	static int group = 0;
	static int group_ant = 0;
	udelay(50);
	group = Chip_GPIO_GetPortValue(LPC_GPIO_PORT, 3);

	Chip_GPIOGP_ClearIntStatus(LPC_GPIOGROUP, 0);
	int diff = group ^ group_ant;

	++count_isr;

	if (diff & (1 << 12))
		count_a++;
	if (diff & (1 << 13))
		count_b++;
	if (diff & (1 << 14))
		count_z++;

	group_ant = group;
}

/**
 * @brief	Main program body
 * @return	Does not return
 */
void encoders_init(void)
{
	//Chip_Clock_Enable(CLK_MX_GPIO);
	count_a = 0;
	count_b = 0;
	count_z = 0;

	/* Set pin back to GPIO (on some boards may have been changed to something
	 else by Board_Init()) */
	Chip_SCU_PinMuxSet(7, 4,
			(SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0));

	Chip_SCU_PinMuxSet(7, 5,
			(SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0));

	Chip_SCU_PinMuxSet(7, 6,
			(SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0));

	/* Group GPIO interrupt 0 will be invoked when the button is pressed. */
	Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, 3, 12);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, 3, 13);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, 3, 14);
//	Chip_GPIOGP_SelectLowLevel(LPC_GPIOGROUP, 0, 3,
//				 1 << 12 | 1 << 13 | 1<< 14);
//	Chip_GPIOGP_SelectHighLevel(LPC_GPIOGROUP, 0, 3,
//			1 << 12 | 1 << 13 | 1<< 14);
	Chip_GPIOGP_EnableGroupPins(LPC_GPIOGROUP, 0, 3,
			1 << 12 | 1 << 13 | 1<< 14 );
	Chip_GPIOGP_SelectAndMode(LPC_GPIOGROUP, 0);

	Chip_GPIOGP_SelectEdgeMode(LPC_GPIOGROUP, 0);

	/* Enable Group GPIO interrupt 0 */
	NVIC_EnableIRQ(GINT0_IRQn);

}
