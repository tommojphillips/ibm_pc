/* keyboard.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Keyboard
 */

#include "keyboard.h"

#include "timing.h"
#include "backend/utility/ring_buffer.h"

#include "backend/chipset/i8259_pic.h"

#define KEYS_SIZE 10

#define KBD_IRQ 1

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

static void reset_check(KBD* kbd) {
	if (kbd->do_reset) {
		kbd->do_reset = 0;
		ring_buffer_reset(&kbd->key_buffer);
		kbd->data = 0xAA;
		i8259_pic_request_interrupt(kbd->pic_p, KBD_IRQ);
	}
}

void kbd_reset(KBD* kbd) {
	kbd->do_reset = 0;
	kbd->enabled = 0;
	kbd->reset_elapsed = timing_get_ticks_ms();
	kbd->data = 0;
	ring_buffer_reset(&kbd->key_buffer);
}

uint8_t kbd_get_data(KBD* kbd) {
	i8259_pic_clear_interrupt(kbd->pic_p, KBD_IRQ);
	return kbd->data;
}

void kbd_set_enable(KBD* kbd, uint8_t enable) {
	if (enable == 0) {
		kbd->enabled = 0;
		kbd->data = 0;
		i8259_pic_clear_interrupt(kbd->pic_p, KBD_IRQ);
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
	if (kbd->enabled && !ring_buffer_is_empty(&kbd->key_buffer)) {
		kbd->data = ring_buffer_pop(&kbd->key_buffer);
		i8259_pic_request_interrupt(kbd->pic_p, KBD_IRQ);
	}
}

int kbd_create(KBD* kbd) {
	if (ring_buffer_create(&kbd->key_buffer, KEYS_SIZE)) {
		dbg_print("[KBD] Failed to allocate key buffer\n");
		return 1;
	}
	return 0;
}

void kbd_destroy(KBD* kbd) {
	ring_buffer_destroy(&kbd->key_buffer);
}

void kbd_init(KBD* kbd, I8259_PIC* pic) {
	kbd->pic_p = pic;
}
