/* upd765.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Intel 8272 (NEC uPD765)
 */

#include <stdint.h>

#include "upd765.h"

/* The DIO and RQM bits in the MSR indicate when Data is ready and-in which
 direction data will be transferred on the Data Bus. */

/* COMMAND SEQUENCE 
 The uPD765 is capable of performing 15 different commands. Each command is 
 initiated by a multi-byte transfer from the CPU, and the result after execution
 of the command may also be a multi-byte transfer back to the CPU. Because of
 this multi-byte interchange of information between the uPD765 and the CPU, it
 is convenient to consider each command as consisting of three phases:

 Command Phase:   Receive all information required to perform an operation.
 Execution Phase: Perform the operation.
 Result Phase:    Status and other housekeeping information are made available. */

#define UPD765_PHASE_COMMAND 0
#define UPD765_PHASE_EXECUTE 1
#define UPD765_PHASE_RESULT  2

#define UPD765_MSR_ADDR  0
#define UPD765_DATA_ADDR 1

static void upd765_write_data(UPD765* upd765, uint8_t value) {

}

static uint8_t upd765_read_data(UPD765* upd765) {
	return 0;
}

static uint8_t upd765_read_msr(UPD765* upd765) {
	return upd765->msr;
}

void upd765_reset(UPD765* upd765) {
	upd765->msr = 0;

	upd765->st0 = 0;
	upd765->st1 = 0;
	upd765->st2 = 0;
	upd765->st3 = 0;
	
	upd765->chs.cylinder = 0;
	upd765->chs.head = 0;
	upd765->chs.sector = 0;
	upd765->sector_bytes = 0;
	
	upd765->phase = UPD765_PHASE_COMMAND;
}

uint8_t upd765_read_io_byte(UPD765* upd765, uint8_t address) {
	switch (address) {

		case UPD765_MSR_ADDR: // (RO)
			return upd765_read_msr(upd765);

		case UPD765_DATA_ADDR: // (R/W)
			return upd765_read_data(upd765);
	}
	return 0;
}
void upd765_write_io_byte(UPD765* upd765, uint8_t address, uint8_t value) {
	switch (address) {
	
		case UPD765_DATA_ADDR: // (R/W)
			upd765_write_data(upd765, value);
			break;
	}
}
