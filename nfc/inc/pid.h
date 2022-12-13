#ifndef PID_H_
#define PID_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//! @brief		Enumerates the controller direction modes
enum pid_controller_direction {
	PID_DIRECT,			//!< Direct drive (+error gives +output)
	PID_REVERSE			//!< Reverse driver (+error gives -output)
};

//! Used to determine whether the output shouldn't be accumulated (distance control),
//! or accumulated (velocity control).
enum pid_output_mode {
	DONT_ACCUMULATE_OUTPUT,
	ACCUMULATE_OUTPUT,
	DISTANCE_PID = DONT_ACCUMULATE_OUTPUT,
	VELOCITY_PID = ACCUMULATE_OUTPUT
};

struct pid {
	//! @brief 		The set-point the PID control is trying to make the output converge to.
	int setpoint;

	//! @brief		The control output.
	//! @details	This is updated when pid_run() is called.
	int output;

	//! @brief		Time-step scaled proportional constant for quick calculation (equal to actualKp)
	int zp;

	//! @brief		Time-step scaled integral constant for quick calculation
	int zi;

	//! @brief		Time-step scaled derivative constant for quick calculation
	int zd;

	//! @brief		Actual (non-scaled) proportional constant
	int kp;

	//! @brief		Actual (non-scaled) integral constant
	int ki;

	//! @brief		Actual (non-scaled) derivative constant
	int kd;

	//! @brief		Actual (non-scaled) proportional constant
	int prev_input;

	//! @brief		The change in input between the current and previous value
	int input_change;

	//! @brief 		The output value calculated the previous time Pid_Run() was called.
	//! @details	Used in ACCUMULATE_OUTPUT mode.
	int prev_output;

	//! @brief		The sample period (in milliseconds) between successive Pid_Run() calls.
	//! @details	The constants with the z prefix are scaled according to this value.
	double sample_period_ms;

	int p_term;//!< The proportional term that is summed as part of the output (calculated in Pid_Run())
	int i_term;//!< The integral term that is summed as part of the output (calculated in Pid_Run())
	int d_term;//!< The derivative term that is summed as part of the output (calculated in Pid_Run())
	int out_min;	//!< The minimum output value. Anything lower will be limited to this floor.
	int out_max;	//!< The maximum output value. Anything higher will be limited to this ceiling.

	//! @brief		Counts the number of times that Run() has be called. Used to stop
	//!				derivative control from influencing the output on the first call.
	//! @details	Safely stops counting once it reaches 2^32-1 (rather than overflowing).
	unsigned int num_times_ran;

	//! @brief		The controller direction (FORWARD or REVERSE).
	enum pid_controller_direction controller_dir;

	//! @brief		The output mode (non-accumulating vs. accumulating) for the control loop.
	enum pid_output_mode output_mode;
};

//! @brief 		Init function
//! @details   	The parameters specified here are those for for which we can't set up
//!    			reliable defaults, so we need to have the user set them.
void pid_init(struct pid *this, int kp, int ki, int kd,
		enum pid_controller_direction controller_dir,
		double sample_period_ms, int min_output, int max_output);

//! @brief 		Computes new PID values
//! @details 	Call once per sampleTimeMs. Output is stored in the pidData structure.
int pid_run(struct pid *this, int setpoint, int input);

void pid_set_output_limits(struct pid *this, int min, int max);

//! @details	The PID will either be connected to a direct acting process (+error leads to +output, aka inputs are positive)
//!				or a reverse acting process (+error leads to -output, aka inputs are negative)
void pid_set_controller_direction(struct pid *this, enum pid_controller_direction controller_dir);

//! @brief		Changes the sample time
void pid_set_sample_period(struct pid *this, unsigned int new_sample_period_ms);

//! @brief		This function allows the controller's dynamic performance to be adjusted.
//! @details	It's called automatically from the init function, but tunings can also
//! 			be adjusted on the fly during normal operation
void pid_set_tunings(struct pid *this, int kp, int ki, int kd);

//! @brief		Returns the actual (not time-scaled) proportional constant.
int pid_get_kp(struct pid *this);

//! @brief		Returns the actual (not time-scaled) integral constant.
int pid_get_ki(struct pid *this);

//! @brief		Returns the actual (not time-scaled) derivative constant.
int pid_get_kd(struct pid *this);

//! @brief		Returns the time-scaled (dependent on sample period) proportional constant.
int pid_get_zp(struct pid *this);

//! @brief		Returns the time-scaled (dependent on sample period) integral constant.
int pid_get_zi(struct pid *this);

//! @brief		Returns the time-scaled (dependent on sample period) derivative constant.
int pid_get_zd(struct pid *this);

#ifdef __cplusplus
}
#endif

#endif /* PID_H_ */
