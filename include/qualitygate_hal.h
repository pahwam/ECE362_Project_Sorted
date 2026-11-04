#ifndef QUALITYGATE_HAL_H
#define QUALITYGATE_HAL_H

#include <stdbool.h>
#include <stdint.h>
#include "qualitygate_types.h"

/* HAL = hardware abstraction layer.
 * These calls are SIMULATED for now, except the Proton's user LEDs.
 * The rest of the sorting code will not change when the real hardware
 * driver is implemented. */
void qg_hal_init(void);
void qg_hal_poll(uint32_t now_ms);
bool qg_hal_object_present(void);
qg_color_t qg_hal_read_color(void);
void qg_hal_move_chute(qg_color_t color);
void qg_hal_set_trapdoor(bool open);
void qg_hal_indicate_color(qg_color_t color);

#endif /* QUALITYGATE_HAL_H */
