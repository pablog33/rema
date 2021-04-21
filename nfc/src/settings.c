#include "settings.h"

#include <stdbool.h>

#include "board.h"
#include "eeprom.h"
#include "lwip/ip_addr.h"
#include "debug.h"

/* Page used for storage */
#define PAGE_ADDR       0x01/* Page number */

static struct settings settings_defaults()
{
	struct settings settings;

	IP4_ADDR(&(settings.gw), 192, 168, 2, 1);
	IP4_ADDR(&(settings.ipaddr), 192, 168, 2, 20);
	IP4_ADDR(&(settings.netmask), 255, 255, 255, 0);
	settings.port = 5020;

	return settings;
}

void settings_init(struct settings settings)
{
	EEPROM_init();
}

/**
 * @brief 	saves settings to EEPROM
 * @return	nothing
 */
void settings_save(struct settings settings)
{
	lDebug(Info, "EEPROM Erase...");
	EEPROM_Erase(PAGE_ADDR);

	lDebug(Info, "EEPROM write...");
	EEPROM_Write(0, PAGE_ADDR, (uint32_t*) &settings,
			(sizeof settings + 3) & ~0x03);
}

struct settings settings_read()
{
	struct settings settings;

	lDebug(Info, "EEPROM Read...");
	EEPROM_Read(0, PAGE_ADDR, (uint32_t*) &settings,
			(sizeof settings + 3) & ~0x03);

	if ((settings.gw.addr == 0) || (settings.ipaddr.addr == 0)
			|| (settings.netmask.addr == 0) || (settings.port == 0)) {
		lDebug(Error, "No network config loaded from EEPROM. Loading default settings");
		return settings_defaults();
	}

	return settings;
}

