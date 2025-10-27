/* keyboard.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Keyboard
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

typedef struct KBD KBD;

typedef struct KBD {
	uint8_t enabled;
	uint8_t do_reset;
	uint8_t data;
	uint64_t reset_elapsed;

	void(*request_irq)(KBD* kbd);
	void(*clear_irq)(KBD* kbd);
} KBD;

uint8_t kbd_get_data(KBD* kbd);
void kbd_set_enable(KBD* kbd, uint8_t enable);
void kbd_set_clk(KBD* kbd, uint8_t clk);
void kbd_reset(KBD*);
void kbd_tick(KBD* kbd);

#endif
