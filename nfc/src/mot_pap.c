#include "mot_pap.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"

#include "misc.h"
#include "debug.h"
#include "relay.h"
#include "tmr.h"

extern bool stall_detection;
extern int count_a;
extern bool x_zs;

SemaphoreHandle_t mot_pap_supervisor_semaphore;

// Frequencies expressed in Khz
static const uint32_t mot_pap_free_run_freqs[] = { 0, 5, 15, 25, 50, 75, 100,
		125 };

#define MOT_PAP_TASK_PRIORITY ( configMAX_PRIORITIES - 1 )
#define MOT_PAP_SUPERVISOR_TASK_PRIORITY ( configMAX_PRIORITIES - 3)

QueueHandle_t mot_pap_supervisor_task_queue = NULL;

void mot_pap_init()
{
	mot_pap_supervisor_task_queue = xQueueCreate(1, sizeof(struct mot_pap*));

	if (mot_pap_supervisor_task_queue != NULL) {
		// Create the 'handler' task, which is the task to which interrupt processing is deferred
		xTaskCreate(mot_pap_supervisor_task, "mot_pap", 2048,
		NULL, MOT_PAP_SUPERVISOR_TASK_PRIORITY, NULL);
		lDebug(Info, "supervisor task created");
	}

}

/**
 * @brief	returns the direction of movement depending if the error is positive or negative
 * @param 	error : the current position error in closed loop positioning
 * @returns	MOT_PAP_DIRECTION_CW if error is positive
 * @returns	MOT_PAP_DIRECTION_CCW if error is negative
 */
enum mot_pap_direction mot_pap_direction_calculate(int32_t error)
{
	return error < 0 ? MOT_PAP_DIRECTION_CW : MOT_PAP_DIRECTION_CCW;
}

/**
 * @brief 	checks if the required FREE RUN speed is in the allowed range
 * @param 	speed : the requested speed
 * @returns	true if speed is in the allowed range
 */
bool mot_pap_free_run_speed_ok(uint32_t speed)
{
	return ((speed > 0) && (speed <= MOT_PAP_MAX_SPEED_FREE_RUN));
}

/**
 * @brief	if allowed, starts a free run movement
 * @param 	me			: struct mot_pap pointer
 * @param 	direction	: either MOT_PAP_DIRECTION_CW or MOT_PAP_DIRECTION_CCW
 * @param 	speed		: integer from 0 to 8
 * @returns	nothing
 */
void mot_pap_move_free_run(struct mot_pap *me, enum mot_pap_direction direction,
		int speed)
{
	if (mot_pap_free_run_speed_ok(speed)) {
		me->stalled = false; // If a new command was received, assume we are not stalled
		me->stalled_counter = 0;
		me->already_there = false;

		if ((me->dir != direction) && (me->type != MOT_PAP_TYPE_STOP)) {
			tmr_stop(&(me->tmr));
			vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
		}
		me->type = MOT_PAP_TYPE_FREE_RUNNING;
		me->dir = direction;
		me->uno = me->dir == MOT_PAP_DIRECTION_CW ? 1:-1;
		gpio_set_pin_state(me->gpios.direction, me->dir);
		me->requested_freq = mot_pap_free_run_freqs[speed] * 1000;

		tmr_stop(&(me->tmr));
		tmr_set_freq(&(me->tmr), me->requested_freq);
		tmr_start(&(me->tmr));
		lDebug(Info, "%s: FREE RUN, speed: %i, direction: %s", me->name,
				me->requested_freq,
				me->dir == MOT_PAP_DIRECTION_CW ? "CW" : "CCW");
	} else {
		lDebug(Warn, "%s: chosen speed out of bounds %i", me->name, speed);
	}
}

void mot_pap_move_steps(struct mot_pap *me, enum mot_pap_direction direction,
		uint32_t speed, uint32_t steps, uint32_t step_time,
		uint32_t step_amplitude_divider)
{
	if (mot_pap_free_run_speed_ok(speed)) {
		me->stalled = false; // If a new command was received, assume we are not stalled
		me->stalled_counter = 0;
		me->already_there = false;

		if ((me->dir != direction) && (me->type != MOT_PAP_TYPE_STOP)) {
			tmr_stop(&(me->tmr));
			vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
		}
		me->type = MOT_PAP_TYPE_STEPS;
		me->dir = direction;
		me->uno = me->dir == MOT_PAP_DIRECTION_CW ? 1:-1;
		me->half_steps_curr = 0;
		me->step_time = step_time;
		me->half_steps_requested = steps << 1;
		gpio_set_pin_state(me->gpios.direction, me->dir);
		me->requested_freq = mot_pap_free_run_freqs[speed] * 1000;
		me->freq_delta = me->requested_freq / step_amplitude_divider;
		me->freq_slope_relation_incr_to_decr = 3;
		me->freq_increment = me->freq_slope_relation_incr_to_decr
				* me->freq_delta;
		me->freq_decrement = me->freq_delta;
		me->current_freq = me->freq_delta;
		me->half_steps_to_middle = me->half_steps_requested >> 1;
		me->max_speed_reached = false;
		me->ticks_last_time = xTaskGetTickCount();
		x_zs = false;

		tmr_stop(&(me->tmr));
		tmr_set_freq(&(me->tmr), me->current_freq);
		tmr_start(&(me->tmr));
		lDebug(Info, "%s: STEPS RUN, speed: %u, direction: %s", me->name,
				me->requested_freq,
				me->dir == MOT_PAP_DIRECTION_CW ? "CW" : "CCW");
	} else {
		lDebug(Warn, "%s: chosen speed out of bounds %i", me->name, speed);
	}
}

/**
 * @brief	if allowed, starts a closed loop movement
 * @param 	me			: struct mot_pap pointer
 * @param 	setpoint	: the resolver value to reach
 * @returns	nothing
 */
void mot_pap_move_closed_loop(struct mot_pap *me, uint16_t setpoint)
{
	int32_t error;
	enum mot_pap_direction dir;
	me->stalled = false; // If a new command was received, assume we are not stalled
	me->stalled_counter = 0;

	me->posCmd = setpoint;
	lDebug(Info, "%s: CLOSED_LOOP posCmd: %i posAct: %i", me->name, me->posCmd,
			me->posAct);

	//calculate position error
	error = me->posCmd - me->posAct;
	me->already_there = (abs(error) < MOT_PAP_POS_THRESHOLD);

	if (me->already_there) {
		tmr_stop(&(me->tmr));
		lDebug(Info, "%s: already there", me->name);
	} else {
		dir = mot_pap_direction_calculate(error);
		if ((me->dir != dir) && (me->type != MOT_PAP_TYPE_STOP)) {
			tmr_stop(&(me->tmr));
			vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
		}
		me->type = MOT_PAP_TYPE_CLOSED_LOOP;
		me->dir = dir;
		me->uno = dir == MOT_PAP_DIRECTION_CW ? 1:-1;
		gpio_set_pin_state(me->gpios.direction, me->dir);
		me->requested_freq = MOT_PAP_MAX_FREQ;
		tmr_stop(&(me->tmr));
		tmr_set_freq(&(me->tmr), me->requested_freq);
		tmr_start(&(me->tmr));
	}
}

/**
 * @brief	if there is a movement in process, stops it
 * @param 	me	: struct mot_pap pointer
 * @returns	nothing
 */
void mot_pap_stop(struct mot_pap *me)
{
	me->type = MOT_PAP_TYPE_STOP;
	tmr_stop(&(me->tmr));
	lDebug(Info, "%s: STOP", me->name);
}

/**
 * @brief 	supervise motor movement for stall or position reached in closed loop
 * @param 	me			: struct mot_pap pointer
 * @returns nothing
 * @note	to be called by the deferred interrupt task handler
 */
void mot_pap_supervisor_task()
{
	while (true) {
		struct mot_pap *me;

		if (xQueueReceive(mot_pap_supervisor_task_queue, &me,
		portMAX_DELAY) == pdPASS) {
			if (stall_detection) {
				if (abs(
						(int) (me->posAct - me->last_pos)) < MOT_PAP_STALL_THRESHOLD) {

					me->stalled_counter++;
					if (me->stalled_counter >= MOT_PAP_STALL_MAX_COUNT) {
						me->stalled = true;
						tmr_stop(&(me->tmr));
						relay_main_pwr(0);
						goto end;
					}
				} else {
					me->stalled_counter = 0;
				}
			}

			if (me->stalled) {
				lDebug(Warn, "%s: stalled", me->name);
				goto end;
			}

			if (me->already_there) {
				lDebug(Info, "%s: position reached", me->name);
				goto end;
			}

			int ticks_now = xTaskGetTickCount();

			if ((ticks_now - me->ticks_last_time) > pdMS_TO_TICKS(me->step_time)) {
				bool first_half_passed = false;
				if (me->type == MOT_PAP_TYPE_STEPS)
					first_half_passed = me->half_steps_curr
							> (me->half_steps_to_middle);

				if (me->type == MOT_PAP_TYPE_CLOSED_LOOP)
					first_half_passed =
							(me->dir == MOT_PAP_DIRECTION_CW) ?
									me->posAct > (me->posCmdMiddle) :
									me->posAct < (me->posCmdMiddle);

				if (!me->max_speed_reached && (!first_half_passed) && !x_zs) {
					me->current_freq += (me->freq_delta);
					if (me->current_freq >= me->requested_freq) {
						me->current_freq = me->requested_freq;
						me->max_speed_reached = true;
						if (me->type == MOT_PAP_TYPE_STEPS)
							me->max_speed_reached_distance =
									me->half_steps_curr;

						if (me->type == MOT_PAP_TYPE_CLOSED_LOOP)
							me->max_speed_reached_distance = me->posAct;
					}
				}

				int distance_left;
				if (me->type == MOT_PAP_TYPE_STEPS)
					distance_left = me->half_steps_requested
							- me->half_steps_curr;

				if (me->type == MOT_PAP_TYPE_CLOSED_LOOP)
					distance_left = me->posCmd - me->posAct;

				if ((me->max_speed_reached
						&& distance_left <= me->max_speed_reached_distance)
						|| (!me->max_speed_reached && first_half_passed) || x_zs) {
					me->current_freq -= (me->freq_delta);
					if (me->current_freq <= me->freq_delta) {
						me->current_freq = me->freq_delta;
						if (x_zs){
							tmr_stop(&(me->tmr));
							goto end;
						}
					}

				}
				tmr_stop(&(me->tmr));
				tmr_set_freq(&(me->tmr), me->current_freq);
				tmr_start(&(me->tmr));

				me->ticks_last_time = ticks_now;
			}
		}
		end: ;
	}
}

/**
 * @brief 	function called by the timer ISR to generate the output pulses
 * @param 	me : struct mot_pap pointer
 */
void mot_pap_isr(struct mot_pap *me)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	me->posAct = count_a;

	if (me->type == MOT_PAP_TYPE_STEPS) {
		me->already_there = (me->half_steps_curr >= me->half_steps_requested);
	}

	int error;
	if (me->type == MOT_PAP_TYPE_CLOSED_LOOP) {
		error = me->posCmd - me->posAct;
		me->already_there = (abs((int) error) < MOT_PAP_POS_THRESHOLD);
	}

	if (me->already_there) {
		me->type = MOT_PAP_TYPE_STOP;
		tmr_stop(&(me->tmr));
		xQueueSendFromISR(mot_pap_supervisor_task_queue, &me,
				&xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		goto cont;
	}

	++me->half_steps_curr;

	gpio_toggle(me->gpios.step);

	if (++(me->half_pulses) == MOT_PAP_SUPERVISOR_RATE) {
		me->half_pulses = 0;
		xQueueSendFromISR(mot_pap_supervisor_task_queue, &me,
				&xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}

	cont: me->last_pos = me->posAct;
}

/**
 * @brief 	updates the current position from encoder
 * @param 	me : struct mot_pap pointer
 */

/**
 * @brief	sets axis offset
 * @param 	offset		: RDC position for 0 degrees
 * @returns	nothing
 */
void mot_pap_set_offset(struct mot_pap *me, uint16_t offset)
{
	me->offset = offset;
}

/**
 * @brief	returns status of the X axis task.
 * @returns copy of status structure of the task
 */
struct mot_pap* mot_pap_get_status(struct mot_pap *me)
{
	mot_pap_read_corrected_pos(me);
	return me;
}

JSON_Value* mot_pap_json(struct mot_pap *me)
{
	JSON_Value *ans = json_value_init_object();
	json_object_set_number(json_value_get_object(ans), "posCmd", me->posCmd);
	json_object_set_number(json_value_get_object(ans), "posAct", me->posAct);
	json_object_set_boolean(json_value_get_object(ans), "stalled", me->stalled);
	json_object_set_number(json_value_get_object(ans), "offset", me->offset);
	return ans;
}

