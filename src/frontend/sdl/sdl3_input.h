/* sdl3_input.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Input
 */

#ifndef SDL_INPUT_H
#define SDL_INPUT_H

void sdl_input_reset_input(void);
int sdl_input_has_input(void);
uint8_t sdl_input_get_input(void);
int sdl_input_set_input(uint8_t v);

typedef struct WINDOW_INSTANCE WINDOW_INSTANCE;
typedef union SDL_Event SDL_Event;

void input_process_event(WINDOW_INSTANCE* instance, SDL_Event* e);

#endif
