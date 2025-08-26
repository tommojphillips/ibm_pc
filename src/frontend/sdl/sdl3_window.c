/* sdl3_window.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Implements sdl3 window instance wrapper
 */

#include <stdint.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "sdl3_window.h"
#include "sdl3_timing.h"
#include "sdl3_font.h"

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

/* Global window manager state */
WINDOW_MANAGER* window_manager = NULL;

static void window_instance_destroy(WINDOW_INSTANCE* instance) {
	if (instance != NULL) {
		
		if (instance->renderer != NULL) {
			SDL_DestroyRenderer(instance->renderer);
			instance->renderer = NULL;
		}
		if (instance->window != NULL) {
			SDL_DestroyWindow(instance->window);
			instance->window = NULL;
		}
	}
}
static void window_instance_process_event(WINDOW_INSTANCE* instance, SDL_Event* e) {
	switch (e->type) {

		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			window_instance_close(instance);
			instance->open = 0;
			break;

		case SDL_EVENT_WINDOW_RESIZED:
			instance->transform.w = e->window.data1;
			instance->transform.h = e->window.data2;
			break;

		case SDL_EVENT_WINDOW_MOVED:
			instance->transform.x = e->window.data1;
			instance->transform.y = e->window.data2;
			break;
	}
}
static void window_instance_render(WINDOW_INSTANCE* instance) {
	sdl_timing_new_frame(&instance->time);
	if (sdl_timing_check_frame(&instance->time)) {
	
		// clear render buffer
		SDL_SetRenderDrawColor(instance->renderer, 0xE0, 0xE0, 0xE0, 0xFF);
		SDL_RenderClear(instance->renderer);

		// render window
		if (instance->on_render != NULL) {
			instance->on_render(instance, instance->on_render_param);
		}

		SDL_RenderPresent(instance->renderer);
	}
}

/* Window Instance */
int window_instance_create(WINDOW_INSTANCE** instance) {
	if (window_manager == NULL) {
		dbg_print("Error: Global Window Manager not created (window_instance_create)\n");
		return 1;
	}

	if (window_manager->instance_index > window_manager->instance_count - 1) {
		dbg_print("Out of Window instances.\n");
		return 1;
	}

	*instance = &window_manager->instances[window_manager->instance_index];
	++window_manager->instance_index;
	return 0;
}
void window_instance_open(WINDOW_INSTANCE* instance) {
	/* init window */
	instance->window = SDL_CreateWindow(instance->title, instance->transform.w, instance->transform.h, SDL_WINDOW_RESIZABLE);
	if (instance->window == NULL) {
		dbg_print("Failed to create window: %s\n", SDL_GetError());
		exit(1);
	}
	instance->window_id = SDL_GetWindowID(instance->window);
	if (instance->window_id == 0) {
		dbg_print("Failed to get window id: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_SetWindowPosition(instance->window, instance->transform.x, instance->transform.y);

	/* create renderer */
	instance->renderer = SDL_CreateRenderer(instance->window, NULL);
	if (instance->renderer == NULL) {
		dbg_print("Failed to create renderer: %s\n", SDL_GetError());
		exit(1);
	}

	instance->text_engine = window_manager->text_engine;
	instance->open = 1;
}
void window_instance_close(WINDOW_INSTANCE* instance) {
	if (window_manager == NULL) {
		dbg_print("Error: Global Window Manager not created (window_instance_close)\n");
		return;
	}

	if (instance != NULL) {

		int found_id = 0;
		for (uint8_t i = 0; i < window_manager->instance_index; ++i) {
			if (instance->window_id == window_manager->instances[i].window_id) {
				found_id = 1;

				if (window_manager->instance_index > 0) {

					window_instance_destroy(&window_manager->instances[i]);

					if (i != window_manager->instance_index - 1) {
						// Move last entry to this slot
						memcpy(&window_manager->instances[i], &window_manager->instances[window_manager->instance_index - 1], sizeof(WINDOW_INSTANCE));
						memset(&window_manager->instances[window_manager->instance_index - 1], 0, sizeof(WINDOW_INSTANCE));
					}
					else {
						memset(&window_manager->instances[i], 0, sizeof(WINDOW_INSTANCE));
					}

					window_manager->instance_index--;
				}
				break;
			}
		}

		if (!found_id) {
			dbg_print("Error: Could not find the window by ID. Failed to remove window instance from the service_list\n");
		}
	}
}
void window_instance_set_transform(WINDOW_INSTANCE* instance, int x, int y, int w, int h) {
	if (instance != NULL) {
		instance->transform.x = x;
		instance->transform.y = y;
		instance->transform.w = w;
		instance->transform.h = h;
		if (instance->open) {
			/* move,resize the window */
			SDL_SetWindowPosition(instance->window, x, y);
			SDL_SetWindowSize(instance->window, w, h);
		}
	}
}
void window_instance_set_min_size(WINDOW_INSTANCE* instance, int w, int h) {
	if (instance != NULL && instance->window != NULL) {
		SDL_SetWindowMinimumSize(instance->window, w, h);
		if (instance->open) {
			/* move the window back to where it was prior to the resize. */
			SDL_SetWindowPosition(instance->window, instance->transform.x, instance->transform.y);
		}
	}
}

void window_instance_set_cb_on_process_event(WINDOW_INSTANCE* instance, WINDOW_INSTANCE_CB cb) {
	if (instance != NULL) {
		instance->on_process_event = cb;
	}
}
void window_instance_set_cb_on_render(WINDOW_INSTANCE* instance, WINDOW_INSTANCE_CB cb, void* cb_param) {
	if (instance != NULL) {
		instance->on_render = cb;
		instance->on_render_param = cb_param;
	}
}

/* Window Instance Manager */
int window_manager_create(int window_count) {
	if (window_manager != NULL) {
		dbg_print("Error: Window Manager already created\n");
		return 1;
	}

	window_manager = (WINDOW_MANAGER*)calloc(1, sizeof(WINDOW_MANAGER));
	if (window_manager == NULL) {
		dbg_print("Error: Failed to allocate Window Manager\n");
		return 1;
	}

	/* init text engine */
	window_manager->text_engine = TTF_CreateSurfaceTextEngine();
	if (window_manager->text_engine == NULL) {
		dbg_print("Failed to init text engine: %s\n", SDL_GetError());
		return 1;
	}

	window_manager->instances = (WINDOW_INSTANCE*)calloc(window_count, sizeof(WINDOW_INSTANCE));
	if (window_manager->instances == NULL) {
		dbg_print("Failed to allocate window instances\n");
		return 1;
	}
	window_manager->instance_count = window_count;
	window_manager->instance_index = 0;
	return 0;
}
void window_manager_destroy() {
	if (window_manager != NULL) {
		if (window_manager->instances != NULL) {

			for (int i = 0; i < window_manager->instance_index; ++i) {
				window_instance_destroy(&window_manager->instances[i]);
			}

			free(window_manager->instances);
			window_manager->instances = NULL;
		}

		window_manager->instance_count = 0;
		window_manager->instance_index = 0;


		/* Free text engine */
		if (window_manager->text_engine != NULL) {
			TTF_DestroySurfaceTextEngine(window_manager->text_engine);
			window_manager->text_engine = NULL;
		}

		/* Free Global window manager instance */
		free(window_manager);
		window_manager = NULL;
	}
}
void window_manager_process_event(SDL_Event* e) {
	for (int i = 0; i < window_manager->instance_index; ++i) {
		if (e->window.windowID == window_manager->instances[i].window_id) {
			window_instance_process_event(&window_manager->instances[i], e);
			if (window_manager->instances[i].on_process_event != NULL) {
				window_manager->instances[i].on_process_event(&window_manager->instances[i], e);
			}
		}
	}
}
void window_manager_update() {
	for (int i = 0; i < window_manager->instance_index; ++i) {
		window_instance_render(&window_manager->instances[i]);
	}
}
