/* memory_map.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Memory Map
 */

#include <stdint.h>
#include <malloc.h>
#include <memory.h>

#include "memory_map.h"

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

/* Mregion Start */
#define MR_START     (map->regions[i].start)

/* Mregion Size */
#define MR_SIZE      (map->regions[i].size)

/* Mregion Mask */
#define MR_MASK      (map->regions[i].mask)

/* Mregion End */
#define MR_END       (MR_START + MR_SIZE)

/* Mregion Pointer */
#define MR_PTR       (map->mem + MR_START + ((address - MR_START) & MR_MASK))

/* Is Mregion write protected */
#define IS_WRITE_PROTECTED(i) (map->regions[i].flags & MREGION_FLAG_WRITE_PROTECTED)

/* Is Mregion not write protected */
#define IS_WRITABLE(i) (!IS_WRITE_PROTECTED(i))

/* Is Mregion removed */
#define IS_REMOVED(i) (map->regions[i].flags & MREGION_FLAG_REMOVED)

/* Is Mregion enabled */
#define IS_ENABLED(i) (map->regions[i].flags & MREGION_FLAG_ENABLED)

/* Is Mregion active */
#define IS_ACTIVE(i)  (!IS_REMOVED(i) && IS_ENABLED(i))

/* Is index in range of start, end */
#define IS_IN_RANGE(i, start, end) ((i) >= (start) && (i) < (end))

/* --- Memory Map --- */
int memory_map_create(MEMORY_MAP* map, uint32_t buffer_size, int region_count) {
	if (map != NULL) {
		/* alloc mregions */
		map->regions = calloc(region_count, sizeof(MEMORY_REGION));
		if (map->regions == NULL) {
			dbg_print("Failed to create memory map; Calloc failed. region_count = %x, region_size = %zu\n", region_count, sizeof(MEMORY_REGION));
			return 1;
		}
		map->region_count = region_count;
		map->region_index = 0;

		/* alloc memory buffer */
		map->mem = calloc(1, buffer_size);
		if (map->mem == NULL) {
			dbg_print("Failed to create memory map; Calloc failed. buffer_size = %x\n", buffer_size);
			return 1;
		}
		map->mem_size = buffer_size;

		return 0;
	}
	dbg_print("Failed to create memory map; Map was NULL.\n");
	return 1;
}
void memory_map_destroy(MEMORY_MAP* map) {
	if (map != NULL) {
		/* free mregions */		
		if (map->regions != NULL) {
			free(map->regions);
			map->regions = NULL;
		}
		map->region_count = 0;
		map->region_index = 0;

		/* free memory buffer */		
		if (map->mem != NULL) {
			free(map->mem);
			map->mem = NULL;
			map->mem_size = 0;
		}
	}
}

uint8_t memory_map_read_byte(MEMORY_MAP* map, uint32_t address) {
	
	/* Handle mregion */
	for (int i = 0; i < map->region_index; ++i) {
		if (IS_ACTIVE(i) && IS_IN_RANGE(address, MR_START, MR_END)) {
			return *(uint8_t*)(map->mem + MR_START + ((address - MR_START) & map->regions[i].mask));
		}
	}

	dbg_print("reading from %x\n", address);
	return 0;
}
void memory_map_write_byte(MEMORY_MAP* map, uint32_t address, uint8_t value) {

	/* Handle mregion */
	for (int i = 0; i < map->region_index; ++i) {
		if (IS_ACTIVE(i) && IS_IN_RANGE(address, MR_START, MR_END)) {
			if (IS_WRITABLE(i)) {
				*(uint8_t*)(map->mem + MR_START + ((address - MR_START) & map->regions[i].mask)) = value;
			}
			return;
		}
	}

	dbg_print("writing %x to %x\n", value, address);
}
void memory_map_set_writeable_region(MEMORY_MAP* map, uint8_t value) {
	for (int i = 0; i < map->region_index; ++i) {
		if (IS_ACTIVE(i) && IS_WRITABLE(i)) {
			for (uint32_t j = 0; j < MR_SIZE; ++j) {
				memory_map_write_byte(map, MR_START + j, value);
			}
			//dbg_print("mregion set %x - %x = %x\n", MR_START, map->regions[i].end, value);
		}
	}
}

int memory_map_validate(MEMORY_MAP* map) {
	if (!map || !map->regions) {
		dbg_print("Memory map not initialized.\n");
		return 1;
	}

	for (int i = 0; i < map->region_index; ++i) {
		if (IS_ACTIVE(i)) {

			uint32_t start = MR_START;
			uint32_t size = MR_SIZE;
			uint32_t end = MR_END;
			uint32_t mask = MR_MASK;

			if (size == 0) {
				dbg_print("Memory Map warning: region[%d] has zero size.\n", i);
			}			

			if (mask == 0) {
				dbg_print("Memory Map warning: region[%d] has zero mask.\n", i);
			}

			/* Overlap check */
			for (int j = 0; j < map->region_index; ++j) {
				if (IS_ACTIVE(j) && j != i) {

					uint32_t other_start = map->regions[j].start;
					uint32_t other_end = other_start + map->regions[j].size;

					if (!(end <= other_start || start >= other_end)) {
						dbg_print("Memory Map warning: region[%d] (%x-%x) overlaps region[%d] (%x-%x).\n", i, start, end - 1, j, other_start, other_end - 1);
					}
				}
			}
		}
	}
	return 0;
}

/* --- Memory Region --- */

int memory_map_add_mregion(MEMORY_MAP* map, uint32_t start, uint32_t size, uint32_t mask, uint32_t flags) {
	int index = -1;

	/* find first removed mregion */
	for (int i = 0; i < map->region_index; ++i) {
		if (IS_REMOVED(i)) {
			index = i;
		}
	}

	/* did not find a removed mregion; add to end */
	if (index < 0) {
		index = map->region_index;
		if (index >= map->region_count) {
			dbg_print("Failed to add mregion; Index out of range. start = %x, size = %x, mask = %x, flags = %x\n", start, size, mask, flags);
			return -1;
		}
		map->region_index++;
	}

	map->regions[index].start = start;
	map->regions[index].size = size;
	map->regions[index].mask = mask;
	map->regions[index].flags = flags | MREGION_FLAG_ENABLED;
	return index;
}
int memory_map_remove_mregion(MEMORY_MAP* map, int index) {
	if (IS_IN_RANGE(index, 0, map->region_index)) {
		map->regions[index].start = 0;
		map->regions[index].size = 0;
		map->regions[index].mask = 0;
		map->regions[index].flags = MREGION_FLAG_REMOVED;
		return 0;
	}
	dbg_print("Failed to remove mregion; Index out of range. index = %d\n", index);
	return 1;
}
int memory_map_enable_mregion(MEMORY_MAP* map, int index) {
	if (IS_IN_RANGE(index, 0, map->region_index) && !IS_REMOVED(index)) {
		map->regions[index].flags |= MREGION_FLAG_ENABLED;
		return 0;
	}
	dbg_print("Failed to enable mregion; Index out of range or mregion removed. index = %d, removed = %x\n", index, IS_REMOVED(index));
	return 1;
}
int memory_map_disable_mregion(MEMORY_MAP* map, int index) {
	if (IS_IN_RANGE(index, 0, map->region_index) && !IS_REMOVED(index)) {
		map->regions[index].flags &= ~MREGION_FLAG_ENABLED;
		return 0;
	}
	dbg_print("Failed to disable mregion; Index out of range or mregion removed. index = %d, removed = %x\n", index, IS_REMOVED(index));
	return 1;
}

MEMORY_REGION* memory_map_get_mregion(MEMORY_MAP* map, int i) {

	if (!IS_IN_RANGE(i, 0, map->region_index)) {
		dbg_print("Failed to get mregion; Index out of range. index = %x\n", i);
		return NULL;
	}

	if (IS_REMOVED(i)) {
		dbg_print("Failed to get mregion; mregion has been removed. index = %x\n", i);
		return NULL;
	}

	return &map->regions[i];
}
