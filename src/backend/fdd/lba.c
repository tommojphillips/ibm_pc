/* lba.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * LBA 
 */

#include <stdint.h>

#include "lba.h"

void lba_to_chs(const uint16_t hpc, const uint16_t spt, const LBA lba, CHS* chs) {
	/*  temp   = LBA % (hpc * spt)
		cyl    = LBA / (hpc * spt)
		head   = temp / spt
		sector = temp % spt + 1 */

	uint16_t temp = lba % (hpc * spt);
	chs->cylinder = (lba / (hpc * spt)) & 0xFFFF;
	chs->head = (temp / spt);
	chs->sector = (temp % spt + 1);
}
void chs_to_lba(const uint16_t hpc, const uint16_t spt, const CHS* chs, LBA* lba) {
	/* LBA = ( ( cyl * HPC + head ) * SPT ) + sector - 1 */

	*lba = ((chs->cylinder * hpc + chs->head) * spt) + (chs->sector - 1);
}
