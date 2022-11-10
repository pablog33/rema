#ifndef MOT_PAP_H_
#define MOT_PAP_H_

#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "tmr.h"
#include "parson.h"
#include "gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MOT_PAP_MAX_FREQ						125000
#define MOT_PAP_MIN_FREQ						100
#define MOT_PAP_CLOSED_LOOP_FREQ_MULTIPLIER  	( MOT_PAP_MAX_FREQ / 100 )
#define MOT_PAP_MAX_SPEED_FREE_RUN				8
#define MOT_PAP_COMPUMOTOR_MAX_FREQ				300000
#define MOT_PAP_DIRECTION_CHANGE_DELAY_MS		500

#define MOT_PAP_SUPERVISOR_RATE    				2500	//2 means one step
#define MOT_PAP_POS_PROXIMITY_THRESHOLD			100
#define MOT_PAP_POS_THRESHOLD 					6
#define MOT_PAP_STALL_THRESHOLD 				3
#define MOT_PAP_STALL_MAX_COUNT					40

enum mot_pap_direction {
	MOT_PAP_DIRECTION_CW, MOT_PAP_DIRECTION_CCW,
};

enum mot_pap_type {
	MOT_PAP_TYPE_FREE_RUNNING, MOT_PAP_TYPE_CLOSED_LOOP, MOT_PAP_TYPE_STOP, MOT_PAP_TYPE_STEPS
};

/**
 * @struct 	mot_pap_msg
 * @brief	messages to axis tasks.
 */
struct mot_pap_msg {
	enum mot_pap_type type;
	enum mot_pap_direction free_run_direction;
	uint32_t free_run_speed;
	uint16_t closed_loop_setpoint;
	uint32_t steps;
};

/**
 * @struct 	mot_pap_gpios
 * @brief	pointers to functions to handle GPIO lines of this stepper motor.
 */
struct mot_pap_gpios {
	struct gpio_entry direction;
	struct gpio_entry step;
};

/**
 * @struct 	mot_pap
 * @brief	structure for axis motors.
 */
struct mot_pap {
	char *name;
	enum mot_pap_type type;
	enum mot_pap_direction dir;
	uint16_t posCmd;
	uint16_t posAct;
	int32_t requested_freq;
	int32_t freq_increment;
	int32_t current_freq;
	bool stalled;
	bool already_there;
	uint16_t stalled_counter;
	struct ad2s1210 *rdc;
	SemaphoreHandle_t supervisor_semaphore;
	struct mot_pap_gpios gpios;
	struct tmr tmr;
	enum mot_pap_direction last_dir;
	uint16_t last_pos;
	uint32_t half_pulses;			// counts steps from the last call to supervisor task
	uint16_t offset;
	int32_t half_steps_requested;
	int32_t half_steps_left;
	int32_t steps_half_way;
	int32_t flat_reached_steps;
	int32_t ticks_last_time;
	bool flat_reached;
};

uint16_t mot_pap_offset_correction(uint16_t pos, uint16_t offset,
		uint8_t resolution);

void mot_pap_read_corrected_pos(struct mot_pap *me);

void mot_pap_supervise(struct mot_pap *me);

void mot_pap_move_free_run(struct mot_pap *me, enum mot_pap_direction direction,
		uint32_t speed);

void mot_pap_move_closed_loop(struct mot_pap *status, uint16_t setpoint);

void mot_pap_stop(struct mot_pap *me);

void mot_pap_isr(struct mot_pap *me);

void mot_pap_update_position(struct mot_pap *me);
uint32_t mot_pap_read_on_condition(void);

JSON_Value *mot_pap_json(struct mot_pap *me);

#ifdef __cplusplus
}
#endif

#endif /* MOT_PAP_H_ */
