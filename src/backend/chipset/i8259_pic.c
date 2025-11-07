/* i8259_pic.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Intel 8259 Programmable Interrupt Controller (PIC)
 */

#include <stdint.h>

#include "i8259_pic.h"

#define ICW1_REQ_ICW4 0x01
#define ICW1_SNGL     0x02
#define ICW1_ADI      0x04
#define ICW1_LTIM     0x08 /* Edge/Level mode; 0 = Edge, 1 = Level */
#define ICW1_INIT     0x10

#define ICW4_8088     0x01 /* 8088/8080 mode */
#define ICW4_AEOI     0x02 /* Auto EOI mode; 0 = Manual, 1 = Auto */
#define ICW4_BUFFERED 0x08 /* Buffered mode */
#define ICW4_NESTED   0x10 /* Fully nested mode */

#define OCW2_OP_MASK  0xE0
#define OCW2_IR_LVL   0x07
#define OCW2_EOI      0x20 /* End of Interrupt */
#define OCW2_EOI_SPEC 0x60
#define OCW2_ROTAUTO  0x80
#define OCW2_SET_PRI  0xC0

#define OCW3           0x08
#define OCW3_READ_MASK 0x03 /* Read IRR/ISR */
#define OCW3_READ_IRR  0x02
#define OCW3_READ_ISR  0x03

//#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

static uint8_t highest_priority_bit(uint8_t byte) {
	for (uint8_t i = 0; i < 8; ++i) {
		if (byte & (1 << i)) {
			return i; /* lowest number = highest priority */
		}
	}
	return 0xFF; /* NONE */
}

static uint8_t get_pending_irq(I8259_PIC* pic) {
	uint8_t ir = pic->irr & ~pic->imr & ~pic->isr;
	if (ir != 0) {
		return highest_priority_bit(ir);
	}
	return 0xFF; /* NONE */
}

static void assert_intr(I8259_PIC* pic, uint8_t irq) {
	uint8_t type = pic->icw[1] | irq;
	pic->assert_intr(type);
	dbg_print("[PIC] IRQ %d\n", irq);
}

static void icw1(I8259_PIC* pic, uint8_t value) {
	/* ICW1 intitalization */
	pic->deassert_intr();
	i8259_pic_reset(pic);
	pic->icw[pic->icw_index++] = value;
	dbg_print("[PIC] ICW1 = %02X\n", value);
}
static void icwx(I8259_PIC* pic, uint8_t value) {
	/* ICW[2/3/4] intitalization */

	dbg_print("[PIC] ICW%d = %02X\n", pic->icw_index + 1, value);

	switch (pic->icw_index) {

		case 1: /* ICW2 */
			/* Mask ICW2 to the top 5 bits */
			pic->icw[pic->icw_index++] = value & 0xF8;

			/* If in single mode; skip ICW3 */
			if (pic->icw[0] & ICW1_SNGL) {
				/* skip ICW3 */
				pic->icw_index++;

				if (!(pic->icw[0] & ICW1_REQ_ICW4)) {
					/* skip ICW4 */
					pic->icw_index++;
				}
			}
			break;

		case 2: /* ICW3 */
			pic->icw[pic->icw_index++] = value;

			if (!(pic->icw[0] & ICW1_REQ_ICW4)) {
				/* skip ICW4 */
				pic->icw_index++;
			}
			break;

		case 3: /* ICW4 */
			pic->icw[pic->icw_index++] = value;
			break;
	}
	
	if (pic->icw_index == I8259_PIC_ICW_COUNT) {
		pic->initialized = 1;
		dbg_print("[PIC] initialized\n");
	}	
}

static void ocw1(I8259_PIC* pic, uint8_t value) {
	/* OCW1 */
	pic->imr = value;
}
static void ocw2(I8259_PIC* pic, uint8_t value) {
	/* OCW2 */
	switch (value & OCW2_OP_MASK) {
		case OCW2_EOI: {
			/* Clear highest priority IR */
			uint8_t ir = highest_priority_bit(pic->isr);
			if (ir != 0xFF) {
				pic->isr &= ~(1 << ir);
			}
			dbg_print("[PIC] EOI %d\n", ir);
		} break;

		case OCW2_EOI_SPEC: {
			/* Clear specific IR */
			pic->isr &= ~(1 << (value & 0x07));
			dbg_print("[PIC] EOI_SPEC %d\n", (value & 0x7));
		} break;

		default:
			dbg_print("[PIC] cmd not implemented: OCW2 = %02X", value);
			break;
	}
}
static void ocw3(I8259_PIC* pic, uint8_t ocw3) {
	/* OCW3 */
	pic->ocw3 = ocw3;
}

static void command_write(I8259_PIC* pic, uint8_t value) {
	if (value & ICW1_INIT) {
		icw1(pic, value);
	}
	else if (pic->initialized) {
		if (!(value & OCW3)) {
			ocw2(pic, value);
		}
		else {
			ocw3(pic, value);
		}
	}
}
static void data_write(I8259_PIC* pic, uint8_t value) {
	if (pic->initialized) {
		ocw1(pic, value);
	}
	else {
		icwx(pic, value);
	}
}

static uint8_t command_read(I8259_PIC* pic) {
	if ((pic->ocw3 & OCW3_READ_MASK) == OCW3_READ_ISR) {
		return pic->isr;
	}
	else {
		return pic->irr;
	}
}
static uint8_t data_read(I8259_PIC* pic) {
	return pic->imr;
}

uint8_t i8259_pic_read_io_byte(I8259_PIC* pic, uint8_t io_address) {
	switch (io_address & 0x1) {
		case 0x0:
			return command_read(pic);
		case 0x1:
			return data_read(pic);			
	}
	return 0;
}
void i8259_pic_write_io_byte(I8259_PIC* pic, uint8_t io_address, uint8_t value) {
	switch (io_address & 0x1) {
		case 0x0:
			command_write(pic, value);
			break;
		case 0x1:
			data_write(pic, value);
			break;
	}
}

void i8259_pic_clear_interrupt(I8259_PIC* pic, uint8_t irq) {
	if (pic->initialized) {
		uint8_t mask = (1 << (irq & 0x07));

		/* Determine highest priority ISR */
		uint8_t highest_irq = highest_priority_bit(pic->isr);

		/* Deassert INTR if IR is in service and IR has highest priority */
		if ((irq & 0x07) == highest_irq) {
			pic->deassert_intr();
			dbg_print("[PIC] Deasserted INTR (%d)\n", irq);
		}

		pic->irr &= ~mask;
		pic->isr &= ~mask; /* Should we be clearing ISR? if the CPU is currently servicing the INT, and sends an EOI to the PIC it could EOI a different IRQ. */
	}
}

void i8259_pic_request_interrupt(I8259_PIC* pic, uint8_t irq) {
	if (pic->initialized) {
		/* Only set if not masked and not already in service */
		uint8_t mask = 1 << (irq & 0x07);
		if (!(pic->isr & mask) && !(pic->irr & mask) && !(pic->imr & mask)) {
			pic->irr |= mask;
		}
	}
}

int i8259_pic_get_interrupt(I8259_PIC* pic) {
	if (!pic->initialized) {
		return 0; /* No pending interrupt */
	}

	/* Find next valid IRQ to service */
	uint8_t irq = get_pending_irq(pic);
	if (irq == 0xFF) {
		return 0; /* No pending interrupt */
	}

	uint8_t mask = 1 << irq;

	/* Set the bit in ISR if not in Auto-EOI mode */
	if (!(pic->icw[3] & ICW4_AEOI)) { 
		pic->isr |= mask;
	}

	/* Clear the bit in IRR if in Edge-Triggered mode */
	if (!(pic->icw[0] & ICW1_LTIM)) {
		pic->irr &= ~mask;
	}

	/* Assert INTR */
	assert_intr(pic, irq);
	return 1;
}

void i8259_pic_reset(I8259_PIC* pic) {
	pic->imr = 0;
	pic->irr = 0;
	pic->isr = 0;
	pic->ocw3 = 0;
	pic->initialized = 0;
	pic->icw_index = 0;
	for (int i = 0; i < I8259_PIC_ICW_COUNT; ++i) {
		pic->icw[i] = 0;
	}
}
