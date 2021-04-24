#ifndef TEMPERATURE_DS18B20_H_
#define TEMPERATURE_DS18B20_H_

#include <stdbool.h>

void temperature_ds18b20_init();

float temperature_ds18b20_read(void);

#ifdef __cplusplus
}
#endif

#endif /* TEMPERATURE_DS18B20_H_ */
