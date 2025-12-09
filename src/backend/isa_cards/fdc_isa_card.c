/* fdc_isa_card.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * FDC ISA Card
 */

#include <stdint.h>
#include "backend/fdc/fdc.h"
#include "backend/io/isa_bus.h"
#include "backend/io/isa_cards.h"

#define FDC_BASE_ADDRESS 0x3F0 // Base port address of the FDC

#define FDC_DIGITAL_OUTPUT (FDC_BASE_ADDRESS + 2) // (RW)
#define FDC_MAIN_STATUS    (FDC_BASE_ADDRESS + 4) // (RO)
#define FDC_DATA_FIFO      (FDC_BASE_ADDRESS + 5) // (RW)

static int isa_fdc_write_io_byte(FDC* hdc, uint16_t port, uint8_t value) {
	switch (port) {
		case FDC_DIGITAL_OUTPUT:
		case FDC_DATA_FIFO:
			upd765_fdc_write_io_byte(hdc, (uint8_t)(port & ~FDC_BASE_ADDRESS), value);
			return 1;
	}
	return 0;
}

static int isa_fdc_read_io_byte(FDC* hdc, uint16_t port, uint8_t* value) {
	switch (port) {
		case FDC_MAIN_STATUS:
		case FDC_DATA_FIFO:
			*value = upd765_fdc_read_io_byte(hdc, (uint8_t)(port & ~FDC_BASE_ADDRESS));
			return 1;
	}
	return 0;
}

static void isa_fdc_update(FDC* fdc, uint64_t cycles) {
	/* fdc cycles are 3/14 of cpu cycles */
	const uint64_t cycle_target = 14; // CPU cycles
	const uint64_t cycle_factor = 3;  // factor
	fdc->accum += cycles * cycle_factor;
	while (fdc->accum >= cycle_target) {
		fdc->accum -= cycle_target;
		upd765_fdc_update(fdc);
	}
}

int isa_card_add_fdc(ISA_BUS* bus, FDC* fdc) {
	int card = isa_bus_add_card(bus, "FDC Card", ISA_CARD_FDC);
	isa_card_add_param(bus, card, fdc);
	isa_card_add_io(bus, card, isa_fdc_write_io_byte, isa_fdc_read_io_byte);
	isa_card_add_reset(bus, card, upd765_fdc_reset);
	isa_card_add_update(bus, card, isa_fdc_update);
	return card;
}
