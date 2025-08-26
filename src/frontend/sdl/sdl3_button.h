/* sdl3_button.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * SDL3 Button implementation
 */

#ifndef SDL3_BUTTON_H
#define SDL3_BUTTON_H

#include <SDL3/SDL.h>
#include <stdint.h>

#include "sdl3_typedefs.h"

typedef struct {
    SDL_FRect draw_rect;    // dimensions of button
    
    COLOR_RGBA colour;

    int pressed;
    const char* title;
} BUTTON;

void button_process_event(BUTTON* btn, const SDL_Event* e);
int update_button(SDL_Renderer* r, BUTTON* btn);

#endif
