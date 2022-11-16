#include "z_axis.h"
#include "mot_pap.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "board.h"

#include "debug.h"
#include "tmr.h"
#include "gpio.h"

struct mot_pap z_axis;

/**
 * @brief 	creates the queues, semaphores and endless tasks to handle X axis movements.
 * @returns	nothing
 */
void z_axis_init()
{
	z_axis.name = "z_axis";
	z_axis.type = MOT_PAP_TYPE_STOP;
	z_axis.last_dir = MOT_PAP_DIRECTION_CW;
	z_axis.half_pulses = 0;
	z_axis.offset = 41230;

	z_axis.gpios.direction = (struct gpio_entry) { 4, 6, SCU_MODE_FUNC0, 2, 6 };	//DOUT2 P4_6 	PIN08  	GPIO2[6]   Z_AXIS_STEP
	z_axis.gpios.step = (struct gpio_entry) { 4, 10, SCU_MODE_FUNC4, 5, 14 };		//DOUT6 P4_10 	PIN35  	GPIO5[14]  Z_AXIS_STEP

	gpio_init_output(z_axis.gpios.direction);
	gpio_init_output(z_axis.gpios.step);

	z_axis.tmr.started = false;
	z_axis.tmr.lpc_timer = LPC_TIMER3;
	z_axis.tmr.rgu_timer_rst = RGU_TIMER3_RST;
	z_axis.tmr.clk_mx_timer = CLK_MX_TIMER3;
	z_axis.tmr.timer_IRQn = TIMER3_IRQn;

	tmr_init(&z_axis.tmr);
}

/**
 * @brief	handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @returns	nothing
 */
void TIMER3_IRQHandler(void)
{
	if (tmr_match_pending(&(z_axis.tmr))) {
		mot_pap_isr(&z_axis);
	}
}
