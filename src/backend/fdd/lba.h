/* lba.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * LBA ( Logical Block Addressing )
 */

#ifndef LBA_H
#define LBA_H

#include <stdint.h>

/* CHS struct */
typedef struct CHS {
	uint8_t c; /* Cylinder */
	uint8_t h;  /* Head */
	uint8_t s;  /* Sector */
} CHS;

/* Logical Block Addressing */
typedef uint32_t LBA;

/* LBA to CHS
 disk_descriptor: (in) Disk descriptor
 lba:             (in)  LBA
 Returns: the CHS struct */
CHS lba_to_chs(const CHS disk_descriptor, const LBA lba);

/* CHS to LBA
 disk_descriptor: (in) Disk descriptor
 chs:             (in) CHS struct
 Returns: the LBA */
LBA chs_to_lba(const CHS disk_descriptor, const CHS chs);

void chs_advance(const CHS disk_descriptor, CHS* const chs);

void chs_set(CHS* const dest, const CHS src);

#endif
