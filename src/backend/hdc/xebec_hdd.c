/* xebec_hdd.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Hard Drive Disk
 */

#include <stdint.h>
#include <malloc.h>
#include <string.h>

#include "xebec_hdd.h"

#include "frontend/utility/file.h"
#include "backend/utility/lba.h"
#include "backend/utility/vhd.h"

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

const XEBEC_HDD_GEOMETRY xebec_hdd_geometry[] = {
	{ .chs = { .c = 0,   .h = 0, .s = 0  }, XEBEC_HDD_TYPE_NONE, "None"                    },
	{ .chs = { .c = 306, .h = 4, .s = 17 }, XEBEC_HDD_TYPE_1,    "10MB Type 1  (306 4 17)" }, /* 10MB Type 1 */
	{ .chs = { .c = 612, .h = 4, .s = 17 }, XEBEC_HDD_TYPE_16,   "20MB Type 16 (612 4 17)" }, /* 20MB Type 16 */
	{ .chs = { .c = 615, .h = 4, .s = 17 }, XEBEC_HDD_TYPE_2,    "20MB Type 2  (615 4 17)" }, /* 20MB Type 2 */
	{ .chs = { .c = 306, .h = 8, .s = 17 }, XEBEC_HDD_TYPE_13,   "20MB Type 13 (306 8 17)" }, /* 20MB Type 13 */
};

const uint32_t xebec_hdd_geometry_count = sizeof(xebec_hdd_geometry) / sizeof(XEBEC_HDD_GEOMETRY);

#define HDD_NAME_SIZE 256

int xebec_hdd_set_geometry(XEBEC_HDD* hdd, CHS geometry) {
	int match_count = 0;
	for (uint32_t i = 1; i < xebec_hdd_geometry_count; ++i) {

		switch (hdd->file_type) {
			case XEBEC_FILE_TYPE_VHD:
				if (xebec_hdd_geometry[i].chs.c == geometry.c &&
					xebec_hdd_geometry[i].chs.h == geometry.h &&
					xebec_hdd_geometry[i].chs.s == geometry.s) {
					hdd->geometry = &xebec_hdd_geometry[i];
					dbg_print("[XEBEC] HDD File Type=VHD, C=%d, H=%d, S=%d, %s\n", hdd->geometry->chs.c, hdd->geometry->chs.h, hdd->geometry->chs.s, hdd->geometry->name);
					return 0;
				}
				break;
			case XEBEC_FILE_TYPE_RAW:
				if (chs_get_total_byte_count(hdd->override_geometry.chs, 512) != 0) {
					if (xebec_hdd_geometry[i].chs.c == hdd->override_geometry.chs.c &&
						xebec_hdd_geometry[i].chs.h == hdd->override_geometry.chs.h &&
						xebec_hdd_geometry[i].chs.s == hdd->override_geometry.chs.s) {
						hdd->geometry = &xebec_hdd_geometry[i];
						hdd->override_geometry.type = xebec_hdd_geometry[i].type;
						dbg_print("[XEBEC] HDD File Type=RAW, C=%d, H=%d, S=%d, %s A\n", hdd->geometry->chs.c, hdd->geometry->chs.h, hdd->geometry->chs.s, hdd->geometry->name);
						return 0;
					}
				}
				else if (hdd->override_geometry.type != XEBEC_HDD_TYPE_NONE) {
					if (xebec_hdd_geometry[i].type == hdd->override_geometry.type) {
						hdd->geometry = &xebec_hdd_geometry[i];
						dbg_print("[XEBEC] HDD File Type=RAW, C=%d, H=%d, S=%d, %s B\n", hdd->geometry->chs.c, hdd->geometry->chs.h, hdd->geometry->chs.s, hdd->geometry->name);
						return 0;
					}
				}
				else if (chs_get_total_byte_count(xebec_hdd_geometry[i].chs, 512) == hdd->file_size) {
					hdd->geometry = &xebec_hdd_geometry[i];
					hdd->override_geometry.type = xebec_hdd_geometry[i].type;
					dbg_print("[XEBEC] HDD File Type=RAW, C=%d, H=%d, S=%d, %s C\n", hdd->geometry->chs.c, hdd->geometry->chs.h, hdd->geometry->chs.s, hdd->geometry->name);
					match_count++;
				}
				break;
			default:
				dbg_print("[XEBEC] Unknown HDD File Type, cannot determine geometry\n");
				break;
		}
	}
	
	if (match_count == 1) {
		return 0;
	}
	else if (match_count > 1) {
		dbg_print("[XEBEC] Ambiguous HDD RAW size, cannot determine geometry\n");
	}
	else {
		dbg_print("[XEBEC] Unknown HDD geometry: C=%d, H=%d, S=%d\n", geometry.c, geometry.h, geometry.s);
	}

	hdd->geometry = &xebec_hdd_geometry[0];
	return 1;
}

void xebec_hdd_set_geometry_override(XEBEC_HDD* hdd, CHS geometry, XEBEC_HDD_TYPE type) {
	hdd->override_geometry.chs.c = geometry.c;
	hdd->override_geometry.chs.h = geometry.h;
	hdd->override_geometry.chs.s = geometry.s;
	hdd->override_geometry.type = type;
}

static void set_file_type(XEBEC_HDD* hdd, XEBEC_FILE_TYPE type) {
	switch (type) {
		case XEBEC_FILE_TYPE_VHD:
			hdd->file_size = vhd_get_file_size(hdd->buffer, hdd->buffer_size);
			hdd->file_type = type;
			break;
		case XEBEC_FILE_TYPE_RAW:
			hdd->file_size = hdd->buffer_size;
			hdd->file_type = type;
			break;
		default:
			hdd->file_size = 0;
			hdd->file_type = XEBEC_FILE_TYPE_NONE;
			break;
	}
}
static XEBEC_FILE_TYPE get_file_type(const char* path) {
	const char* ext = file_get_extension(path);
	if (ext != NULL) {
		if (strcmp(ext, "VHD") == 0 || strcmp(ext, "vhd") == 0) {
			return (XEBEC_FILE_TYPE_VHD);
		}
		else if (strcmp(ext, "RAW") == 0 || strcmp(ext, "raw") == 0 || strcmp(ext, "IMG") == 0 || strcmp(ext, "img") == 0) {
			return (XEBEC_FILE_TYPE_RAW);
		}
	}
	return (XEBEC_FILE_TYPE_NONE);
}

static int hdd_create_raw(CHS geometry, uint8_t** buffer, size_t* buffer_size) {
	size_t size = chs_get_total_byte_count(geometry, 512);
	*buffer = calloc(1, size);
	if (*buffer == NULL) {
		return 1;
	}
	*buffer_size = size;
	return 0;
}

static void reset_hdd_keep_path_and_overrides(XEBEC_HDD* hdd) {
	if (hdd != NULL) {
		if (hdd->buffer != NULL) {
			free(hdd->buffer);
			hdd->buffer = NULL;
		}
		hdd->buffer_size = 0;
		hdd->inserted = 0;
		hdd->dirty = 0;
		hdd->geometry = &xebec_hdd_geometry[0];
	}
}
static void reset_hdd(XEBEC_HDD* hdd) {
	if (hdd != NULL) {
		reset_hdd_keep_path_and_overrides(hdd);
		hdd->path[0] = '\0';
		chs_reset(&hdd->override_geometry.chs);
		hdd->override_geometry.type = XEBEC_HDD_TYPE_NONE;
		set_file_type(hdd, XEBEC_FILE_TYPE_NONE);
	}
}
static int insert_hdd(XEBEC_HDD* hdd, CHS geometry) {

	int result = xebec_hdd_set_geometry(hdd, geometry);
	if (result != 0) {
		return result;
	}

	hdd->inserted = 1;
	hdd->dirty = 0;

	return 0;
}

int xebec_hdd_insert(XEBEC_HDD* hdd, const char* filename) {
	if (hdd->inserted) {
		return 1;
	}

	if (filename == NULL) {
		if (hdd->path == NULL || hdd->path[0] == '\0') {
			dbg_print("[XEBEC] Invalid filename on insert\n");
			return 1;
		}

		if (file_read_alloc_buffer(hdd->path, &hdd->buffer, &hdd->buffer_size)) {
			return 1;
		}
	}
	else {
		if (file_read_alloc_buffer(filename, &hdd->buffer, &hdd->buffer_size)) {
			return 1;
		}
		strncpy_s(hdd->path, HDD_NAME_SIZE, filename, HDD_NAME_SIZE - 1);
	}

	CHS vhd_geometry = { 0 };

	XEBEC_FILE_TYPE type = get_file_type(hdd->path);

	switch (type) {
		case XEBEC_FILE_TYPE_VHD:
			if (vhd_verify(hdd->buffer, hdd->buffer_size)) {
				dbg_print("[XEBEC] Invalid VHD\n");
				return 1;
			}
			chs_set(&vhd_geometry, vhd_get_geometry(hdd->buffer, hdd->buffer_size));
			break;
		case XEBEC_FILE_TYPE_RAW:
			/* geometry is set by the file size or overrides */
			break;
		default:
			dbg_print("[XEBEC] Unknown file type\n");
			return 1;
	}
	
	set_file_type(hdd, type);

	int result = insert_hdd(hdd, vhd_geometry);
	if (result != 0) {
		reset_hdd(hdd);
		return result;
	}

	return 0;
}
int xebec_hdd_reinsert(XEBEC_HDD* hdd) {
	if (!hdd->inserted) {
		return 1;
	}

	reset_hdd_keep_path_and_overrides(hdd);
	if (xebec_hdd_insert(hdd, NULL)) {
		return 1;
	}

	return 0;
}

void xebec_hdd_eject(XEBEC_HDD* hdd) {
	if (hdd->inserted) {
		reset_hdd(hdd);
	}
}

int xebec_hdd_save(XEBEC_HDD* hdd) {
	if (!hdd->inserted) {
		return 1;
	}

	if (file_write_from_buffer(hdd->path, hdd->buffer, hdd->buffer_size)) {
		return 1;
	}

	hdd->dirty = 0;
	return 0;
}
int xebec_hdd_save_as(XEBEC_HDD* hdd, const char* filename) {
	if (!hdd->inserted) {
		return 1;
	}

	strncpy_s(hdd->path, HDD_NAME_SIZE, filename, HDD_NAME_SIZE - 1);
	return xebec_hdd_save(hdd);
}

int xebec_hdd_new(XEBEC_HDD* hdd, CHS geometry, XEBEC_FILE_TYPE file_type) {
	if (hdd->inserted) {
		return 1;
	}

	switch (file_type) {
		case XEBEC_FILE_TYPE_VHD:
			if (vhd_create(geometry, &hdd->buffer, &hdd->buffer_size)) {
				dbg_print("[XEBEC] Error: could not create vhd for new hdd\n");
				return 1;
			}
			sprintf(hdd->path, "hdd_%zuMB.vhd", vhd_get_file_size(hdd->buffer, hdd->buffer_size) / 1024 / 1024);
			break;

		case XEBEC_FILE_TYPE_RAW:
			if (hdd_create_raw(geometry, &hdd->buffer, &hdd->buffer_size)) {
				dbg_print("[XEBEC] Error: could not create raw for new hdd\n");
				return 1;
			}
			sprintf(hdd->path, "hdd_%zuMB.img", hdd->buffer_size / 1024 / 1024);
			break;

		default:
			dbg_print("[XEBEC] Error: unknown file type\n");
			break;
	}

	set_file_type(hdd, file_type);

	int result = insert_hdd(hdd, geometry);
	if (result != 0) {
		reset_hdd(hdd);
		return result;
	}

	hdd->dirty = 1;

	return 0;
}

uint8_t xebec_hdd_read_byte(XEBEC_HDD* hdd, size_t offset) {
	if (!hdd->inserted) {
		return 0xFF;
	}
	if (offset >= hdd->file_size) {
		dbg_print("[XEBEC] Error: Out of bounds read. offset = %zx\n", offset);
		return 0xFF;
	}

	return hdd->buffer[offset];
}
void xebec_hdd_write_byte(XEBEC_HDD* hdd, size_t offset, uint8_t value) {
	if (!hdd->inserted) {
		return;
	}
	if (offset >= hdd->file_size) {
		dbg_print("[XEBEC] Error: Out of bounds write. offset = %zx\n", offset);
		return;
	}

	hdd->dirty = 1;
	hdd->buffer[offset] = value;
}
