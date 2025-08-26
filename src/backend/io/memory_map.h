/* memory_map.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Memory Map
 */

#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#include <stdint.h>

#define MREGION_FLAG_NONE            0
#define MREGION_FLAG_WRITE_PROTECTED 1
#define MREGION_FLAG_MIRRORED        4
#define MREGION_FLAG_REMOVED         8

#define USE_REGIONS 1

typedef struct {
	uint32_t start;
	uint32_t size;
	uint32_t mask;
	int flags;
} MEMORY_REGION;

typedef struct {
	MEMORY_REGION* regions;
	int region_count;
	int region_index;
	uint8_t* mem;
	int mem_size;
	uint8_t* internal_buffer;
	int internal_buffer_size;
} MEMORY_MAP;

/* Create memory map 
	map:	 (out) points to the created map
	Returns: 1 if error or 0 if success */
int create_memory_map(MEMORY_MAP* map, int memory_buffer_size);

/* Destroy memory map */
void destroy_memory_map(MEMORY_MAP* map);

/* Add a memory region to the memory map.
   map:     the map to add a region to
   start:   the region start
   size:    the region size
   mask:    the region address mask
   flags:   the region flags
   Returns: -1 if error or the index of the added memory region on success */
int add_memory_region(MEMORY_MAP* map, uint32_t start, uint32_t size, uint32_t mask, int flags);

/* Remove a memory region to the memory map. 
   map:     the map to add a region to
   index:   the region index
   Returns: 1 if error or 0 on success */
int remove_memory_region(MEMORY_MAP* map, int index);

/* Read byte from memory region */
uint8_t memory_map_read_byte(MEMORY_MAP* map, uint32_t address);

/* Write byte to memory region */
void memory_map_write_byte(MEMORY_MAP* map, uint32_t address, uint8_t value);

/* Read word from memory region */
uint16_t memory_map_read_word(MEMORY_MAP* map, uint32_t address);

/* Write word to memory region */
void memory_map_write_word(MEMORY_MAP* map, uint32_t address, uint16_t value);

/* Set all writable regions to value */
void memory_map_set_writeable_region(MEMORY_MAP* map, uint8_t value);

#endif
