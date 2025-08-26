/* sdl3_timing.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Timing
 */

#ifndef SDL_TIMING_H
#define SDL_TIMING_H

#include <stdint.h>

#include "../../backend/timing.h"

typedef int TIMER_ID;
typedef uint32_t(*TIMER_CALLBACK_MS)(void* params, TIMER_ID id, uint32_t interval);
typedef uint64_t(*TIMER_CALLBACK_NS)(void* params, TIMER_ID id, uint64_t interval);

/* Create Timer - nanosecond precision */
TIMER_ID sdl_create_timer_ns(uint64_t ns, TIMER_CALLBACK_NS callback, void* userdata);

/* Create Timer - millisecond precision */
TIMER_ID sdl_create_timer_ms(uint32_t ms, TIMER_CALLBACK_MS callback, void* userdata);

/* Destroy Timer */
void sdl_destroy_timer(TIMER_ID id);

/* Get ticks in milli seconds since startup */
uint64_t sdl_timing_get_ticks_ms();

/* Get ticks in nano seconds since startup */
uint64_t sdl_timing_get_ticks_ns();

int sdl_timing_reset_frame(FRAME_STATE* time);
int sdl_timing_new_frame(FRAME_STATE* time);
int sdl_timing_check_frame(FRAME_STATE* time);

#endif
