#include <stdio.h>
#include <stdint.h>

#include "qualitygate_config.h"
#include "qualitygate_hal.h"
#include "sorter.h"

static qg_state_t s_state = QG_WAITING;
static qg_color_t s_selected_color = QG_COLOR_UNKNOWN;
static uint32_t s_deadline_ms = 0;
static uint32_t s_counts[QG_COLOR_BIN_COUNT] = {0};

/* Signed subtraction is safe for these short delays even if the board's
 * 32-bit millisecond clock eventually wraps around. */
static bool deadline_reached(uint32_t now_ms, uint32_t deadline_ms) {
    return (int32_t)(now_ms - deadline_ms) >= 0;
}

static void change_state(qg_state_t new_state) {
    s_state = new_state;
    printf("[SORTER] -> %s\n", qg_state_name(new_state));
}

static void print_counts(void) {
    printf("[COUNTS] RED=%lu GREEN=%lu BLUE=%lu YELLOW=%lu\n",
           (unsigned long)s_counts[QG_COLOR_RED],
           (unsigned long)s_counts[QG_COLOR_GREEN],
           (unsigned long)s_counts[QG_COLOR_BLUE],
           (unsigned long)s_counts[QG_COLOR_YELLOW]);
}

void qg_sorter_init(uint32_t now_ms) {
    s_selected_color = QG_COLOR_UNKNOWN;
    s_deadline_ms = now_ms;
    for (unsigned i = 0; i < QG_COLOR_BIN_COUNT; ++i) {
        s_counts[i] = 0;
    }
    qg_hal_set_trapdoor(false);
    qg_hal_indicate_color(QG_COLOR_UNKNOWN);
    change_state(QG_WAITING);
}

void qg_sorter_update(uint32_t now_ms) {
    qg_hal_poll(now_ms);

    switch (s_state) {
        case QG_WAITING:
            if (qg_hal_object_present()) {
                change_state(QG_SCANNING);
                s_deadline_ms = now_ms + QG_SCAN_DELAY_MS;
            }
            break;

        case QG_SCANNING:
            if (!deadline_reached(now_ms, s_deadline_ms)) break;
            s_selected_color = qg_hal_read_color();
            printf("[SORTER] Detected color: %s\n",
                   qg_color_name(s_selected_color));

            if (s_selected_color == QG_COLOR_UNKNOWN) {
                /* Fail safe: keep trapdoor closed for unknown objects. */
                qg_hal_set_trapdoor(false);
                qg_hal_indicate_color(QG_COLOR_UNKNOWN);
                change_state(QG_FAULT);
                break;
            }

            qg_hal_indicate_color(s_selected_color);
            qg_hal_move_chute(s_selected_color);
            change_state(QG_POSITIONING_CHUTE);
            s_deadline_ms = now_ms + QG_CHUTE_SETTLE_MS;
            break;

        case QG_POSITIONING_CHUTE:
            if (!deadline_reached(now_ms, s_deadline_ms)) break;
            /* Later this transition will check actual servo ADC feedback. */
            qg_hal_set_trapdoor(true);
            change_state(QG_DROPPING);
            s_deadline_ms = now_ms + QG_TRAPDOOR_OPEN_MS;
            break;

        case QG_DROPPING:
            if (!deadline_reached(now_ms, s_deadline_ms)) break;
            qg_hal_set_trapdoor(false);
            ++s_counts[s_selected_color];
            print_counts();
            change_state(QG_RESETTING);
            s_deadline_ms = now_ms + QG_RESET_DELAY_MS;
            break;

        case QG_RESETTING:
            if (deadline_reached(now_ms, s_deadline_ms) &&
                !qg_hal_object_present()) {
                qg_hal_indicate_color(QG_COLOR_UNKNOWN);
                s_selected_color = QG_COLOR_UNKNOWN;
                change_state(QG_WAITING);
            }
            break;

        case QG_FAULT:
            /* Real hardware fault recovery will be added after its inputs
             * and safety requirements are specified. Never release here. */
            break;

        default:
            qg_hal_set_trapdoor(false);
            change_state(QG_FAULT);
            break;
    }
}

qg_state_t qg_sorter_get_state(void) {
    return s_state;
}

uint32_t qg_sorter_get_count(qg_color_t color) {
    if (color >= QG_COLOR_RED && color < QG_COLOR_BIN_COUNT) {
        return s_counts[color];
    }
    return 0;
}
