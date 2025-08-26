/* i8259_pic.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Intel 8259 Programmable Interrupt Controller (PIC)
 */

#include <stdint.h>

#include "i8259_pic.h"

#define ICW1_ICW4 0x01
#define ICW1_SNGL 0x02

#define OCW1 0x10

#define OCW2_OP_MASK  0xE0
#define OCW2_IR_LVL   0x07
#define OCW2_EOI      0x20 /* End of Interrupt */
#define OCW2_EOI_SPEC 0x60
#define OCW2_ROTAUTO  0x80
#define OCW2_SET_PRI  0xC0

#define OCW3           0x08
#define OCW3_READ_MASK 0x03
#define OCW3_READ_IRR  0x02
#define OCW3_READ_ISR  0x03

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

void i8259_pic_clear_interrupt(I8259_PIC* pic, uint8_t irq) {
	if (!pic->initialized) return;
	uint8_t irr = (1 << (irq & 0x7));
	pic->irr &= ~irr;
}

void i8259_pic_request_interrupt(I8259_PIC* pic, uint8_t irq) {
	if (!pic->initialized) return;
	pic->irr |= (1 << irq) & ~pic->imr;
}
int i8259_pic_get_interrupt(I8259_PIC* pic, uint8_t* type) {
	if (!pic->initialized) return 0;
	uint8_t irr = (pic->irr & ~pic->imr) & ~pic->isr;
	for (uint8_t i = 0; i < 8; ++i) {
		if ((irr >> i) & 1) {
			pic->irr &= ~(1 << i);
			pic->isr |= (1 << i);    // set interrupt in-service
			*type = pic->icw[1] + i; // service <type> interrupt
			return 1;
		}
	}
	return 0; // no interrupt to service
}

static void icw1_init(I8259_PIC* pic, uint8_t icw1) {
	/* ICW1 intiitalization */
	i8259_pic_reset(pic);
	pic->icw[pic->icw_index++] = icw1;
	dbg_print("[PIC] ICW%d = %02X\n", pic->icw_index, icw1);
}
static void ocw2_cmd(I8259_PIC* pic, uint8_t cmd) {
	/* OCW2 */
	if (!pic->initialized) return;
	pic->ocw2 = (cmd & OCW2_OP_MASK);
	switch (pic->ocw2) {
		case OCW2_EOI:
#ifdef DBG_PRINT
			if (pic->isr & ~0x1) { /* ignore int 8 */
				dbg_print("[PIC] EOI\n");
			}
#endif
			pic->irr &= ~pic->isr;
			pic->isr = 0x00;
			break;
		case OCW2_EOI_SPEC: // specific EOI
			pic->irr &= ~(1 << (cmd & 0x03));
			pic->isr &= ~(1 << (cmd & 0x03));
			dbg_print("[PIC] EOI_SPEC\n");
			break;
		case OCW2_SET_PRI:
			pic->irl = (cmd & OCW2_IR_LVL);
			dbg_print("[PIC] IR_LVL\n");
			break;
		default:
			dbg_print("[PIC] cmd not implemented: OCW2 = %02X", pic->ocw2);
			break;
	}
}
static void ocw3_req(I8259_PIC* pic, uint8_t ocw3) {
	/* OCW3 request */
	if (!pic->initialized) return;
	pic->ocw3 = ocw3;
}

void i8259_pic_reset(I8259_PIC* pic) {
	pic->imr = 0;
	pic->irr = 0;
	pic->isr = 0;
	pic->ocw2 = 0;
	pic->ocw3 = 0;
	pic->initialized = 0;
	pic->icw_index = 0;
	for (int i = 0; i < I8259_PIC_ICW_COUNT; ++i) {
		pic->icw[i] = 0;
	}
}

uint8_t i8259_pic_read_io_byte(I8259_PIC* pic, uint8_t io_address) {
	switch (io_address) {

		case 0x0:
			if ((pic->ocw3 & OCW3_READ_MASK) == OCW3_READ_ISR) {
				/* Read in-service register */
				return pic->isr;
			}
			else {
				/* Read interrupt request register */
				return pic->irr;
			}
			break;

		case 0x1:
			/* Read interrupt mask register */
			return pic->imr;
	}
	return 0;
}
void i8259_pic_write_io_byte(I8259_PIC* pic, uint8_t io_address, uint8_t value) {
	switch (io_address) {

		case 0x0:
			if (value & OCW1) {
				/* ICW1 */
				icw1_init(pic, value);
			}
			else if (!(value & OCW3)) {
				/* OCW2 */
				ocw2_cmd(pic, value);
			}
			else {
				/* OCW3 */
				ocw3_req(pic, value);
			}
			break;

		case 0x1:
			if (pic->icw_index < I8259_PIC_ICW_COUNT) {
				pic->icw[pic->icw_index++] = value;
				dbg_print("[PIC] ICW%d = %02X\n", pic->icw_index, value);

				if (pic->icw_index == 2 && (pic->icw[0] & ICW1_SNGL)) {
					pic->icw_index++;
					dbg_print("[PIC] ICW%d = %02X\n", pic->icw_index, value);
				}
				if (pic->icw_index == 3 && !(pic->icw[0] & ICW1_ICW4)) {
					pic->icw_index++;
					dbg_print("[PIC] ICW%d = %02X\n", pic->icw_index, value);
				}

				if (pic->icw_index >= I8259_PIC_ICW_COUNT) {
					pic->initialized = 1;
					dbg_print("[PIC] initialized\n");
				}
			}
			else {
				/* OCW1 Write (Write Interrupt Mask) */
				pic->imr = value;
			}
			break;
	}
}
