/* sdl3_input.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Input
 */

#ifndef SDL_INPUT_H
#define SDL_INPUT_H

#include <stdint.h>

typedef struct WINDOW_INSTANCE WINDOW_INSTANCE;
typedef union SDL_Event SDL_Event;

void input_process_event(WINDOW_INSTANCE* instance, SDL_Event* e);

#endif
