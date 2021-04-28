#include <stdlib.h>

#include "board.h"
#include "debug.h"
#include "one-wire.h"
#include "ds18b20.h"
#include "temperature_ds18b20.h"

#define STB_DEFINE

#include "misc.h"

#define TEMPERATURE_DS18B20_TASK_PRIORITY ( configMAX_PRIORITIES - 4 )

static float temperature;

float temperature_ds18b20_get() {
	return temperature;
}

/**
 * @brief	returns status temperature of the NFC.
 * @return 	the temperature value in °C
 */
float temperature_ds18b20_read(void)
{
	int err;
	float temp = 0.f;
	err = ds18b20_start_conversion(0);
	lDebug(Info, "start_conversion: %i", err);
	err = ds18b20_read_temperature(0);
	lDebug(Info, "read_temperature: %i", err);
	err = ds18b20_get_temperature_float(0, &temp);
	lDebug(Info, "get_tempeature_float: %i", err);

//	char buff[6];
//	snprint_double(buff, sizeof buff / sizeof buff[0], temp, 2);
//	lDebug(Info, "Temperatura leida del DS18B20: %s", buff);

	return temp;

}

static void temperature_ds18b20_task(void *par)
{
	while (true) {

		temperature = temperature_ds18b20_read();

		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

/**
 * @brief 	initializes ADC to read temperature sensor.
 * @return	nothing
 */
void temperature_ds18b20_init()
{
	one_wire_init();
	ds18b20_init();
	lDebug(Info, "search_And_assign_ROM: %i", ds18b20_search_and_assign_ROM_codes());
	//lDebug(Info, "read_ROM: %i", ds18b20_read_ROM(0));
	xTaskCreate(temperature_ds18b20_task, "DS18B20", configMINIMAL_STACK_SIZE * 2, NULL,
			TEMPERATURE_DS18B20_TASK_PRIORITY, NULL);
	lDebug(Info, "DS18B20: task created");
}
