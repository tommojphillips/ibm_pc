/* fdc.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Floppy Disk Controller
 */

#ifndef FDC_H
#define FDC_H

#include <stdint.h>
#include <stdio.h>

#include "upd765.h"

#define FDC_FIFO_SIZE 1024

#define FDD_MAX   4

typedef struct FDD_DISK_DESCRIPTOR {
	uint8_t cpt;   /* Cylinders per track */
	uint8_t spt;   /* Sectors per track */
	uint8_t sides; /* Sides per disk */
} FDD_DISK_DESCRIPTOR;

typedef struct FDD_DISK_STATUS {
	uint8_t inserted; /* Has the floppy disk been inserted? */
	uint8_t motor_on;
	uint8_t seeking;
} FDD_DISK_STATUS;

typedef struct FDD_FILE_DESCRIPTOR {
	uint32_t size;  /* Disk size */
	FILE* file; /* Disk file */
} FDD_FILE_DESCRIPTOR;

typedef struct FDD_DISK {
	FDD_FILE_DESCRIPTOR file_descriptor;
	FDD_DISK_DESCRIPTOR disk_descriptor;
	FDD_DISK_STATUS status;
} FDD_DISK;

typedef struct FDC {
	UPD765 upd765; /* NEC uPD765 */
	uint8_t dir; /* digital_input_register*/
	uint8_t dor; /* digital_output_register */
	uint8_t dor_enable;
	uint8_t dor_dma;
	uint8_t dio;
	
	uint8_t fdd_select;
	FDD_DISK fdd[FDD_MAX];

	void(*do_irq)(void* fdc);
	uint8_t operation;
} FDC;

void fdc_reset(FDC* fdc);
uint8_t fdc_read_io_byte(FDC* fdc, const uint8_t address);
void fdc_write_io_byte(FDC* fdc, const uint8_t address, const uint8_t value);

#define FDC_INSERT_DISK_ERROR_DRIVE_LETTER 1
#define FDC_INSERT_DISK_ERROR_FILE         2
#define FDC_INSERT_DISK_ERROR_UNK_FLOPPY   3

/* returns FDC_INSERT_DISK_ERROR_ */
int fdc_insert_disk(FDC* fdc, const uint8_t disk_select, const char* file);
int fdc_remove_disk(FDC* fdc, const uint8_t disk_select);
void fdc_remove_disks(FDC* fdc);

#endif
