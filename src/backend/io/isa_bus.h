/* isa_bus.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * ISA Bus
 */

#ifndef ISA_BUS_H
#define ISA_BUS_H

#include <stdint.h>

#include "memory_map.h"

/* ISA Card has no flags */
#define ISA_CARD_FLAG_NONE    0x00

/* ISA Card has IO (port mapped IO) */
#define ISA_CARD_FLAG_HAS_IO  0x01

/* ISA Card has memory region (ram, rom) */
#define ISA_CARD_FLAG_HAS_MM  0x02

/* ISA Card is enabled */
#define ISA_CARD_FLAG_ENABLED 0x04

/* ISA Card has been removed; A new ISA Card can overwrite this card */
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

/* ISA Card */
typedef struct ISA_CARD {
	int mregion_index;              /* index to mregion. used if the isa card has memory mapped (HAS_MM) */
	uint32_t flags;                 /* flags for the isa card. ISA_CARD_FLAG_XXX */
	ISA_BUS_WRITE_IO write_io_byte; /* write io byte. used if the isa card has io mapped (HAS_IO) */
	ISA_BUS_READ_IO read_io_byte;   /* read io byte. used if the isa card has io mapped (HAS_IO) */
	void* param;                    /* param passed to read/write io funcs. used if the isa card has io mapped (HAS_IO) */
	char* name;
} ISA_CARD;

/* ISA BUS */
typedef struct ISA_BUS {
	int card_count;  /* max ISA CARDS */
	int card_index;  /* current ISA CARDS */
	ISA_CARD* cards;
	MEMORY_MAP* map; /* memory map struct ptr */
} ISA_BUS;

/* Create an ISA Bus
	bus:	the bus instance
	map:	the memory map instance
	slots:  the number of isa card slots on the bus
	Returns: 1 if error. 0 on success */
int isa_bus_create(ISA_BUS* bus, MEMORY_MAP* map, int slots);

/* Destroy an ISA Bus
	bus: the bus instance */
void isa_bus_destroy(ISA_BUS* bus);

/* Add an ISA Card to the bus and enables it
   bus:     the isa bus instance
   name:    the isa card name; can be NULL
   Returns: -1 if error or the index of the added card on success */
int isa_bus_add_card(ISA_BUS* bus, const char* name);

/* Remove an ISA Card from the bus
   bus:     the isa bus instance
   index:   the isa card index
   Returns: 1 if error or 0 on success */
int isa_bus_remove_card(ISA_BUS* bus, int index);

/* Remove all ISA Cards from the bus
   bus:     the isa bus instance
   Returns: 1 if error or 0 on success */
int isa_bus_remove_all_cards(ISA_BUS* bus);

/* Enable an ISA Card on bus
   bus:     the isa bus instance
   index:   the isa card index
   Returns: 1 if error or 0 on success */
int isa_bus_enable_card(ISA_BUS* bus, int index);

/* Disable an ISA Card from the bus
   bus:     the isa bus instance
   index:   the isa card index
   Returns: 1 if error or 0 on success */
int isa_bus_disable_card(ISA_BUS* bus, int index);

/* Add a memory region to an ISA CARD
   bus:     the isa bus instance
   index:   the isa card index
   start:   the region start
   size:    the region size
   mask:    the region address mask
   flags:   the region flags
   Returns: 1 if error or 0 on success */
int isa_card_add_mm(ISA_BUS* bus, int index, uint32_t start, uint32_t size, uint32_t mask, uint32_t flags);

/* Remove mregon from an ISA Card on the bus
   bus:     the isa bus instance
   index:   the isa card index
   Returns: 1 if error or 0 on success */
int isa_card_remove_mm(ISA_BUS* bus, int index);

/* Add port mapped io to an ISA CARD
   bus:           the isa bus instance
   index:         the isa card index
   write_io_byte: write io byte function
   read_io_byte:  read io byte function
   param:         the param to pass to the write and read functions.
   Returns:       1 if error or 0 on success */
int isa_card_add_io(ISA_BUS* bus, int index, ISA_BUS_WRITE_IO write_io_byte, ISA_BUS_READ_IO read_io_byte, void* param);

/* Remove io from an ISA Card on the bus
   bus:     the isa bus instance
   index:   the isa card index
   Returns: 1 if error or 0 on success */
int isa_card_remove_io(ISA_BUS* bus, int index);

/* ISA Bus Read port mapped io
	port:   the IO port address
	value   the value read from the port
	Return: 1 if a ISA Card has handled the read. 0 if not */
int isa_bus_read_io_byte(ISA_BUS* bus, uint16_t port, uint8_t* value);

/* ISA Bus Write port mapped io
	port:   the IO port address
	value:  the value write to the port
	Return: 1 if a ISA Card has handled the write. 0 if not */
int isa_bus_write_io_byte(ISA_BUS* bus, uint16_t port, uint8_t value);

#endif
