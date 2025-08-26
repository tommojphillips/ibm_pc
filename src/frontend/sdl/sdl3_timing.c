/* sdl3_timing.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Timing
 */

#include <stdio.h>

#include <SDL3/SDL.h>

#include "sdl3_timing.h"
#include "../../backend/timing.h"

TIMER_ID sdl_create_timer_ns(uint64_t interval_ns, TIMER_CALLBACK_NS callback, void* param) {
	TIMER_ID id =  SDL_AddTimerNS(interval_ns, (SDL_NSTimerCallback)callback, param);
	if (id == 0) {
		printf("TimerNS init error: %s\n", SDL_GetError());
	}
	return id;
}
TIMER_ID sdl_create_timer_ms(uint32_t interval_ms, TIMER_CALLBACK_MS callback, void* param) {
	TIMER_ID id =  SDL_AddTimer(interval_ms, (SDL_TimerCallback)callback, param);
	if (id == 0) {
		printf("TimerMS init error: %s\n", SDL_GetError());
	}
	return id;
}
void sdl_destroy_timer(TIMER_ID id) {
	if (!SDL_RemoveTimer(id)) {
		printf("Timer destroy error: %s\n", SDL_GetError());
	}
}

uint64_t sdl_timing_get_ticks_ms() {
	return SDL_GetTicks();
}
uint64_t sdl_timing_get_ticks_ns() {
	return SDL_GetTicksNS();
}

int sdl_timing_reset_frame(FRAME_STATE* time) {
	/* Use Performance Counter */
	time->ms = 0;
	time->last_ms = 0;
	time->start_frame_time = 0;
	return 0;
}

int sdl_timing_new_frame(FRAME_STATE* time) {
	/* Use Performance Counter */
	time->ms += ((SDL_GetPerformanceCounter() - time->start_frame_time) / (double)SDL_GetPerformanceFrequency() * 1000.0);
	time->start_frame_time = SDL_GetPerformanceCounter();
	return 0;
}

int sdl_timing_check_frame(FRAME_STATE* time) {
	/* Use Performance Counter */
	if (time->ms >= time->target_ms) {
		time->last_ms = time->ms;
		time->ms = 0;
		return 1;
	}
	return 0;
}
