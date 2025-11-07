/* fdd.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Floppy Disk Drive
 */

#include <malloc.h>
#include <string.h>

#include "fdd.h"
#include "frontend/utility/file.h"

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

#define FDD_NAME_SIZE 256

const DISK_GEOMETRY disk_geometry[] = {
	{ .size = ( 160 * 1024), .geometry = { .c = 40, .h = 1, .s =  8 } }, /* 160KB */
	{ .size = ( 180 * 1024), .geometry = { .c = 40, .h = 1, .s =  9 } }, /* 180KB */
	{ .size = ( 320 * 1024), .geometry = { .c = 40, .h = 2, .s =  8 } }, /* 320KB */
	{ .size = ( 360 * 1024), .geometry = { .c = 40, .h = 2, .s =  9 } }, /* 360KB */
	{ .size = ( 720 * 1024), .geometry = { .c = 80, .h = 2, .s =  9 } }, /* 720KB */
	{ .size = (1200 * 1024), .geometry = { .c = 80, .h = 2, .s = 15 } }, /* 1.2MB */
	{ .size = (1440 * 1024), .geometry = { .c = 80, .h = 2, .s = 18 } }, /* 1.44MB */
	{ .size = (2880 * 1024), .geometry = { .c = 80, .h = 2, .s = 36 } }, /* 2.88MB */
};
const uint32_t disk_geometry_count = sizeof(disk_geometry) / sizeof(DISK_GEOMETRY);

static int set_disk_descriptor(FDD_DISK_DESCRIPTOR* disk, size_t size) {

	for (uint32_t i = 0; i < disk_geometry_count; ++i) {
		if (disk_geometry[i].size == size) {
			disk->c = disk_geometry[i].geometry.c;
			disk->h = disk_geometry[i].geometry.h;
			disk->s = disk_geometry[i].geometry.s;
			return FDD_INSERT_DISK_OK;
		}
	}

	disk->c = 0;
	disk->h = 0;
	disk->s = 0;
	dbg_print("[FDC] Unknown floppy disk %zu KB\n", size >> 10);
	return FDD_INSERT_DISK_ERROR_UNK_FLOPPY;
}

int char_to_drive(char ch, uint8_t* disk) {
	/* Convert A-Z, a-z to disk number */
	if (ch >= 0 && ch <= 26) {
		*disk = ch;
	}
	else if ((ch >= 'A' && ch <= 'Z')) {
		*disk = ch - 'A';
	}
	else if (ch >= 'a' && ch <= 'z') {
		*disk = ch - 'a';
	}
	else if (ch >= '0' && ch <= '9') {
		*disk = ch - '0';
	}
	else {
		*disk = 0;
		return 1;
	}

	return 0;
}

int fdd_new_disk(FDD_DISK* fdd, uint32_t buffer_size) {

	if (fdd->status.inserted) {
		return FDD_INSERT_DISK_ERROR_IN_USE;
	}

	int result = set_disk_descriptor(&fdd->disk_descriptor, buffer_size);
	if (result != FDD_INSERT_DISK_OK) {		
		return result;
	}

	fdd->buffer = calloc(1, buffer_size);
	if (fdd->buffer == NULL) {
		dbg_print("Error: could not alloc memory for new disk\n");
		return 1;
	}
	fdd->buffer_size = buffer_size;

	sprintf(fdd->path, "disk_%dKB.img", buffer_size / 1024);
	fdd->status.inserted = 1;
	fdd->status.dirty = 1;

	/* Theres no disk in the drive. So we only assert the ready signal here. if the
	   motors not on then a write to the DOR register will assert the ready signal */
	if (fdd->status.motor_on) {
		fdd->status.ready = 1;
	}

	printf("[FDD] NEW DISK: %s\n", fdd->path);
	return FDD_INSERT_DISK_OK;
}

int fdd_insert_disk(FDD_DISK* fdd, const char* file) {

	if (fdd->status.inserted) {
		return FDD_INSERT_DISK_ERROR_IN_USE;
	}

	if (file_read_alloc_buffer(file, &fdd->buffer, &fdd->buffer_size)) {
		return FDD_INSERT_DISK_ERROR_FILE;
	}
	
	int result = set_disk_descriptor(&fdd->disk_descriptor, fdd->buffer_size);
	if (result != FDD_INSERT_DISK_OK) {
		if (fdd->buffer != NULL) {
			free(fdd->buffer);
			fdd->buffer = NULL;
			fdd->buffer_size = 0;
		}
		return result;
	}
	
	strncpy_s(fdd->path, FDD_NAME_SIZE, file, FDD_NAME_SIZE - 1);
	fdd->status.inserted = 1;
	fdd->status.dirty = 0;

	/* Theres no disk in the drive. So we only assert the ready signal here. if the
	   motors not on then a write to the DOR register will assert the ready signal */
	if (fdd->status.motor_on) {
		fdd->status.ready = 1;
	}

	printf("[FDD] INSERT DISK: %s\n", fdd->path);
	return result;
}
void fdd_eject_disk(FDD_DISK* fdd) {
	if (fdd->status.inserted) {
		printf("[FDD] EJECT DISK: %s\n", fdd->path);

		fdd->status.inserted = 0;
		fdd->status.dirty = 0;

		/* Theres no disk in the drive. so we only deassert the ready signal here. */
		fdd->status.ready = 0;

		fdd->disk_descriptor.c = 0;
		fdd->disk_descriptor.h = 0;
		fdd->disk_descriptor.s = 0;

		fdd->path[0] = '\0';

		if (fdd->buffer != NULL) {
			free(fdd->buffer);
			fdd->buffer = NULL;
		}

		fdd->buffer_size = 0;
	}
}
void fdd_save_disk(FDD_DISK* fdd) {
	if (fdd->status.inserted) {
		if (file_write_from_buffer(fdd->path, fdd->buffer, fdd->buffer_size)) {
			printf("[FDD] FAILED TO SAVE DISK\n");
		}
		else {
			fdd->status.dirty = 0;
			printf("[FDD] SAVE DISK: %s\n", fdd->path);
		}
	}
}
void fdd_save_as_disk(FDD_DISK* fdd, const char* filename) {
	if (fdd->status.inserted) {
		strncpy_s(fdd->path, FDD_NAME_SIZE, filename, FDD_NAME_SIZE - 1);
		fdd_save_disk(fdd);
	}
}

void fdd_write_protect(FDD_DISK* fdd, uint8_t write_protect) {
	fdd->status.write_protect = write_protect;
}

uint8_t fdd_read_byte(FDD_DISK* fdd, size_t offset) {
	if (fdd->status.inserted && offset < fdd->buffer_size) {
		return fdd->buffer[offset];
	}
	return 0xFF;
}

void fdd_write_byte(FDD_DISK* fdd, size_t offset, uint8_t value) {
	if (fdd->status.inserted && offset < fdd->buffer_size) {
		fdd->status.dirty = 1;
		fdd->buffer[offset] = value;
	}
}
