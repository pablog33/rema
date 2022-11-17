#ifndef Z_AXIS_H_
#define Z_AXIS_H_

#include <stdint.h>
#include <stdbool.h>

#include "mot_pap.h"

#ifdef __cplusplus
extern "C" {
#endif

// Declaration needed because TEST_GUI calls this IRQ handler as a standard function
void TIMER3_IRQHandler(void);

void z_axis_init();

#ifdef __cplusplus
}
#endif

#endif /* Z_AXIS_H_ */
