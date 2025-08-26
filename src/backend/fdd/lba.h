/* lba.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * LBA ( Logical Block Addressing )
 */

#ifndef LBA_H
#define LBA_H

#include <stdint.h>

/* Cylinder-Head-Sector */
typedef struct CHS {
	uint16_t cylinder;
	uint16_t head;
	uint16_t sector;
} CHS;

/* Logical Block Addressing */
typedef uint32_t LBA;

/* LBA to CHS 
 hpc: (in)  max heads per cylinder
 spt: (in)  max sectors per track
 lba: (in)  lba
 chs: (out) cylinder-head-sector */
void lba_to_chs(const uint16_t hpc, const uint16_t spt, const LBA lba, CHS* chs);

/* CHS to LBA 
 hpc: (in)  max heads per cylinder
 spt: (in)  max sectors per track
 chs: (in)  cylinder-head-sector
 lba: (out) lba */
void chs_to_lba(const uint16_t hpc, const uint16_t spt, const CHS* chs, LBA* lba);

#endif
