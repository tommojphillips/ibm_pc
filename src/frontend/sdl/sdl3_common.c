/* sdl3_common.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
*/

#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "sdl3_common.h"

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

/* Global SDL State */
SDL* sdl = NULL;

/* SDL struct */
int sdl_create(void) {
	if (sdl != NULL) {
		dbg_print("Error: Global SDL instance already created\n");
		return 1;
	}

	sdl = calloc(1, sizeof(SDL));
	if (sdl == NULL) {
		dbg_print("Failed to allocate Global SDL instance\n");
		return 1;
	}

	if (ring_buffer_create(&sdl->key_state, KEYS_SIZE)) {
		dbg_print("Failed to allocate key state ring buffer\n");
		return 1;
	}

	// init SDL video 
	if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		dbg_print("Failed to init sub sysyem: %s\n", SDL_GetError());
		return 1;
	}

	/* init TTF */
	if (!TTF_Init()) {
		dbg_print("Failed to init ttf: %s\n", SDL_GetError());
		return 1;
	}

	sdl->quit = 0;

	sdl->on_process_event = NULL;
	sdl->on_process_event_count = 0;
	sdl->on_process_event_index = 0;
	
	sdl->on_update = NULL;
	sdl->on_update_count = 0;
	sdl->on_update_index = 0;
	return 0;
}
void sdl_destroy(void) {

	if (sdl != NULL) {

		TTF_Quit();
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		SDL_Quit();

		ring_buffer_destroy(&sdl->key_state);

		if (sdl->on_update != NULL) {
			free(sdl->on_update);
			sdl->on_update = NULL;
		}
		sdl->on_update_count = 0;
		sdl->on_update_index = 0;

		if (sdl->on_process_event != NULL) {
			free(sdl->on_process_event);
			sdl->on_process_event = NULL;
		}
		sdl->on_process_event_count = 0;
		sdl->on_process_event_index = 0;

		/* Free Global SDL State */
		free(sdl);
		sdl = NULL;
	}
}

void sdl_update(void) {
	while (SDL_PollEvent(&sdl->e)) {
		/* call on_process_event() */
		for (int i = 0; i < sdl->on_process_event_index; ++i) {
			if (sdl->on_process_event[i](&sdl->e)) {
				sdl->quit = 1;
				return;
			}
		}
	}

	/* call on_update() */
	for (int i = 0; i < sdl->on_update_index; ++i) {
		sdl->on_update[i]();
	}
}

int sdl_add_cb_on_process_event(SDL_PROCESS_EVENT_CB cb) {
	int index = sdl->on_process_event_index;

	if (index >= sdl->on_process_event_count) {
		SDL_PROCESS_EVENT_CB* new_cb = realloc(sdl->on_process_event, ((size_t)sdl->on_process_event_count + 1) * sizeof(SDL_PROCESS_EVENT_CB));
		if (new_cb == NULL) {
			printf("[SDL] Failed to realloc on_process_event\n");
			return -1;
		}

		sdl->on_process_event = new_cb;
		sdl->on_process_event_count += 1;
	}

	sdl->on_process_event[index] = cb;
	sdl->on_process_event_index++;
	return index;
}
int sdl_add_cb_on_update(SDL_UPDATE_CB cb) {
	int index = sdl->on_update_index;

	if (index >= sdl->on_update_count) {
		SDL_UPDATE_CB* new_cb = realloc(sdl->on_update, ((size_t)sdl->on_update_count + 1) * sizeof(SDL_UPDATE_CB));
		if (new_cb == NULL) {
			printf("[SDL] Failed to realloc on_update\n");
			return -1;
		}

		sdl->on_update = new_cb;
		sdl->on_update_count += 1;
	}

	sdl->on_update[index] = cb;
	sdl->on_update_index++;
	return index;
}
