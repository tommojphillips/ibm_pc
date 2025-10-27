/* sdl3_common.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
*/

#ifndef SDL3_COMMON_H
#define SDL3_COMMON_H

#include <stdint.h>

#include <SDL3/SDL.h>

#include "backend/ring_buffer.h"

#define KEYS_SIZE 10

typedef int(*SDL_PROCESS_EVENT_CB)(SDL_Event* e);
typedef void(*SDL_UPDATE_CB)(void);

/* Window SDL */
typedef struct SDL {
	SDL_Event e;
	int quit;
	RING_BUFFER key_state;
	int kbd_new_input;

	SDL_UPDATE_CB* on_update;
	int32_t on_update_count;
	int32_t on_update_index;

	SDL_PROCESS_EVENT_CB* on_process_event;
	int32_t on_process_event_count;
	int32_t on_process_event_index;
} SDL;

/* Global SDL State */
extern SDL* sdl;

/* SDL Create 
 \return 1 if error; 0 if success */
int sdl_create(void);

/* SDL Destroy */
void sdl_destroy(void);

/* SDL Update */
void sdl_update(void);

/* SDL Add on_process_event Callback 
 \return -1 if error; index of on_process_event if success */
int sdl_add_cb_on_process_event(SDL_PROCESS_EVENT_CB cb);

/* SDL Add on_update Callback
 \return -1 if error; index of on_update if success */
int sdl_add_cb_on_update(SDL_UPDATE_CB cb);

#endif
