#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "board.h"
#include "pid.h"
#include "limits.h"
#include "debug.h"

void pid_init(struct pid *this, int kp, int ki, int kd,
		enum pid_controller_direction controller_dir,
		double sample_period_ms, int min_output, int max_output, int min_abs_output)
{
	pid_set_output_limits(this, min_output, max_output, min_abs_output);

	this->sample_period_ms = sample_period_ms;

	pid_set_controller_direction(this, controller_dir);

	// Set tunings with provided constants
	pid_set_tunings(this, kp, ki, kd);
	this->prev_input = 0;
	this->prev_output = 0;

	this->p_term = 0.0;
	this->i_term = 0.0;
	this->d_term = 0.0;
	this->num_times_ran = 0;
}

void pid_restart(struct pid *this, int input) {
	this->prev_input = input;
	this->prev_error = 0;
	this->prev_setpoint = input;
	this->num_times_ran = 1;
}


int pid_run(struct pid *this, int setpoint, int input) {
#define RAMP_RATE	0.01		//Change the setpoint by at most 0.1 per iteration

//	if (this->num_times_ran == 1) {
//		this->setpoint_jump = abs(setpoint - this->prev_setpoint);
//	}
//
//	if (setpoint > 0 && setpoint > (this->prev_setpoint + (this->setpoint_jump * RAMP_RATE))) {
//		setpoint = this->prev_setpoint + (this->setpoint_jump * RAMP_RATE);
//	} else if (setpoint < 0 && setpoint < (this->prev_setpoint - (this->setpoint_jump * RAMP_RATE))){
//		setpoint = this->prev_setpoint - (this->setpoint_jump * RAMP_RATE);
//	}

	int error = (setpoint - input);

	// PROPORTIONAL CALCS
	this->p_term = this->zp * error;

	// INTEGRAL CALCS
	this->i_term += (this->zi * error);

	// Perform min/max bound checking on integral term
	if (this->i_term > this->out_max)
		this->i_term = this->out_max;
	else if (this->i_term < this->out_min)
		this->i_term = this->out_min;

	//===== DERIVATIVE CALS =====//
	// Only calculate derivative if run once or more already.
	//this->input_change = (input - this->prev_input);
	//this->d_term = -this->zd * this->input_change;
	//this->d_term = -this->zd * error;
	int error_change = error - this->prev_error;

	if (error != 0) {
		this->d_term = -this->zd * abs(error_change) / error;
	}

	// Perform min/max bound checking on derivative term
	if (this->d_term > this->out_max)
		this->d_term = this->out_max;
	else if (this->d_term < this->out_min)
		this->d_term = this->out_min;

	lDebug(Info, "p_term: %i", this->p_term);
	lDebug(Info, "                i_term: %i", this->i_term);
	lDebug(Info, "                                d_term: %i", this->d_term);
	this->output = this->p_term + this->i_term + this->d_term;

	// Limit output
	if (this->output > this->out_max)
		this->output = this->out_max;
	else if (this->output < this->out_min)
		this->output = this->out_min;

	// Remember input value to next call
	this->prev_input = input;
	// Remember last output for next call
	this->prev_output = this->output;
	this->prev_error = error;

	this->prev_setpoint = setpoint;

	// Increment the Run() counter, after checking to make sure it hasn't reached
	// max value.
	if (this->num_times_ran < INT_MAX)
		this->num_times_ran++;

	float attenuation = (this->num_times_ran < 10) ? this->num_times_ran * 0.1 : 1;
	int out = this->output * attenuation;
	if (out >= 0 && out < this->out_abs_min) {
		return this->out_abs_min;
	}
	if (out < 0 && out > -this->out_abs_min) {
		return -this->out_abs_min;
	}
	return out;
}

//! @brief		Sets the PID tunings.
//! @warning	Make sure samplePeriodMs is set before calling this funciton.
void pid_set_tunings(struct pid *this, int kp, int ki, int kd) {
	if (kp < 0 || ki < 0 || kd < 0)
		return;

	this->kp = kp;
	this->ki = ki;
	this->kd = kd;

	// Calculate time-step-scaled PID terms
	this->zp = kp;

	// The next bit requires double->int casting functionality.
	this->zi = (ki * this->sample_period_ms) / 1000.0;
	this->zd = (kd * 1000) / this->sample_period_ms;

	if (this->controller_dir == PID_REVERSE) {
		this->zp = (0 - this->zp);
		this->zi = (0 - this->zi);
		this->zd = (0 - this->zd);
	}

	lDebug(Info, "ZP: %f", this->zp);
	lDebug(Info, "ZI: %f", this->zi);
	lDebug(Info, "ZD: %f", this->zd);
}

int pid_get_kp(struct pid *this) {
	return this->kp;
}

int pid_get_ki(struct pid *this) {
	return this->ki;
}

int pid_get_kd(struct pid *this) {
	return this->kd;
}

int pid_get_zp(struct pid *this) {
	return this->zp;
}

int pid_get_zi(struct pid *this) {
	return this->zi;
}

int pid_get_zd(struct pid *this) {
	return this->zd;
}

void pid_set_sample_period(struct pid *this, unsigned int new_sample_period_ms) {
	if (new_sample_period_ms > 0) {
		int ratio = (int) new_sample_period_ms / (double) this->sample_period_ms;
		this->zi *= ratio;
		this->zd /= ratio;
		this->sample_period_ms = new_sample_period_ms;
	}
}

void pid_set_output_limits(struct pid *this, int min, int max, int min_abs) {
	this->out_min = min;
	this->out_max = max;
	this->out_abs_min = min_abs;
}

void pid_set_controller_direction(struct pid *this, enum pid_controller_direction controller_dir) {
	if (controller_dir != this->controller_dir) {
		// Invert control constants
		this->zp = (0 - this->zp);
		this->zi = (0 - this->zi);
		this->zd = (0 - this->zd);
	}
	this->controller_dir = controller_dir;
}
