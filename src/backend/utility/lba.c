/* lba.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * LBA 
 */

#include <stdint.h>

#include "lba.h"

CHS lba_to_chs(const CHS geometry, const LBA lba) {
	uint32_t spc = geometry.h * geometry.s;
	uint32_t temp = lba % spc;
	
	CHS chs = { 0 };
	chs.c = (lba / spc) & 0xFFFF;
	chs.h = (temp / geometry.s) & 0xFF;
	chs.s = (temp % geometry.s) + 1;
	return chs;
}
LBA chs_to_lba(const CHS geometry, const CHS chs) {
	LBA lba = ((chs.c * geometry.h + chs.h) * geometry.s) + (chs.s - 1);
	return lba;
}

void chs_advance(const CHS geometry, CHS* const chs) {
	chs->s++;
	if (chs->s > geometry.s) {
		chs->s = 1;
		chs->h++;
		if (chs->h >= geometry.h) {
			chs->h = 0;
			chs->c++;
			if (chs->c >= geometry.c) {
				chs->c = 0;
			}
		}
	}
}

void chs_advance_sector(const CHS geometry, CHS* const chs) {
	chs->s++;
	if (chs->s > geometry.s) {
		chs->s = 1;
	}
}

void chs_set(CHS* const dest, const CHS src) {
	dest->c = src.c;
	dest->h = src.h;
	dest->s = src.s;
}

void chs_reset(CHS* const dest) {
	dest->c = 0;
	dest->h = 0;
	dest->s = 0;
}

size_t lba_to_offset(LBA lba, uint16_t sector_size, size_t index) {
	return (lba * sector_size) + index;
}

LBA offset_to_lba(size_t offset, uint16_t sector_size, size_t index) {
	return (offset - index) / sector_size;
}

size_t chs_to_offset(const CHS geometry, CHS const chs, uint16_t sector_size, size_t index) {
	LBA lba = chs_to_lba(geometry, chs);
	return lba_to_offset(lba, sector_size, index);
}

CHS offset_to_chs(const CHS geometry, size_t offset, uint16_t sector_size, size_t index) {
	LBA lba = offset_to_lba(offset, sector_size, index);
	return lba_to_chs(geometry, lba);
}

size_t chs_get_total_byte_count(const CHS geometry, uint16_t sector_size) {
	return ((size_t)geometry.c * geometry.h * geometry.s * sector_size);
}
