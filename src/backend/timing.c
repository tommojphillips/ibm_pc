/* timing.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * timing
 */

#include <stdint.h>

#include "timing.h"

static TIMING_GET_TICKS_CB ticks_ms_cb = NULL;
static TIMING_GET_TICKS_CB ticks_ns_cb = NULL;
static TIMING_INIT_FRAME_STATE_CB init_frame_cb = NULL;
static TIMING_FRAME_STATE_CB reset_frame_cb = NULL;
static TIMING_FRAME_STATE_CB new_frame_cb   = NULL;
static TIMING_FRAME_STATE_CB check_frame_cb = NULL;

void timing_set_cb_get_ticks_ms(TIMING_GET_TICKS_CB cb) {
    ticks_ms_cb = cb;
}

void timing_set_cb_get_ticks_ns(TIMING_GET_TICKS_CB cb) {
    ticks_ns_cb = cb;
}

void timing_set_cb_init_frame(TIMING_INIT_FRAME_STATE_CB cb) {
    init_frame_cb = cb;
}

void timing_set_cb_reset_frame(TIMING_FRAME_STATE_CB cb) {
    reset_frame_cb = cb;
}

void timing_set_cb_new_frame(TIMING_FRAME_STATE_CB cb) {
    new_frame_cb = cb;
}

void timing_set_cb_check_frame(TIMING_FRAME_STATE_CB cb) {
    check_frame_cb = cb;
}

uint64_t timing_get_ticks_ms(void) {
    return ticks_ms_cb();
}

uint64_t timing_get_ticks_ns(void) {
    return ticks_ns_cb();
}

int timing_init_frame(FRAME_STATE* time, double target_ms) {
    return init_frame_cb(time, target_ms);
}
int timing_reset_frame(FRAME_STATE* time) {
    return reset_frame_cb(time);
}
int timing_new_frame(FRAME_STATE* time) {
    return new_frame_cb(time);
}
int timing_check_frame(FRAME_STATE* time) {
    return check_frame_cb(time);
}
