#ifndef QUALITYGATE_SORTER_H
#define QUALITYGATE_SORTER_H

#include <stdint.h>
#include "qualitygate_types.h"

void qg_sorter_init(uint32_t now_ms);
void qg_sorter_update(uint32_t now_ms);
qg_state_t qg_sorter_get_state(void);
uint32_t qg_sorter_get_count(qg_color_t color);

#endif /* QUALITYGATE_SORTER_H */
