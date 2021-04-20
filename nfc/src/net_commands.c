#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "debug.h"

#include "net_commands.h"
#include "mot_pap.h"
#include "parson.h"
#include "json_wp.h"

#define PROTOCOL_VERSION  	"JSON_1.0"

typedef struct {
	char *cmd_name;
	JSON_Value* (*cmd_function)(JSON_Value const *pars);
} cmd_entry;

JSON_Value* telemetria_cmd(JSON_Value const *pars)
{
	struct mot_pap arm;
	static int fake_pos = 123;

	fake_pos++;
	arm.posCmd = 848;
	arm.posAct = fake_pos;
	arm.stalled = false;
	arm.offset = 40510;

	JSON_Value *ans = json_value_init_object();
	json_object_set_value(json_value_get_object(ans), "ARM",
			mot_pap_json(&arm));
	json_object_set_value(json_value_get_object(ans), "POLE",
			mot_pap_json(&arm));
	return ans;
}

JSON_Value* logs_cmd(JSON_Value const *pars)
{
	if (pars && json_value_get_type(pars) == JSONObject) {
		double quantity = json_object_get_number(json_value_get_object(pars),
				"quantity");

		JSON_Value *ans = json_value_init_object();
		JSON_Value *msg_array = json_value_init_array();
		char msg[50];

		for (int x = 0; x < quantity; x++) {
			snprintf(msg, 50, "Mensaje de debug numero %i ", x);
			json_array_append_string(json_value_get_array(msg_array), msg);
		}

		json_object_set_value(json_value_get_object(ans), "DEBUG_MSGS",
				msg_array);

		return ans;

	}
	return NULL;
}

JSON_Value* lift_cmd(JSON_Value const *pars)
{
	lDebug(Info, "LLAMADA lift_up_cmd");
	JSON_Value *ans = json_value_init_object();
	return ans;
}

JSON_Value* protocol_version_cmd(JSON_Value const *pars)
{
	JSON_Value *ans = json_value_init_object();
	json_object_set_string(json_value_get_object(ans), "Version", PROTOCOL_VERSION);
	return ans;
}

JSON_Value* control_enable_cmd(JSON_Value const *pars)
{
	static bool enabled = false;
	enabled = !enabled;

	JSON_Value *ans = json_value_init_object();
	json_object_set_boolean(json_value_get_object(ans), "ACK", enabled);
	return ans;
}

JSON_Value* pole_closed_loop_cmd(JSON_Value const *pars)
{
	lDebug(Info, "LLAMADA pole_closed_loop_cmd");

	JSON_Value *ans = json_value_init_object();
	json_object_set_boolean(json_value_get_object(ans), "ACK", true);
	return ans;
}


JSON_Value* arm_free_run_cmd(JSON_Value const *pars)
{
	if (pars && json_value_get_type(pars) == JSONObject) {
		char const *dir = json_object_get_string(json_value_get_object(pars),
				"dir");
		double speed = json_object_get_number(json_value_get_object(pars),
				"speed");

		if (dir && speed != 0) {
			lDebug(Info, "ARM_FREE_RUN DIR: %s, SPEED: %d", dir, (int ) speed);
		}
		JSON_Value *ans = json_value_init_object();
		json_object_set_boolean(json_value_get_object(ans), "ACK", true);
		return ans;
	}
	return NULL;
}

// @formatter:off
const cmd_entry cmds_table[] = {
		{
				"PROTOCOL_VERSION",		/* Command name */
				protocol_version_cmd,	/* Associated function */
		},
		{
				"CONTROL_ENABLE",
				control_enable_cmd,
		},
		{
				"LIFT_UP",
				lift_cmd,
		},
		{
				"POLE_CLOSED_LOOP",
				pole_closed_loop_cmd,
		},
		{
				"ARM_FREE_RUN",
				arm_free_run_cmd,
		},
		{
				"TELEMETRIA",
				telemetria_cmd,
		},
		{
				"LOGS",
				logs_cmd,
		},
};
// @formatter:on


/**
 * @brief 	searchs for a matching command name in cmds_table[], passing the parameters
 * 			as a JSON object for the called function to parse them.
 * @param 	*cmd 	:name of the command to execute
 * @param   *pars   :JSON object containing the passed parameters to the called function
 */
JSON_Value* cmd_execute(char const *cmd, JSON_Value const *pars)
{
	bool cmd_found = false;
	for (int i = 0; i < (sizeof(cmds_table) / sizeof(cmds_table[0])); i++) {
		if (!strcmp(cmd, cmds_table[i].cmd_name)) {
			return cmds_table[i].cmd_function(pars);
		}
	}
	if (!cmd_found) {
		lDebug(Info, "No matching command found");
	}
	return NULL;
}
