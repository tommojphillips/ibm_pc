/* sdl3_input.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Input
 */

#include <stdio.h>

#include <SDL3/SDL.h>

#include "sdl3_common.h"
#include "sdl3_window.h"
#include "sdl3_keys.h"

#include "backend/ibm_pc.h"
#include "backend/utility/ring_buffer.h"

static void check_keys(WINDOW_INSTANCE* instance, SDL_Event* e) {

	switch (e->key.scancode) {
		
		case SDL_SCANCODE_F11:
			if (e->key.down) {
				ibm_pc_reset();
			}
			return; /* ignore key */

		case SDL_SCANCODE_KP_ENTER:
			if (e->key.down) {
				if (ibm_pc->step) {
					ibm_pc->step = 0;
				}
				else {
					ibm_pc->step = 1;
				}
			}
			return; /* ignore key */
				
		case SDL_SCANCODE_KP_PLUS:
			if (e->key.down) {
				if (ibm_pc->step) {
					ibm_pc->step = 2;
				}
			}
			return; /* ignore key */

		case SDL_SCANCODE_RETURN:
			if (e->key.mod & SDL_KMOD_ALT) {
				if (e->key.down) {
					window_instance_toggle_full_screen(instance);
				}
				return; /* ignore key */
			}
			break;
	}

	if (e->key.repeat) {
		return; /* ignore key; repeat */
	}

	if (pc_scancode[e->key.scancode] == 0xFF) {
		return; /* ignore key; scancode is not a pc scancode */
	}

	if (e->key.down) {
		ring_buffer_push(&ibm_pc->kbd.key_buffer, pc_scancode[e->key.scancode]);
	}
	else {
		ring_buffer_push(&ibm_pc->kbd.key_buffer, pc_scancode[e->key.scancode] | 0x80);
	}
}

void input_process_event(WINDOW_INSTANCE* instance, SDL_Event* e) {
	switch (e->type) {
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
			check_keys(instance, e);
			break;
	}
}
