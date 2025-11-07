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

/* Window Instance */
int window_instance_create(WINDOW_MANAGER* manager, WINDOW_INSTANCE** instance) {
	if (manager == NULL) {
		dbg_print("Failed to create window instance. Window manager is NULL\n");
		return 1;
	}

	int index = -1;

	/* find first removed instance */
	for (int i = 0; i < manager->instance_index; ++i) {
		if (!(manager->instances[i].window_state & WINDOW_INSTANCE_STATE_CREATED)) {
			index = i;
		}
	}

	/* did not find a removed instance; add to end */
	if (index < 0) {
		index = manager->instance_index;
		if (index >= manager->instance_count) {
			dbg_print("Failed to create window instance. Window instance index out of range.\n");
			return 1;
		}
		manager->instance_index++;
	}

	*instance = &manager->instances[index];
	memset(*instance, 0, sizeof(WINDOW_INSTANCE));
	(*instance)->manager = manager;
	(*instance)->window_state = WINDOW_INSTANCE_STATE_CREATED;
	return 0;
}
int window_instance_destroy(WINDOW_INSTANCE* instance) {

	if (instance == NULL) {
		dbg_print("Failed to destroy window instance. Window instance is NULL\n");
		return 1;
	}

	if (instance->manager == NULL) {
		dbg_print("Failed to destroy window instance. Window manager is NULL\n");
		return 1;
	}

	if ((instance->window_state & WINDOW_INSTANCE_STATE_CREATED) == 0) {
		return 1; /* Already Destroyed */
	}
	
	if (instance->on_process_event != NULL) {
		free(instance->on_process_event);
		instance->on_process_event = NULL;
	}
	instance->on_process_event_count = 0;
	instance->on_process_event_index = 0;

	if (instance->on_render != NULL) {
		free(instance->on_render);
		instance->on_render = NULL;
	}
	instance->on_render_count = 0;
	instance->on_render_index = 0;

	if (instance->on_render_param1 != NULL) {
		free(instance->on_render_param1);
		instance->on_render_param1 = NULL;
	}

	if (instance->on_render_param2 != NULL) {
		free(instance->on_render_param2);
		instance->on_render_param2 = NULL;
	}

	instance->window_state &= ~WINDOW_INSTANCE_STATE_CREATED;		
	return 0;
}

int window_instance_open(WINDOW_INSTANCE* instance) {

	if (instance == NULL) {
		dbg_print("[WINDOW] Failed to open window instance. Window instance is NULL\n");
		return 1;
	}
	
	if (instance->manager == NULL) {
		dbg_print("[WINDOW] Failed to open window instance. Window manager is NULL\n");
		return 1;
	}

	if ((instance->window_state & WINDOW_INSTANCE_STATE_CREATED) == 0) {
		dbg_print("[WINDOW] Failed to open window instance. Window instance not created\n");
		return 1;
	}

	if (instance->window_state & WINDOW_INSTANCE_STATE_OPEN) {
		dbg_print("[WINDOW] Failed to open window instance. Window instance already open\n");
		return 1;
	}

	/* init window */
	instance->window = SDL_CreateWindow(instance->title, instance->transform.w, instance->transform.h, SDL_WINDOW_RESIZABLE);
	if (instance->window == NULL) {
		dbg_print("[WINDOW] Failed to open window instance. Could not create window: %s\n", SDL_GetError());
		return 1;
	}

	/* create renderer */
	instance->renderer = SDL_CreateRenderer(instance->window, NULL);
	if (instance->renderer == NULL) {
		dbg_print("[WINDOW] Failed to open window instance. Could not create renderer: %s\n", SDL_GetError());
		return 1;
	}

	/* get window id */
	instance->window_id = SDL_GetWindowID(instance->window);
	if (instance->window_id == 0) {
		dbg_print("[WINDOW] Failed to open window instance. Could not get window id: %s\n", SDL_GetError());
		return 1;
	}

	SDL_SetWindowPosition(instance->window, instance->transform.x, instance->transform.y);

	instance->text_engine = instance->manager->text_engine;

	instance->window_state |= WINDOW_INSTANCE_STATE_OPEN;
	instance->manager->instances_open++;
	return 0;
}
int window_instance_close(WINDOW_INSTANCE* instance) {

	if (instance == NULL) {
		dbg_print("[WINDOW] Failed to close window instance. Window instance is NULL\n");
		return 1;
	}

	if (instance->manager == NULL) {
		dbg_print("[WINDOW] Failed to close window instance. Window manager is NULL\n");
		return 1;
	}

	if ((instance->window_state & WINDOW_INSTANCE_STATE_OPEN) == 0) {
		return 1; /* Already Closed */
	}

	if (instance->renderer != NULL) {
		SDL_DestroyRenderer(instance->renderer);
		instance->renderer = NULL;
	}
	if (instance->window != NULL) {
		SDL_DestroyWindow(instance->window);
		instance->window = NULL;
	}

	instance->window_id = 0;

	instance->text_engine = NULL;

	instance->window_state &= ~WINDOW_INSTANCE_STATE_OPEN;
	instance->manager->instances_open--;
	return 0;
}

static void window_instance_process_event(WINDOW_INSTANCE* instance, SDL_Event* e) {
	switch (e->type) {

		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			window_instance_close(instance);
			window_instance_destroy(instance);
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

	// call on_process_event() for window instance
	for (int i = 0; i < instance->on_process_event_index; ++i) {
		instance->on_process_event[i](instance, e);
	}
}

static void window_instance_render(WINDOW_INSTANCE* instance) {
	sdl_timing_new_frame(&instance->time);
	if (sdl_timing_check_frame(&instance->time)) {

		// clear render buffer
		SDL_SetRenderDrawColor(instance->renderer, 0xE0, 0xE0, 0xE0, 0xFF);
		SDL_RenderClear(instance->renderer);

		// call on_render() for window instance
		for (int i = 0; i < instance->on_process_event_index; ++i) {
			void* param1 = instance->on_render_param1[i];
			if (param1 == NULL) {
				param1 = instance;
			}
			instance->on_render[i](param1, instance->on_render_param2[i]);
		}

		SDL_RenderPresent(instance->renderer);
	}
}

void window_instance_set_transform(WINDOW_INSTANCE* instance, int32_t x, int32_t y, int32_t w, int32_t h) {
	if (instance != NULL) {
		instance->transform.x = x;
		instance->transform.y = y;
		instance->transform.w = w;
		instance->transform.h = h;
		if (instance->window_state & WINDOW_INSTANCE_STATE_OPEN) {
			/* move,resize the window */
			SDL_SetWindowPosition(instance->window, x, y);
			SDL_SetWindowSize(instance->window, w, h);
		}
	}
}
void window_instance_set_min_size(WINDOW_INSTANCE* instance, int32_t w, int32_t h) {
	if (instance != NULL && instance->window != NULL) {
		SDL_SetWindowMinimumSize(instance->window, w, h);
		if (instance->window_state & WINDOW_INSTANCE_STATE_OPEN) {
			/* move the window back to where it was prior to the resize. */
			SDL_SetWindowPosition(instance->window, instance->transform.x, instance->transform.y);
		}
	}
}

int window_instance_is_full_screen(WINDOW_INSTANCE* instance) {
	return (instance->window_state & WINDOW_INSTANCE_STATE_FULL_SCREEN);
}
void window_instance_set_full_screen(WINDOW_INSTANCE* instance, int fullscreen) {
	int fs = instance->window_state & WINDOW_INSTANCE_STATE_FULL_SCREEN;
	if (fs != fullscreen) {
		if (fullscreen == 0) {
			instance->window_state &= ~WINDOW_INSTANCE_STATE_FULL_SCREEN;
		}
		else {
			instance->window_state |= WINDOW_INSTANCE_STATE_FULL_SCREEN;
		}
		SDL_SetWindowFullscreen(instance->window, fullscreen);
	}
}
void window_instance_toggle_full_screen(WINDOW_INSTANCE* instance) {
	instance->window_state ^= WINDOW_INSTANCE_STATE_FULL_SCREEN;
	int fullscreen = instance->window_state & WINDOW_INSTANCE_STATE_FULL_SCREEN;
	SDL_SetWindowFullscreen(instance->window, fullscreen);
}

int window_instance_add_cb_on_process_event(WINDOW_INSTANCE* instance, WINDOW_INSTANCE_CB_ON_PROCESS_EVENT cb) {
	if (instance == NULL) {
		dbg_print("[WINDOW] Failed to add cb on_process_event. Window instance is NULL\n");
		return -1;
	}

	int index = instance->on_process_event_index;

	if (index >= instance->on_process_event_count) {
		WINDOW_INSTANCE_CB_ON_PROCESS_EVENT* new_cb = realloc(instance->on_process_event, (instance->on_process_event_count + 1) * sizeof(WINDOW_INSTANCE_CB_ON_PROCESS_EVENT));
		if (new_cb == NULL) {
			printf("[WINDOW] Failed to add cb on_process_event. Realloc failed for on_process_event\n");
			return -1;
		}

		instance->on_process_event = new_cb;
		instance->on_process_event_count += 1;
	}

	instance->on_process_event[index] = cb;
	instance->on_process_event_index++;
	return index;
}
int window_instance_add_cb_on_render(WINDOW_INSTANCE* instance, WINDOW_INSTANCE_CB cb, void* cb_param1, void* cb_param2) {
	if (instance == NULL) {
		dbg_print("[WINDOW] Failed to add cb on_render. Window instance is NULL\n");
		return -1;
	}

	int index = instance->on_render_index;

	if (index >= instance->on_render_count) {
		WINDOW_INSTANCE_CB* new_cb = realloc(instance->on_render, (instance->on_render_count + 1) * sizeof(WINDOW_INSTANCE_CB));
		if (new_cb == NULL) {
			dbg_print("[WINDOW] Failed to add cb on_render. Realloc failed for on_render\n");
			return -1;
		}
			
		void* new_param1 = realloc(instance->on_render_param1, (instance->on_render_count + 1) * sizeof(void*));
		if (new_param1 == NULL) {
			dbg_print("[WINDOW] Failed to add cb on_render. Realloc failed for on_render_param1\n");
			return -1;
		}
			
		void* new_param2 = realloc(instance->on_render_param2, (instance->on_render_count + 1) * sizeof(void*));
		if (new_param2 == NULL) {
			dbg_print("[WINDOW] Failed to add cb on_render. Realloc failed for on_render_param2\n");
			return -1;
		}

		instance->on_render = new_cb;
		instance->on_render_param1 = new_param1;
		instance->on_render_param2 = new_param2;
		instance->on_render_count += 1;
	}

	instance->on_render[index] = cb;
	instance->on_render_param1[index] = cb_param1;
	instance->on_render_param2[index] = cb_param2;
	instance->on_render_index++;
	return index;
}

int window_instance_set_cb_on_process_event(WINDOW_INSTANCE* instance, int index, WINDOW_INSTANCE_CB_ON_PROCESS_EVENT cb) {
	if (instance == NULL) {
		dbg_print("[WINDOW] Failed to set cb on_process_event. Window instance is NULL\n");
		return 1;
	}
	
	if (index >= instance->on_process_event_index) {
		dbg_print("[WINDOW] Failed to set cb on_process_event. Index out of range\n");
		return 1;
	}

	instance->on_process_event[index] = cb;
	return 0;
}
int window_instance_set_cb_on_render(WINDOW_INSTANCE* instance, int index, WINDOW_INSTANCE_CB cb, void* cb_param1, void* cb_param2) {
	if (instance == NULL) {
		dbg_print("[WINDOW] Failed to set cb on_render. Window instance is NULL\n");
		return 1;
	}

	if (index >= instance->on_render_index) {
		dbg_print("[WINDOW] Failed to set cb on_render. Index out of range\n");
		return 1;
	}

	instance->on_render[index] = cb;
	instance->on_render_param1[index] = cb_param1;
	instance->on_render_param2[index] = cb_param2;
	return 0;
}

/* Window Instance Manager */
int window_manager_create(WINDOW_MANAGER** manager, uint16_t window_count) {
	if (*manager != NULL) {
		dbg_print("[WINDOW MANAGER] Failed to create window manager. manager not NULL\n");
		return 1;
	}

	*manager = calloc(1, sizeof(WINDOW_MANAGER));
	if (*manager == NULL) {
		dbg_print("[WINDOW MANAGER] Failed to create window manager. Could not allocate memory for window manager\n");
		return 1;
	}

	/* init text engine */
	(*manager)->text_engine = TTF_CreateSurfaceTextEngine();
	if ((*manager)->text_engine == NULL) {
		dbg_print("[WINDOW MANAGER] Failed to create window manager. Could not init text engine: %s\n", SDL_GetError());
		return 1;
	}

	(*manager)->instances = calloc(window_count, sizeof(WINDOW_INSTANCE));
	if ((*manager)->instances == NULL) {
		dbg_print("[WINDOW MANAGER] Failed to create window manager. Could not allocate memory for window instances\n");
		return 1;
	}
	(*manager)->instance_count = window_count;
	(*manager)->instance_index = 0;
	return 0;
}
void window_manager_destroy(WINDOW_MANAGER* manager) {
	if (manager != NULL) {
		if (manager->instances != NULL) {

			for (int i = 0; i < manager->instance_index; ++i) {
				window_instance_close(&manager->instances[i]);
				window_instance_destroy(&manager->instances[i]);
			}

			free(manager->instances);
			manager->instances = NULL;
		}
		manager->instance_count = 0;
		manager->instance_index = 0;

		/* Free text engine */
		if (manager->text_engine != NULL) {
			TTF_DestroySurfaceTextEngine(manager->text_engine);
			manager->text_engine = NULL;
		}

		/* Free window manager instance */
		free(manager);
		manager = NULL;
	}
}

int window_manager_process_event(WINDOW_MANAGER* manager, SDL_Event* e) {
	for (int i = 0; i < manager->instance_index; ++i) {
		if ((manager->instances[i].window_state & WINDOW_INSTANCE_STATE_OPEN) && e->window.windowID == manager->instances[i].window_id) {
			window_instance_process_event(&manager->instances[i], e);
		}
	}

	/* If multiple windows are open and all the windows are closed at the same time, SDL_EVENT_QUIT is 
	 never sent. Instead, a SDL_EVENT_WINDOW_DESTROYED is sent. If there is only 1 window open and its
	 closed, then SDL_EVENT_QUIT is sent and SDL_EVENT_WINDOW_DESTROYED is not */
	if (e->type == SDL_EVENT_WINDOW_DESTROYED && manager->instances_open == 0) {
		return 1; /* QUIT */
	}

	return 0;
}
void window_manager_update(WINDOW_MANAGER* manager) {
	for (int i = 0; i < manager->instance_index; ++i) {
		if (manager->instances[i].window_state & WINDOW_INSTANCE_STATE_OPEN) {
			window_instance_render(&manager->instances[i]);
		}
	}
}
