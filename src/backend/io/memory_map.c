/* memory_map.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Memory Map
 */

#include <stdint.h>
#include <malloc.h>

#include "memory_map.h"

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

int create_memory_map(MEMORY_MAP* map, int memory_buffer_size) {
	
	(*map).regions = (MEMORY_REGION*)calloc(10, sizeof(MEMORY_REGION));
	if ((*map).regions == NULL) {
		return 1;
	}

	(*map).internal_buffer_size = 2;
	(*map).internal_buffer = (uint8_t*)calloc(1, (*map).internal_buffer_size);
	if ((*map).internal_buffer == NULL) {
		return 1;
	}

	(*map).region_count = 10;
	(*map).region_index = 0;

	(*map).mem_size = memory_buffer_size;
	(*map).mem = (uint8_t*)calloc(1, memory_buffer_size);
	if ((*map).mem == NULL) {
		return 1;
	}

	return 0;
}
void destroy_memory_map(MEMORY_MAP* map) {
	if (map != NULL) {
		
		if (map->regions != NULL) {
			free(map->regions);
			map->regions = NULL;
		}

		if (map->mem != NULL) {
			free(map->mem);
			map->mem = NULL;
			map->mem_size = 0;
		}

		if (map->internal_buffer != NULL) {
			free(map->internal_buffer);
			map->internal_buffer = NULL;
			map->internal_buffer_size = 0;
		}
		
		map->region_count = 0;
		map->region_index = 0;
	}
}

int add_memory_region(MEMORY_MAP* map, uint32_t start, uint32_t size, uint32_t mask, int flags) {

	int index = -1;
	for (int i = 0; i < map->region_index; ++i) {
		if (map->regions[i].flags & MREGION_FLAG_REMOVED) {
			index = i;
		}
	}

	if (index < 0) {
		index = map->region_index;
		map->region_index++;
		if (index >= map->region_count - 1) {
			void* regions = realloc(map->regions, (size_t)((map->region_count + 10) * sizeof(MEMORY_REGION)));
			if (regions == NULL) {
				return -1;
			}
			map->regions = regions;
			map->region_count += 10;
		}
	}

	map->regions[index].start = start;
	map->regions[index].size = size;
	map->regions[index].mask = mask;
	map->regions[index].flags = flags;
	return index;
}
int remove_memory_region(MEMORY_MAP* map, int index) {
	if (index < map->region_count) {
		map->regions[index].start = 0;
		map->regions[index].size = 0;
		map->regions[index].flags = MREGION_FLAG_REMOVED;

		if (index == map->region_index - 1) {
			map->region_index--;
		}
	}
	else {
		return 1;
	}
	return 0;
}

#define B_START     map->regions[i].start
#define B_SIZE      map->regions[i].size
#define B_END       (B_START + B_SIZE)
#define B_PTR       (map->mem + B_START + ((address - B_START) & map->regions[i].mask))
#define IS_WRITABLE (!(map->regions[i].flags & MREGION_FLAG_WRITE_PROTECTED))

uint8_t memory_map_read_byte(MEMORY_MAP* map, uint32_t address) {
	for (int i = 0; i < map->region_index; ++i) {
		if (address < B_END && address >= B_START) {
			return *(uint8_t*)B_PTR;
		}
	}
	dbg_print("reading from %x\n", address);
	return 0;
}
void memory_map_write_byte(MEMORY_MAP* map, uint32_t address, uint8_t value) {
	for (int i = 0; i < map->region_index; ++i) {
		if (address < B_END && address >= B_START) {
			if (IS_WRITABLE) {
				*(uint8_t*)B_PTR = value;
			}
			return;
		}
	}
	dbg_print("writing %x to %x\n", value, address);
}

void* __cdecl memset(void* _Dst, int _Val, size_t _Size);

void memory_map_set_writeable_region(MEMORY_MAP* map, uint8_t value) {
	for (int i = 0; i < map->region_index; ++i) {
		if (IS_WRITABLE) {
			memset(map->mem + B_START, value, B_SIZE);
		}
	}
}
