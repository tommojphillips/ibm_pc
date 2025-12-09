/* xebec_hdd.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Hard Drive Disk
 */

#ifndef XEBEC_HDD_H
#define XEBEC_HDD_H

#include <stdint.h>
#include "backend/utility/lba.h"

typedef enum XEBEC_HDD_TYPE {
	XEBEC_HDD_TYPE_NONE,
	XEBEC_HDD_TYPE_1,    /* 10MB - 306 4 17 - Type 1 */
	XEBEC_HDD_TYPE_16,   /* 20MB - 612 4 17 - Type 16 */
	XEBEC_HDD_TYPE_2,    /* 20MB - 615 4 17 - Type 2 */
	XEBEC_HDD_TYPE_13,   /* 20MB - 306 8 17 - Type 13 */
} XEBEC_HDD_TYPE;

typedef enum XEBEC_FILE_TYPE {
	XEBEC_FILE_TYPE_NONE,
	XEBEC_FILE_TYPE_VHD,
	XEBEC_FILE_TYPE_RAW,
} XEBEC_FILE_TYPE;

typedef struct XEBEC_HDD_GEOMETRY {
	CHS chs;
	XEBEC_HDD_TYPE type;
	const char* name;
} XEBEC_HDD_GEOMETRY;

typedef struct XEBEC_HDD {
	CHS chs;
	uint8_t inserted;
	uint8_t dirty;
	XEBEC_FILE_TYPE file_type;
	size_t file_size;
	const XEBEC_HDD_GEOMETRY* geometry;
	XEBEC_HDD_GEOMETRY override_geometry;
	char* path;
	uint8_t* buffer;
	size_t buffer_size;
} XEBEC_HDD;

extern const XEBEC_HDD_GEOMETRY xebec_hdd_geometry[];
extern const uint32_t xebec_hdd_geometry_count;

int xebec_hdd_insert(XEBEC_HDD* hdd, const char* file);
int xebec_hdd_reinsert(XEBEC_HDD* hdd);

void xebec_hdd_eject(XEBEC_HDD* hdd);

int xebec_hdd_save(XEBEC_HDD* hdd);
int xebec_hdd_save_as(XEBEC_HDD* hdd, const char* filename);

int xebec_hdd_new(XEBEC_HDD* hdd, CHS geometry, XEBEC_FILE_TYPE file_type);

void xebec_hdd_set_geometry_override(XEBEC_HDD* hdd, CHS geometry, XEBEC_HDD_TYPE type);
int xebec_hdd_set_geometry(XEBEC_HDD* hdd, CHS geometry);

uint8_t xebec_hdd_read_byte(XEBEC_HDD* hdd, size_t offset);
void xebec_hdd_write_byte(XEBEC_HDD* hdd, size_t offset, uint8_t value);

#endif
