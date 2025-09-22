/* isa_bus.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * ISA Bus
 */

#include <stdint.h>
#include <malloc.h>
#include <memory.h>

#include "isa_bus.h"
#include "memory_map.h"

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

#define IS_REMOVED(i)  (bus->cards[i].flags & ISA_CARD_FLAG_REMOVED)
#define IS_ENABLED(i)  (bus->cards[i].flags & ISA_CARD_FLAG_ENABLED)
#define IS_ACTIVE(i)   (!IS_REMOVED(i) && IS_ENABLED(i))
#define HAS_IO(i)      (bus->cards[i].flags & ISA_CARD_FLAG_HAS_IO)
#define HAS_MM(i)      (bus->cards[i].flags & ISA_CARD_FLAG_HAS_MM)
#define IS_IN_RANGE(i) ((i) != -1 && (i) < bus->card_index)

int isa_bus_create(ISA_BUS* bus, MEMORY_MAP* map, int slots) {
	if (bus != NULL) {
		bus->cards = (ISA_CARD*)calloc(slots, sizeof(ISA_CARD));
		if (bus->cards == NULL) {
			dbg_print("Failed to create isa bus; Calloc failed. slot_count = %x, isa_card_size = %zu\n", slots, sizeof(ISA_CARD));
			return 1;
		}
		bus->card_count = slots;
		bus->card_index = 0;
		bus->map = map;
		return 0;
	}
	dbg_print("Failed to create isa bus; Bus was NULL.\n");
	return 1;
}
void isa_bus_destroy(ISA_BUS* bus) {
	if (bus != NULL) {		
		if (bus->cards != NULL) {
			free(bus->cards);
			bus->cards = NULL;
		}
		bus->card_count = 0;
		bus->card_index = 0;
	}
}

int isa_bus_add_card(ISA_BUS* bus) {
	int index = -1;

	/* find first removed card */
	for (int i = 0; i < bus->card_index; ++i) {
		if (IS_REMOVED(i)) {
			index = i;
		}
	}

	/* did not find a removed card; add to end */
	if (index < 0) {
		index = bus->card_index;
		if (index >= bus->card_count) {
			dbg_print("Failed to add isa card; Index out of range. index = %d\n", index);
			return -1;
		}
		bus->card_index++;
	}

	bus->cards[index].mregion_index = -1;
	bus->cards[index].write_io_byte = NULL;
	bus->cards[index].read_io_byte = NULL;
	bus->cards[index].param = NULL;
	bus->cards[index].flags = ISA_CARD_FLAG_ENABLED;
	return index;
}
int isa_bus_remove_card(ISA_BUS* bus, int index) {
	if (IS_IN_RANGE(index)) {
		bus->cards[index].mregion_index = -1;
		bus->cards[index].write_io_byte = NULL;
		bus->cards[index].read_io_byte = NULL;
		bus->cards[index].param = NULL;
		bus->cards[index].flags = ISA_CARD_FLAG_REMOVED;
		return 0;
	}
	dbg_print("Failed to remove isa card; Index out of range. index = %d\n", index);
	return 1;
}

int isa_bus_enable_card(ISA_BUS* bus, int index) {
	if (IS_IN_RANGE(index) && !IS_REMOVED(index)) {
		bus->cards[index].flags |= ISA_CARD_FLAG_ENABLED;

		if (HAS_MM(index)) {
			memory_map_enable_mregion(bus->map, bus->cards[index].mregion_index);
		}
		return 0;
	}
	dbg_print("Failed to enable isa card; Index out of range. index = %d\n", index);
	return 1;
}
int isa_bus_disable_card(ISA_BUS* bus, int index) {
	if (IS_IN_RANGE(index) && !IS_REMOVED(index)) {
		bus->cards[index].flags &= ~ISA_CARD_FLAG_ENABLED;

		if (HAS_MM(index)) {
			memory_map_disable_mregion(bus->map, bus->cards[index].mregion_index);
		}
		return 0;
	}
	dbg_print("Failed to disable isa card; Index out of range. index = %d\n", index);
	return 1;
}

int isa_bus_read_io_byte(ISA_BUS* bus, uint16_t port, uint8_t* value) {
	for (int i = 0; i < bus->card_index; ++i) {
		if (!IS_REMOVED(i) && IS_ENABLED(i) && HAS_IO(i)) {
			if (bus->cards[i].read_io_byte(bus->cards[i].param, port, value)) {
				return 1; /* Read handled */
			}
		}
	}
	return 0; /* Read not handled */
}
int isa_bus_write_io_byte(ISA_BUS* bus, uint16_t port, uint8_t value) {
	for (int i = 0; i < bus->card_index; ++i) {
		if (!IS_REMOVED(i) && IS_ENABLED(i) && HAS_IO(i)) {
			if (bus->cards[i].write_io_byte(bus->cards[i].param, port, value)) {
				return 1; /* Write handled */
			}
		}
	}
	return 0; /* Write not handled */
}

int isa_card_add_mm(ISA_BUS* bus, int index, uint32_t start, uint32_t size, uint32_t mask, uint32_t flags) {
	if (IS_IN_RANGE(index) && !IS_REMOVED(index)) {
		bus->cards[index].flags |= ISA_CARD_FLAG_HAS_MM;
		bus->cards[index].mregion_index = memory_map_add_mregion(bus->map, start, size, mask, flags);
		if (bus->cards[index].mregion_index == -1) {
			return 1;
		}
		return 0;
	}
	dbg_print("Failed to add MM to isa card; Index out of range or card removed. index = %d, removed = %d\n", index, IS_REMOVED(index));
	return 1;
}
int isa_card_add_io(ISA_BUS* bus, int index, ISA_BUS_WRITE_IO write_io_byte, ISA_BUS_READ_IO read_io_byte, void* param) {
	if (IS_IN_RANGE(index) && !IS_REMOVED(index)) {
		bus->cards[index].flags |= ISA_CARD_FLAG_HAS_IO;
		bus->cards[index].write_io_byte = write_io_byte;
		bus->cards[index].read_io_byte = read_io_byte;
		bus->cards[index].param = param;
		return 0;
	}
	dbg_print("Failed to add IO to isa card; Index out of range or card removed. index = %d, removed = %d\n", index, IS_REMOVED(index));
	return 1;
}
