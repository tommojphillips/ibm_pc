/* keyboard.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Keyboard
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#include "backend/utility/ring_buffer.h"

typedef struct I8259_PIC I8259_PIC;

typedef struct KBD {
	uint8_t enabled;
	uint8_t do_reset;
	uint8_t data;
	uint64_t reset_elapsed;
	RING_BUFFER key_buffer;
	I8259_PIC* pic_p;
} KBD;

uint8_t kbd_get_data(KBD* kbd);
void kbd_set_enable(KBD* kbd, uint8_t enable);
void kbd_set_clk(KBD* kbd, uint8_t clk);
void kbd_reset(KBD*);
void kbd_tick(KBD* kbd);

int kbd_create(KBD* kbd);
void kbd_destroy(KBD* kbd);

void kbd_init(KBD* kbd, I8259_PIC* pic);

#endif
