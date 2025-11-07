/* cga_isa_card.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * CGA ISA Card
 */

#include <stdint.h>
#include "backend/video/cga.h"
#include "backend/io/isa_bus.h"

#define CGA_BASE_ADDRESS CGA_IO_BASE_ADDRESS // Base port address of the MDA Card

int isa_cga_write_io_byte(CGA* cga, uint16_t port, uint8_t value) {
	switch (port) {
		case CGA_BASE_ADDRESS + 0x0: // CGA Index
		case CGA_BASE_ADDRESS + 0x1: // CGA Data
		case CGA_BASE_ADDRESS + 0x2: // CGA Index
		case CGA_BASE_ADDRESS + 0x3: // CGA Data 
		case CGA_BASE_ADDRESS + 0x4: // CGA Index
		case CGA_BASE_ADDRESS + 0x5: // CGA Data
		case CGA_BASE_ADDRESS + 0x6: // CGA Index
		case CGA_BASE_ADDRESS + 0x7: // CGA Data
		case CGA_BASE_ADDRESS + 0x8: // CGA Mode
		case CGA_BASE_ADDRESS + 0x9: // CGA Color
		case CGA_BASE_ADDRESS + 0xA: // CGA Status
			cga_write_io_byte(cga, (uint8_t)(port & ~CGA_BASE_ADDRESS), value);
			return 1;
	}
	return 0;
}

int isa_cga_read_io_byte(CGA* cga, uint16_t port, uint8_t* value) {
	switch (port) {
		case CGA_BASE_ADDRESS + 0x0: // CGA Index
		case CGA_BASE_ADDRESS + 0x1: // CGA Data
		case CGA_BASE_ADDRESS + 0x2: // CGA Index
		case CGA_BASE_ADDRESS + 0x3: // CGA Data 
		case CGA_BASE_ADDRESS + 0x4: // CGA Index
		case CGA_BASE_ADDRESS + 0x5: // CGA Data
		case CGA_BASE_ADDRESS + 0x6: // CGA Index
		case CGA_BASE_ADDRESS + 0x7: // CGA Data
		case CGA_BASE_ADDRESS + 0x8: // CGA Mode
		case CGA_BASE_ADDRESS + 0x9: // CGA Color
		case CGA_BASE_ADDRESS + 0xA: // CGA Status
			*value = cga_read_io_byte(cga, (uint8_t)(port & ~CGA_BASE_ADDRESS));
			return 1;
	}
	return 0;
}

void isa_cga_update(CGA* cga, uint64_t cycles) {
	/* cga cycles are 3/1 of cpu cycles */
	const uint64_t cycle_target = 1; // CPU cycles
	const uint64_t cycle_factor = 3; // factor

	cga->accum += cycles * cycle_factor;
	while (cga->accum >= cycle_target) {
		cga->accum -= cycle_target;
		cga_update(cga);
	}
}

int isa_card_add_cga(ISA_BUS* bus, CGA* cga) {
	/* CGA Card; VIDEO RAM - B8000 - BBFFF (0x4000 16K) mirrored up to 0xBFFFF (0x8000 32K) x2 */
	int card = isa_bus_add_card(bus, "CGA Card");
	isa_card_add_mm(bus, card, CGA_MM_BASE_ADDRESS, 0x8000, CGA_MM_ADDRESS_MASK, MREGION_FLAG_NONE);
	isa_card_add_param(bus, card, cga);
	isa_card_add_io(bus, card, isa_cga_write_io_byte, isa_cga_read_io_byte);
	isa_card_add_reset(bus, card, cga_reset);
	isa_card_add_update(bus, card, isa_cga_update);
	return card;
}
