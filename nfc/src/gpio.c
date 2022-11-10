#include <gpio.h>
#include <stdbool.h>

#include "board.h"
#include "mot_pap.h"

//struct gpio_entry pole_dir = { 4, 10, SCU_MODE_FUNC4, 5, 14 };		//DOUT6 P4_10 	PIN35	GPIO5[14] 	POLE_DIR
//struct gpio_entry pole_step = { 1, 5, SCU_MODE_FUNC0, 1, 8 };		//DOUT7 P1_5 	PIN48 	GPIO1[8]   	POLE_PULSE

/**
 * @brief	 inits one gpio passed as output
 * @returns nothing
 */
void gpio_init_output(struct gpio_entry gpio) {
	Chip_SCU_PinMuxSet(gpio.pin_port, gpio.pin_bit, gpio.scu_mode);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, gpio.gpio_port, gpio.gpio_bit);
}

/**
 * @brief	 inits one gpio passed as output
 * @returns nothing
 */
void gpio_init_input(struct gpio_entry gpio) {
	Chip_SCU_PinMuxSet(gpio.pin_port, gpio.pin_bit, gpio.scu_mode);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, gpio.gpio_port, gpio.gpio_bit);
}


/**
 * @brief	 GPIO sets pin passed as parameter to the state specified
 * @returns nothing
 */
void gpio_set_pin_state(struct gpio_entry gpio, bool state) {
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, gpio.gpio_port, gpio.gpio_bit, state);
}

/**
 * @brief	toggles GPIO corresponding pin passed as parameter
 * @returns nothing
 */
void gpio_toggle(struct gpio_entry gpio) {
	Chip_GPIO_SetPinToggle(LPC_GPIO_PORT, gpio.gpio_port, gpio.gpio_bit);
}
