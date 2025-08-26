/* isa_bus.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * ISA Bus
 */

#include <stdint.h>
#include <malloc.h>
#include <malloc.h>
#include <memory.h>

#include "isa_bus.h"
#include "memory_map.h"

int isa_bus_create(ISA_BUS** bus, MEMORY_MAP* map) {
	*bus = (ISA_BUS*)calloc(1, sizeof(ISA_BUS));
	if (*bus == NULL) {
		return 1;
	}
	(**bus).cards = (ISA_CARD*)calloc(10, sizeof(ISA_CARD));
	if ((**bus).cards == NULL) {
		return 1;
	}
	(**bus).card_count = 10;
	(**bus).card_index = 0;
	(**bus).map = map;
	return 0;
}
void isa_bus_destroy(ISA_BUS* bus) {
	if (bus != NULL) {
		
		if (bus->cards != NULL) {
			free(bus->cards);
			bus->cards = NULL;
		}

		bus->card_count = 0;
		bus->card_index = 0;

		free(bus);
	}
}

int isa_bus_create_card(ISA_BUS* bus, int flags, const char* name) {
	int index = bus->card_index;
	bus->card_index++;
	if (index >= bus->card_count - 1) {
		void* regions = realloc(bus->cards, (size_t)((bus->card_count + 10) * sizeof(ISA_CARD)));
		if (regions == NULL) {
			return -1;
		}
		bus->cards = regions;
		bus->card_count += 10;
	}

	bus->cards[index].flags = flags;
	bus->cards[index].name = name;
	bus->cards[index].memory_region_index = -1;
	return index;
}
int isa_bus_enable_card(ISA_BUS* bus, int index) {
	if (index < bus->card_count && !(bus->cards[index].flags & ISA_CARD_FLAG_ENABLED)) {
		bus->cards[index].flags |= ISA_CARD_FLAG_ENABLED;
#ifdef USE_REGIONS
		if (bus->cards[index].flags & ISA_CARD_FLAG_HAS_MM) {
			bus->cards[index].memory_region_index = add_memory_region(bus->map, bus->cards[index].region.start, bus->cards[index].region.size, bus->cards[index].region.mask, bus->cards[index].region.flags);
		}
#endif
		return 0;
	}
	return 1;
}
int isa_bus_disable_card(ISA_BUS* bus, int index) {
	if (index < bus->card_count && (bus->cards[index].flags & ISA_CARD_FLAG_ENABLED)) {
#ifdef USE_REGIONS
		if (bus->cards[index].flags & ISA_CARD_FLAG_HAS_MM) {
			remove_memory_region(bus->map, bus->cards->memory_region_index);
		}
#endif
		bus->cards[index].flags &= ~ISA_CARD_FLAG_ENABLED;
		bus->cards[index].memory_region_index = -1;
		return 0;
	}
	return 1;
}

int isa_card_add_memory_region(ISA_BUS* bus, int card_index, uint32_t start, uint32_t size, uint32_t mask, int flags) {
	bus->cards[card_index].flags |= ISA_CARD_FLAG_HAS_MM;
	bus->cards[card_index].region.flags = flags;
	bus->cards[card_index].region.mask = mask;
	bus->cards[card_index].region.size = size;
	bus->cards[card_index].region.start = start;
	return 0;
}
int isa_card_add_io(ISA_BUS* bus, int card_index, ISA_BUS_WRITE_IO write_io_byte, ISA_BUS_READ_IO read_io_byte, void* param) {
	bus->cards[card_index].flags |= ISA_CARD_FLAG_HAS_IO;
	bus->cards[card_index].write_io_byte = write_io_byte;
	bus->cards[card_index].read_io_byte = read_io_byte;
	bus->cards[card_index].param = param;
	return 0;
}

int isa_bus_read_io_byte(ISA_BUS* bus, uint16_t port, uint8_t* v) {
	for (int i = 0; i < bus->card_index; ++i) {
		if (bus->cards[i].flags & (ISA_CARD_FLAG_ENABLED | ISA_CARD_FLAG_HAS_IO)) {
			if (bus->cards[i].read_io_byte(bus->cards[i].param, port, v)) {
				return 1;
			}
		}
	}
	return 0;
}
int isa_bus_write_io_byte(ISA_BUS* bus, uint16_t port, uint8_t value) {
	for (int i = 0; i < bus->card_index; ++i) {
		if (bus->cards[i].flags & (ISA_CARD_FLAG_ENABLED | ISA_CARD_FLAG_HAS_IO)) {
			if (bus->cards[i].write_io_byte(bus->cards[i].param, port, value)) {
				return 1;
			}
		}
	}
	return 0;
}
