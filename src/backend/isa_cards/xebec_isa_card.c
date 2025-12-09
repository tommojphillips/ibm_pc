/* xebec_isa_card.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * XEBEC HDC ISA Card
 */

#include <stdint.h>
#include "backend/hdc/xebec.h"
#include "backend/io/isa_bus.h"
#include "backend/io/isa_cards.h"

#define XEBEC_BASE_ADDRESS 0x320 // Base port address of the XEBEC Card

#define HDC_DATA     (XEBEC_BASE_ADDRESS + 0) // (RW)
#define HDC_STATUS   (XEBEC_BASE_ADDRESS + 1) // (RO)
#define HDC_RESET    (XEBEC_BASE_ADDRESS + 1) // (WO)
#define HDC_READ_DIP (XEBEC_BASE_ADDRESS + 2) // (RO)
#define HDC_SELECT   (XEBEC_BASE_ADDRESS + 2) // (WO)
#define HDC_MASK     (XEBEC_BASE_ADDRESS + 3) // (WO)

static int isa_xebec_write_io_byte(XEBEC_HDC* hdc, uint16_t port, uint8_t value) {
	switch (port) {
		case HDC_DATA:
		case HDC_RESET:
		case HDC_SELECT:
		case HDC_MASK:
			xebec_hdc_write_io_byte(hdc, (uint8_t)(port & ~XEBEC_BASE_ADDRESS), value);
			return 1;
	}
	return 0;
}

static int isa_xebec_read_io_byte(XEBEC_HDC* hdc, uint16_t port, uint8_t* value) {
	switch (port) {
		case HDC_DATA:
		case HDC_STATUS:
		case HDC_READ_DIP:
			*value = xebec_hdc_read_io_byte(hdc, (uint8_t)(port & ~XEBEC_BASE_ADDRESS));
			return 1;
	}
	return 0;
}

static void isa_xebec_update(XEBEC_HDC* hdc, uint64_t cycles) {
	/* xebec cycles are 500/477 of cpu cycles */
	const uint64_t cycle_target = 477;  // CPU cycles
	const uint64_t cycle_factor = 500;  // factor
	hdc->accum += cycles * cycle_factor;
	while (hdc->accum >= cycle_target) {
		hdc->accum -= cycle_target;
		xebec_hdc_update(hdc);
	}
}

int isa_card_add_xebec(ISA_BUS* bus, XEBEC_HDC* hdc) {
	int card = isa_bus_add_card(bus, "Xebec Card", ISA_CARD_XEBEC);
	isa_card_add_param(bus, card, hdc);
	isa_card_add_io(bus, card, isa_xebec_write_io_byte, isa_xebec_read_io_byte);
	isa_card_add_reset(bus, card, xebec_hdc_reset);
	isa_card_add_update(bus, card, isa_xebec_update);
	return card;
}
