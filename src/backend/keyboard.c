/* keyboard.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Keyboard
 */

#include "keyboard.h"

#include "timing.h"
#include "input.h"

static void reset_check(KBD* kbd) {
	if (kbd->do_reset) {
		kbd->do_reset = 0;
		input_reset_input();
		kbd->data = 0xAA;
		kbd->request_irq(kbd);
	}
}

void kbd_reset(KBD* kbd) {
	kbd->do_reset = 0;
	kbd->enabled = 0;
	kbd->reset_elapsed = timing_get_ticks_ms();
	kbd->data = 0;
	input_reset_input();
}

uint8_t kbd_get_data(KBD* kbd) {
	kbd->clear_irq(kbd);
	return kbd->data;
}

void kbd_set_enable(KBD* kbd, uint8_t enable) {
	if (enable == 0) {
		kbd->enabled = 0;
		kbd->data = 0;
		kbd->clear_irq(kbd);
	}
	else {
		kbd->enabled = 1;
	}
}

void kbd_set_clk(KBD* kbd, uint8_t clk) {
	if (clk == 0) {
		kbd->reset_elapsed = timing_get_ticks_ms();
	}
	else {
		uint64_t ticks = timing_get_ticks_ms() - kbd->reset_elapsed;
		if (ticks > 10) {
			kbd->do_reset = 1;
		}
		kbd->reset_elapsed = 0;
	}
}

void kbd_tick(KBD* kbd) {
	reset_check(kbd);
	if (kbd->enabled && input_has_input()) {
		kbd->data = input_get_input();
		kbd->request_irq(kbd);
	}
}
