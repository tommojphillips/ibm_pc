/* fdc.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Floppy Disk Controller
 */

#include "fdc.h"

#include "../bit_utils.h"

/* I/O Port Addresses */

#define PORT_STATUS_REGISTER 4 // RO
#define PORT_DATA_REGISTER   5 // R/W

#define PORT_TAPE_DRIVE      3 // R/W

#define PORT_STATUS_A        0 // RO
#define PORT_STATUS_B        1 // RO
#define PORT_DATA_FIFO       5 // RO
#define PORT_DIGITAL_INPUT   7 // RO

#define PORT_DIGITAL_OUTPUT  2 // WO - (DOR) Used to control drive motors, drive selection, reset fdc, and feature enable
#define PORT_DATARATE_SELECT 4 // WO
#define PORT_DATA_COMMAND    5 // WO
#define PORT_CONFIG_CONTROL  7 // WO

#define CMD_READ_TRACK         2 // generates IRQ6
#define CMD_SPECIFY            3 // set drive parameters
#define CMD_SENSE_DRIVE_STATUS 4
#define CMD_WRITE_DATA         5 // write to the disk
#define CMD_READ_DATA          6 // read from the disk
#define CMD_RECALIBRATE        7 // seek to cylinder 0
#define CMD_SENSE_INTERRUPT    8 // ack IRQ6, get status of last command
#define CMD_WRITE_DELETED_DATA 9
#define CMD_READ_ID            10 // generates IRQ6
#define CMD_READ_DELETED_DATA  12
#define CMD_FORMAT_TRACK       13
#define CMD_DUMPREG            14
#define CMD_SEEK               15 // seek both heads to cylinder X
#define CMD_VERSION            16 // used during initialization, once
#define CMD_SCAN_EQUAL         17
#define CMD_PERPENDICULAR_MODE 18 // used during initialization, once, maybe
#define CMD_CONFIGURE          19 // set controller parameters
#define CMD_LOCK               20 // protect controller params from a reset
#define CMD_VERIFY             22
#define CMD_SCAN_LOW_OR_EQUAL  25
#define CMD_SCAN_HIGH_OR_EQUAL 29

/* These bits are decoded by the hardware to select
 one drive if its motor is on */
#define DOR_FDD_SELECT_MASK 0x03
#define DOR_SELECT_FDD0     0x00 // Select FDD A if the motor is on
#define DOR_SELECT_FDD1     0x01 // Select FDD B if the motor is on
#define DOR_SELECT_FDD2     0x02 // Select FDD C if the motor is on
#define DOR_SELECT_FDD3     0x03 // Select FDD D if the motor is on

#define FDC_DESELECT_FDD 0xFF
#define FDC_SELECT_FDD0  DOR_SELECT_FDD0
#define FDC_SELECT_FDD1  DOR_SELECT_FDD1
#define FDC_SELECT_FDD2  DOR_SELECT_FDD2
#define FDC_SELECT_FDD3  DOR_SELECT_FDD3

/* The FDC is held reset when this bit is clear. It
 must be set by the program to enable the FDC */
#define DOR_FDC_RESET_MASK    0x04

/* This bit allows the FDC interrupt and DMA requests to be gated onto 
 the I/O interface. If this bit is cleared, the interrupt and DMA 
 request I/O interface drivers are disabled. */
#define DOR_DMA_INT_MASK      0x08

/* These bits control the motors of drives 0,1,2,3.
 If a bit is clear, the motor is off, and the drive cannot be selected. */
#define DOR_FDD_MOTOR_ON_MASK   0x0F
#define DOR_FDD_MOTOR_ON_BASE   0x10 // Turn FDD0 Motor on/off
#define DOR_FDD_MOTOR_ON_A      0x10 // Turn FDD0 Motor on/off
#define DOR_FDD_MOTOR_ON_B      0x20 // Turn FDD1 Motor on/off
#define DOR_FDD_MOTOR_ON_C      0x40 // Turn FDD2 Motor on/off
#define DOR_FDD_MOTOR_ON_D      0x80 // Turn FDD3 Motor on/off

/* DIO - Indicates direction of data transfer between the FDC and CPU.
 If DIO = 1, then transfer is to the CPU.
 If DIO = 0, then transfer is from the CPU. */
#define MSR_DIO               0x40

#define IO_MODE_TO_CPU        0
#define IO_MODE_FROM_CPU      1

#define DATA_MODE_PIO         0
#define DATA_MODE_DMA         1

/* MRQ - Indicates Data Register is Master ready to send or receive data to or
 from the Processor. Both bits DIO and RQM should be used to perform the 
 handshaking functions of "ready" and "direction" to the processor */
#define MSR_MRQ 0x80

#define DOR_ENABLE       0x04

#define DBG_PRINT
#ifdef DBG_PRINT
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

void fdc_operation_reset(FDC* fdc);
void fdc_operation_calibrate(FDC* fdc);
void fdc_operation_seek(FDC* fdc);
void fdc_operation_read_data(FDC* fdc);
void fdc_operation_read_track(FDC* fdc);
void fdc_operation_write_data(FDC* fdc);
void fdc_operation_format_track(FDC* fdc);

static void fdc_command(FDC* fdc, uint8_t command) {
	
	switch (command) {
		case CMD_READ_TRACK:
			dbg_print("[FDC] CMD read_track\n");
			break;
		case CMD_SPECIFY:
			dbg_print("[FDC] CMD specify\n");
			break;
		case CMD_SENSE_DRIVE_STATUS:
			dbg_print("[FDC] CMD drive_status\n");
			break;
		case CMD_WRITE_DATA:
			dbg_print("[FDC] CMD write_data\n");
			break;
		case CMD_READ_DATA:
			dbg_print("[FDC] CMD read_data\n");
			break;
		case CMD_RECALIBRATE:
			dbg_print("[FDC] CMD recalibrate\n");
			break;
		case CMD_SENSE_INTERRUPT:
			dbg_print("[FDC] CMD sense_interrupt\n");
			break;
		case CMD_WRITE_DELETED_DATA:
			dbg_print("[FDC] CMD write_deleted_data\n");
			break;
		case CMD_READ_ID:
			dbg_print("[FDC] CMD read_id\n");
			break;
		case CMD_READ_DELETED_DATA:
			dbg_print("[FDC] CMD read_deleted_data\n");
			break;
		case CMD_FORMAT_TRACK:
			dbg_print("[FDC] CMD format_track\n");
			break;
		case CMD_DUMPREG:
			dbg_print("[FDC] CMD dump_reg\n");
			break;
		case CMD_SEEK:
			dbg_print("[FDC] CMD seek\n");
			break;
		case CMD_VERSION:
			dbg_print("[FDC] CMD version\n");
			break;
		case CMD_SCAN_EQUAL:
			dbg_print("[FDC] CMD scan equal\n");
			break;
		case CMD_PERPENDICULAR_MODE:
			dbg_print("[FDC] CMD perpendicular_mode\n");
			break;
		case CMD_CONFIGURE:
			dbg_print("[FDC] CMD configure\n");
			break;
		case CMD_LOCK:
			dbg_print("[FDC] CMD lock\n");
			break;
		case CMD_VERIFY:
			dbg_print("[FDC] CMD verfiy\n");
			break;
		case CMD_SCAN_LOW_OR_EQUAL:
			dbg_print("[FDC] CMD scan_le\n");
			break;
		case CMD_SCAN_HIGH_OR_EQUAL:
			dbg_print("[FDC] CMD scan_he\n");
			break;
	}
}

static void fdc_write_dor(FDC* fdc, uint8_t v) {
	/* The Digital Output Register (DOR) is an output only register used to
	 control drive motors, drive selection, and feature enable. All bits are
	 cleared by the I/O interface reset line */

	/* set fdc enable */
	fdc->dor_enable = (v & DOR_ENABLE) >> 2;

	/* This bit allows the FDC interrupt and DMA requests to be gated onto the
		I/O interface. If this bit is cleared, the interrupt and DMA request I/O
		interface drivers are disabled */
	fdc->dor_dma = (v & DOR_DMA_INT_MASK) >> 3;

	/* The FDC is held reset when bit 2 is clear. It
		must be set by the program to enable the FDC */
	if (IS_RISING_EDGE(DOR_ENABLE, fdc->dor, v)) {
		fdc_operation_reset(fdc);
	}

	/* turn on drive motors */
	for (int i = 0; i < FDD_MAX; ++i) {
		fdc->fdd[i].status.motor_on = (v >> (4 + i)) & 0x1;
	}

	/* select drive */
	fdc->fdd_select = (v & DOR_FDD_SELECT_MASK);

	/* check selected drive motor is on. */
	if (!fdc->fdd[fdc->fdd_select].status.motor_on) {
		fdc->fdd_select = FDC_DESELECT_FDD;
	}
	
	fdc->dor = v;
}

static int fdc_set_disk_descriptor(FDD_DISK_DESCRIPTOR* disk, uint32_t size) {

	switch (size) {
	
		case 163840U:
			disk->cpt   = 40;
			disk->spt   = 8;
			disk->sides = 1;
			break;
		case 184320U:
			disk->cpt   = 40;
			disk->spt   = 9;
			disk->sides = 1;
			break;
		case 368640U:
			disk->cpt = 40;
			disk->spt = 9;
			disk->sides = 2;
			break;
		case 737280U:
			disk->cpt = 80;
			disk->spt = 9;
			disk->sides = 2;
			break;
		case 1228800U:
			disk->cpt = 80;
			disk->spt = 15;
			disk->sides = 2;
			break;

		default:
			dbg_print("[FDC] Unknown floppy disk %u KB\n", size >> 10);
			return FDC_INSERT_DISK_ERROR_UNK_FLOPPY;
	}

	return 0;
}

int fdc_insert_disk(FDC* fdc, uint8_t num, const char* file) {
	
	if (num > FDD_MAX-1) {
		return FDC_INSERT_DISK_ERROR_DRIVE_LETTER;
	}

	fdc_remove_disk(fdc, num);

	fopen_s(&fdc->fdd[num].file_descriptor.file, file, "rb");
	if (fdc->fdd[num].file_descriptor.file == NULL) {
		dbg_print("[FDC] error opening disk %s for drive %c\n", file, 'A' + num);
		return FDC_INSERT_DISK_ERROR_FILE;
	}

	fseek(fdc->fdd[num].file_descriptor.file, 0, SEEK_END);
	fdc->fdd[num].file_descriptor.size = ftell(fdc->fdd[num].file_descriptor.file);
	fseek(fdc->fdd[num].file_descriptor.file, 0, SEEK_SET);

	fdc_set_disk_descriptor(&fdc->fdd[num].disk_descriptor, fdc->fdd[num].file_descriptor.size);

	fdc->fdd[num].status.inserted = 1;

	dbg_print("[FDC] Inserted floppy disk into %c: %s (%lu KB)\n", 'A' + num, file, fdc->fdd[num].file_descriptor.size >> 10);
	return 0;
}
int fdc_remove_disk(FDC* fdc, uint8_t num) {

	if (num > FDD_MAX-1) {
		return FDC_INSERT_DISK_ERROR_DRIVE_LETTER;
	}

	if (fdc->fdd[num].status.inserted) {
		
		fdc->fdd[num].status.inserted = 0;

		fdc->fdd[num].disk_descriptor.cpt = 0;
		fdc->fdd[num].disk_descriptor.spt = 0;
		fdc->fdd[num].disk_descriptor.sides = 0;

		if (fdc->fdd[num].file_descriptor.file != NULL) {
			fclose(fdc->fdd[num].file_descriptor.file);
			fdc->fdd[num].file_descriptor.file = NULL;
		}
		fdc->fdd[num].file_descriptor.size = 0;

		dbg_print("[FDC] Removed floppy disk from %c:\n", 'A' + num);
	}
	return 0;
}
void fdc_remove_disks(FDC* fdc) {
	for (uint8_t i = 0; i < FDD_MAX; ++i) {
		fdc_remove_disk(fdc, i);
	}
}

void fdc_reset(FDC* fdc) {
	fdc->dir = 0;
	fdc->dor = 0; 
	fdc->fdd_select = FDC_DESELECT_FDD;
	upd765_reset(&fdc->upd765);
}

uint8_t fdc_read_io_byte(FDC* fdc, uint8_t address) {
	switch (address) {

		case PORT_STATUS_REGISTER:
		case PORT_DATA_REGISTER:
			upd765_read_io_byte(&fdc->upd765, (address & 0x1));
			break;
	}
	return 0;
}
void fdc_write_io_byte(FDC* fdc, uint8_t address, uint8_t value) {
	switch (address) {

		case PORT_DIGITAL_OUTPUT: // (WO)
			fdc_write_dor(fdc, value);
			break;

		case PORT_STATUS_REGISTER:
		case PORT_DATA_REGISTER:
			upd765_write_io_byte(&fdc->upd765, (address & 0x1), value);
			break;
	}
}

void fdc_operation_reset(FDC* fdc) {
	upd765_reset(&fdc->upd765);
	fdc->upd765.msr |= UPD765_MSR_DIO; /* IO Mode - From CPU */
	fdc->upd765.msr |= UPD765_MSR_RQM; /* Request for master */
	fdc->fdd_select = FDC_DESELECT_FDD;
	fdc->do_irq(fdc);
	dbg_print("[FDC] FDC reset\n");
}
void fdc_operation_calibrate(FDC* fdc) {

}
void fdc_operation_seek(FDC* fdc) {

}
void fdc_operation_read_data(FDC* fdc) {

}
void fdc_operation_read_track(FDC* fdc) {

}
void fdc_operation_write_data(FDC* fdc) {

}
void fdc_operation_format_track(FDC* fdc) {

}
