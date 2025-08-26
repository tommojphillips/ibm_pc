/* sdl3_button.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * SDL3 Button implementation
 */

#include <SDL3/SDL.h>
#include <stdint.h>

#include "sdl3_button.h"

void button_process_event(BUTTON* btn, const SDL_Event* e) {
    if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (e->button.button == SDL_BUTTON_LEFT &&
            e->button.x >= btn->draw_rect.x &&
            e->button.x <= (btn->draw_rect.x + btn->draw_rect.w) &&
            e->button.y >= btn->draw_rect.y &&
            e->button.y <= (btn->draw_rect.y + btn->draw_rect.h)) {
            btn->pressed = true;
        }
    }
}

int update_button(SDL_Renderer* r, BUTTON* btn) {
    SDL_SetRenderDrawColor(r, btn->colour.r, btn->colour.g, btn->colour.b, btn->colour.a);
    SDL_RenderFillRect(r, &btn->draw_rect);
    if (btn->pressed) {
        btn->pressed = 0;
        return 1;
    }
    return 0;
}
