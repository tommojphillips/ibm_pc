/* upd765.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * NEC uPD765
 */

#ifndef UPD765_H
#define UPD765_H

#include <stdint.h>

#include "lba.h"

#define UPD765_MSR_FDD0_BUSY 0x01 /* D0B - FDD0 is in seek mode */
#define UPD765_MSR_FDD1_BUSY 0x02 /* D1B - FDD1 is in seek mode */
#define UPD765_MSR_FDD2_BUSY 0x04 /* D2B - FDD2 is in seek mode */
#define UPD765_MSR_FDD3_BUSY 0x08 /* D3B - FDD3 is in seek mode */
#define UPD765_MSR_FDC_BUSY  0x10 /* CB  - FDC Busy; A command is in progress; FDC will not accept new commands */
#define UPD765_MSR_EXM       0x20 /* EXM - This bit is only set during execution phase in non-DMA mode */
#define UPD765_MSR_DIO       0x40 /* DIO - direction of data. If set, FDC -> CPU. If clear, CPU -> FDC */
#define UPD765_MSR_RQM       0x80 /* RQM - Request for master */

/* NEC uPD765 */
typedef struct UPD765 {
	uint8_t msr;          // MSR

	uint8_t st0;          // ST0
	uint8_t st1;          // ST1
	uint8_t st2;          // ST2
	uint8_t st3;          // ST3

	CHS chs; // C, H, S(R)
	uint8_t sector_bytes; // N

	int phase; /* Phase; Command, Execute, Result */
} UPD765;

void upd765_reset(UPD765* upd765);
uint8_t upd765_read_io_byte(UPD765* upd765, uint8_t address);
void upd765_write_io_byte(UPD765* upd765, uint8_t address, uint8_t value);

#endif
