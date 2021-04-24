/**
 * This is a component for the Maxim DS18B20 1-Wire temperature sensor.
 **
 ** * Copyright (c) Original implementation: Omar Isa� Pinales Ayala, 2014, all rights reserved.
 **  * Updated and maintained by Erich Styger, 2014-2020
 **  * Web:         https://mcuoneclipse.com
 **  * SourceForge: https://sourceforge.net/projects/mcuoneclipse
 **  * Git:         https://github.com/ErichStyger/McuOnEclipse_PEx
 **  * All rights reserved.
 **  *
 **  * Redistribution and use in source and binary forms, with or without modification,
 **  * are permitted provided that the following conditions are met:
 **  *
 **  * - Redistributions of source code must retain the above copyright notice, this list
 **  *   of conditions and the following disclaimer.
 **  *
 **  * - Redistributions in binary form must reproduce the above copyright notice, this
 **  *   list of conditions and the following disclaimer in the documentation and/or
 **  *   other materials provided with the distribution.
 **  *
 **  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 **  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 **  * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 **  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 **  * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 **  * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 **  * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 **  * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 **  * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 **  * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ds18b20.h"
#include "one-wire.h" /* interface to 1-Wire */
#include "wait.h"

#define DS18B20_CONFIG_READ_AUTO				(false)
#define DS18B20_CONFIG_MULTIPLE_BUS_DEVICES		(false)
#define DS18B20_CONFIG_NUMBER_OF_SENSORS		(1)

/* Events */
enum {
	EV_NOTHING, EV_INIT, EV_NO_BUSY, EV_READ_ROM, EV_READ_TEMP, EV_READ_TEMP_ALL
};

/* Rom commands */
#define RC_READ_ROM          0x33
#define RC_MATCH_ROM         0x55
#define RC_SKIP_ROM          0xCC
#define RC_RELEASE           0xFF

/* Function commands */
#define FC_CONVERT_T         0x44
#define FC_WRITE_SCRATCHPAD  0x4E
#define FC_READ_SCRATCHPAD   0xBE
#define FC_COPY_SCRATCHPAD   0x48

/* configuration byte */
#define DS18B20_CONFIG_SHIFT_R0   (5)  /* R0 in configuration byte starts at bit 0 */
#define DS18B20_CONFIG_ONE_MASK   (0x1F) /* bit 0 to bit 4 are one in configuration byte */

#define DS18B20_NOF_SCRATCHPAD_BYTES  (9) /* number of bytes in scratchpad */

typedef struct {
	int32_t Temperature;
	uint8_t Rom[DS18B20_ROM_CODE_SIZE]; /* family code (0x28), ROM code (6 bytes), CRC (1 byte) */
	DS18B20_ResolutionBits resolution; /* DS18B20_ResolutionBits */
} Sensor_t;

struct {
	int32_t Value;
	uint8_t Data[10];
#if DS18B20_CONFIG_MULTIPLE_BUS_DEVICES
	uint8_t WorkSensor;
	unsigned MaxResolution :2;
#endif
	unsigned Busy :1;
} Device;

/* @formatter:off */
											 /* conversion time based on resolution */
static const uint16_t ConvTime[4] = { 	94,  /* DS18B20_RESOLUTION_BITS_9:  93.75 ms */
										188, /* DS18B20_RESOLUTION_BITS_10: 187.5 ms */
										375, /* DS18B20_RESOLUTION_BITS_11: 375 ms */
										750  /* DS18B20_RESOLUTION_BITS_12: 750 ms */
};
/* @formatter:on */

static Sensor_t Sensor[DS18B20_CONFIG_NUMBER_OF_SENSORS];

static uint8_t ds18b20_read_scratchpad(uint8_t sensor_index,
		uint8_t data[DS18B20_NOF_SCRATCHPAD_BYTES])
{
	uint8_t res;

	if (Device.Busy) {
		return ERR_BUSY;
	}
#if DS18B20_CONFIG_NUMBER_OF_SENSORS>1
  if(sensor_index>=DS18B20_CONFIG_NUMBER_OF_SENSORS) {
    return ERR_RANGE; /* error */
  }
#else
	(void) sensor_index; /* not used */
#endif
	Device.Busy = TRUE;
	one_wire_send_reset();
#if DS18B20_CONFIG_MULTIPLE_BUS_DEVICES
	one_wire_send_byte(RC_MATCH_ROM);
	one_wire_send_bytes(Sensor[sensor_index].Rom,
			sizeof(Sensor[sensor_index].Rom));
#else /* single device on the bus, can skip ROM code */
	one_wire_send_byte(RC_SKIP_ROM);
#endif
	one_wire_send_byte(FC_READ_SCRATCHPAD);
	one_wire_receive(DS18B20_NOF_SCRATCHPAD_BYTES);
	one_wire_send_byte(0xFF);
	Device.Busy = FALSE;

	/* extract data */
	res = one_wire_get_bytes(&data[0], DS18B20_NOF_SCRATCHPAD_BYTES); /* scratchpad with 9 bytes */
	if (res != ERR_OK) {
		return res; /* failed getting data */
	}
	/* check CRC */
	if (one_wire_calc_CRC(&data[0], 8) != data[8]) { /* last byte is CRC */
		return ERR_CRC; /* CRC error */
	}
	return ERR_OK;
}

/**
 * @brief   starts the conversion of temperature in the sensor.
 * @param   sensor_index    - Sensor index, use zero
 *                           if only using one sensor
 * @returns error code
 */
uint8_t ds18b20_start_conversion(uint8_t sensor_index)
{
#if DS18B20_CONFIG_READ_AUTO
  uint8_t res;
#endif

	if (Device.Busy) {
		return ERR_BUSY;
	}
#if DS18B20_CONFIG_NUMBER_OF_SENSORS>1
  if(sensor_index>=DS18B20_CONFIG_NUMBER_OF_SENSORS) {
    return ERR_RANGE; /* error */
  }
#endif
	Device.Busy = TRUE;
	one_wire_send_reset();
#if DS18B20_CONFIG_MULTIPLE_BUS_DEVICES
	one_wire_send_byte(RC_MATCH_ROM);
	one_wire_send_bytes(Sensor[sensor_index].Rom, DS18B20_ROM_CODE_SIZE);
	one_wire_send_byte(FC_CONVERT_T);
#else /* only single device on the bus, can skip ROM code */
	one_wire_send_byte(RC_SKIP_ROM);
	one_wire_send_byte(FC_CONVERT_T);
#endif
	one_wire_strong_pull_up(ConvTime[Sensor[sensor_index].resolution]);
	Device.Busy = FALSE;
#if DS18B20_CONFIG_READ_AUTO
  vTaskDelay(pdMS_TO_TICKS(ConvTime[Sensor[sensor_index].resolution]));
  res = ds18b20_read_temperature(sensor_index);
  if (res!=ERR_OK) {
    return res;
  }
#endif
	return ERR_OK;
}

/**
 * @brief 	sets the resolution
 * @param  	sensor_index    - Index of the sensor to set the resolution.
 *         	config_bits     - Two bits resolution config
 *                            value:
 *                            [0b00] - 9 bits.
 *                            [0b01] - 10 bits.
 *                            [0b10] - 11 bits.
 *                            [0b11] - 12 bits.
 * @returns error code
 */
uint8_t ds18b20_set_resolution(uint8_t sensor_index,
		DS18B20_ResolutionBits resolution)
{
	uint8_t config;

	if (Device.Busy) {
		return ERR_BUSY;
	}
#if ds18b20_CONFIG_NUMBER_OF_SENSORS>1
  if(sensor_index>=DS18B20_CONFIG_NUMBER_OF_SENSORS) {
    return ERR_RANGE; /* error */
  }
#else
	(void) sensor_index; /* not used */
#endif
	Sensor[sensor_index].resolution = resolution;
	config = (((uint8_t) (resolution << DS18B20_CONFIG_SHIFT_R0))
			| DS18B20_CONFIG_ONE_MASK); /* build proper configuration register mask */
	Device.Busy = TRUE;
	one_wire_send_reset();
#if DS18B20_CONFIG_MULTIPLE_BUS_DEVICES
	Device.WorkSensor = sensor_index;
	one_wire_send_byte(RC_MATCH_ROM);
	one_wire_send_bytes(Sensor[sensor_index].Rom, DS18B20_ROM_CODE_SIZE);
	one_wire_send_byte(FC_WRITE_SCRATCHPAD);
	one_wire_send_byte(0x01);
	one_wire_send_byte(0x10);
	one_wire_send_byte(config);
#else /* only single device on the bus, can skip ROM code */
	one_wire_send_byte(RC_SKIP_ROM);
	one_wire_send_byte(FC_WRITE_SCRATCHPAD);
	one_wire_send_byte(0x01);
	one_wire_send_byte(0x10);
	one_wire_send_byte(config);
#endif
	Device.Busy = FALSE;
	return ERR_OK;
}

/**
 * @brief	read the temperature value from the sensor and stores it in memory
 * @param  	sensor_index    - Sensor index, use zero
 *                           if only using one sensor
 * @returns error code
 */
uint8_t ds18b20_read_temperature(uint8_t sensor_index)
{
	uint8_t data[DS18B20_NOF_SCRATCHPAD_BYTES]; /* scratchpad data */
	uint8_t res;

	res = ds18b20_read_scratchpad(sensor_index, &data[0]);
	if (res != ERR_OK) {
		return res;
	}
	/* temperature is in data[0] (LSB) and data[1] (MSB) */
#if DS18B20_CONFIG_NUMBER_OF_SENSORS>1
  Sensor[sensor_index].Temperature = (data[1]<<8)+data[0];
#else
	Sensor[0].Temperature = (data[1] << 8) + data[0];
#endif
	return ERR_OK;
}

/**
 * @brief   read the sensor resolution sensor and stores it in memory
 * @param   sensor_index    - Sensor index, use zero if only using one sensor
 * @returns error code
 */
uint8_t ds18b20_read_resolution(uint8_t sensor_index)
{
	uint8_t data[DS18B20_NOF_SCRATCHPAD_BYTES]; /* scratchpad data */
	uint8_t res;

#if DS18B20_CONFIG_NUMBER_OF_SENSORS>1
  if(sensor_index>=DS18B20_CONFIG_NUMBER_OF_SENSORS) {
    return ERR_RANGE; /* error */
  }
#endif
	res = ds18b20_read_scratchpad(sensor_index, &data[0]);
	if (res != ERR_OK) {
		return res;
	}
	/* configuration is in data[4] */
	Sensor[sensor_index].resolution = ((data[4]) >> DS18B20_CONFIG_SHIFT_R0)
			& 0x3; /* only two bits */
	return ERR_OK;
}

/**
 * @brief  	gets the temperature from memory.
 * @param 	*sensor_index    - Index of the sensor to
 *                           get the temperature value.
 * @returns error code
 */
uint8_t ds18b20_get_temperature_raw(uint16_t sensor_index, uint32_t *raw)
{
#if DS18B20_CONFIG_NUMBER_OF_SENSORS>1
  if(sensor_index>=DS18B20_CONFIG_NUMBER_OF_SENSORS) {
    return ERR_RANGE; /* error */
  }
#else
	(void) sensor_index; /* not used */
#endif
#if DS18B20_CONFIG_NUMBER_OF_SENSORS>1
  *raw = Sensor[sensor_index].Temperature;
#else
	*raw = Sensor[0].Temperature;
#endif
	return ERR_OK;
}

/**
 *         Returns the temperature from memory in floating point format.
 *         sensor_index    - Sensor index, use zero
 *                           if only using one sensor
 *       * temperature     - Pointer to where to store
 *                           the value
 * @eturns Error code
 */
uint8_t ds18b20_get_temperature_float(uint8_t sensor_index, float *temperature)
{
	int16_t raw, intPart, fracPart;

#if DS18B20_CONFIG_NUMBER_OF_SENSORS>1
  if(sensor_index>=DS18B20_CONFIG_NUMBER_OF_SENSORS) {
    return ERR_RANGE; /* error */
  }
#else
	(void) sensor_index; /* not used */
#endif
#if DS18B20_CONFIG_NUMBER_OF_SENSORS>1
  raw = Sensor[sensor_index].Temperature;
#else
	raw = Sensor[0].Temperature;
#endif
	intPart = raw >> 4; /* integral part */
	fracPart = raw & 0xf; /* fractional part */
	*temperature = ((float) intPart) + (((float) fracPart) * 0.0625);
	return ERR_OK;
}

/**
 * @brief   Returns TRUE if there are a operation in progress.
 * @returns TRUE if the device is busy
 * 			FALSE if its ready to operate
 */
bool ds18b20_is_busy(void)
{
	return Device.Busy;
}

/**
 * @brief	gets the rom code from the memory.
 * @param 	sensor_index    - Sensor index, use zero if only using one sensor
 * @param 	*romCodePtr      - Pointer to a pointer for the return value
 * @returns error code
 */
uint8_t ds18b20_get_ROM_code(uint8_t sensor_index, uint8_t **romCodePtr)
{
#if DS18B20_CONFIG_NUMBER_OF_SENSORS>1
  if(sensor_index>=DS18B20_CONFIG_NUMBER_OF_SENSORS) {
    return ERR_RANGE; /* error */
  }
#else
	(void) sensor_index; /* not used */
#endif
#if DS18B20_CONFIG_NUMBER_OF_SENSORS>1
  *romCodePtr = &Sensor[sensor_index].Rom[0];
#else
	*romCodePtr = &Sensor[0].Rom[0];
#endif
	return ERR_OK; /* ok */
}

/**
 * @brief   starts to read the rom code and saves it in memory.
 * @param   sensor_index    - Sensor index, use zero
 *                           if only using one sensor
 * @returns error code
 */
uint8_t ds18b20_read_ROM(uint8_t sensor_index)
{
	uint8_t res;

	if (Device.Busy) {
		return ERR_BUSY;
	}
#if DS18B20_CONFIG_NUMBER_OF_SENSORS>1
  if(sensor_index>=DS18B20_CONFIG_NUMBER_OF_SENSORS) {
    return ERR_RANGE; /* error */
  }
#else
	(void) sensor_index; /* not used */
#endif
	Device.Busy = TRUE;
#if DS18B20_CONFIG_NUMBER_OF_SENSORS>1
  Device.WorkSensor = sensor_index;
#endif
#if DS18B20_CONFIG_NUMBER_OF_SENSORS>1
  res = one_wire_read_rom_code(Sensor[sensor_index].Rom);
#else
	res = one_wire_read_rom_code(Sensor[0].Rom); /* 8 bytes */
#endif
	Device.Busy = FALSE;
	/* index 0  : family code
	 index 1-6: 48bit serial number
	 index 7  : CRC
	 */
	if (res != ERR_OK) {
		return res; /* error */
	}
#if DS18B20_CONFIG_NUMBER_OF_SENSORS>1
  if (Sensor[sensor_index].Rom[0]!=DS18B20_FAMILY_CODE) {
#else
	if (Sensor[0].Rom[0] != DS18B20_FAMILY_CODE) {
#endif
		return ERR_FAILED; /* not a DS18B20? */
	}
	return ERR_OK; /* ok */
}

/**
 * @brief 	initializes the device.
 * @returns nothing
 */
void ds18b20_init(void)
{
#if DS18B20_CONFIG_MULTIPLE_BUS_DEVICES
	int i;

	for (i = 0; i < DS18B20_CONFIG_NUMBER_OF_SENSORS; i++) {
		Sensor[i].resolution = DS18B20_RESOLUTION_BITS_12;
	}
	memcpy(Sensor[0].Rom, "\x28\x00\x00\x00\x00\x00\x00\x00", 8);
#else
	memset(&Sensor[0], 0, sizeof(Sensor[0])); /* initialize all fields with zero */
	Sensor[0].resolution = DS18B20_RESOLUTION_BITS_12; /* default resolution */
#endif
	Device.Value = 8500000; /* dummy value or 85�C */
}

/**
 * @brief	scans the devices on the bus and assigns the ROM codes to
 *	        the list of available sensors
 * @returns error code
 */
uint8_t ds18b20_search_and_assign_ROM_codes(void)
{
	uint8_t rom[one_wire_ROM_CODE_SIZE];
	bool found;
	int i, nofFound = 0;

	one_wire_reset_search(); /* reset search fields */
	one_wire_target_search(DS18B20_FAMILY_CODE); /* only search for DS18B20 */
	do {
		found = one_wire_search(&rom[0], TRUE);
		if (found) {
			/* store ROM code in device list */
			for (i = 0; i < one_wire_ROM_CODE_SIZE; i++) {
				Sensor[nofFound].Rom[i] = rom[i];
			}
			nofFound++;
		}
	} while (found && nofFound < DS18B20_CONFIG_NUMBER_OF_SENSORS);
	if (nofFound == 0) {
		return ERR_FAILED; /* nothing found */
	}
	return ERR_OK;
}
