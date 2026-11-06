#ifndef QUALITYGATE_TYPES_H
#define QUALITYGATE_TYPES_H

#include <stdint.h>

/* The four accepted bin colors. UNKNOWN is deliberately not a bin:
 * an unclassified part must be held, not dropped into a random bin. */
typedef enum {
    QG_COLOR_RED = 0,
    QG_COLOR_GREEN,
    QG_COLOR_BLUE,
    QG_COLOR_YELLOW,
    QG_COLOR_UNKNOWN,
    QG_COLOR_BIN_COUNT = QG_COLOR_UNKNOWN
} qg_color_t;

typedef enum {
    QG_WAITING = 0,
    QG_SCANNING,
    QG_POSITIONING_CHUTE,
    QG_DROPPING,
    QG_RESETTING,
    QG_FAULT
} qg_state_t;

const char *qg_color_name(qg_color_t color);
const char *qg_state_name(qg_state_t state);

#endif /* QUALITYGATE_TYPES_H */
