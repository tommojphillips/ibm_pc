/* i8253_pit.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Intel 8253 Programmable Interval Timer (PIT)
 */

#include <stdint.h>
#include "i8253_pit.h"
#include "backend/utility/bit_utils.h"

#define CHANNEL_0_PORT 0x0 // R/W
#define CHANNEL_1_PORT 0x1 // R/W
#define CHANNEL_2_PORT 0x2 // R/W
#define CONTROL_PORT   0x3 // W

#define RW_LATCH  0x00
#define RW_LSB    0x10
#define RW_MSB    0x20
#define RW_BOTH   0x30

#define LOAD_STATE_LSB 0
#define LOAD_STATE_MSB 1
					   
#define LOAD_TYPE_INIT 0
#define LOAD_TYPE_SEQU 1

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

static void i8253_timer_set_output(I8253_TIMER* timer, uint8_t out) {
	if (timer->out != out) {
		timer->out = out;
		if (timer->on_timer != NULL) {
			timer->on_timer(timer);
		}
	}
}
static void i8253_timer_load_counter(I8253_TIMER* timer) {
	/* count_register could be 0 resulting in 0x10000 iterations */
	timer->reload = timer->count_register;

	if (timer->load_type == LOAD_TYPE_INIT) {
		timer->channel_state = I8253_TIMER_STATE_WAITING_LOAD_CYCLE;
		timer->load_type = LOAD_TYPE_SEQU;
	}
	else {
		if ((timer->ctrl & I8253_PIT_CTRL_MODE) == I8253_PIT_MODE0 ||
			(timer->ctrl & I8253_PIT_CTRL_MODE) == I8253_PIT_MODE4) {
			timer->channel_state = I8253_TIMER_STATE_WAITING_LOAD_CYCLE;
		}
	}

	timer->active = 1;
}
static void i8253_timer_count(I8253_TIMER* timer) {
	
	if (timer->ctrl & I8253_PIT_CTRL_BCD) {
		/* bcd mode - not implemented */
	}
	else {
		/* binary mode; counter will overflow; this is intentional so 0 results in 0x10000 iterations */
		timer->counter--;
	}
	
	if (!timer->count_is_latched) {
		timer->counter_latch = timer->counter;
	}
}

static void i8253_timer_update(I8253_TIMER* timer) {
	if (!timer->active) {
		return;
	}

	switch (timer->ctrl & I8253_PIT_CTRL_MODE) {

		case I8253_PIT_MODE0: // terminal count
			i8253_timer_count(timer);
			if (timer->counter == 0) {
				i8253_timer_set_output(timer, 1);
			}
			break;

		case I8253_PIT_MODE1: // programmable one-shot
			dbg_print("[PIT] MODE1 not implemented\n");
			break;

		case I8253_PIT_MODE2: // rate generator
		case I8253_PIT_MODE6: // rate generator
			i8253_timer_count(timer);
			if (timer->counter == 1) {
				i8253_timer_set_output(timer, 0);
				timer->out_on_reload = 1;
				timer->channel_state = I8253_TIMER_STATE_WAITING_LOAD_CYCLE;
			}
			break;

		case I8253_PIT_MODE3: // square wave generator
		case I8253_PIT_MODE7: // square wave generator
			i8253_timer_count(timer);
			if (timer->counter == 0) {
				i8253_timer_set_output(timer, !timer->out);
				timer->counter = timer->reload;
				timer->channel_state = I8253_TIMER_STATE_WAITING_LOAD_CYCLE;
			}
			break;

		case I8253_PIT_MODE4: // sw strobe
			dbg_print("[PIT] MODE4 not implemented\n");
			break;

		case I8253_PIT_MODE5: // hw strobe
			dbg_print("[PIT] MODE5 not implemented\n");
			break;

		default:
			dbg_print("[PIT] unknown mode not implemented\n");
			break;
	}
}
static void i8253_timer_set_gate(I8253_TIMER* timer) {

	uint8_t gate = 1;
	if (timer->gate_ptr != NULL) {
		gate = *timer->gate_ptr;
	}

	if (timer->channel_state != I8253_TIMER_STATE_WAITING_FOR_RELOAD) {
		if (!timer->gate && gate) { // is raising edge of gate
			switch (timer->ctrl & I8253_PIT_CTRL_MODE) {
				case I8253_PIT_MODE0:
				case I8253_PIT_MODE4:
					break; // no effect

				case I8253_PIT_MODE2:
				case I8253_PIT_MODE6:
				case I8253_PIT_MODE3:
				case I8253_PIT_MODE7:
				case I8253_PIT_MODE1:
				case I8253_PIT_MODE5:
					timer->channel_state = I8253_TIMER_STATE_WAITING_LOAD_CYCLE;
					break;
			}
		}
		else if (timer->gate && !gate) { // is falling edge of gate
			switch (timer->ctrl & I8253_PIT_CTRL_MODE) {
				case I8253_PIT_MODE0:
				case I8253_PIT_MODE1:
				case I8253_PIT_MODE5:
					break; // no effect

				case I8253_PIT_MODE2:
				case I8253_PIT_MODE6:
				case I8253_PIT_MODE3:
				case I8253_PIT_MODE7:
					i8253_timer_set_output(timer, 1);
					timer->channel_state = I8253_TIMER_STATE_WAITING_FOR_GATE;
					break;
				case I8253_PIT_MODE4:
					timer->channel_state = I8253_TIMER_STATE_WAITING_FOR_GATE;
					break;
			}
		}
	}

	timer->gate = gate;
}
static void i8253_timer_write(I8253_TIMER* timer, uint8_t value) {
	switch (timer->ctrl & I8253_PIT_CTRL_RW) {

		case RW_LSB: // LSB only
			timer->count_register = value;
			i8253_timer_load_counter(timer);
			break;

		case RW_MSB: // MSB only
			timer->count_register = (value << 8);
			i8253_timer_load_counter(timer);
			break;

		case RW_BOTH: // LSB then MSB
			if (timer->load_state == LOAD_STATE_LSB) {
				timer->count_register = (timer->count_register & 0xFF00) | value;
				timer->load_state = LOAD_STATE_MSB;
			}
			else {
				timer->count_register = (value << 8) | (timer->count_register & 0x00FF);
				i8253_timer_load_counter(timer);
				
				if ((timer->ctrl & I8253_PIT_CTRL_MODE) == I8253_PIT_MODE0) {
					// A load will stop a timer in MODE0 and set output low
					i8253_timer_set_output(timer, 0);
					timer->channel_state = I8253_TIMER_STATE_WAITING_FOR_RELOAD;
				}

				timer->load_state = LOAD_STATE_LSB;
			}
			break;
	}
}
static uint8_t i8253_timer_read(I8253_TIMER* timer) {
	uint8_t v = 0;
	switch (timer->ctrl & I8253_PIT_CTRL_RW) {

		case RW_LSB: // LSB
			timer->count_is_latched = 0;
			v = timer->counter_latch & 0xFF;
			break;

		case RW_MSB: // MSB
			timer->count_is_latched = 0;
			v = (timer->counter_latch >> 8) & 0xFF;
			break;

		case RW_BOTH: // LSB then MSB
			if (timer->load_state == LOAD_STATE_LSB) {
				v = timer->counter_latch & 0xFF;
				timer->load_state = LOAD_STATE_MSB;
			}
			else {
				timer->count_is_latched = 0;
				v = (timer->counter_latch >> 8) & 0xFF;
				timer->load_state = LOAD_STATE_LSB;
			}
			break;
	}
	return v;
}

static void i8253_pit_control_write(I8253_PIT* pit, uint8_t value) {
	uint8_t i = (value >> 6) & 0x3;
	if (i == 0x3) return; // illegal
	
	if ((value & I8253_PIT_CTRL_RW) == RW_LATCH) {
		// latching command
		pit->timer[i].counter_latch = pit->timer[i].counter;
		pit->timer[i].count_is_latched = 1;
	}
	else {
		// set timer mode, read/write mode, BCD mode; reset timer state.
		pit->timer[i].ctrl = value;
		pit->timer[i].count_is_latched = 0;
		pit->timer[i].counter = 0;
		pit->timer[i].channel_state = I8253_TIMER_STATE_WAITING_FOR_RELOAD;
		pit->timer[i].load_state = LOAD_STATE_LSB;
		pit->timer[i].load_type = LOAD_TYPE_INIT;
		
		switch (pit->timer[i].ctrl & I8253_PIT_CTRL_MODE) {
			case I8253_PIT_MODE0:
				i8253_timer_set_output(&pit->timer[i], 0);
				pit->timer[i].out_on_reload = 0;
				break;
			case I8253_PIT_MODE1:
				i8253_timer_set_output(&pit->timer[i], 1);
				pit->timer[i].out_on_reload = 0;
				break;
			case I8253_PIT_MODE2:
			case I8253_PIT_MODE6:
				i8253_timer_set_output(&pit->timer[i], 1);
				pit->timer[i].out_on_reload = 1;
				break;
			case I8253_PIT_MODE3:
			case I8253_PIT_MODE7:
				i8253_timer_set_output(&pit->timer[i], 1);
				pit->timer[i].out_on_reload = 1;
				break;
			case I8253_PIT_MODE4:
				i8253_timer_set_output(&pit->timer[i], 1);
				pit->timer[i].out_on_reload = 0;
				break;
			case I8253_PIT_MODE5:
				i8253_timer_set_output(&pit->timer[i], 1);
				pit->timer[i].out_on_reload = 0;
				break;
		}
	}

	pit->timer[i].load_state = 0;
}

uint8_t i8253_pit_read(I8253_PIT* pit, uint8_t i) {
	switch (i) {
		case CHANNEL_0_PORT:
		case CHANNEL_1_PORT:
		case CHANNEL_2_PORT:
			return i8253_timer_read(&pit->timer[i]);

		default:
		case CONTROL_PORT: // control word
			return 0xFF;
	}
}
void i8253_pit_write(I8253_PIT* pit, uint8_t i, uint8_t value) {
	switch (i) {
		case CHANNEL_0_PORT:
		case CHANNEL_1_PORT:
		case CHANNEL_2_PORT:
			i8253_timer_write(&pit->timer[i], value);
			break;

		case CONTROL_PORT: // control word
			i8253_pit_control_write(pit, value);
			break;
	}
}

void i8253_pit_reset(I8253_PIT* pit) {
	for (int i = 0; i < I8253_PIT_NUM_TIMERS; ++i) {
        pit->timer[i].ctrl = 0;
		pit->timer[i].reload = 0;
        pit->timer[i].counter = 0;
        pit->timer[i].counter_latch = 0;
        pit->timer[i].out_on_reload = 0;
		pit->timer[i].active = 0;
		pit->timer[i].count_is_latched = 0;
		pit->timer[i].count_register = 0;
		pit->timer[i].out = 0;
		pit->timer[i].gate = 0;
		pit->timer[i].load_state = LOAD_STATE_LSB;
		pit->timer[i].load_type = LOAD_TYPE_INIT;
		pit->timer[i].channel_state = I8253_TIMER_STATE_WAITING_FOR_RELOAD;
    }
}
void i8253_pit_update(I8253_PIT* pit) {
	
	for (int i = 0; i < I8253_PIT_NUM_TIMERS; ++i) {

		i8253_timer_set_gate(&pit->timer[i]);
				
		switch (pit->timer[i].channel_state) {
			case I8253_TIMER_STATE_WAITING_FOR_RELOAD:
				break; /* do nothing */
			case I8253_TIMER_STATE_WAITING_FOR_GATE:
				break; /* do nothing */
			case I8253_TIMER_STATE_DELAY_LOAD_CYCLE:
				pit->timer[i].channel_state = I8253_TIMER_STATE_WAITING_LOAD_CYCLE;
				break;
			case I8253_TIMER_STATE_WAITING_LOAD_CYCLE:
				pit->timer[i].counter = pit->timer[i].reload;
				pit->timer[i].out = pit->timer[i].out_on_reload;
				pit->timer[i].channel_state = I8253_TIMER_STATE_COUNTING;
				break;
			case I8253_TIMER_STATE_COUNTING:
				i8253_timer_update(&pit->timer[i]);
				break;
		}
	}
}

void i8253_pit_set_timer_cb(I8253_PIT* pit, int timer_index, on_timer_cb on_timer, gate_cb gate_ptr) {
	pit->timer[timer_index].on_timer = on_timer;
	pit->timer[timer_index].gate_ptr = gate_ptr;
}
