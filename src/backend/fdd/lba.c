/* lba.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * LBA 
 */

#include <stdint.h>

#include "lba.h"

CHS lba_to_chs(const CHS disk_descriptor, const LBA lba) {
	uint32_t spc = disk_descriptor.h * disk_descriptor.s;
	uint32_t temp = lba % spc;
	
	CHS chs = { 0 };
	chs.c = (lba / spc) & 0xFF;
	chs.h = (temp / disk_descriptor.s) & 0xFF;
	chs.s = (temp % disk_descriptor.s) + 1;
	return chs;
}
LBA chs_to_lba(const CHS disk_descriptor, const CHS chs) {
	LBA lba = ((chs.c * disk_descriptor.h + chs.h) * disk_descriptor.s) + (chs.s - 1);
	return lba;
}

void chs_advance(const CHS disk_descriptor, CHS* const chs) {
	chs->s++;
	if (chs->s > disk_descriptor.s) {
		chs->s = 1;
		chs->h++;
		if (chs->h >= disk_descriptor.h) {
			chs->h = 0;
			chs->c++;
		}
	}
}

void chs_set(CHS* const dest, const CHS src) {
	dest->c = src.c;
	dest->h = src.h;
	dest->s = src.s;
}
