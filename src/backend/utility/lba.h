/* lba.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * LBA ( Logical Block Addressing )
 */

#ifndef LBA_H
#define LBA_H

#include <stdint.h>

/* CHS struct */
typedef struct CHS {
	uint16_t c; /* Cylinder */
	uint8_t h;  /* Head */
	uint8_t s;  /* Sector */
} CHS;

/* Logical Block Addressing */
typedef size_t LBA;

/* LBA to CHS
 geometry: (in) Geometry
 lba:      (in) LBA
 Returns:  The CHS struct */
CHS lba_to_chs(const CHS geometry, const LBA lba);

/* CHS to LBA
 geometry: (in) Geometry
 chs:      (in) CHS struct
 Returns:  The LBA */
LBA chs_to_lba(const CHS geometry, const CHS chs);

void chs_advance(const CHS geometry, CHS* const chs);
void chs_advance_sector(const CHS geometry, CHS* const chs);

void chs_set(CHS* const dest, const CHS src);
void chs_reset(CHS* const dest);

size_t lba_to_offset(LBA lba, uint16_t sector_size, size_t index);
LBA offset_to_lba(size_t offset, uint16_t sector_size, size_t index);

size_t chs_to_offset(const CHS geometry, const CHS chs, uint16_t sector_size, size_t index);
CHS offset_to_chs(const CHS geometry, size_t offset, uint16_t sector_size, size_t index);

size_t chs_get_total_byte_count(const CHS geometry, uint16_t sector_size);

#endif
