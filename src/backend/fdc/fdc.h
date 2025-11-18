/* fdc.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Floppy Disk Controller NEC uPD765 (Intel i8272)
 */

#ifndef FDC_H
#define FDC_H

#include <stdint.h>

#include "backend/utility/ring_buffer.h"
#include "backend/utility/lba.h"
#include "fdd.h"

#define FDD_MAX 4

typedef struct I8237_DMA I8237_DMA;

#define FDC_ERROR_CRC_ERROR       0x01
#define FDC_ERROR_OVERRUN         0x02
#define FDC_ERROR_END_OF_CYLINDER 0x04
#define FDC_ERROR_WRONG_CYLINDER  0x08
#define FDC_ERROR_BAD_CYLINDER    0x10
#define FDC_ERROR_DELETED_DATA    0x20

typedef struct FDC_COMMAND {
	/* Command State */
	
	uint8_t byte;
	uint8_t param_count;
	uint8_t recieving;
	uint8_t received;
	uint8_t async;
	uint8_t error;

	/* Command Parameters */

	uint8_t dhs;      /* HD/US1/US0 */
	CHS chs;          /* C/H/R */
	uint8_t n;        /* N */
	uint8_t eot;      /* EOT */
	uint8_t gap_len;  /* GPL */
	uint8_t data_len; /* DTL */
} FDC_COMMAND;

typedef struct FDC {
	uint8_t msr;          /* MSR */
	uint8_t dor;          /* digital_output_register */

	uint8_t st0;
	uint8_t st1;
	uint8_t st2;
	uint8_t st3;

	uint8_t fdd_select;
	uint8_t dma;          /* DMA/PIO */
	FDC_COMMAND command;

	uint16_t sector_size;
	uint32_t byte_index;

	RING_BUFFER data_register_out;
	RING_BUFFER data_register_in;

	FDD_DISK fdd[FDD_MAX];

	void(*do_irq)(void* fdc);

	uint8_t(*read_mem_byte)(uint32_t);         /* read mem byte */
	void(*write_mem_byte)(uint32_t, uint8_t);  /* write mem byte */

	I8237_DMA* dma_p;                          /* DMA Controller pointer */
} FDC;

int upd765_fdc_create(FDC* fdc);
void upd765_fdc_destroy(FDC* fdc);

void upd765_fdc_reset(FDC* fdc);
uint8_t upd765_fdc_read_io_byte(FDC* fdc, const uint8_t address);
void upd765_fdc_write_io_byte(FDC* fdc, const uint8_t address, const uint8_t value);

void upd765_fdc_update(FDC* fdc);

#endif
