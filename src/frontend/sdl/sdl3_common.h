/* sdl3_common.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
*/

#ifndef SDL3_COMMON_H
#define SDL3_COMMON_H

#include <stdint.h>

#include <SDL3/SDL.h>

typedef int(*SDL_PROCESS_EVENT_CB)(void* param1, SDL_Event* e);
typedef void(*SDL_UPDATE_CB)(void* param1);

/* Window SDL */
typedef struct SDL {
	SDL_Event e;
	int quit;

	SDL_UPDATE_CB* on_update;
	void** on_update_param1;
	int32_t on_update_count;
	int32_t on_update_index;

	SDL_PROCESS_EVENT_CB* on_process_event;
	void** on_process_event_param1;
	int32_t on_process_event_count;
	int32_t on_process_event_index;
} SDL;

/* SDL Create 
 \return 1 if error; 0 if success */
int sdl_create(SDL** sdl);

/* SDL Destroy */
void sdl_destroy(SDL* sdl);

/* SDL Update */
void sdl_update(SDL* sdl);

/* SDL Add on_process_event Callback 
 \return -1 if error; index of on_process_event if success */
int sdl_add_cb_on_process_event(SDL* sdl, SDL_PROCESS_EVENT_CB cb, void* cb_param1);

/* SDL Add on_update Callback
 \return -1 if error; index of on_update if success */
int sdl_add_cb_on_update(SDL* sdl, SDL_UPDATE_CB cb, void* cb_param1);

#endif
