/* xebec.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Hard Disk Controller
 */

#ifndef XEBEC_H
#define XEBEC_H

#include <stdint.h>

#include "backend/utility/lba.h"
#include "backend/utility/ring_buffer.h"

#include "xebec_hdd.h"

typedef struct I8237_DMA I8237_DMA;
typedef struct I8259_PIC I8259_PIC;

#define HDD_MAX 2

typedef struct XEBEC_DCB {
	CHS chs;
	uint8_t drive_select;
	union {
		uint8_t block_count;
		uint8_t interleave;
	} u;
	uint8_t disable_retry;
	uint8_t step;
} XEBEC_DCB;

typedef struct XEBEC_HDC_COMMAND {
	/* Command State */
	uint8_t byte;
	uint8_t param_count;
	uint8_t state;
} XEBEC_HDC_COMMAND;

typedef struct XEBEC_HDC {
	uint8_t hdd_select;
	uint8_t status_byte;
	uint8_t status_register;
	uint8_t error;
	uint8_t int_enabled;
	uint8_t dma_enabled; 
	uint8_t dipswitch;
	XEBEC_HDC_COMMAND command;

	uint32_t byte_index;
	uint32_t sector_index;
	uint32_t sector_count;

	/* TODO: condense in/out into single fifo. */
	RING_BUFFER data_register_out;
	RING_BUFFER data_register_in;

	XEBEC_HDD hdd[HDD_MAX];

	I8237_DMA* dma_p; /* DMA pointer */
	I8259_PIC* pic_p; /* PIC pointer */

	uint64_t accum;
} XEBEC_HDC;

int xebec_hdc_create(XEBEC_HDC* hdc);
void xebec_hdc_destroy(XEBEC_HDC* hdc);
void xebec_hdc_init(XEBEC_HDC* hdc, I8237_DMA* dma, I8259_PIC* pic);

void xebec_hdc_reset(XEBEC_HDC* hdc);
uint8_t xebec_hdc_read_io_byte(XEBEC_HDC* hdc, uint8_t address);
void xebec_hdc_write_io_byte(XEBEC_HDC* hdc, uint8_t address, uint8_t value);
void xebec_hdc_update(XEBEC_HDC* hdc);

int xebec_hdc_insert_hdd(XEBEC_HDC* hdc, int hdd, const char* path);
void xebec_hdc_eject_hdd(XEBEC_HDC* hdc, int hdd);
int xebec_hdc_reinsert_hdd(XEBEC_HDC* hdc, int hdd);
void xebec_hdc_save_hdd(XEBEC_HDC* hdc, int hdd);
void xebec_hdc_save_as_hdd(XEBEC_HDC* hdc, int hdd, const char* filename);
int xebec_hdc_new_hdd(XEBEC_HDC* hdc, int hdd, CHS geometry, XEBEC_FILE_TYPE file_type);
void xebec_hdc_set_geometry_override_hdd(XEBEC_HDC* hdc, int hdd, CHS geometry, XEBEC_HDD_TYPE type);
int xebec_hdc_set_geometry_hdd(XEBEC_HDC* hdc, int hdd, CHS geometry);
void xebec_hdc_set_dipswitch(XEBEC_HDC* hdc, int hdd, uint8_t type);

#endif
