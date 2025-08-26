/* i8259_pic.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Intel 8259 Programmable Interrupt Controller (PIC)
 */

#ifndef I8259_PIC_H
#define I8259_PIC_H

#include <stdint.h>

#define I8259_PIC_ICW_COUNT 0x04

typedef struct I8259_PIC {
	uint8_t imr; // interrupt mask register
	uint8_t irr; // interrupt request register
	uint8_t isr; // in-service register
	uint8_t irl; // interrupt request level
	uint8_t ocw2;
	uint8_t ocw3;
	uint8_t initialized;
	uint8_t icw_index;
	uint8_t icw[I8259_PIC_ICW_COUNT];
} I8259_PIC;

void i8259_pic_reset(I8259_PIC* pic);
uint8_t i8259_pic_read_io_byte(I8259_PIC* pic, uint8_t io_address);
void i8259_pic_write_io_byte(I8259_PIC* pic, uint8_t io_address, uint8_t value);

/* Get Interrupt
   type: (out) the type of (number) interrupt to service
   Returns:    1 if there is an interrupt to service. 0 if no interrupt */
int i8259_pic_get_interrupt(I8259_PIC* pic, uint8_t* type);

/* Request Interrupt 
   irq: (in) the interrupt/s to request */
void i8259_pic_request_interrupt(I8259_PIC* pic, uint8_t irq);

/* Clear Interrupt
   irq: (in) the interrupt/s to clear */
void i8259_pic_clear_interrupt(I8259_PIC* pic, uint8_t irq);

#endif
