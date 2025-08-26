/* timing.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * timing
 */

#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>

#define HZ_TO_MS(x) (1000.0 / (x))
#define MS_TO_HZ(x) ((x) * 1000.0)

/* Frame State */
typedef struct FRAME_STATE {
	uint64_t start_frame_time; /* start time of current frame in ms. */
	double ms;                 /* current frame time in ms. */
	double last_ms;            /* last frame time in ms. */
	double target_ms;          /* target frame time in ms. */
} FRAME_STATE;

/* get ticks callback */
typedef uint64_t(*TIMING_GET_TICKS_CB)();

typedef int(*TIMING_FRAME_STATE_CB)(FRAME_STATE* state);

/* Set get_ticks_ms() callback */
void timing_set_cb_get_ticks_ms(TIMING_GET_TICKS_CB get_ticks_ms);

/* Set get_ticks_ns() callback */
void timing_set_cb_get_ticks_ns(TIMING_GET_TICKS_CB get_ticks_ns);

/* set reset_frame() callback */
void timing_set_cb_reset_frame(TIMING_FRAME_STATE_CB cb);

/* set new_frame() callback */
void timing_set_cb_new_frame(TIMING_FRAME_STATE_CB cb);

/* set check_frame() callback */
void timing_set_cb_check_frame(TIMING_FRAME_STATE_CB cb);

/* Get ticks since startup in milliseconds */
uint64_t timing_get_ticks_ms();

/* Get ticks since startup in nanoseconds */
uint64_t timing_get_ticks_ns();

int timing_reset_frame(FRAME_STATE* time);
int timing_new_frame(FRAME_STATE* time);
int timing_check_frame(FRAME_STATE* time);

#endif
