#include <stdbool.h>
#include <stdint.h>

#include "board.h"
#include "pid.h"
#include "limits.h"

void pid_init(struct pid *this, int kp, int ki, int kd,
		enum pid_controller_direction controller_dir,
		double sample_period_ms, int min_output, int max_output)
{
	pid_set_output_limits(this, min_output, max_output);

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

int pid_run(struct pid *this, int setpoint, int input) {
	int error = setpoint - input;

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
	if (this->num_times_ran > 0) {
		this->input_change = (input - this->prev_input);
		this->d_term = -this->zd * this->input_change;
	} else {
		this->d_term = 0;
	}

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

	// Increment the Run() counter, after checking to make sure it hasn't reached
	// max value.
	if (this->num_times_ran < UINT_MAX)
		this->num_times_ran++;
	return this->output;
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
	this->zi = ki * (int) (this->sample_period_ms / 1000.0);
	this->zd = kd / (int) (this->sample_period_ms / 1000.0);

	if (this->controller_dir == PID_REVERSE) {
		this->zp = (0 - this->zp);
		this->zi = (0 - this->zi);
		this->zd = (0 - this->zd);
	}
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

void pid_set_output_limits(struct pid *this, int min, int max) {
	if (min >= max)
		return;
	this->out_min = min;
	this->out_max = max;
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
