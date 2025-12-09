/* i8253_pit.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Intel 8253 Programmable Interval Timer (PIT)
 */

#ifndef I8253_PIT_H
#define I8253_PIT_H

#include <stdint.h>

#define I8253_PIT_NUM_TIMERS 3
#define I8253_PIT_TIMER0     0
#define I8253_PIT_TIMER1     1
#define I8253_PIT_TIMER2     2

#define I8253_PIT_MODE0 0x00 /* interrupt on Terminal Count (TC) */
#define I8253_PIT_MODE1 0x02 /* programmable one-shot */
#define I8253_PIT_MODE2 0x04 /* rate generator */
#define I8253_PIT_MODE3 0x06 /* square wave generator */
#define I8253_PIT_MODE4 0x08 /* software-triggered strobe */
#define I8253_PIT_MODE5 0x0A /* hardware-triggered strobe */
#define I8253_PIT_MODE6 0x0C /* rate generator */
#define I8253_PIT_MODE7 0x0E /* square wave generator */

#define I8253_PIT_CTRL_BCD  0x01 /*  */
#define I8253_PIT_CTRL_MODE 0x0E /*  */
#define I8253_PIT_CTRL_RW   0x30 /*  */

#define I8253_PIT_RW_LATCH  0x00 /*  */

#define I8253_PIT_RB_STATUS 0x10 /*  */
#define I8253_PIT_RB_COUNTS 0x20 /*  */
#define I8253_PIT_RB_NULL   0x40 /*  */
#define I8253_PIT_RB_OUT    0x80 /*  */

#define I8253_PIT_CTRL_SC       0xC0 /*  */
#define I8253_PIT_CTRL_SC_SHIFT 0x06 /*  */
#define I8253_PIT_CTRL_SC_BACK  0xC0 /*  */
#define I8253_PIT_RB_CTR0       0x00 /*  */
#define I8253_PIT_RB_CTR1       0x40 /*  */
#define I8253_PIT_RB_CTR2       0x80 /*  */


#define I8253_TIMER_STATE_WAITING_FOR_RELOAD 0
#define I8253_TIMER_STATE_WAITING_FOR_GATE   1
#define I8253_TIMER_STATE_WAITING_LOAD_CYCLE 2
#define I8253_TIMER_STATE_DELAY_LOAD_CYCLE   3
#define I8253_TIMER_STATE_COUNTING           4

typedef void(*on_timer_cb)(void* timer);
typedef uint8_t* gate_cb;

typedef struct I8253_TIMER {
	uint16_t count_register;
	uint16_t counter_latch;
	uint16_t counter;
	uint16_t reload;

	uint8_t ctrl;
	uint8_t active;
	uint8_t out;
	uint8_t read_state;
	uint8_t load_state;
	uint8_t load_type;
	uint8_t channel_state;
	uint8_t gate;
	uint8_t out_on_reload;
	uint8_t count_is_latched;
	on_timer_cb on_timer; /* on timer cb */
	gate_cb gate_ptr;     /* gate ptr */

} I8253_TIMER;

typedef struct I8253_PIT {
	I8253_TIMER timer[I8253_PIT_NUM_TIMERS];
} I8253_PIT;

uint8_t i8253_pit_read(I8253_PIT* pit, uint8_t io_address);
void i8253_pit_write(I8253_PIT* pit, uint8_t io_address, uint8_t value);
void i8253_pit_reset(I8253_PIT* pit);
void i8253_pit_update(I8253_PIT* pit);

void i8253_pit_set_timer_cb(I8253_PIT* pit, int timer_index, on_timer_cb on_timer, gate_cb gate_ptr);

#endif
