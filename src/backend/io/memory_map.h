/* memory_map.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Memory Map
 */

#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#include <stdint.h>

typedef struct MEMORY_REGION MEMORY_REGION;

 /* --- Memory Map --- */

/* Memory Map */
typedef struct MEMORY_MAP {
	MEMORY_REGION* regions;
	int region_count;
	int region_index;
	uint8_t* mem;
	uint32_t mem_size;
} MEMORY_MAP;

/* Creates a memory map; allocates memory for the mregions, memory buffer
	map:	      the map instance
	buffer_size:  the size of the memory buffer
	region_count: the number of memory regions in the memory map
	Returns:      1 if error or 0 if success */
int memory_map_create(MEMORY_MAP* map, uint32_t buffer_size, int region_count);

/* Destroys a memory map
	map: the map instance */
void memory_map_destroy(MEMORY_MAP* map);

/* Read byte from memory map
	map:     the map instance
	address: the address to read from
	Returns: the value that was read at the address */
uint8_t memory_map_read_byte(MEMORY_MAP* map, uint32_t address);

/* Write byte to memory map
	map:     the map instance
	address: the address to write to
	value:   the value to write at the address */
void memory_map_write_byte(MEMORY_MAP* map, uint32_t address, uint8_t value);

/* Set all writable memory regions to value
	map:     the map instance
	value:   the value to write */
void memory_map_set_writeable_region(MEMORY_MAP* map, uint8_t value);

int memory_map_validate(MEMORY_MAP* map);

/* --- Memory Region --- */

/* Memory region has no flags */
#define MREGION_FLAG_NONE            0x00

/* Memory region is write protected */
#define MREGION_FLAG_WRITE_PROTECTED 0x01

/* Memory region is enabled */
#define MREGION_FLAG_ENABLED         0x02

 /* Memory region has been removed; A new mregion can override this mregion */
#define MREGION_FLAG_REMOVED         0x04

/* Memory Region */
typedef struct MEMORY_REGION {
	uint32_t start; /* the mregion start address */
	uint32_t size;  /* the mregion size */
	uint32_t mask;  /* the mregion address mask */
	uint32_t flags; /* flags for the mregion (MREGION_FLAG_XXX) */
} MEMORY_REGION;

/* Add a mregion to the memory map and enables it
	map:     the map instance
	start:   the region start offset
	size:    the region size
	mask:    the region address mask
	flags:   the region flags
	Returns: -1 if error or the index of the added mregion on success */
int memory_map_add_mregion(MEMORY_MAP* map, uint32_t start, uint32_t size, uint32_t mask, uint32_t flags);

/* Remove a mregion from the memory map
	map:     the map instance
	index:   the region index
	Returns: 1 if error or 0 on success */
int memory_map_remove_mregion(MEMORY_MAP* map, int index);

/* Enable a mregion in the memory map
	map:     the map instance
	index:   the region index
	Returns: 1 if error or 0 on success */
int memory_map_enable_mregion(MEMORY_MAP* map, int index);

/* Disable a mregion in the memory map
	map:     the map instance
	index:   the region index
	Returns: 1 if error or 0 on success */
int memory_map_disable_mregion(MEMORY_MAP* map, int index);

/* Get a mregion in the memory map
	map:     the map instance
	index:   the region index
	Returns: NULL if error or the mregion on success */
MEMORY_REGION* memory_map_get_mregion(MEMORY_MAP* map, int i);

#endif /* MEMORY_MAP_H */
