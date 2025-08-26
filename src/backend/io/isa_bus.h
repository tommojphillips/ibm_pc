/* isa_bus.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * ISA Bus
 */

#ifndef ISA_BUS_H
#define ISA_BUS_H

#include <stdint.h>

#include "memory_map.h"

/* ISA Card has IO function */
#define ISA_CARD_FLAG_HAS_IO  0x01

/* ISA Card has memory region */
#define ISA_CARD_FLAG_HAS_MM  0x02

/* ISA Card is enabled */
#define ISA_CARD_FLAG_ENABLED 0x04

/* ISA Card has been removed */
#define ISA_CARD_FLAG_REMOVED 0x08

/* ISA Bus Write IO 
	port:  the IO port address 
	value: the value written to the port
	Return: 1 if ISA Card has handled the write. 0 if not */
typedef int(*ISA_BUS_WRITE_IO)(void* param, uint16_t port, uint8_t value);

/* ISA Bus Read IO 
	port:  the IO port address 
	ptr:   the value read from the port
	Return: 1 if ISA Card has handled the read. 0 if not */
typedef int(*ISA_BUS_READ_IO)(void* param, uint16_t port, uint8_t* ptr);

typedef struct ISA_CARD {
	int flags;
	int memory_region_index;
	ISA_BUS_WRITE_IO write_io_byte; // write io byte. param, port, value
	ISA_BUS_READ_IO read_io_byte;   // read io byte. param, port
	void* param;
	MEMORY_REGION region;
	const char* name;
} ISA_CARD;

typedef struct ISA_BUS {
	ISA_CARD* cards;
	int card_count;
	int card_index;
	MEMORY_MAP* map; // ptr to memory map struct
} ISA_BUS;

/* Create isa bus 
	bus:	(out) points to the created bus
	map:	(in)  the memory map instance
	Returns: 1 if error. 0 on success */
int isa_bus_create(ISA_BUS** bus, MEMORY_MAP* map);

/* Destroy memory map 
	bus:	(in) the bus to destroy */
void isa_bus_destroy(ISA_BUS* bus);

/* Create an ISA Card on the bus
   bus:     the bus to add a card to
   flags:   card flags
   Returns: -1 if error or the index of the card on success */
int isa_bus_create_card(ISA_BUS* bus, int flags, const char* name);

/* Add (enable) an ISA Card on bus
   bus:     the bus to add a card to
   index:   the index of the card
   Returns: 1 if error or 0 on success */
int isa_bus_enable_card(ISA_BUS* bus, int index);

/* Remove (disable) an ISA Card from the bus
   bus:     the bus to remove a card from
   index:   the index of the card
   Returns: 1 if error or 0 on success */
int isa_bus_disable_card(ISA_BUS* bus, int index);

/* Add a memory region to the ISA Bus's memory map.
   bus:     the bus to add a region to
   card_index: the card index
   start:   the region start
   size:    the region size
   mask:    the region address mask
   flags:   the region flags
   Returns: -1 if error or the index of the added memory region on success */
int isa_card_add_memory_region(ISA_BUS* bus, int card_index, uint32_t start, uint32_t size, uint32_t mask, int flags);

/* Add io functions to the isa card
   bus:     the bus to add that holds the card
   card_index: the card index
   write_io_byte: write io byte function
   write_io_byte: write io byte function
   param: the param to pass to the write and read functions.
   Returns: -1 if error or 0 on success */
int isa_card_add_io(ISA_BUS* bus, int card_index, ISA_BUS_WRITE_IO write_io_byte, ISA_BUS_READ_IO read_io_byte, void* param);

/* ISA Bus Read IO
	port:  the IO port address
	ptr:   the value read from the port
	Return: 1 if a ISA Card has handled the read. 0 if not */
int isa_bus_read_io_byte(ISA_BUS* bus, uint16_t port, uint8_t* v);

/* ISA Bus Write IO
	port:  the IO port address
	value: the value write to the port
	Return: 1 if a ISA Card has handled the write. 0 if not */
int isa_bus_write_io_byte(ISA_BUS* bus, uint16_t port, uint8_t value);

#endif
