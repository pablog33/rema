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

// Frequencies expressed in Khz
static const uint32_t mot_pap_free_run_freqs[] = { 0, 5, 15, 25, 50, 75, 75,
		100, 125 };

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
 * @brief 	supervise motor movement for stall or position reached in closed loop
 * @param 	me			: struct mot_pap pointer
 * @returns nothing
 * @note	to be called by the deferred interrupt task handler
 */
void mot_pap_supervise(struct mot_pap *me)
{
	int32_t error;
	bool already_there;
	enum mot_pap_direction dir;

//	me->posAct = mot_pap_offset_correction(ad2s1210_read_position(me->rdc),
//			me->offset, me->rdc->resolution);

	if (stall_detection) {
		if (abs((int) (me->posAct - me->last_pos)) < MOT_PAP_STALL_THRESHOLD) {

			me->stalled_counter++;
			if (me->stalled_counter >= MOT_PAP_STALL_MAX_COUNT) {
				me->stalled = true;
				tmr_stop(&(me->tmr));
				relay_main_pwr(0);
				lDebug(Warn, "%s: stalled", me->name);
				goto cont;
			}
		} else {
			me->stalled_counter = 0;
		}
	}

	if (me->type == MOT_PAP_TYPE_CLOSED_LOOP) {
		error = me->posCmd - me->posAct;

		if ((abs((int) error) < MOT_PAP_POS_PROXIMITY_THRESHOLD)) {
			me->requested_freq = MOT_PAP_MAX_FREQ / 4;
			tmr_stop(&(me->tmr));
			tmr_set_freq(&(me->tmr), MOT_PAP_MAX_FREQ / 4);
			tmr_start(&(me->tmr));
		}

		already_there = (abs((int) error) < MOT_PAP_POS_THRESHOLD);

		if (already_there) {
			me->already_there = true;
			me->type = MOT_PAP_TYPE_STOP;
			tmr_stop(&(me->tmr));
			lDebug(Info, "%s: position reached", me->name);
		} else {
			dir = mot_pap_direction_calculate(error);
			if (me->dir != dir) {
				tmr_stop(&(me->tmr));
				vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
				me->dir = dir;
				gpio_set_pin_state(me->gpios.direction, me->dir);
				tmr_start(&(me->tmr));
			}
		}
	}

	cont: me->last_pos = me->posAct;
}

/**
 * @brief	if allowed, starts a free run movement
 * @param 	me			: struct mot_pap pointer
 * @param 	direction	: either MOT_PAP_DIRECTION_CW or MOT_PAP_DIRECTION_CCW
 * @param 	speed		: integer from 0 to 8
 * @returns	nothing
 */
void mot_pap_move_free_run(struct mot_pap *me, enum mot_pap_direction direction,
		uint32_t speed)
{
	if (mot_pap_free_run_speed_ok(speed)) {
		if ((me->dir != direction) && (me->type != MOT_PAP_TYPE_STOP)) {
			tmr_stop(&(me->tmr));
			vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
		}
		me->type = MOT_PAP_TYPE_FREE_RUNNING;
		me->dir = direction;
		gpio_set_pin_state(me->gpios.direction, me->dir);
		me->requested_freq = mot_pap_free_run_freqs[speed] * 1000;

		tmr_stop(&(me->tmr));
		tmr_set_freq(&(me->tmr), me->requested_freq);
		tmr_start(&(me->tmr));
		lDebug(Info, "%s: FREE RUN, speed: %u, direction: %s", me->name,
				me->requested_freq,
				me->dir == MOT_PAP_DIRECTION_CW ? "CW" : "CCW");
	} else {
		lDebug(Warn, "%s: chosen speed out of bounds %u", me->name, speed);
	}
}

void mot_pap_move_steps(struct mot_pap *me, enum mot_pap_direction direction,
		uint32_t speed, uint32_t steps)
{
	if (mot_pap_free_run_speed_ok(speed)) {
		if ((me->dir != direction) && (me->type != MOT_PAP_TYPE_STOP)) {
			tmr_stop(&(me->tmr));
			vTaskDelay(pdMS_TO_TICKS(MOT_PAP_DIRECTION_CHANGE_DELAY_MS));
		}
		me->type = MOT_PAP_TYPE_STEPS;
		me->dir = direction;
		me->half_steps_left = steps << 1;
		me->half_steps_requested = steps << 1;
		gpio_set_pin_state(me->gpios.direction, me->dir);
		me->requested_freq = mot_pap_free_run_freqs[speed] * 1000;
		me->freq_increment = me->requested_freq / 50;
		me->current_freq = me->freq_increment;
		me->steps_half_way = steps;
		me->flat_reached = false;
		me->ticks_last_time = xTaskGetTickCount();

		tmr_stop(&(me->tmr));
		tmr_set_freq(&(me->tmr), me->current_freq);
		tmr_start(&(me->tmr));
		lDebug(Info, "%s: STEPS RUN, speed: %u, direction: %s", me->name,
				me->requested_freq,
				me->dir == MOT_PAP_DIRECTION_CW ? "CW" : "CCW");
	} else {
		lDebug(Warn, "%s: chosen speed out of bounds %u", me->name, speed);
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
	bool already_there;
	enum mot_pap_direction dir;

	me->posCmd = setpoint;
	lDebug(Info, "%s: CLOSED_LOOP posCmd: %u posAct: %u", me->name, me->posCmd,
			me->posAct);

	//calculate position error
	error = me->posCmd - me->posAct;
	already_there = (abs(error) < MOT_PAP_POS_THRESHOLD);

	if (already_there) {
		me->already_there = true;
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
 * @brief 	function called by the timer ISR to generate the output pulses
 * @param 	me : struct mot_pap pointer
 */
void mot_pap_isr(struct mot_pap *me)
{
	int ticks_now = xTaskGetTickCountFromISR();
	BaseType_t xHigherPriorityTaskWoken;

	if (me->dir != me->last_dir) {
		me->half_pulses = 0;
		me->last_dir = me->dir;
	}

	if (me->type == MOT_PAP_TYPE_STEPS) {
		if (me->half_steps_left <= 0) {
			me->already_there = true;
			me->type = MOT_PAP_TYPE_STOP;
			tmr_stop(&(me->tmr));
			return;
		}

		if (me->half_steps_left < (me->steps_half_way)) {
			volatile int i = 0;
			i++;
		}

		if ((ticks_now - me->ticks_last_time) > pdMS_TO_TICKS(50)) {
			if (!me->flat_reached
					&& (me->half_steps_left > (me->steps_half_way))) {
				me->current_freq += (me->freq_increment);
				if (me->current_freq >= MOT_PAP_MAX_FREQ) {
					me->current_freq = MOT_PAP_MAX_FREQ;
					me->flat_reached = true;
					me->flat_reached_steps = (me->half_steps_requested
							- me->half_steps_left);
				}
			}
			if ((me->flat_reached
					&& me->half_steps_left <= me->flat_reached_steps)
					|| (!me->flat_reached && (me->half_steps_left < (me->steps_half_way)))) {
				me->current_freq -= (me->freq_increment);
				if (me->current_freq <= me->freq_increment) {
					me->current_freq = me->freq_increment;
				}
			}

			tmr_stop(&(me->tmr));
			tmr_set_freq(&(me->tmr), me->current_freq);
			tmr_start(&(me->tmr));
			me->ticks_last_time = ticks_now;
		}

		--me->half_steps_left;
	}

	gpio_toggle(me->gpios.step);

	if (++(me->half_pulses) == MOT_PAP_SUPERVISOR_RATE) {
		me->half_pulses = 0;
		xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(me->supervisor_semaphore,
				&xHigherPriorityTaskWoken);

		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

/**
 * @brief 	updates the current position from RDC
 * @param 	me : struct mot_pap pointer
 */
void mot_pap_update_position(struct mot_pap *me)
{
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

