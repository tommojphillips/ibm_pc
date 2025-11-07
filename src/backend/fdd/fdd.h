/* fdd.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Floppy Disk Drive
 */

#ifndef FDD_H
#define FDD_H

#include <stdint.h>

#include "lba.h"

#define FDD_INSERT_DISK_OK                 0
#define FDD_INSERT_DISK_ERROR_DRIVE_LETTER 1
#define FDD_INSERT_DISK_ERROR_FILE         2
#define FDD_INSERT_DISK_ERROR_UNK_FLOPPY   3
#define FDD_INSERT_DISK_ERROR_IN_USE       4

typedef struct DISK_GEOMETRY {
	uint32_t size;
	CHS geometry;
} DISK_GEOMETRY;

extern const DISK_GEOMETRY disk_geometry[];
extern const uint32_t disk_geometry_count;

typedef CHS FDD_DISK_DESCRIPTOR;

typedef struct FDD_DISK_STATUS {
	uint8_t inserted;
	uint8_t ready;         /* fdd ready signal */
	uint8_t motor_on; 
	uint8_t write_protect; /* fdd write protect signal */
	uint8_t dirty;         /* fdd modified signal */
} FDD_DISK_STATUS;

typedef struct FDD_DISK {
	FDD_DISK_DESCRIPTOR disk_descriptor;
	FDD_DISK_STATUS status;
	uint8_t* buffer;
	size_t buffer_size;
	char* path;
} FDD_DISK;

int char_to_drive(char ch, uint8_t* disk);

int fdd_insert_disk(FDD_DISK* fdd, const char* file);
void fdd_eject_disk(FDD_DISK* fdd);

void fdd_save_disk(FDD_DISK* fdd);
void fdd_save_as_disk(FDD_DISK* fdd, const char* filename);

void fdd_write_protect(FDD_DISK* fdd, uint8_t write_protect);

uint8_t fdd_read_byte(FDD_DISK* fdd, size_t offset);
void fdd_write_byte(FDD_DISK* fdd, size_t offset, uint8_t value);

int fdd_new_disk(FDD_DISK* fdd, uint32_t buffer_size);

#endif
