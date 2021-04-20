#include "board.h"

/* Read data from EEPROM */
/* size must be multiple of 4 bytes */
void EEPROM_Read(uint32_t pageOffset, uint32_t pageAddr, uint32_t *ptr,
		uint32_t size)
{
	uint32_t i = 0;
	uint32_t *pEepromMem = (uint32_t*) EEPROM_ADDRESS(pageAddr, pageOffset);
	for (i = 0; i < size / 4; i++) {
		ptr[i] = pEepromMem[i];
	}
}

/* Erase a page in EEPROM */
void EEPROM_Erase(uint32_t pageAddr)
{
	uint32_t i = 0;
	uint32_t *pEepromMem = (uint32_t*) EEPROM_ADDRESS(pageAddr, 0);
	for (i = 0; i < EEPROM_PAGE_SIZE / 4; i++) {
		pEepromMem[i] = 0;
#if AUTOPROG_ON
		Chip_EEPROM_WaitForIntStatus(LPC_EEPROM, EEPROM_INT_ENDOFPROG);
#endif
	}
#if (AUTOPROG_ON == 0)
	Chip_EEPROM_EraseProgramPage(LPC_EEPROM);
#endif
}

/* Write data to a page in EEPROM */
/* size must be multiple of 4 bytes */
void EEPROM_Write(uint32_t pageOffset, uint32_t pageAddr, uint32_t *ptr,
		uint32_t size)
{
	uint32_t i = 0;
	uint32_t *pEepromMem = (uint32_t*) EEPROM_ADDRESS(pageAddr, pageOffset);

	if (size > EEPROM_PAGE_SIZE - pageOffset)
		size = EEPROM_PAGE_SIZE - pageOffset;

	for (i = 0; i < size / 4; i++) {
		pEepromMem[i] = ptr[i];
#if AUTOPROG_ON
		Chip_EEPROM_WaitForIntStatus(LPC_EEPROM, EEPROM_INT_ENDOFPROG);
#endif
	}

#if (AUTOPROG_ON == 0)
	Chip_EEPROM_EraseProgramPage(LPC_EEPROM);
#endif
}
