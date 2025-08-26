/* sdl3_common.h
* GitHub: https:\\github.com\tommojphillips
*/

#ifndef SDL3_COMMON_H
#define SDL3_COMMON_H

#include <stdint.h>

#include <SDL3/SDL.h>

#include "..\..\backend\ring_buffer.h"

#define KEYS_SIZE 10

/* Window SDL */
typedef struct SDL {
	SDL_Event e;
	int quit;
	RING_BUFFER key_state;
	int kbd_new_input;
} SDL;

/* Global SDL State */
extern SDL* sdl;

/* SDL Create; Allocates memory for the SDL structure 
 Returns: 1 if error; 0 if success */
int sdl_create();

/* SDL Destroy */
void sdl_destroy();

/* SDL Update; Updates all window instances from the service_list.
 Calls on_process_event() for all windows in the service_list.
 Checks if the desired frame time for every window has elapsed and calls on_render(). */
void sdl_update();

#endif
