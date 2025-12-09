/* mda_isa_card.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * MDA ISA Card
 */

#include <stdint.h>
#include "backend/video/mda.h"
#include "backend/io/isa_bus.h"
#include "backend/io/isa_cards.h"

#define MDA_BASE_ADDRESS MDA_IO_BASE_ADDRESS // Base port address of the MDA Card

static int isa_mda_write_io_byte(MDA* mda, uint16_t port, uint8_t value) {
	switch (port) {
		case MDA_BASE_ADDRESS + 0x0: // MDA Index
		case MDA_BASE_ADDRESS + 0x1: // MDA Data
		case MDA_BASE_ADDRESS + 0x2: // MDA Index
		case MDA_BASE_ADDRESS + 0x3: // MDA Data 
		case MDA_BASE_ADDRESS + 0x4: // MDA Index
		case MDA_BASE_ADDRESS + 0x5: // MDA Data
		case MDA_BASE_ADDRESS + 0x6: // MDA Index
		case MDA_BASE_ADDRESS + 0x7: // MDA Data
		case MDA_BASE_ADDRESS + 0x8: // MDA Mode
		case MDA_BASE_ADDRESS + 0x9: // MDA Color
		case MDA_BASE_ADDRESS + 0xA: // MDA Status
			mda_write_io_byte(mda, (uint8_t)(port & ~MDA_BASE_ADDRESS), value);
			return 1;
	}
	return 0;
}

static int isa_mda_read_io_byte(MDA* mda, uint16_t port, uint8_t* value) {
	switch (port) {
		case MDA_BASE_ADDRESS + 0x0: // MDA Index
		case MDA_BASE_ADDRESS + 0x1: // MDA Data
		case MDA_BASE_ADDRESS + 0x2: // MDA Index
		case MDA_BASE_ADDRESS + 0x3: // MDA Data
		case MDA_BASE_ADDRESS + 0x4: // MDA Index
		case MDA_BASE_ADDRESS + 0x5: // MDA Data
		case MDA_BASE_ADDRESS + 0x6: // MDA Index
		case MDA_BASE_ADDRESS + 0x7: // MDA Data
		case MDA_BASE_ADDRESS + 0x8: // MDA Mode
		case MDA_BASE_ADDRESS + 0x9: // MDA Color
		case MDA_BASE_ADDRESS + 0xA: // MDA Status
			*value = mda_read_io_byte(mda, (uint8_t)(port & ~MDA_BASE_ADDRESS));
			return 1;
	}
	return 0;
}

static void isa_mda_update(MDA* mda, uint64_t cycles) {
	/* mda cycles are ?/? of cpu cycles */
	const uint64_t cycle_target = 4; // CPU cycles
	const uint64_t cycle_factor = 5; // factor
	mda->accum += cycles * cycle_factor;
	while (mda->accum >= cycle_target) {
		mda->accum -= cycle_target;
		mda_update(mda);
	}
}

int isa_card_add_mda(ISA_BUS* bus, MDA* mda) {
	/* MDA Card; VIDEO RAM - B0000 - B0FFF (0x1000 04K) mirrored up to 0xB7FFF (0x8000 32K) x8 */
	int card = isa_bus_add_card(bus, "MDA Card", ISA_CARD_MDA);
	isa_card_add_mm(bus, card, MDA_MM_BASE_ADDRESS, 0x8000, MDA_MM_ADDRESS_MASK, MREGION_FLAG_NONE);
	isa_card_add_param(bus, card, mda);
	isa_card_add_io(bus, card, isa_mda_write_io_byte, isa_mda_read_io_byte);
	isa_card_add_reset(bus, card, mda_reset);
	isa_card_add_update(bus, card, isa_mda_update);
	return card;
}
