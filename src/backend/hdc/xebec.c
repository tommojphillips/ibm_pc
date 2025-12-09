/* xebec.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Hard Disk Controller
 */

#include <stdint.h>
#include <malloc.h>
#include <string.h>

#include "xebec.h"

#include "backend/chipset/i8237_dma.h"
#include "backend/chipset/i8259_pic.h"

#include "frontend/utility/file.h"
#include "backend/utility/ring_buffer.h"
#include "backend/utility/lba.h"
#include "backend/utility/vhd.h"

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

 /* I/O Port Addresses */

#define PORT_READ_DATA    0 // RO - Read data from controller to cpu
#define PORT_READ_STATUS  1 // RO - Read controller hardware status
#define PORT_READ_DIP     2 // RO - Read configuration switches

#define PORT_WRITE_DATA   0 // WO - Write data from cpu to controller
#define PORT_RESET        1 // WO - Controller reset
#define PORT_WRITE_SELECT 2 // WO - Generate controller select pulse
#define PORT_WRITE_MASK   3 // WO - Write pattern to DMA and interrupt mask register

#define COMMAND_STATE_IDLE      0 /* Idle; Waiting for command. */
#define COMMAND_STATE_RECIEVING 1 /* Recieved command byte; Waiting for parameter bytes. */
#define COMMAND_STATE_RECIEVED  2 /* Recieved all command bytes; Waiting for comand execution. */
#define COMMAND_STATE_EXECUTING 4 /* Executing command; Setting up command. */
#define COMMAND_STATE_ASYNC     8 /* Executing command; Waiting for command finish. */

 /* Commands */

#define CMD_TEST_DRIVE      0x00
#define CMD_RECALIBRATE     0x01
#define CMD_SENSE           0x03
#define CMD_FORMAT_DRIVE    0x04
#define CMD_CHECK_TRACK     0x05
#define CMD_FORMAT_TRACK    0x06
#define CMD_FORMAT_BAD      0x07
#define CMD_READ            0x08
#define CMD_WRITE           0x0A
#define CMD_SEEK            0x0B
#define CMD_INIT_DRIVE      0x0C /* Initialize Drive Characteristics. The DBC must be followed by 8 additional bytes */
#define CMD_READ_ECC        0x0D
#define CMD_READ_BUFFER     0x0E
#define CMD_WRITE_BUFFER    0x0F
#define CMD_RAM_DIAG        0xE0
#define CMD_DRIVE_DIAG      0xE3
#define CMD_CONTROLLER_DIAG 0xE4
#define CMD_READ_LONG       0xE5 /* Reads 512 bytes + 4 bytes from ECC */
#define CMD_WRITE_LONG      0xE6 /* Writes 512 bytes + 4 bytes to ECC */

#define CONTROL_BYTE_STEP_0 3000 /* MICRO SECONDS */
#define CONTROL_BYTE_STEP_4 200  /* MICRO SECONDS */
#define CONTROL_BYTE_STEP_5 70   /* MICRO SECONDS */
#define CONTROL_BYTE_STEP_6 3000 /* MICRO SECONDS */
#define CONTROL_BYTE_STEP_7 3000 /* MICRO SECONDS */

#define HDC_DMA      3 /* HDC DMA Channel */

#define HDC_IRQ      5 /* HDC IRQ */

#define R1_REQ    0x01 /* Request bit */
#define R1_IOMODE 0x02 /* Mode bit. Direction? */
#define R1_BUS    0x04 /* Command/Data bit  */
#define R1_BUSY   0x08 /* Busy bit */
#define R1_INT    0x20 /* Interrupt bit */

#define ERROR_OK               0x00 /* No error during execution of the previous command */
#define ERROR_INDEX_SIGNAL     0x01 /* No index signal */
#define ERROR_SEEK_COMPLETE    0x02 /* No seek-complete signal after a seek operation */
#define ERROR_WRITE_FAULT      0x03 /* Write fault */
#define ERROR_READY_SIGNAL     0x04 /* After the controller selected the drive; the drive did not respond with a ready signal */
#define ERROR_T0               0x06 /* No Track 0 signal after stepping the max number of cylinders */
#define ERROR_SEEKING          0x08 /* The drive is still seeking. Reported by the test drive ready command. */

#define ERROR_ID_READ          0x10 /* ECC error in target ID field on disk */
#define ERROR_DATA_READ        0x11
#define ERROR_AM               0x12 /* */
#define ERROR_SECTOR_NOT_FOUND 0x14 /* */
#define ERROR_SEEK_ERROR       0x15 /* */
#define ERROR_CORR_DATA_ERROR  0x18 /* Correctable data error */
#define ERROR_BAD_TRACK        0x19 /* Bad track flag detected on last track */

#define ERROR_INVALID_COMMAND  0x20 /* Received an invalid command from the CPU */
#define ERROR_ILLEGAL_ADDRESS  0x21 /* Address is beyond the max range */

#define ERROR_RAM_ERROR        0x30
#define ERROR_CHECKSUM_ERROR   0x31
#define ERROR_ECC_ERROR        0x32

#define STATUS 0 /* Send Status byte */
#define SENSE  1 /* Send Sense byte */ 

#define NO_IRQ 0 /* Dont do an IRQ */
#define IRQ    1 /* Do an IRQ */

static XEBEC_DCB decode_dcb(XEBEC_HDC* hdc) {
	/* Decode Device Control Block */
	uint8_t byte0 = ring_buffer_pop(&hdc->data_register_in); /* Drive select (bits 5), Head number (bits 0-4) */
	uint8_t byte1 = ring_buffer_pop(&hdc->data_register_in); /* Cylinder High (bits 6-7), Sector number (bits 0-5) */
	uint8_t byte2 = ring_buffer_pop(&hdc->data_register_in); /* Cylinder low (bits 0-7) */
	uint8_t byte3 = ring_buffer_pop(&hdc->data_register_in); /* Interleave/Block count (bits 0-7) */
	uint8_t byte4 = ring_buffer_pop(&hdc->data_register_in); /* Control field (bits 0-7) */

	XEBEC_DCB dcb = { 0 };

	dcb.drive_select = (byte0 >> 5) & 0x01;
	dcb.chs.h = byte0 & 0x1F;

	dcb.chs.s = (byte1 & 0x3F);
	dcb.chs.c = (((uint16_t)byte1 & 0xC0) << 2) | byte2;

	dcb.u.block_count = byte3;               /* Block count or Interleave */

	dcb.step = byte4 & 0x07;                 /* Selects the step option */
	dcb.disable_retry = (byte4 >> 7) & 0x01; /* Disables the 4 retries by the controller on all disk-access commands. */

	return dcb;
}
static void discard_dcb(XEBEC_HDC* hdc) {
	/* Discard Device Control Block */
	ring_buffer_discard(&hdc->data_register_in, 5);
}

static void advance_byte_index(XEBEC_HDC* hdc) {
	hdc->byte_index += 1;
	if (hdc->byte_index >= 512) {
		/* Finished sector */
		hdc->byte_index = 0;
		hdc->sector_index++;

		/* Advance CHS to next sector */
		chs_advance(hdc->hdd[hdc->hdd_select].geometry->chs, &hdc->hdd[hdc->hdd_select].chs);
	}
}

static void send_sense_bytes(XEBEC_HDC* hdc) {
	uint8_t byte0 = hdc->error;
	uint8_t byte1 = (hdc->hdd_select << 5) | hdc->hdd[hdc->hdd_select].chs.h;
	uint8_t byte2 = ((hdc->hdd[hdc->hdd_select].chs.c >> 2) & 0x0C) | (hdc->hdd[hdc->hdd_select].chs.s & 0x1F);
	uint8_t byte3 = hdc->hdd[hdc->hdd_select].chs.c & 0xFF;

	ring_buffer_push(&hdc->data_register_out, byte0);
	ring_buffer_push(&hdc->data_register_out, byte1);
	ring_buffer_push(&hdc->data_register_out, byte2);
	ring_buffer_push(&hdc->data_register_out, byte3);

	hdc->status_register = (R1_BUSY | R1_IOMODE | R1_REQ);
}
static void send_status_byte(XEBEC_HDC* hdc) {
	uint8_t status = 0;

	status |= hdc->hdd_select << 5;
	if (hdc->error) {
		status |= 0x02;
	}
	hdc->status_byte = status;

	hdc->status_register = (R1_BUSY | R1_BUS | R1_IOMODE | R1_REQ);
}

static void command_reset(XEBEC_HDC* hdc) {
	/* Reset command */
	hdc->command.byte = 0;
	hdc->command.param_count = 0;
	hdc->command.state = COMMAND_STATE_IDLE;
}

static void command_set(XEBEC_HDC* hdc, uint8_t command) {
	/* Set command */
	hdc->command.byte = command;

	switch (command) {
		case CMD_TEST_DRIVE:
			hdc->command.param_count = 5;
			break;
		case CMD_RECALIBRATE:
			hdc->command.param_count = 5;
			break;
		case CMD_SENSE:
			hdc->command.param_count = 5;
			break;
		case CMD_FORMAT_DRIVE:
			hdc->command.param_count = 5;
			break;
		case CMD_CHECK_TRACK:
			hdc->command.param_count = 5;
			break;
		case CMD_FORMAT_TRACK:
			hdc->command.param_count = 5;
			break;
		case CMD_FORMAT_BAD:
			hdc->command.param_count = 5;
			break;
		case CMD_READ:
			hdc->command.param_count = 5;
			break;
		case CMD_WRITE:
			hdc->command.param_count = 5;
			break;
		case CMD_SEEK:
			hdc->command.param_count = 5;
			break;
		case CMD_INIT_DRIVE:
			hdc->command.param_count = 5 + 8;
			break;
		case CMD_READ_ECC:
			hdc->command.param_count = 5;
			break;
		case CMD_READ_BUFFER:
			hdc->command.param_count = 5;
			break;
		case CMD_WRITE_BUFFER:
			hdc->command.param_count = 5;
			break;
		case CMD_RAM_DIAG:
			hdc->command.param_count = 5;
			break;
		case CMD_DRIVE_DIAG:
			hdc->command.param_count = 5;
			break;
		case CMD_CONTROLLER_DIAG:
			hdc->command.param_count = 5;
			break;
		case CMD_READ_LONG:
			hdc->command.param_count = 5;
			break;
		case CMD_WRITE_LONG:
			hdc->command.param_count = 5;
			break;
		default:
			hdc->command.param_count = 0;
			break;
	}

	if (hdc->command.param_count == 0) {
		hdc->command.state = COMMAND_STATE_RECIEVED;
		hdc->status_register = R1_BUSY | R1_BUS;
	}
	else {
		hdc->command.state = COMMAND_STATE_RECIEVING;
		hdc->status_register = R1_BUSY | R1_REQ;
	}

	if (!ring_buffer_is_empty(&hdc->data_register_out)) {
		dbg_print("[XEBEC] Command started. OUT FIFO not empty!\n");
	}
}
static void command_set_parameter(XEBEC_HDC* hdc, uint8_t value) {
	/* Set command parameter */
	ring_buffer_push(&hdc->data_register_in, value);
	hdc->command.param_count--;

	if (hdc->command.param_count == 0) {
		hdc->command.state = COMMAND_STATE_RECIEVED;
		hdc->status_register = R1_BUSY | R1_BUS;
	}
}
static void command_set_async(XEBEC_HDC* hdc) {
	/* Set command async */
	hdc->command.state = COMMAND_STATE_EXECUTING | COMMAND_STATE_ASYNC;
}

static void command_finalize(XEBEC_HDC* hdc, uint8_t send, uint8_t irq) {
	/* Finalize command */

	if (send == SENSE) {
		/* Sense status command; send sense bytes */
		send_sense_bytes(hdc);
	}
	else {
		/* All other commands; send status byte */
		send_status_byte(hdc);
	}

	if (irq == IRQ && hdc->int_enabled) {
		i8259_pic_request_interrupt(hdc->pic_p, HDC_IRQ);
		hdc->status_register |= R1_INT;
	}

	if (!ring_buffer_is_empty(&hdc->data_register_in)) {
		dbg_print("[XEBEC] Command finalized. IN FIFO not empty!\n");
	}

	command_reset(hdc);
}

/* Synchronous commands */
static void cmd_reset(XEBEC_HDC* hdc) {
	xebec_hdc_reset(hdc);
	dbg_print("[XEBEC] reset\n");
}
static void cmd_test_drive(XEBEC_HDC* hdc) {
	XEBEC_DCB dcb = decode_dcb(hdc);

	hdc->hdd_select = dcb.drive_select;

	/* BIOS issues a 1701 error if i send ERROR_READY_SIGNAL when theres no hard disk. */
	hdc->error = ERROR_OK;
	command_finalize(hdc, STATUS, IRQ);
	dbg_print("[XEBEC] Test Drive\n");
}
static void cmd_init_drive(XEBEC_HDC* hdc) {
	ring_buffer_discard(&hdc->data_register_in, 13);

	hdc->error = ERROR_OK;
	command_finalize(hdc, STATUS, IRQ);
	dbg_print("[XEBEC] Init drive\n");
}

static void cmd_recalibrate(XEBEC_HDC* hdc) {
	XEBEC_DCB dcb = decode_dcb(hdc);

	hdc->hdd_select = dcb.drive_select;
	hdc->hdd[hdc->hdd_select].chs.c = 0;
	hdc->hdd[hdc->hdd_select].chs.h = 0;
	hdc->hdd[hdc->hdd_select].chs.s = 1; /* XEBEC is zero based; make 1 based */

	/* BIOS issues a 1701 error if i send ERROR_READY_SIGNAL when theres no hard disk. */
	hdc->error = ERROR_OK;
	command_finalize(hdc, STATUS, IRQ);
	dbg_print("[XEBEC] Recalibrate\n");
}
static void cmd_seek(XEBEC_HDC* hdc) {
	XEBEC_DCB dcb = decode_dcb(hdc);

	hdc->hdd_select = dcb.drive_select;
	hdc->hdd[hdc->hdd_select].chs.c = dcb.chs.c;
	hdc->hdd[hdc->hdd_select].chs.h = dcb.chs.h;
	hdc->hdd[hdc->hdd_select].chs.s = 1; /* XEBEC is zero based; make 1 based */

	/* TODO: Find out if ERROR_READY_SIGNAL is sent when theres no hard disk. Probably the same as recalibrate. */
	if (hdc->hdd[hdc->hdd_select].inserted) {
		hdc->error = ERROR_OK;
	}
	else {
		hdc->error = ERROR_READY_SIGNAL;
	}

	command_finalize(hdc, STATUS, IRQ);
	dbg_print("[XEBEC] Seek\n");
}

static void cmd_sense(XEBEC_HDC* hdc) {
	XEBEC_DCB dcb = decode_dcb(hdc);

	hdc->hdd_select = dcb.drive_select;

	command_finalize(hdc, SENSE, IRQ);
	dbg_print("[XEBEC] Sense status\n");
}

static void cmd_format_drive(XEBEC_HDC* hdc) {
	XEBEC_DCB dcb = decode_dcb(hdc);

	hdc->hdd_select = dcb.drive_select;
	hdc->hdd[hdc->hdd_select].chs.c = dcb.chs.c;
	hdc->hdd[hdc->hdd_select].chs.h = dcb.chs.h;
	hdc->hdd[hdc->hdd_select].chs.s = 1; /* XEBEC is zero based; make 1 based */

	if (hdc->hdd[hdc->hdd_select].inserted) {
		hdc->error = ERROR_OK;
	}
	else {
		hdc->error = ERROR_READY_SIGNAL;
	}

	command_finalize(hdc, STATUS, IRQ);
	dbg_print("[XEBEC] Format drive\n");
}

static void cmd_check_track(XEBEC_HDC* hdc) {
	XEBEC_DCB dcb = decode_dcb(hdc);

	hdc->hdd_select = dcb.drive_select;
	hdc->hdd[hdc->hdd_select].chs.c = dcb.chs.c;
	hdc->hdd[hdc->hdd_select].chs.h = dcb.chs.h;
	hdc->hdd[hdc->hdd_select].chs.s = dcb.chs.s + 1; /* XEBEC is zero based; make 1 based */

	if (hdc->hdd[hdc->hdd_select].inserted) {
		hdc->error = ERROR_OK;
	}
	else {
		hdc->error = ERROR_READY_SIGNAL;
	}

	command_finalize(hdc, STATUS, IRQ);
	dbg_print("[XEBEC] Check track\n");
}
static void cmd_format_track(XEBEC_HDC* hdc) {
	XEBEC_DCB dcb = decode_dcb(hdc);

	hdc->hdd_select = dcb.drive_select;
	hdc->hdd[hdc->hdd_select].chs.c = dcb.chs.c;
	hdc->hdd[hdc->hdd_select].chs.h = dcb.chs.h;
	hdc->hdd[hdc->hdd_select].chs.s = 1; /* XEBEC is zero based; make 1 based */

	if (hdc->hdd[hdc->hdd_select].inserted) {
		hdc->error = ERROR_OK;
	}
	else {
		hdc->error = ERROR_READY_SIGNAL;
	}

	command_finalize(hdc, STATUS, IRQ);
	dbg_print("[XEBEC] Format track\n");
}
static void cmd_format_bad_track(XEBEC_HDC* hdc) {
	XEBEC_DCB dcb = decode_dcb(hdc);

	hdc->hdd_select = dcb.drive_select;
	hdc->hdd[hdc->hdd_select].chs.c = dcb.chs.c;
	hdc->hdd[hdc->hdd_select].chs.h = dcb.chs.h;
	hdc->hdd[hdc->hdd_select].chs.s = 1; /* XEBEC is zero based; make 1 based */

	if (hdc->hdd[hdc->hdd_select].inserted) {
		hdc->error = ERROR_OK;
	}
	else {
		hdc->error = ERROR_READY_SIGNAL;
	}

	command_finalize(hdc, STATUS, IRQ);
	dbg_print("[XEBEC] Format bad track\n");
}

static void cmd_read_ecc(XEBEC_HDC* hdc) {
	discard_dcb(hdc);

	hdc->error = ERROR_OK;
	command_finalize(hdc, STATUS, IRQ);
	dbg_print("[XEBEC] Read ECC\n");
}

static void cmd_read(XEBEC_HDC* hdc) {
	XEBEC_DCB dcb = decode_dcb(hdc);

	hdc->hdd_select = dcb.drive_select;
	hdc->hdd[hdc->hdd_select].chs.c = dcb.chs.c;
	hdc->hdd[hdc->hdd_select].chs.h = dcb.chs.h;
	hdc->hdd[hdc->hdd_select].chs.s = dcb.chs.s + 1; /* XEBEC is zero based; make 1 based */

	if (!hdc->hdd[hdc->hdd_select].inserted) {
		hdc->error = ERROR_READY_SIGNAL;
		command_finalize(hdc, STATUS, IRQ);
		return;
	}

	uint32_t transfer_address = i8237_dma_get_transfer_address(hdc->dma_p, HDC_DMA);
	uint32_t transfer_size = i8237_dma_get_transfer_size(hdc->dma_p, HDC_DMA);

	hdc->byte_index = 0;
	hdc->sector_index = 0;
	hdc->sector_count = transfer_size / 512;

	command_set_async(hdc);

	dbg_print("[XEBEC] Read data address=%x, size=%x, sector_count=%d\n", transfer_address, transfer_size, hdc->sector_count);
}
static void cmd_write(XEBEC_HDC* hdc) {
	XEBEC_DCB dcb = decode_dcb(hdc);

	hdc->hdd_select = dcb.drive_select;
	hdc->hdd[hdc->hdd_select].chs.c = dcb.chs.c;
	hdc->hdd[hdc->hdd_select].chs.h = dcb.chs.h;
	hdc->hdd[hdc->hdd_select].chs.s = dcb.chs.s + 1; /* XEBEC is zero based; make 1 based */

	if (!hdc->hdd[hdc->hdd_select].inserted) {
		hdc->error = ERROR_READY_SIGNAL;
		command_finalize(hdc, STATUS, IRQ);
		return;
	}

	uint32_t transfer_address = i8237_dma_get_transfer_address(hdc->dma_p, HDC_DMA);
	uint32_t transfer_size = i8237_dma_get_transfer_size(hdc->dma_p, HDC_DMA);

	hdc->byte_index = 0;
	hdc->sector_index = 0;
	hdc->sector_count = transfer_size / 512;

	command_set_async(hdc);

	dbg_print("[XEBEC] Write data address=%x, size=%x, sector_count=%d\n", transfer_address, transfer_size, hdc->sector_count);
}
static void cmd_read_buffer(XEBEC_HDC* hdc) {
	discard_dcb(hdc);

	uint32_t transfer_address = i8237_dma_get_transfer_address(hdc->dma_p, HDC_DMA);
	uint32_t transfer_size = i8237_dma_get_transfer_size(hdc->dma_p, HDC_DMA);

	hdc->byte_index = 0;
	hdc->sector_index = 0;
	hdc->sector_count = transfer_size / 512;

	command_set_async(hdc);

	dbg_print("[XEBEC] Read buffer transfer_address=%x, size=%x\n", transfer_address, transfer_size);
}
static void cmd_write_buffer(XEBEC_HDC* hdc) {
	discard_dcb(hdc);

	uint32_t transfer_address = i8237_dma_get_transfer_address(hdc->dma_p, HDC_DMA);
	uint32_t transfer_size = i8237_dma_get_transfer_size(hdc->dma_p, HDC_DMA);

	hdc->byte_index = 0;
	hdc->sector_index = 0;
	hdc->sector_count = transfer_size / 512;

	command_set_async(hdc);

	dbg_print("[XEBEC] Write buffer transfer_address=%x, size=%x\n", transfer_address, transfer_size);
}
static void cmd_read_long(XEBEC_HDC* hdc) {
	XEBEC_DCB dcb = decode_dcb(hdc);

	hdc->hdd_select = dcb.drive_select;
	hdc->hdd[hdc->hdd_select].chs.c = dcb.chs.c;
	hdc->hdd[hdc->hdd_select].chs.h = dcb.chs.h;
	hdc->hdd[hdc->hdd_select].chs.s = dcb.chs.s + 1; /* XEBEC is zero based; make 1 based */

	if (!hdc->hdd[hdc->hdd_select].inserted) {
		hdc->error = ERROR_READY_SIGNAL;
		command_finalize(hdc, STATUS, IRQ);
		return;
	}

	uint32_t transfer_address = i8237_dma_get_transfer_address(hdc->dma_p, HDC_DMA);
	uint32_t transfer_size = i8237_dma_get_transfer_size(hdc->dma_p, HDC_DMA);

	hdc->byte_index = 0;
	hdc->sector_index = 0;
	hdc->sector_count = transfer_size / 512;

	command_set_async(hdc);

	dbg_print("[XEBEC] Read long transfer_address=%x, size=%x\n", transfer_address, transfer_size);
}
static void cmd_write_long(XEBEC_HDC* hdc) {
	XEBEC_DCB dcb = decode_dcb(hdc);

	hdc->hdd_select = dcb.drive_select;
	hdc->hdd[hdc->hdd_select].chs.c = dcb.chs.c;
	hdc->hdd[hdc->hdd_select].chs.h = dcb.chs.h;
	hdc->hdd[hdc->hdd_select].chs.s = dcb.chs.s + 1; /* XEBEC is zero based; make 1 based */

	if (!hdc->hdd[hdc->hdd_select].inserted) {
		hdc->error = ERROR_READY_SIGNAL;
		command_finalize(hdc, STATUS, IRQ);
		return;
	}

	uint32_t transfer_address = i8237_dma_get_transfer_address(hdc->dma_p, HDC_DMA);
	uint32_t transfer_size = i8237_dma_get_transfer_size(hdc->dma_p, HDC_DMA);

	hdc->byte_index = 0;
	hdc->sector_index = 0;
	hdc->sector_count = transfer_size / 512;

	command_set_async(hdc);

	dbg_print("[XEBEC] Write long transfer_address=%x, size=%x\n", transfer_address, transfer_size);
}

static void cmd_ram_diag(XEBEC_HDC* hdc) {
	discard_dcb(hdc);

	hdc->error = ERROR_OK;
	command_finalize(hdc, STATUS, IRQ);
	dbg_print("[XEBEC] Ram Diag\n");
}
static void cmd_drive_diag(XEBEC_HDC* hdc) {
	discard_dcb(hdc);

	hdc->error = ERROR_OK;
	command_finalize(hdc, STATUS, IRQ);
	dbg_print("[XEBEC] Drive Diag\n");
}
static void cmd_controller_diag(XEBEC_HDC* hdc) {
	discard_dcb(hdc);

	hdc->error = ERROR_OK;
	command_finalize(hdc, STATUS, IRQ);
	dbg_print("[XEBEC] Controller Diag\n");
}

static void cmd_nop(XEBEC_HDC* hdc) {
	ring_buffer_reset(&hdc->data_register_in);

	hdc->error = ERROR_INVALID_COMMAND;
	command_finalize(hdc, STATUS, IRQ);
	dbg_print("[XEBEC] Invalid command\n");
}

/* Asynchronous commands */
static void cmd_read_async(XEBEC_HDC* hdc) {
	if (!i8237_dma_terminal_count(hdc->dma_p, HDC_DMA)) {
		if (hdc->dma_enabled) {

			size_t offset = chs_to_offset(hdc->hdd[hdc->hdd_select].geometry->chs, hdc->hdd[hdc->hdd_select].chs, 512, hdc->byte_index);

			if (hdc->byte_index == 0) {
				dbg_print("[XEBEC] Read data (sector) HDD%d - c = %d, h = %d, s = %d\n",
					hdc->hdd_select, hdc->hdd[hdc->hdd_select].chs.c, hdc->hdd[hdc->hdd_select].chs.h, hdc->hdd[hdc->hdd_select].chs.s);
			}
			/* Read data from hdd */
			uint8_t byte = xebec_hdd_read_byte(&hdc->hdd[hdc->hdd_select], offset);

			/* Write data to dma */
			i8237_dma_write_byte(hdc->dma_p, HDC_DMA, byte);

			/* Advance byte index */
			advance_byte_index(hdc);

		}
	}
	else {
		hdc->error = ERROR_OK;
		command_finalize(hdc, STATUS, IRQ);
	}
}
static void cmd_write_async(XEBEC_HDC* hdc) {
	if (!i8237_dma_terminal_count(hdc->dma_p, HDC_DMA)) {
		if (hdc->dma_enabled) {

			size_t offset = chs_to_offset(hdc->hdd[hdc->hdd_select].geometry->chs, hdc->hdd[hdc->hdd_select].chs, 512, hdc->byte_index);

			if (hdc->byte_index == 0) {
				dbg_print("[XEBEC] Write data (sector) HDD%d - c = %d, h = %d, s = %d\n",
					hdc->hdd_select, hdc->hdd[hdc->hdd_select].chs.c, hdc->hdd[hdc->hdd_select].chs.h, hdc->hdd[hdc->hdd_select].chs.s);
			}

			/* Read data from dma */
			uint8_t byte = i8237_dma_read_byte(hdc->dma_p, HDC_DMA);

			/* Write data to fdd */
			xebec_hdd_write_byte(&hdc->hdd[hdc->hdd_select], offset, byte);

			/* Advance byte index */
			advance_byte_index(hdc);
		}
	}
	else {
		hdc->error = ERROR_OK;
		command_finalize(hdc, STATUS, IRQ);
	}
}
static void cmd_read_buffer_async(XEBEC_HDC* hdc) {
	if (!i8237_dma_terminal_count(hdc->dma_p, HDC_DMA)) {
		if (hdc->dma_enabled) {

			size_t offset = chs_to_offset(hdc->hdd[hdc->hdd_select].geometry->chs, hdc->hdd[hdc->hdd_select].chs, 512, hdc->byte_index);

			/* Read data from hdd */
			uint8_t byte = 0;// xebec_hdd_read_byte(&hdc->hdd[hdc->hdd_select], offset);

			/* Write data to dma */
			i8237_dma_write_byte(hdc->dma_p, HDC_DMA, byte);

			/* Advance byte index */
			advance_byte_index(hdc);
		}
	}
	else {
		hdc->error = ERROR_OK;
		command_finalize(hdc, STATUS, IRQ);
	}
}
static void cmd_write_buffer_async(XEBEC_HDC* hdc) {
	if (!i8237_dma_terminal_count(hdc->dma_p, HDC_DMA)) {
		if (hdc->dma_enabled) {

			size_t offset = chs_to_offset(hdc->hdd[hdc->hdd_select].geometry->chs, hdc->hdd[hdc->hdd_select].chs, 512, hdc->byte_index);

			/* Read data from dma */
			uint8_t byte = i8237_dma_read_byte(hdc->dma_p, HDC_DMA);

			/* Write data to fdd */
			//xebec_hdd_write_byte(&hdc->hdd[hdc->hdd_select], offset, byte);

			/* Advance byte index */
			advance_byte_index(hdc);
		}
	}
	else {
		hdc->error = ERROR_OK;
		command_finalize(hdc, STATUS, IRQ);
	}
}
static void cmd_read_long_async(XEBEC_HDC* hdc) {
	hdc->error = ERROR_OK;
	command_finalize(hdc, STATUS, IRQ);
}
static void cmd_write_long_async(XEBEC_HDC* hdc) {
	hdc->error = ERROR_OK;
	command_finalize(hdc, STATUS, IRQ);
}

/* Execute command */
static void command_execute(XEBEC_HDC* hdc) {
	hdc->command.state = COMMAND_STATE_EXECUTING;
	switch (hdc->command.byte) {
		case CMD_TEST_DRIVE:
			cmd_test_drive(hdc);
			break;
		case CMD_RECALIBRATE:
			cmd_recalibrate(hdc);
			break;
		case CMD_SENSE:
			cmd_sense(hdc);
			break;
		case CMD_FORMAT_DRIVE:
			cmd_format_drive(hdc);
			break;
		case CMD_CHECK_TRACK:
			cmd_check_track(hdc);
			break;
		case CMD_FORMAT_TRACK:
			cmd_format_track(hdc);
			break;
		case CMD_FORMAT_BAD:
			cmd_format_bad_track(hdc);
			break;
		case CMD_READ:
			cmd_read(hdc);
			break;
		case CMD_WRITE:
			cmd_write(hdc);
			break;
		case CMD_SEEK:
			cmd_seek(hdc);
			break;
		case CMD_INIT_DRIVE:
			cmd_init_drive(hdc);
			break;
		case CMD_READ_ECC:
			cmd_read_ecc(hdc);
			break;
		case CMD_READ_BUFFER:
			cmd_read_buffer(hdc);
			break;
		case CMD_WRITE_BUFFER:
			cmd_write_buffer(hdc);
			break;
		case CMD_RAM_DIAG:
			cmd_ram_diag(hdc);
			break;
		case CMD_DRIVE_DIAG:
			cmd_drive_diag(hdc);
			break;
		case CMD_CONTROLLER_DIAG:
			cmd_controller_diag(hdc);
			break;
		case CMD_READ_LONG:
			cmd_read_long(hdc);
			break;
		case CMD_WRITE_LONG:
			cmd_write_long(hdc);
			break;
		default:
			cmd_nop(hdc);
			break;
	}
}
static void command_execute_async(XEBEC_HDC* hdc) {
	switch (hdc->command.byte) {
		case CMD_READ:
			cmd_read_async(hdc);
			break;
		case CMD_WRITE:
			cmd_write_async(hdc);
			break;
		case CMD_READ_BUFFER:
			cmd_read_buffer_async(hdc);
			break;
		case CMD_WRITE_BUFFER:
			cmd_write_buffer_async(hdc);
			break;
		case CMD_READ_LONG:
			cmd_read_long_async(hdc);
			break;
		case CMD_WRITE_LONG:
			cmd_write_long_async(hdc);
			break;
	}
}

static uint8_t read_data(XEBEC_HDC* hdc) {
	if (!ring_buffer_is_empty(&hdc->data_register_out)) {
		uint8_t data = ring_buffer_pop(&hdc->data_register_out);
		if (ring_buffer_is_empty(&hdc->data_register_out)) {
			/* All sense bytes have been sent; Send status byte */
			send_status_byte(hdc);
		}
		return data;
	}
	/* Send status byte */
	return hdc->status_byte;
}
static uint8_t read_dipswitch(XEBEC_HDC* hdc) {
	dbg_print("[XEBEC] read dipswitch\n");
	return hdc->dipswitch;
}
static uint8_t read_status(XEBEC_HDC* hdc) {
	return hdc->status_register;
}

static void write_data(XEBEC_HDC* hdc, uint8_t value) {
	if (hdc->command.state == COMMAND_STATE_IDLE) {
		command_set(hdc, value);
	}
	else if (hdc->command.state == COMMAND_STATE_RECIEVING) {
		command_set_parameter(hdc, value);
	}

	if (hdc->command.state == COMMAND_STATE_RECIEVED) {
		command_execute(hdc);
	}
}
static void write_mask(XEBEC_HDC* hdc, uint8_t value) {
	hdc->int_enabled = (value & 0x02) >> 1;
	hdc->dma_enabled = value & 0x01;
	hdc->status_register = (R1_BUSY | R1_BUS | R1_REQ);
}
static void select_controller(XEBEC_HDC* hdc, uint8_t value) {
	(void)hdc;
	(void)value;
}

int xebec_hdc_create(XEBEC_HDC* hdc) {
	if (ring_buffer_create(&hdc->data_register_out, 10)) {
		dbg_print("[HDC] Failed to allocate data register out ring buffer\n");
		return 1;
	}
	if (ring_buffer_create(&hdc->data_register_in, 18)) {
		dbg_print("[HDC] Failed to allocate data register in ring buffer\n");
		return 1;
	}

	for (uint8_t i = 0; i < HDD_MAX; ++i) {
		hdc->hdd[i].path = calloc(1, 256);
		if (hdc->hdd[i].path == NULL) {
			dbg_print("[HDC] Failed to allocate path buffer\n");
			return 1;
		}
	}

	return 0;
}
void xebec_hdc_destroy(XEBEC_HDC* hdc) {
	ring_buffer_destroy(&hdc->data_register_out);
	ring_buffer_destroy(&hdc->data_register_in);

	for (uint8_t i = 0; i < HDD_MAX; ++i) {
		if (hdc->hdd[i].path != NULL) {
			free(hdc->hdd[i].path);
			hdc->hdd[i].path = NULL;
		}
	}
}

void xebec_hdc_init(XEBEC_HDC* hdc, I8237_DMA* dma, I8259_PIC* pic) {
	hdc->dma_p = dma;
	hdc->pic_p = pic;
}

void xebec_hdc_reset(XEBEC_HDC* hdc) {
	hdc->accum = 0;
	hdc->byte_index = 0;
	hdc->status_register = 0;
	hdc->error = 0;
	command_reset(hdc);
	ring_buffer_reset(&hdc->data_register_in);
	ring_buffer_reset(&hdc->data_register_out);
}

uint8_t xebec_hdc_read_io_byte(XEBEC_HDC* hdc, uint8_t address) {
	switch (address) {
		case PORT_READ_DATA:
			return read_data(hdc);
		case PORT_READ_DIP:
			return read_dipswitch(hdc);
		case PORT_READ_STATUS:
			return read_status(hdc);
		default:
			dbg_print("[XEBEC_HDC] read byte %x\n", address);
			break;
	}
	return 0;
}
void xebec_hdc_write_io_byte(XEBEC_HDC* hdc, uint8_t address, uint8_t value) {
	switch (address) {
		case PORT_WRITE_DATA:
			write_data(hdc, value);
			break;
		case PORT_RESET:
			cmd_reset(hdc);
			break;
		case PORT_WRITE_SELECT:
			select_controller(hdc, value);
			break;
		case PORT_WRITE_MASK:
			write_mask(hdc, value);
			break;
		default:
			dbg_print("[XEBEC_HDC] write byte %x\n", address);
			break;
	}
}

void xebec_hdc_update(XEBEC_HDC* hdc) {
	if (hdc->command.state == (COMMAND_STATE_EXECUTING | COMMAND_STATE_ASYNC)) {
		command_execute_async(hdc);
	}
}

void xebec_hdc_set_dipswitch(XEBEC_HDC* hdc, int hdd, uint8_t type) {
	/* set dipswitch (4 bit register)
	 HDD 1:
	  0b0000 = 306, 4, 17
	  0b0001 = 612, 4, 17
	  0b0010 = 615, 4, 17
	  0b0011 = 306, 8, 17
	 HDD 0:
	  0b0000 = 306, 4, 17
	  0b0100 = 612, 4, 17
	  0b1000 = 615, 4, 17
	  0b1100 = 306, 8, 17 */

	uint8_t bits = 0;
	switch (type) {
		case XEBEC_HDD_TYPE_1:  /* 10MB - 306 4 17 - Type 1 */
			bits = 0x0;
			break;
		case XEBEC_HDD_TYPE_16: /* 20MB - 612 4 17 - Type 16 */
			bits = 0x1;
			break;
		case XEBEC_HDD_TYPE_2:  /* 20MB - 615 4 17 - Type 2 */
			bits = 0x2;
			break;
		case XEBEC_HDD_TYPE_13: /* 20MB - 306 8 17 - Type 13 */
			bits = 0x3;
			break;
	}
	int shift = (1 - hdd) * 2;
	hdc->dipswitch &= ~(0x3 << shift);
	hdc->dipswitch |= bits << shift;
}
int xebec_hdc_insert_hdd(XEBEC_HDC* hdc, int hdd, const char* path) {
	if (hdd > 1) {
		return 1;
	}

	if (hdc->hdd[hdd].inserted) {
		return 1;
	}

	if (xebec_hdd_insert(&hdc->hdd[hdd], path)) {
		dbg_print("[XEBEC] Failed to Insert HDD%d: %s\n", hdd, path);
		return 1;
	}

	xebec_hdc_set_dipswitch(hdc, hdd, hdc->hdd[hdd].geometry->type);

	dbg_print("[XEBEC] Insert HDD%d: %s\n", hdd, hdc->hdd[hdd].path);
	return 0;
}
void xebec_hdc_eject_hdd(XEBEC_HDC* hdc, int hdd) {
	if (hdd > 1) {
		return;
	}
	
	if (!hdc->hdd[hdd].inserted) {
		return;
	}

	dbg_print("[XEBEC] Eject HDD%d: %s\n", hdd, hdc->hdd[hdd].path);

	xebec_hdd_eject(&hdc->hdd[hdd]);
}
int xebec_hdc_reinsert_hdd(XEBEC_HDC* hdc, int hdd) {
	if (hdd > 1) {
		return 1;
	}

	if (!hdc->hdd[hdd].inserted) {
		return 1;
	}

	if (xebec_hdd_reinsert(&hdc->hdd[hdd])) {
		dbg_print("[XEBEC] Failed to Reinsert HDD%d: %s\n", hdd, hdc->hdd[hdd].path);
		return 1;
	}

	/* set dip (4 bits) */
	xebec_hdc_set_dipswitch(hdc, hdd, hdc->hdd[hdd].geometry->type);

	dbg_print("[XEBEC] Reinsert HDD%d: %s\n", hdd, hdc->hdd[hdd].path);
	return 0;
}

void xebec_hdc_save_hdd(XEBEC_HDC* hdc, int hdd) {
	if (hdd > 1) {
		return;
	}

	if (!hdc->hdd[hdd].inserted) {
		return;
	}

	if (xebec_hdd_save(&hdc->hdd[hdd])) {
		dbg_print("[XEBEC] Failed to save HDD%d: %s\n", hdd, hdc->hdd[hdd].path);
	}
	else {
		dbg_print("[XEBEC] Save HDD%d: %s\n", hdd, hdc->hdd[hdd].path);
	}
}
void xebec_hdc_save_as_hdd(XEBEC_HDC* hdc, int hdd, const char* filename) {
	if (hdd > 1) {
		return;
	}

	if (!hdc->hdd[hdd].inserted) {
		return;
	}

	if (xebec_hdd_save_as(&hdc->hdd[hdd], filename)) {
		dbg_print("[XEBEC] Failed to save HDD%d: %s\n", hdd, hdc->hdd[hdd].path);
	}
	else {
		dbg_print("[XEBEC] Save HDD%d: %s\n", hdd, hdc->hdd[hdd].path);
	}
}

int xebec_hdc_new_hdd(XEBEC_HDC* hdc, int hdd, CHS geometry, XEBEC_FILE_TYPE file_type) {
	if (hdd > 1) {
		return 1;
	}

	if (hdc->hdd[hdd].inserted) {
		return 1;
	}

	if (xebec_hdd_new(&hdc->hdd[hdd], geometry, file_type)) {
		return 1;
	}

	xebec_hdc_set_dipswitch(hdc, hdd, hdc->hdd[hdd].geometry->type);

	dbg_print("[XEBEC] New HDD%d: %s\n", hdd, hdc->hdd[hdd].path);
	return 0;
}

void xebec_hdc_set_geometry_override_hdd(XEBEC_HDC* hdc, int hdd, CHS geometry, XEBEC_HDD_TYPE type) {
	if (hdd > 1) {
		return;
	}
	xebec_hdd_set_geometry_override(&hdc->hdd[hdd], geometry, type);
}
int xebec_hdc_set_geometry_hdd(XEBEC_HDC* hdc, int hdd, CHS geometry) {
	if (hdd > 1) {
		return 1;
	}
	return xebec_hdd_set_geometry(&hdc->hdd[hdd], geometry);
}
