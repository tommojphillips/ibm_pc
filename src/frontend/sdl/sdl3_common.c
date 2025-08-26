/* sdl3_common.c
* GitHub: https:\\github.com\tommojphillips
*/

#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "sdl3_common.h"
#include "sdl3_window.h"

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

/* Global SDL State */
SDL* sdl = NULL;

static void process_event(SDL_Event* e) {
	switch (e->type) {
		case SDL_EVENT_QUIT:
			sdl->quit = 1;
			break;
	}
}

/* SDL struct */
int sdl_create() {
	if (sdl != NULL) {
		dbg_print("Error: Global SDL instance already created\n");
		return 1;
	}

	sdl = (SDL*)calloc(1, sizeof(SDL));
	if (sdl == NULL) {
		dbg_print("Failed to allocate Global SDL instance\n");
		return 1;
	}

	if (ring_buffer_init(&sdl->key_state, KEYS_SIZE)) {
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
	return 0;
}
void sdl_destroy() {

	if (sdl != NULL) {

		window_manager_destroy();

		TTF_Quit();
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		SDL_Quit();

		ring_buffer_destroy(&sdl->key_state);

		/* Free Global SDL State */
		free(sdl);
		sdl = NULL;
	}
}
void sdl_update() {
	while (SDL_PollEvent(&sdl->e)) {
		process_event(&sdl->e);
		window_manager_process_event(&sdl->e);
	}
	window_manager_update();
}
