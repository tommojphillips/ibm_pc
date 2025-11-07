/* isa_bus.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * ISA Bus
 */

#include <stdint.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>

#include "isa_bus.h"
#include "memory_map.h"

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

#define ISA_CARD_NAME_SIZE 256

#define IS_REMOVED(i)  (bus->cards[i].flags & ISA_CARD_FLAG_REMOVED)
#define IS_ENABLED(i)  (bus->cards[i].flags & ISA_CARD_FLAG_ENABLED)
#define IS_ACTIVE(i)   (!IS_REMOVED(i) && IS_ENABLED(i))
#define HAS_IO(i)      (bus->cards[i].flags & ISA_CARD_FLAG_HAS_IO)
#define HAS_MM(i)      (bus->cards[i].flags & ISA_CARD_FLAG_HAS_MM)
#define HAS_RESET(i)   (bus->cards[i].flags & ISA_CARD_FLAG_HAS_RESET)
#define HAS_UPDATE(i)  (bus->cards[i].flags & ISA_CARD_FLAG_HAS_UPDATE)
#define IS_IN_RANGE(i) ((i) != -1 && (i) < bus->card_index)

int isa_bus_create(ISA_BUS* bus, MEMORY_MAP* map, int slots) {
	if (bus != NULL) {
		bus->cards = calloc(slots, sizeof(ISA_CARD));
		if (bus->cards == NULL) {
			dbg_print("Failed to create isa bus; Calloc failed. slot_count = %x, isa_card_size = %zu\n", slots, sizeof(ISA_CARD));
			return 1;
		}
		bus->card_count = slots;
		bus->card_index = 0;
		bus->map = map;

		for (int i = 0; i < slots; ++i) {
			bus->cards[i].name = calloc(1, ISA_CARD_NAME_SIZE);
			if (bus->cards[i].name == NULL) {
				dbg_print("Failed to create isa card; Calloc failed. slots = %x, slot = %x\n", slots, i);
				return 1;
			}
		}

		return 0;
	}
	dbg_print("Failed to create isa bus; bus was NULL.\n");
	return 1;
}
void isa_bus_destroy(ISA_BUS* bus) {
	if (bus != NULL) {		
		if (bus->cards != NULL) {
			for (int i = 0; i < bus->card_index; ++i) {

				isa_bus_remove_card(bus, i);

				if (bus->cards[i].name != NULL) {
					free(bus->cards[i].name);
					bus->cards[i].name = NULL;
				}
			}

			free(bus->cards);
			bus->cards = NULL;
		}
		bus->card_count = 0;
		bus->card_index = 0;
		bus->map = NULL;
	}
}

int isa_bus_add_card(ISA_BUS* bus, const char* name) {
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

	if (name != NULL) {
		strncpy_s(bus->cards[index].name, ISA_CARD_NAME_SIZE, name, ISA_CARD_NAME_SIZE - 1);
	}
	else {
		sprintf(bus->cards[index].name, "Unknown Card %d", index);
	}
	return index;
}
int isa_bus_remove_card(ISA_BUS* bus, int index) {
	if (IS_IN_RANGE(index) && !IS_REMOVED(index)) {

		if (HAS_MM(index)) {
			isa_card_remove_mm(bus, index);
		}

		bus->cards[index].mregion_index = -1;
		bus->cards[index].write_io_byte = NULL;
		bus->cards[index].read_io_byte = NULL;
		bus->cards[index].reset = NULL;
		bus->cards[index].update = NULL;
		bus->cards[index].param = NULL;
		bus->cards[index].flags = ISA_CARD_FLAG_REMOVED;
		bus->cards[index].name[0] = '\0';
		return 0;
	}
	dbg_print("Failed to remove isa card; Index out of range or card removed. index = %d, removed = %d\n", index, IS_REMOVED(index));
	return 1;
}
int isa_bus_remove_all_cards(ISA_BUS* bus) {
	int r = 0;
	for (int i = 0; i < bus->card_index; ++i) {
		if (isa_bus_remove_card(bus, i)) {
			r = 1;
		}
	}
	return r;
}

int isa_bus_enable_card(ISA_BUS* bus, int index) {
	if (IS_IN_RANGE(index) && !IS_REMOVED(index)) {
		bus->cards[index].flags |= ISA_CARD_FLAG_ENABLED;

		if (HAS_MM(index)) {
			memory_map_enable_mregion(bus->map, bus->cards[index].mregion_index);
		}
		return 0;
	}
	dbg_print("Failed to enable isa card; Index out of range or card removed. index = %d, removed = %d\n", index, IS_REMOVED(index));
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
	dbg_print("Failed to disable isa card; Index out of range or card removed. index = %d, removed = %d\n", index, IS_REMOVED(index));
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

void isa_bus_reset(ISA_BUS* bus) {
	for (int i = 0; i < bus->card_index; ++i) {
		if (!IS_REMOVED(i) && IS_ENABLED(i) && HAS_RESET(i)) {
			bus->cards[i].reset(bus->cards[i].param);
		}
	}
}

void isa_bus_update(ISA_BUS* bus, uint64_t cycles) {
	for (int i = 0; i < bus->card_index; ++i) {
		if (!IS_REMOVED(i) && IS_ENABLED(i) && HAS_UPDATE(i)) {
			bus->cards[i].update(bus->cards[i].param, cycles);
		}
	}
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
int isa_card_remove_mm(ISA_BUS* bus, int index) {
	if (IS_IN_RANGE(index) && !IS_REMOVED(index)) {
		bus->cards[index].flags &= ~ISA_CARD_FLAG_HAS_MM;
		int r = memory_map_remove_mregion(bus->map, bus->cards[index].mregion_index);
		bus->cards[index].mregion_index = -1;
		return r;
	}
	dbg_print("Failed to remove MM from isa card; Index out of range or card removed. index = %d, removed = %d\n", index, IS_REMOVED(index));
	return 1;
}

int isa_card_add_io(ISA_BUS* bus, int index, ISA_BUS_WRITE_IO write_io_byte, ISA_BUS_READ_IO read_io_byte) {
	if (IS_IN_RANGE(index) && !IS_REMOVED(index)) {
		bus->cards[index].flags |= ISA_CARD_FLAG_HAS_IO;
		bus->cards[index].write_io_byte = write_io_byte;
		bus->cards[index].read_io_byte = read_io_byte;
		return 0;
	}
	dbg_print("Failed to add IO to isa card; Index out of range or card removed. index = %d, removed = %d\n", index, IS_REMOVED(index));
	return 1;
}
int isa_card_remove_io(ISA_BUS* bus, int index) {
	if (IS_IN_RANGE(index) && !IS_REMOVED(index)) {
		bus->cards[index].flags &= ~ISA_CARD_FLAG_HAS_IO;
		bus->cards[index].write_io_byte = NULL;
		bus->cards[index].read_io_byte = NULL;
		return 0;
	}
	dbg_print("Failed to remove IO from isa card; Index out of range or card removed. index = %d, removed = %d\n", index, IS_REMOVED(index));
	return 1;
}

int isa_card_add_reset(ISA_BUS* bus, int index, ISA_BUS_RESET reset) {
	if (IS_IN_RANGE(index) && !IS_REMOVED(index)) {
		bus->cards[index].flags |= ISA_CARD_FLAG_HAS_RESET;
		bus->cards[index].reset = reset;
		return 0;
	}
	dbg_print("Failed to add RESET to isa card; Index out of range or card removed. index = %d, removed = %d\n", index, IS_REMOVED(index));
	return 1;
}
int isa_card_remove_reset(ISA_BUS* bus, int index) {
	if (IS_IN_RANGE(index) && !IS_REMOVED(index)) {
		bus->cards[index].flags &= ~ISA_CARD_FLAG_HAS_RESET;
		bus->cards[index].reset = NULL;
		return 0;
	}
	dbg_print("Failed to remove RESET from isa card; Index out of range or card removed. index = %d, removed = %d\n", index, IS_REMOVED(index));
	return 1;
}

int isa_card_add_update(ISA_BUS* bus, int index, ISA_BUS_UPDATE update) {
	if (IS_IN_RANGE(index) && !IS_REMOVED(index)) {
		bus->cards[index].flags |= ISA_CARD_FLAG_HAS_UPDATE;
		bus->cards[index].update = update;
		return 0;
	}
	dbg_print("Failed to add UPDATE to isa card; Index out of range or card removed. index = %d, removed = %d\n", index, IS_REMOVED(index));
	return 1;
}
int isa_card_remove_update(ISA_BUS* bus, int index) {
	if (IS_IN_RANGE(index) && !IS_REMOVED(index)) {
		bus->cards[index].flags &= ~ISA_CARD_FLAG_HAS_UPDATE;
		bus->cards[index].update = NULL;
		return 0;
	}
	dbg_print("Failed to remove UPDATE from isa card; Index out of range or card removed. index = %d, removed = %d\n", index, IS_REMOVED(index));
	return 1;
}

int isa_card_add_param(ISA_BUS* bus, int index, void* param) {
	if (IS_IN_RANGE(index) && !IS_REMOVED(index)) {
		bus->cards[index].param = param;
		return 0;
	}
	dbg_print("Failed to add PARAM to isa card; Index out of range or card removed. index = %d, removed = %d\n", index, IS_REMOVED(index));
	return 1;
}
int isa_card_remove_param(ISA_BUS* bus, int index) {
	if (IS_IN_RANGE(index) && !IS_REMOVED(index)) {
		bus->cards[index].param = NULL;
		return 0;
	}
	dbg_print("Failed to remove PARAM from isa card; Index out of range or card removed. index = %d, removed = %d\n", index, IS_REMOVED(index));
	return 1;
}
