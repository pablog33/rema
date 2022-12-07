#include "x_axis.h"
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

struct mot_pap x_axis;

/**
 * @brief 	creates the queues, semaphores and endless tasks to handle X axis movements.
 * @returns	nothing
 */
void x_axis_init()
{
	x_axis.name = "x_axis";
	x_axis.type = MOT_PAP_TYPE_STOP;
//	x_axis.last_dir = MOT_PAP_DIRECTION_CW;
	x_axis.half_pulses = 0;
	x_axis.offset = 41230;
	x_axis.encoder_count = 0;

	x_axis.gpios.direction = (struct gpio_entry) { 2, 1, SCU_MODE_FUNC4, 5, 1 };	//DOUT0 P2_1 	PIN81  	GPIO5[1]   X_AXIS_DIR
	x_axis.gpios.step = (struct gpio_entry) { 4, 8, SCU_MODE_FUNC4, 5, 12 };		//DOUT4 P4_8 	PIN15  	GPIO5[12]  X_AXIS_STEP

	gpio_init_output(x_axis.gpios.direction);
	gpio_init_output(x_axis.gpios.step);

	x_axis.tmr.started = false;
	x_axis.tmr.lpc_timer = LPC_TIMER1;
	x_axis.tmr.rgu_timer_rst = RGU_TIMER1_RST;
	x_axis.tmr.clk_mx_timer = CLK_MX_TIMER1;
	x_axis.tmr.timer_IRQn = TIMER1_IRQn;

	tmr_init(&x_axis.tmr);
}

/**
 * @brief	handle interrupt from 32-bit timer to generate pulses for the stepper motor drivers
 * @returns	nothing
 */
void TIMER1_IRQHandler(void)
{
	if (tmr_match_pending(&(x_axis.tmr))) {
		mot_pap_isr(&x_axis);
	}
}
