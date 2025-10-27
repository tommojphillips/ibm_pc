/* timing.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * timing
 */

#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>

#define HZ_TO_MS(x) (1000.0 / (x))
#define MS_TO_HZ(x) (1000.0 / (x))

/* Frame State */
typedef struct FRAME_STATE {
	uint64_t start_frame_time; /* start time of current frame in ms. */
	uint64_t freq;             /* counter freq */
	double ms;                 /* current frame time in ms. */
	double last_ms;            /* last frame time in ms. */
	double target_ms;          /* target frame time in ms. */
} FRAME_STATE;

/* get ticks callback */
typedef uint64_t(*TIMING_GET_TICKS_CB)(void);

/* frame state callback */
typedef int(*TIMING_FRAME_STATE_CB)(FRAME_STATE* state);

/* init frame state callback */
typedef int(*TIMING_INIT_FRAME_STATE_CB)(FRAME_STATE* state, double target_ms);

/* Set get_ticks_ms() callback */
void timing_set_cb_get_ticks_ms(TIMING_GET_TICKS_CB get_ticks_ms);

/* Set get_ticks_ns() callback */
void timing_set_cb_get_ticks_ns(TIMING_GET_TICKS_CB get_ticks_ns);

/* set init_frame() callback */
void timing_set_cb_init_frame(TIMING_INIT_FRAME_STATE_CB cb);

/* set reset_frame() callback */
void timing_set_cb_reset_frame(TIMING_FRAME_STATE_CB cb);

/* set new_frame() callback */
void timing_set_cb_new_frame(TIMING_FRAME_STATE_CB cb);

/* set check_frame() callback */
void timing_set_cb_check_frame(TIMING_FRAME_STATE_CB cb);

/* Get ticks since startup in milliseconds */
uint64_t timing_get_ticks_ms(void);

/* Get ticks since startup in nanoseconds */
uint64_t timing_get_ticks_ns(void);

/* Init frame state; set target_ms */
int timing_init_frame(FRAME_STATE* time, double target_ms);

/* Reset frame */
int timing_reset_frame(FRAME_STATE* time);

/* New frame */
int timing_new_frame(FRAME_STATE* time);

/* Check frame; Check if target_ms has elasped
	Returns: 1 if target_ms has elasped. otherwise 0. */
int timing_check_frame(FRAME_STATE* time);

#endif
