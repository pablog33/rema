#include "y_axis.h"
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

struct mot_pap y_axis;

/**
 * @brief 	creates the queues, semaphores and endless tasks to handle Y axis movements.
 * @returns	nothing
 */
void y_axis_init()
{
	y_axis.name = "y_axis";
	y_axis.type = MOT_PAP_TYPE_STOP;
	y_axis.last_dir = MOT_PAP_DIRECTION_CW;
	y_axis.half_pulses = 0;
	y_axis.offset = 41230;

	y_axis.gpios.direction = (struct gpio_entry) { 4, 5, SCU_MODE_FUNC0, 2, 5 };	//DOUT1 P4_5 	PIN10  	GPIO2[5]   Y_AXIS_DIR
	y_axis.gpios.step = (struct gpio_entry) { 4, 9, SCU_MODE_FUNC4, 5, 13 };		//DOUT5 P4_9 	PIN33  	GPIO5[13]  Y_AXIS_STEP

	gpio_init_output(y_axis.gpios.direction);
	gpio_init_output(y_axis.gpios.step);

	y_axis.tmr.started = false;
	y_axis.tmr.lpc_timer = LPC_TIMER2;
	y_axis.tmr.rgu_timer_rst = RGU_TIMER2_RST;
	y_axis.tmr.clk_mx_timer = CLK_MX_TIMER2;
	y_axis.tmr.timer_IRQn = TIMER2_IRQn;

	tmr_init(&y_axis.tmr);
}

/**
 * @brief	handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @returns	nothing
 * @note 	calls the supervisor task every x number of generated steps
 */
void TIMER2_IRQHandler(void)
{
	if (tmr_match_pending(&(y_axis.tmr))) {
		mot_pap_isr(&y_axis);
	}
}
