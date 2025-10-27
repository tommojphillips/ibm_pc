/* fdc.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Floppy Disk Controller NEC uPD765 (Intel i8272)
 */

#include <malloc.h>

#include "fdc.h"
#include "fdd.h"

#include "backend/bit_utils.h"
#include "backend/ring_buffer.h"
#include "backend/chipset/i8237_dma.h"

/* I/O Port Addresses */

#define PORT_STATUS_A        0 // RO
#define PORT_STATUS_B        1 // RO
#define PORT_DIGITAL_OUTPUT  2 // WO - (DOR) Used to control drive motors, drive selection, reset fdc, and feature enable
#define PORT_TAPE_DRIVE      3 // R/W
#define PORT_STATUS_REGISTER 4 // RO
#define PORT_DATARATE_SELECT 4 // WO
#define PORT_DATA_REGISTER   5 // R/W
#define PORT_DATA_COMMAND    5 // WO
#define PORT_DATA_FIFO       5 // RO
#define PORT_DIGITAL_INPUT   7 // RO
#define PORT_CONFIG_CONTROL  7 // WO

/* Commands */

#define CMD_READ_TRACK         2
#define CMD_SPECIFY            3 // set drive parameters
#define CMD_SENSE_DRIVE_STATUS 4
#define CMD_WRITE_DATA         5 // write to the disk
#define CMD_READ_DATA          6 // read from the disk
#define CMD_RECALIBRATE        7 // seek to cylinder 0
#define CMD_SENSE_INTERRUPT    8 // ack IRQ6, get status of last command
#define CMD_WRITE_DELETED_DATA 9
#define CMD_READ_ID            10
#define CMD_READ_DELETED_DATA  12
#define CMD_FORMAT_TRACK       13
#define CMD_SEEK               15 // seek both heads to cylinder X
#define CMD_SCAN_EQUAL         17
#define CMD_SCAN_LOW_OR_EQUAL  25
#define CMD_SCAN_HIGH_OR_EQUAL 29

/* DOR */

/* These bits are decoded by the hardware to select one drive if its motor is on */
#define DOR_FDD_SELECT_MASK    0x03
#define DOR_SELECT_FDD0        0x00 /* Select FDD A if the motor is on */
#define DOR_SELECT_FDD1        0x01 /* Select FDD B if the motor is on */
#define DOR_SELECT_FDD2        0x02 /* Select FDD C if the motor is on */
#define DOR_SELECT_FDD3        0x03 /* Select FDD D if the motor is on */

 /* The FDC is held reset when this bit is clear. It  must be set by the program to enable the FDC */
#define DOR_ENABLE            0x04

/* This bit allows the FDC interrupt and DMA requests to be gated onto 
 the I/O interface. If this bit is cleared, the interrupt and DMA 
 request I/O interface drivers are disabled. */
#define DOR_DMA_INT_MASK      0x08

/* These bits control the motors of drives 0,1,2,3.
 If a bit is clear, the motor is off, and the drive cannot be selected. */
#define DOR_FDD_MOTOR_ON_MASK   0xF0
#define DOR_FDD_MOTOR_ON_BASE   0x10 // Turn FDD0 Motor on/off
#define DOR_FDD_MOTOR_ON_A      0x10 // Turn FDD0 Motor on/off
#define DOR_FDD_MOTOR_ON_B      0x20 // Turn FDD1 Motor on/off
#define DOR_FDD_MOTOR_ON_C      0x40 // Turn FDD2 Motor on/off
#define DOR_FDD_MOTOR_ON_D      0x80 // Turn FDD3 Motor on/off

/* MSR */

#define MSR_FDD0_BUSY          0x01 /* D0B - FDD0 is in seek mode */
#define MSR_FDD1_BUSY          0x02 /* D1B - FDD1 is in seek mode */
#define MSR_FDD2_BUSY          0x04 /* D2B - FDD2 is in seek mode */
#define MSR_FDD3_BUSY          0x08 /* D3B - FDD3 is in seek mode */
#define MSR_FDC_BUSY           0x10 /* CB  - FDC Busy; A command is in progress; FDC will not accept new commands */
#define MSR_EXM                0x20 /* EXM - This bit is only set during execution phase in non-DMA mode */
#define MSR_DIO                0x40 /* DIO - Indicates direction of data transfer between the FDC and CPU. 0 = from cpu; 0x40 = to cpu */
#define MSR_RQM                0x80 /* RQM - Request for master */

#define US_MASK     0x03 /* Unit Select mask */

#define ST0_IC_MASK 0xC0 /* Interrupt Code mask */ 
#define ST0_RESET   0xC0 /* Interrupt Code - reset */
#define ST0_AT2     0xC0 /* Interrupt Code - abnormal termination; during execution, the ready signal changed state */
#define ST0_IC      0x80 /* Interrupt Code - invalid command; command issued never started */
#define ST0_AT      0x40 /* Interrupt Code - abnormal termination; command execution started but never completed */
#define ST0_NT      0x00 /* Interrupt Code - normal termination; command was completed and executed correctly */

#define ST0_SE      0x20 /* Seek End; when FDC completes the seek command; this bit is set */
#define ST0_EC      0x10 /* Equipment Check; fault signal received from FDD or Track0 signal failed to occur after 77 step pulses in the recalibrate command */
#define ST0_NR      0x08 /* Not Ready; read/write command issued to FDD and FDD was not ready */
#define ST0_HD      0x04 /* Head Address; State of the head at interrupt */
#define ST0_US1     0x02 /* Unit Select 1; Indicates drive unit at interrupt  */
#define ST0_US0     0x01 /* Unit Select 0; Indicates drive unit at interrupt */

#define ST1_EN      0x80 /* End of Cylinder; FDC tried to access a sector beyond the final sector of a cylinder */
#define ST1_DE      0x20 /* Data Error; FDC detected a CRC error in the ID field or data field */
#define ST1_OR      0x10 /* Overrun; */
#define ST1_ND      0x04 /* No Data; */
#define ST1_NW      0x02 /* Not Writable; FDC detected a write protected signal from the FDD */
#define ST1_MA      0x01 /* Missing Address Mark; */

#define ST2_CM      0x40 /* Control Mark; Sector contains a delete data address mark */
#define ST2_DD      0x20 /* Data Error in Data Field; CRC error in data field */
#define ST2_WC      0x10 /* Wrong Cylinder; */
#define ST2_SH      0x08 /* Scan Equal Hit; */
#define ST2_SN      0x04 /* Scan Not Satisified; */
#define ST2_BC      0x02 /* Bad Cylinder; */
#define ST2_MD      0x01 /* Missing Address Mark in Data Field; */

#define ST3_FT      0x80 /* Fault; Status of the fault signal from the FDD */
#define ST3_WP      0x40 /* Write Protected; Status of the write protected signal from the FDD */
#define ST3_RY      0x20 /* Ready; Status of the ready signal from the FDD */
#define ST3_T0      0x10 /* Track 0; Status of the track 0 signal from the FDD */
#define ST3_TS      0x08 /* Two Side; Status of the two side signal from the FDD */
#define ST3_HD      0x04 /* Head Address; Status of side select signal from the FDD */
#define ST3_US1     0x02 /* Unit Select 1; Status of the unit select 1 signal to the FDD */
#define ST3_US0     0x01 /* Unit Select 0; Status of the unit select 0 signal to the FDD */

#define CMD_BYTE    0x1F
#define CMD_MT      0x80
#define CMD_MFM     0x40
#define CMD_SK      0x20

#define NO_IRQ       0 /* Dont do an IRQ */
#define IRQ          1 /* Do an IRQ */

#define RECEIVE_DATA 0 /* Set FDC to receive data */
#define SEND_DATA    1 /* Set FDC to send data */

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

/* FDC DMA Channel */
#define FDC_DMA 2

static uint16_t decode_sector_size(uint8_t value) {
	return (1 << (value + 7));
}

static void send_data(FDC* fdc) {
	/* FDC has data to send */
	fdc->msr |= (MSR_FDC_BUSY | MSR_DIO | MSR_RQM);
}
static void receive_data(FDC* fdc) {
	/* FDC is ready to receive data */
	fdc->msr &= ~(MSR_FDC_BUSY | MSR_DIO);
	fdc->msr |= MSR_RQM;
}

static void st0(FDC* fdc, uint8_t ic, uint8_t se) {
	fdc->st0 = ic & ST0_IC_MASK;

	fdc->st0 |= fdc->fdd_select & US_MASK;

	if (fdc->command.chs.h == 1) {
		fdc->st0 |= ST0_HD;
	}
	
	/* Is the drive not ready? */
	if (!fdc->fdd[fdc->fdd_select].status.ready) {
		fdc->st0 |= ST0_NR;
	}
	
	if (se) {
		fdc->st0 |= ST0_SE;
	}
}
static void st1(FDC* fdc) {
	fdc->st1 = 0;

	if (fdc->command.error & FDC_ERROR_END_OF_CYLINDER) {
		fdc->st1 |= ST1_EN;
	}

	if (fdc->command.error & FDC_ERROR_CRC_ERROR) {
		fdc->st1 |= ST1_DE;
	}

	if (fdc->command.error & FDC_ERROR_OVERRUN) {
		fdc->st1 |= ST1_OR;
	}

	if (fdc->fdd[fdc->fdd_select].status.write_protect) {
		fdc->st1 |= ST1_NW;
	}

	if (!fdc->fdd[fdc->fdd_select].status.ready) {
		fdc->st1 |= (ST1_ND | ST1_MA);
	}
}
static void st2(FDC* fdc) {
	fdc->st2 = 0;

	if (fdc->command.error & FDC_ERROR_WRONG_CYLINDER) {
		fdc->st2 |= ST2_WC;
	}

	if (fdc->command.error & FDC_ERROR_BAD_CYLINDER) {
		fdc->st2 |= ST2_BC;
	}
}
static void st3(FDC* fdc) {
	fdc->st3 = 0;

	fdc->st3 |= fdc->fdd_select & US_MASK;

	/* Is the drive on head 2? */
	if (fdc->command.chs.h == 1) {
		fdc->st3 |= ST3_HD;
	}

	/* Does the drive have two heads? */
	if (fdc->fdd[fdc->fdd_select].disk_descriptor.h > 1) {
		fdc->st3 |= ST3_TS;
	}

	/* Is the drive on sector 0? */
	if (fdc->command.chs.c == 0) {
		fdc->st3 |= ST3_T0;
	}

	/* Is the drive ready? */
	if (fdc->fdd[fdc->fdd_select].status.ready) {
		fdc->st3 |= ST3_RY;
	}

	/* Is the drive write protected? */
	if (fdc->fdd[fdc->fdd_select].status.write_protect) {
		fdc->st3 |= ST3_WP;
	}
}

static void command_set(FDC* fdc, uint8_t command) {
	/* Set command */
	fdc->command.byte = command;

	switch (command & CMD_BYTE) {
		case CMD_READ_DATA:
			fdc->command.param_count = (8);
			break;
		case CMD_READ_TRACK:
			fdc->command.param_count = (8);
			break;
		case CMD_READ_DELETED_DATA:
			fdc->command.param_count = (8);
			break;
		case CMD_READ_ID:
			fdc->command.param_count = (1);
			break;
		case CMD_WRITE_DATA:
			fdc->command.param_count = (8);
			break;
		case CMD_FORMAT_TRACK:
			fdc->command.param_count = (5);
			break;
		case CMD_WRITE_DELETED_DATA:
			fdc->command.param_count = (8);
			break;
		case CMD_SCAN_EQUAL:
			fdc->command.param_count = (8);
			break;
		case CMD_SCAN_LOW_OR_EQUAL:
			fdc->command.param_count = (8);
			break;
		case CMD_SCAN_HIGH_OR_EQUAL:
			fdc->command.param_count = (8);
			break;
		case CMD_RECALIBRATE:
			fdc->command.param_count = (1);
			break;
		case CMD_SEEK:
			fdc->command.param_count = (2);
			break;
		case CMD_SENSE_DRIVE_STATUS:
			fdc->command.param_count = (1);
			break;
		case CMD_SENSE_INTERRUPT:
			fdc->command.param_count = (0);
			break;
		case CMD_SPECIFY:
			fdc->command.param_count = (2);
			break;
		default:
			fdc->command.param_count = (0);
			break;
	}

	if (fdc->command.param_count == 0) {
		fdc->command.recieving = 0;
		fdc->command.received = 1;
	}
	else {
		fdc->command.recieving = 1;
		fdc->command.received = 0;
	}
}
static void command_set_parameter(FDC* fdc, uint8_t value) {
	/* Set command parameter */
	ring_buffer_push(&fdc->data_register_in, value);
	fdc->command.param_count--;

	if (fdc->command.param_count == 0) {
		fdc->command.recieving = 0;
		fdc->command.received = 1;
	}
}
static void command_finalize(FDC* fdc, uint8_t irq, uint8_t data_direction) {
	/* Finalize command */
	if (irq == IRQ) {
		fdc->do_irq(fdc);
	}

	if (data_direction == SEND_DATA) {
		/* FDC has data to send */
		send_data(fdc);
	}
	else {
		/* FDC is ready to receive data */
		receive_data(fdc);
	}
}
static void command_reset(FDC* fdc, uint8_t irq, uint8_t data_direction) {
	/* Reset command */
	fdc->command.byte = 0;
	fdc->command.param_count = 0;
	fdc->command.recieving = 0;
	fdc->command.received = 0;
	fdc->command.async = 0;
	fdc->command.error = 0;

	command_finalize(fdc, irq, data_direction);
}
static void command_set_async(FDC* fdc) {
	/* Set command async */
	fdc->command.recieving = 0;
	fdc->command.received = 0;
	fdc->command.async = 1;
}
static void command_results(FDC* fdc, uint8_t ic, uint8_t irq) {
	/* Send command results */

	/* Set ST0, ST1, ST2 */
	st0(fdc, ic, 0);
	st1(fdc);
	st2(fdc);

	ring_buffer_push(&fdc->data_register_out, fdc->st0);           /* ST0 */
	ring_buffer_push(&fdc->data_register_out, fdc->st1);           /* ST1 */
	ring_buffer_push(&fdc->data_register_out, fdc->st2);           /* ST2 */
	ring_buffer_push(&fdc->data_register_out, fdc->command.chs.c); /* C */
	ring_buffer_push(&fdc->data_register_out, fdc->command.chs.h); /* H */
	ring_buffer_push(&fdc->data_register_out, fdc->command.chs.s); /* R */
	ring_buffer_push(&fdc->data_register_out, fdc->command.n);     /* N */

	/* Reset command; set IRQ; send data */
	command_reset(fdc, irq, SEND_DATA);
}

/* Synchronous commands */
static void cmd_reset(FDC* fdc) {
	/* Reset FDC */
	upd765_fdc_reset(fdc);
	
	/* Set ST0 */
	fdc->st0 = ST0_RESET;
	
	/* Finalize command; set IRQ; receive data */
	command_finalize(fdc, IRQ, RECEIVE_DATA);
	
	dbg_print("[FDC] reset\n");
}
static void cmd_read_data(FDC* fdc) {

	/* Read command parameters */
	fdc->command.dhs = ring_buffer_pop(&fdc->data_register_in);      /* HD/US1/US0 */
	fdc->command.chs.c = ring_buffer_pop(&fdc->data_register_in);    /* C */
	fdc->command.chs.h = ring_buffer_pop(&fdc->data_register_in);    /* H */
	fdc->command.chs.s = ring_buffer_pop(&fdc->data_register_in);    /* R */
	fdc->command.n = ring_buffer_pop(&fdc->data_register_in);        /* N */
	fdc->command.eot = ring_buffer_pop(&fdc->data_register_in);      /* EOT */
	fdc->command.gap_len = ring_buffer_pop(&fdc->data_register_in);  /* GPL */
	fdc->command.data_len = ring_buffer_pop(&fdc->data_register_in); /* DTL */

	/* Load drive select */
	fdc->fdd_select = fdc->command.dhs & US_MASK;

	if (fdc->dor & DOR_DMA_INT_MASK) {
		fdc->msr &= ~MSR_RQM; /* Clear RQM; Dont let CPU poll */
	}
	else {
		fdc->msr |= MSR_RQM; /* Set RQM; Let CPU poll */
	}

	fdc->sector_size = decode_sector_size(fdc->command.n);
	fdc->byte_index = 0;
	
	dbg_print("[FDC] Read data %s dhs=%u, c=%u, h=%u, s=%u, n=%u, eot=%u, gpl=%u, dtl=%u\n",
		(fdc->dma ? "DMA" : "PIO"), fdc->command.dhs, fdc->command.chs.c, fdc->command.chs.h, fdc->command.chs.s, 
		fdc->command.n, fdc->command.eot, fdc->command.gap_len, fdc->command.data_len);

	command_set_async(fdc);
}
static void cmd_read_track(FDC* fdc) {

	/* Read command parameters */
	fdc->command.dhs = ring_buffer_pop(&fdc->data_register_in);      /* HD/US1/US0 */
	fdc->command.chs.c = ring_buffer_pop(&fdc->data_register_in);    /* C */
	fdc->command.chs.h = ring_buffer_pop(&fdc->data_register_in);    /* H */
	fdc->command.chs.s = ring_buffer_pop(&fdc->data_register_in);    /* R */
	fdc->command.n = ring_buffer_pop(&fdc->data_register_in);        /* N */
	fdc->command.eot = ring_buffer_pop(&fdc->data_register_in);      /* EOT */
	fdc->command.gap_len = ring_buffer_pop(&fdc->data_register_in);  /* GPL */
	fdc->command.data_len = ring_buffer_pop(&fdc->data_register_in); /* DTL */

	/* Load drive select */
	fdc->fdd_select = fdc->command.dhs & US_MASK;

	if (fdc->dor & DOR_DMA_INT_MASK) {
		fdc->msr &= ~MSR_RQM; /* Clear RQM; Dont let CPU poll */
	}
	else {
		fdc->msr |= MSR_RQM; /* Set RQM; Let CPU poll */
	}

	fdc->sector_size = decode_sector_size(fdc->command.n);
	fdc->byte_index = 0;

	dbg_print("[FDC] Read track %s dhs=%u, c=%u, h=%u, s=%u, n=%u, eot=%u, gpl=%u, dtl=%u\n",
		(fdc->dma ? "DMA" : "PIO"), fdc->command.dhs, fdc->command.chs.c, fdc->command.chs.h, fdc->command.chs.s, 
		fdc->command.n, fdc->command.eot, fdc->command.gap_len, fdc->command.data_len);

	command_set_async(fdc);
}
static void cmd_read_deleted_data(FDC* fdc) {

	/* Read command parameters */
	fdc->command.dhs = ring_buffer_pop(&fdc->data_register_in);      /* HD/US1/US0 */
	fdc->command.chs.c = ring_buffer_pop(&fdc->data_register_in);    /* C */
	fdc->command.chs.h = ring_buffer_pop(&fdc->data_register_in);    /* H */
	fdc->command.chs.s = ring_buffer_pop(&fdc->data_register_in);    /* R */
	fdc->command.n = ring_buffer_pop(&fdc->data_register_in);        /* N */
	fdc->command.eot = ring_buffer_pop(&fdc->data_register_in);      /* EOT */
	fdc->command.gap_len = ring_buffer_pop(&fdc->data_register_in);  /* GPL */
	fdc->command.data_len = ring_buffer_pop(&fdc->data_register_in); /* DTL */

	/* Load drive select */
	fdc->fdd_select = fdc->command.dhs & US_MASK;

	if (fdc->dor & DOR_DMA_INT_MASK) {
		fdc->msr &= ~MSR_RQM; /* Clear RQM; Dont let CPU poll */
	}
	else {
		fdc->msr |= MSR_RQM; /* Set RQM; Let CPU poll */
	}

	dbg_print("[FDC] Read deleted data %s dhs=%u, c=%u, h=%u, s=%u, n=%u, eot=%u, gpl=%u, dtl=%u\n",
		(fdc->dma ? "DMA" : "PIO"), fdc->command.dhs, fdc->command.chs.c, fdc->command.chs.h, fdc->command.chs.s, 
		fdc->command.n, fdc->command.eot, fdc->command.gap_len, fdc->command.data_len);

	/* Send command results */
	command_results(fdc, ST0_NT, NO_IRQ);
}
static void cmd_write_data(FDC* fdc) {

	/* Read command parameters */
	fdc->command.dhs = ring_buffer_pop(&fdc->data_register_in);      /* HD/US1/US0 */
	fdc->command.chs.c = ring_buffer_pop(&fdc->data_register_in);    /* C */
	fdc->command.chs.h = ring_buffer_pop(&fdc->data_register_in);    /* H */
	fdc->command.chs.s = ring_buffer_pop(&fdc->data_register_in);    /* R */
	fdc->command.n = ring_buffer_pop(&fdc->data_register_in);        /* N */
	fdc->command.eot = ring_buffer_pop(&fdc->data_register_in);      /* EOT */
	fdc->command.gap_len = ring_buffer_pop(&fdc->data_register_in);  /* GPL */
	fdc->command.data_len = ring_buffer_pop(&fdc->data_register_in); /* DTL */

	/* Load drive select */
	fdc->fdd_select = fdc->command.dhs & US_MASK;

	if (fdc->dor & DOR_DMA_INT_MASK) {
		fdc->msr &= ~MSR_RQM; /* Clear RQM; Dont let CPU poll */
	}
	else {
		fdc->msr |= MSR_RQM; /* Set RQM; Let CPU poll */
	}

	fdc->sector_size = decode_sector_size(fdc->command.n);
	fdc->byte_index = 0;

	dbg_print("[FDC] Write data %s dhs=%u, c=%u, h=%u, s=%u, n=%u, eot=%u, gpl=%u, dtl=%u\n",
		(fdc->dma ? "DMA" : "PIO"), fdc->command.dhs, fdc->command.chs.c, fdc->command.chs.h, fdc->command.chs.s, 
		fdc->command.n, fdc->command.eot, fdc->command.gap_len, fdc->command.data_len);

	command_set_async(fdc);
}
static void cmd_format_track(FDC* fdc) {

	/* Read command parameters */
	fdc->command.dhs = ring_buffer_pop(&fdc->data_register_in);     /* HD/US1/US0 */
	fdc->command.n = ring_buffer_pop(&fdc->data_register_in);       /* N */
	uint8_t sc = ring_buffer_pop(&fdc->data_register_in);   /* SC */
	fdc->command.gap_len = ring_buffer_pop(&fdc->data_register_in); /* GPL */
	uint8_t d = ring_buffer_pop(&fdc->data_register_in);    /* D */

	/* Load drive select */
	fdc->fdd_select = fdc->command.dhs & US_MASK;

	if (fdc->dor & DOR_DMA_INT_MASK) {
		fdc->msr &= ~MSR_RQM; /* Clear RQM; Dont let CPU poll */
	}
	else {
		fdc->msr |= MSR_RQM; /* Set RQM; Let CPU poll */
	}

	fdc->sector_size = decode_sector_size(fdc->command.n);
	fdc->byte_index = 0;

	dbg_print("[FDC] Format track %s dhs=%u, n=%u, sc=%u, gpl=%u, d=%u\n",
		(fdc->dma ? "DMA" : "PIO"), fdc->command.dhs, fdc->command.n, sc, fdc->command.gap_len, d);

	command_set_async(fdc);
}
static void cmd_write_deleted_data(FDC* fdc) {

	/* Read command parameters */
	fdc->command.dhs = ring_buffer_pop(&fdc->data_register_in);      /* HD/US1/US0 */
	fdc->command.chs.c = ring_buffer_pop(&fdc->data_register_in);    /* C */
	fdc->command.chs.h = ring_buffer_pop(&fdc->data_register_in);    /* H */
	fdc->command.chs.s = ring_buffer_pop(&fdc->data_register_in);    /* R */
	fdc->command.n = ring_buffer_pop(&fdc->data_register_in);        /* N */
	fdc->command.eot = ring_buffer_pop(&fdc->data_register_in);      /* EOT */
	fdc->command.gap_len = ring_buffer_pop(&fdc->data_register_in);  /* GPL */
	fdc->command.data_len = ring_buffer_pop(&fdc->data_register_in); /* DTL */

	/* Load drive select */
	fdc->fdd_select = fdc->command.dhs & US_MASK;

	if (fdc->dor & DOR_DMA_INT_MASK) {
		fdc->msr &= ~MSR_RQM; /* Clear RQM; Dont let CPU poll */
	}
	else {
		fdc->msr |= MSR_RQM; /* Set RQM; Let CPU poll */
	}

	/* Send command results */
	command_results(fdc, ST0_NT, NO_IRQ);

	dbg_print("[FDC] Write deleted data %s dhs=%u, c=%u, h=%u, s=%u, n=%u, eot=%u, gpl=%u, dtl=%u\n",
		(fdc->dma ? "DMA" : "PIO"), fdc->command.dhs, fdc->command.chs.c, fdc->command.chs.h, fdc->command.chs.s, 
		fdc->command.n, fdc->command.eot, fdc->command.gap_len, fdc->command.data_len);
}

static void cmd_scan_e(FDC* fdc) {

	/* Read command parameters */
	fdc->command.dhs = ring_buffer_pop(&fdc->data_register_in);     /* HD/US1/US0 */
	fdc->command.chs.c = ring_buffer_pop(&fdc->data_register_in);   /* C */
	fdc->command.chs.h = ring_buffer_pop(&fdc->data_register_in);   /* H */
	fdc->command.chs.s = ring_buffer_pop(&fdc->data_register_in);   /* R */
	fdc->command.n = ring_buffer_pop(&fdc->data_register_in);       /* N */
	fdc->command.eot = ring_buffer_pop(&fdc->data_register_in);     /* EOT */
	fdc->command.gap_len = ring_buffer_pop(&fdc->data_register_in); /* GPL */
	uint8_t stp = ring_buffer_pop(&fdc->data_register_in);          /* STP */
	(void)stp;

	/* Send command results */
	command_results(fdc, ST0_NT, NO_IRQ);

	dbg_print("[FDC] scan e (NOT IMPLEMENTED)\n");
}
static void cmd_scan_le(FDC* fdc) {

	/* Read command parameters */
	fdc->command.dhs = ring_buffer_pop(&fdc->data_register_in);     /* HD/US1/US0 */
	fdc->command.chs.c = ring_buffer_pop(&fdc->data_register_in);   /* C */
	fdc->command.chs.h = ring_buffer_pop(&fdc->data_register_in);   /* H */
	fdc->command.chs.s = ring_buffer_pop(&fdc->data_register_in);   /* R */
	fdc->command.n = ring_buffer_pop(&fdc->data_register_in);       /* N */
	fdc->command.eot = ring_buffer_pop(&fdc->data_register_in);     /* EOT */
	fdc->command.gap_len = ring_buffer_pop(&fdc->data_register_in); /* GPL */
	uint8_t stp = ring_buffer_pop(&fdc->data_register_in);          /* STP */
	(void)stp;

	/* Send command results */
	command_results(fdc, ST0_NT, NO_IRQ);

	dbg_print("[FDC] scan le (NOT IMPLEMENTED)\n");
}
static void cmd_scan_he(FDC* fdc) {

	/* Read command parameters */
	fdc->command.dhs = ring_buffer_pop(&fdc->data_register_in);     /* HD/US1/US0 */
	fdc->command.chs.c = ring_buffer_pop(&fdc->data_register_in);   /* C */
	fdc->command.chs.h = ring_buffer_pop(&fdc->data_register_in);   /* H */
	fdc->command.chs.s = ring_buffer_pop(&fdc->data_register_in);   /* R */
	fdc->command.n = ring_buffer_pop(&fdc->data_register_in);       /* N */
	fdc->command.eot = ring_buffer_pop(&fdc->data_register_in);     /* EOT */
	fdc->command.gap_len = ring_buffer_pop(&fdc->data_register_in); /* GPL */
	uint8_t stp = ring_buffer_pop(&fdc->data_register_in);          /* STP */
	(void)stp;

	/* Send command results */
	command_results(fdc, ST0_NT, NO_IRQ);

	dbg_print("[FDC] scan he (NOT IMPLEMENTED)\n");
}

static void cmd_recalibrate(FDC* fdc) {

	/* Read command parameters */
	fdc->command.dhs = ring_buffer_pop(&fdc->data_register_in); /* US1/US0 */

	/* Load drive select */
	fdc->fdd_select = fdc->command.dhs & US_MASK;

	/* Seek to cylinder 0 */
	fdc->command.chs.c = 0;

	/* Set ST0 */
	st0(fdc, ST0_NT, ST0_SE);

	/* Finish command */
	command_reset(fdc, IRQ, RECEIVE_DATA);

	dbg_print("[FDC] recalibrate\n");
}
static void cmd_seek(FDC* fdc) {

	/* Read command parameters */
	fdc->command.dhs = ring_buffer_pop(&fdc->data_register_in);   /* HD/US1/US0 */
	fdc->command.chs.c = ring_buffer_pop(&fdc->data_register_in); /* NCN */

	/* Load drive select */
	fdc->fdd_select = fdc->command.dhs & US_MASK;

	/* Set ST0 */
	if (fdc->command.chs.c < fdc->fdd[fdc->fdd_select].disk_descriptor.c) {
		st0(fdc, ST0_NT, ST0_SE);
	}
	else {
		st0(fdc, ST0_AT, 0);
	}
		
	/* Finish command */
	command_reset(fdc, IRQ, RECEIVE_DATA);

	dbg_print("[FDC] seek\n");
}

static void cmd_sense_interrupt(FDC* fdc) {

	/* Send command results */
	ring_buffer_push(&fdc->data_register_out, fdc->st0);           /* ST0 */
	ring_buffer_push(&fdc->data_register_out, fdc->command.chs.c); /* PCN */

	/* Finish command */
	command_reset(fdc, NO_IRQ, SEND_DATA);

	dbg_print("[FDC] sense interrupt\n");
}
static void cmd_sense_drive_status(FDC* fdc) {

	/* Read command parameters */
	fdc->command.dhs = ring_buffer_pop(&fdc->data_register_in); /* HD/US1/US0 */

	/* Load drive select */
	fdc->fdd_select = fdc->command.dhs & US_MASK;

	/* Set ST3 */
	st3(fdc);

	/* Send command results */
	ring_buffer_push(&fdc->data_register_out, fdc->st3); /* ST3 */

	/* Finish command */
	command_reset(fdc, NO_IRQ, SEND_DATA);

	dbg_print("[FDC] sense drive status\n");
}

static void cmd_read_id(FDC* fdc) {

	/* Read command parameters */
	fdc->command.dhs = ring_buffer_pop(&fdc->data_register_in); /* HD/US1/US0 */

	/* Load drive select */
	fdc->fdd_select = fdc->command.dhs & US_MASK;

	dbg_print("[FDC] read id dhs=%u\n", fdc->command.dhs);

	/* Send command results */
	command_results(fdc, ST0_NT, IRQ);

	/* Advance CHS to next sector */
	chs_advance(fdc->fdd[fdc->fdd_select].disk_descriptor, &fdc->command.chs);
}
static void cmd_specify(FDC* fdc) {

	/* Read command parameters */
	uint8_t tmp1 = ring_buffer_pop(&fdc->data_register_in); /* SRT/HUT (Step Rate Time/Head Unload Time)*/
	uint8_t tmp2 = ring_buffer_pop(&fdc->data_register_in); /* HLT/ND (Head Load Time/Non-DMA Mode)*/

	uint8_t srt = (tmp1 & 0xF0) >> 4;
	uint8_t hut = (tmp1 & 0x0F);
	uint8_t hlt = (tmp2 & 0xFE) >> 1;
	uint8_t nd = tmp2 & 0x1;

	(void)srt;
	(void)hut;
	(void)hlt;

	/* Set DMA/PIO mode */
	if (nd) {
		fdc->dma = 0;
	}
	else {
		fdc->dma = 1;
	}

	/* Finish command */
	command_reset(fdc, NO_IRQ, RECEIVE_DATA);

	dbg_print("[FDC] specify\n");
	/*dbg_print("[FDC] Head Load Time:   %d\n", hlt);
	dbg_print("[FDC] Head Unload Time: %d\n", hut);
	dbg_print("[FDC] Step Rate Time:   %d\n", srt);
	dbg_print("[FDC] Non-DMA:          %d (%d)\n", !nd, fdc->dma);*/
}
static void cmd_nop(FDC* fdc) {
	
	/* Set ST0 */
	fdc->st0 = ST0_IC;

	/* Send command results */
	ring_buffer_push(&fdc->data_register_out, fdc->st0); /* ST0 */

	/* Finish command */
	command_reset(fdc, NO_IRQ, SEND_DATA);

	dbg_print("[FDC] nop %02X\n", fdc->command.byte & CMD_BYTE);
}

/* Asynchronous commands */
static void cmd_read_data_async(FDC* fdc) {
	if (!fdc->dma) {
		dbg_print("[FDC] Read data. PIO mode not implemented\n");
		/* Send command results */
		command_results(fdc, ST0_AT, IRQ);
		return;
	}

	/* Check if the fdd ready signal changed state */
	if (!fdc->fdd[fdc->fdd_select].status.ready) {
		/* Send command results */
		command_results(fdc, ST0_AT2, IRQ);
		return;
	}

	if (!i8237_dma_terminal_count(fdc->dma_p, FDC_DMA)) {

		if (i8237_dma_channel_ready(fdc->dma_p, FDC_DMA)) {

			/* Read data from fdd */
			LBA lba = chs_to_lba(fdc->fdd[fdc->fdd_select].disk_descriptor, fdc->command.chs);
			uint32_t offset = (lba * fdc->sector_size) + fdc->byte_index;
			uint8_t byte = fdd_read_byte(&fdc->fdd[fdc->fdd_select], offset);

			/* Write data to dma */
			i8237_dma_write_byte(fdc->dma_p, FDC_DMA, byte);

			if (fdc->byte_index == 0) {
				//dbg_print("[FDC] Reading data FDD%d C=%d H=%d S=%d -> LBA=%u -> OFFSET=%u\n", fdc->fdd_select, fdc->chs.c, fdc->chs.h, fdc->chs.s, lba, offset);
			}

			fdc->byte_index += 1;
			if (fdc->byte_index >= fdc->sector_size) {
				/* Finished sector */
				fdc->byte_index = 0;

				/* Advance CHS to next sector */
				chs_advance(fdc->fdd[fdc->fdd_select].disk_descriptor, &fdc->command.chs);
			}
		}
	}
	else {

		//dbg_print("[FDC] Read data completed. C=%u H=%u S=%u\n", fdc->chs.c, fdc->chs.h, fdc->chs.s);

		fdc->byte_index = 0;

		/* Send command results */
		command_results(fdc, ST0_NT, IRQ);
	}
}
static void cmd_read_track_async(FDC* fdc) {
	if (!fdc->dma) {
		dbg_print("[FDC] Read track. PIO mode not implemented\n");
		/* Send command results */
		command_results(fdc, ST0_AT, IRQ);
		return;
	}

	/* Check if the fdd ready signal changed state */
	if (!fdc->fdd[fdc->fdd_select].status.ready) {
		/* Send command results */
		command_results(fdc, ST0_AT2, IRQ);
		return;
	}

	if (!i8237_dma_terminal_count(fdc->dma_p, FDC_DMA)) {

		if (i8237_dma_channel_ready(fdc->dma_p, FDC_DMA)) {

			/* Read data from fdd */
			LBA lba = chs_to_lba(fdc->fdd[fdc->fdd_select].disk_descriptor, fdc->command.chs);
			uint32_t offset = (lba * fdc->sector_size) + fdc->byte_index;
			uint8_t byte = fdd_read_byte(&fdc->fdd[fdc->fdd_select], offset);

			/* Write data to dma */
			i8237_dma_write_byte(fdc->dma_p, FDC_DMA, byte);

			if (fdc->byte_index == 0) {
				//dbg_print("[FDC] Reading track FDD%d C=%d H=%d S=%d -> LBA=%u -> OFFSET=%u\n", fdc->fdd_select, fdc->chs.c, fdc->chs.h, fdc->chs.s, lba, offset);
			}

			fdc->byte_index += 1;
			if (fdc->byte_index >= fdc->sector_size) {
				/* Finished sector */
				fdc->byte_index = 0;

				/* Advance CHS to next sector */
				chs_advance(fdc->fdd[fdc->fdd_select].disk_descriptor, &fdc->command.chs);
			}
		}
	}
	else {

		//dbg_print("[FDC] Read track completed. C=%u H=%u S=%u\n", fdc->chs.c, fdc->chs.h, fdc->chs.s);

		fdc->byte_index = 0;

		/* Send command results */
		command_results(fdc, ST0_NT, IRQ);
	}
}
static void cmd_write_data_async(FDC* fdc) {
	if (!fdc->dma) {
		dbg_print("[FDC] Write data. PIO mode not implemented\n");
		/* Send command results */
		command_results(fdc, ST0_AT, IRQ);
		return;
	}

	/* Check if the fdd ready signal changed state */
	if (!fdc->fdd[fdc->fdd_select].status.ready) {
		/* Send command results */
		command_results(fdc, ST0_AT2, IRQ);
		return;
	}

	/* check if the fdd write protect signal is active */
	if (fdc->fdd[fdc->fdd_select].status.write_protect) {
		/* Send command results */
		command_results(fdc, ST0_AT, IRQ);
		return;
	}

	if (!i8237_dma_terminal_count(fdc->dma_p, FDC_DMA)) {

		if (i8237_dma_channel_ready(fdc->dma_p, FDC_DMA)) {

			/* Read data from dma */
			uint8_t byte = i8237_dma_read_byte(fdc->dma_p, FDC_DMA);

			/* Write data to fdd */
			LBA lba = chs_to_lba(fdc->fdd[fdc->fdd_select].disk_descriptor, fdc->command.chs);
			uint32_t offset = (lba * fdc->sector_size) + fdc->byte_index;
			fdd_write_byte(&fdc->fdd[fdc->fdd_select], offset, byte);

			if (fdc->byte_index == 0) {
				//dbg_print("[FDC] Writing data FDD%d C=%d H=%d S=%d -> LBA=%u -> OFFSET=%u\n", fdc->fdd_select, fdc->chs.c, fdc->chs.h, fdc->chs.s, lba, offset);
			}

			fdc->byte_index += 1;
			if (fdc->byte_index >= fdc->sector_size) {
				/* Finished sector */
				fdc->byte_index = 0;

				/* Advance CHS to next sector */
				chs_advance(fdc->fdd[fdc->fdd_select].disk_descriptor, &fdc->command.chs);
			}
		}
	}
	else {

		//dbg_print("[FDC] Write data completed, C=%u H=%u S=%u\n", fdc->chs.c, fdc->chs.h, fdc->chs.s);

		fdc->byte_index = 0;

		/* Send command results */
		command_results(fdc, ST0_NT, IRQ);
	}
}
static void cmd_format_track_async(FDC* fdc) {
	if (!fdc->dma) {
		dbg_print("[FDC] Write track. PIO mode not implemented\n");
		/* Send command results */
		command_results(fdc, ST0_AT, IRQ);
		return;
	}

	/* Check if the fdd ready signal changed state */
	if (!fdc->fdd[fdc->fdd_select].status.ready) {
		/* Send command results */
		command_results(fdc, ST0_AT2, IRQ);
		return;
	}

	/* Check if the fdd write protect signal is active */
	if (fdc->fdd[fdc->fdd_select].status.write_protect) {
		/* Send command results */
		command_results(fdc, ST0_AT, IRQ);
		return;
	}

	if (!i8237_dma_terminal_count(fdc->dma_p, FDC_DMA)) {

		if (i8237_dma_channel_ready(fdc->dma_p, FDC_DMA)) {

			/* Read data from dma */
			uint8_t byte = i8237_dma_read_byte(fdc->dma_p, FDC_DMA);

			/* Write data to fdd */
			LBA lba = chs_to_lba(fdc->fdd[fdc->fdd_select].disk_descriptor, fdc->command.chs);
			uint32_t offset = (lba * fdc->sector_size) + fdc->byte_index;
			fdd_write_byte(&fdc->fdd[fdc->fdd_select], offset, byte);

			if (fdc->byte_index == 0) {
				//dbg_print("[FDC] Writing track FDD%d C=%d H=%d S=%d -> LBA=%u -> OFFSET=%u\n", fdc->fdd_select, fdc->chs.c, fdc->chs.h, fdc->chs.s, lba, offset);
			}

			fdc->byte_index += 1;
			if (fdc->byte_index >= fdc->sector_size) {
				/* Finished sector */
				fdc->byte_index = 0;

				/* Advance CHS to next sector */
				chs_advance(fdc->fdd[fdc->fdd_select].disk_descriptor, &fdc->command.chs);
			}
		}
	}
	else {

		//dbg_print("[FDC] Write track completed, C=%u H=%u S=%u\n", fdc->chs.c, fdc->chs.h, fdc->chs.s);

		fdc->byte_index = 0;

		/* Send command results */
		command_results(fdc, ST0_NT, IRQ);
	}
}

/* Execute command */
static void command_execute(FDC* fdc) {
	switch (fdc->command.byte & CMD_BYTE) {
		case CMD_READ_DATA:
			cmd_read_data(fdc);
			break;
		case CMD_READ_TRACK:
			cmd_read_track(fdc);
			break;
		case CMD_READ_DELETED_DATA:
			cmd_read_deleted_data(fdc);
			break;
		case CMD_READ_ID:
			cmd_read_id(fdc);
			break;
		case CMD_WRITE_DATA:
			cmd_write_data(fdc);
			break;
		case CMD_FORMAT_TRACK:
			cmd_format_track(fdc);
			break;
		case CMD_WRITE_DELETED_DATA:
			cmd_write_deleted_data(fdc);
			break;
		case CMD_SCAN_EQUAL:
			cmd_scan_e(fdc);
			break;
		case CMD_SCAN_LOW_OR_EQUAL:
			cmd_scan_le(fdc);
			break;
		case CMD_SCAN_HIGH_OR_EQUAL:
			cmd_scan_he(fdc);
			break;
		case CMD_RECALIBRATE:
			cmd_recalibrate(fdc);
			break;
		case CMD_SEEK:
			cmd_seek(fdc);
			break;
		case CMD_SENSE_DRIVE_STATUS:
			cmd_sense_drive_status(fdc);
			break;
		case CMD_SENSE_INTERRUPT:
			cmd_sense_interrupt(fdc);
			break;
		case CMD_SPECIFY:
			cmd_specify(fdc);
			break;
		default:
			cmd_nop(fdc);
			break;
	}
}
static void command_execute_async(FDC* fdc) {
	switch (fdc->command.byte & CMD_BYTE) {
		case CMD_READ_DATA:
			cmd_read_data_async(fdc);
			break;
		case CMD_READ_TRACK:
			cmd_read_track_async(fdc);
			break;
		case CMD_WRITE_DATA:
			cmd_write_data_async(fdc);
			break;
		case CMD_FORMAT_TRACK:
			cmd_format_track_async(fdc);
			break;
	}
}

static void write_dor(FDC* fdc, uint8_t v) {
	/* The Digital Output Register (DOR) is an output only register used to
	 control drive motors, drive selection, and feature enable. All bits are
	 cleared by the I/O interface reset line */

#ifdef DBG_PRINT
	 //dbg_print("[FDC] WRITE DOR=%02X\n", v);

	if (IS_RISING_EDGE(DOR_ENABLE, fdc->dor, v)) {
		dbg_print("[FDC] DOR ENABLE FDC\n");
	}
	else if (IS_FALLING_EDGE(DOR_ENABLE, fdc->dor, v)) {
		dbg_print("[FDC] DOR DISABLE FDC\n");
	}

	if (IS_RISING_EDGE(DOR_DMA_INT_MASK, fdc->dor, v)) {
		dbg_print("[FDC] DOR ENABLE DMA/INT\n");
	}
	else if (IS_FALLING_EDGE(DOR_DMA_INT_MASK, fdc->dor, v)) {
		dbg_print("[FDC] DOR DISABLE DMA/INT\n");
	}

	if (HAS_BITS_CHANGED(DOR_FDD_SELECT_MASK, fdc->dor, v)) {
		dbg_print("[FDC] DOR SELECT FDD%d\n", v & DOR_FDD_SELECT_MASK);
	}

	if (HAS_BITS_CHANGED(DOR_FDD_MOTOR_ON_MASK, fdc->dor, v)) {
		for (int i = 0; i < FDD_MAX; ++i) {
			if (HAS_BITS_CHANGED(1 << (4 + i), fdc->dor, v)) {
				dbg_print("[FDC] DOR MOTOR %s FDD%d\n", (v & (1 << (4 + i))) ? "ON" : "OFF", i);
			}
		}
	}
#endif

	/* The FDC is held reset when bit 2 is clear. It
		must be set by the program to enable the FDC */
	if (IS_RISING_EDGE(DOR_ENABLE, fdc->dor, v)) {
		cmd_reset(fdc);
	}

	/* Turn drive motors on/off */
	for (int i = 0; i < FDD_MAX; ++i) {
		fdc->fdd[i].status.motor_on = (v >> (4 + i)) & 0x1;

		/* We assert/deassert the ready signal here. if theres no disk present
		   and the motor is on, then inserting a disk will assert the ready signal. */
		if (fdc->fdd[i].status.motor_on && fdc->fdd[i].status.inserted) {
			fdc->fdd[i].status.ready = 1;
		}
		else {
			fdc->fdd[i].status.ready = 0;
		}
	}

	/* select drive */
	fdc->fdd_select = (v & DOR_FDD_SELECT_MASK);

	fdc->dor = v;
}
static void write_data(FDC* fdc, uint8_t value) {

	if (!fdc->command.recieving) {
		command_set(fdc, value);
	}
	else {
		command_set_parameter(fdc, value);
	}

	if (fdc->command.received) {
		command_execute(fdc);
	}
}
static uint8_t read_data(FDC* fdc) {
	if (!ring_buffer_is_empty(&fdc->data_register_out)) {
		uint8_t data = ring_buffer_pop(&fdc->data_register_out);
		if (ring_buffer_is_empty(&fdc->data_register_out)) {
			/* FDC is ready to receive data */
			receive_data(fdc);
		}
		return data;
	}
	return 0; /* shouldnt hit */
}
static uint8_t read_msr(FDC* fdc) {
	//dbg_print("[FDC] READ MSR=%02X\n", fdc->msr);
	return fdc->msr;
}

int upd765_fdc_create(FDC* fdc) {
	if (ring_buffer_create(&fdc->data_register_out, 10)) {
		dbg_print("[FDC] Failed to allocate data register out ring buffer\n");
		return 1;
	}
	if (ring_buffer_create(&fdc->data_register_in, 10)) {
		dbg_print("[FDC] Failed to allocate data register in ring buffer\n");
		return 1;
	}

	for (uint8_t i = 0; i < FDD_MAX; ++i) {
		fdc->fdd[i].path = calloc(1, 256);
		if (fdc->fdd[i].path == NULL) {
			dbg_print("[FDC] Failed to allocate path buffer\n");
			return 1;
		}
	}

	return 0;
}
void upd765_fdc_destroy(FDC* fdc) {
	ring_buffer_destroy(&fdc->data_register_out);
	ring_buffer_destroy(&fdc->data_register_in);

	for (uint8_t i = 0; i < FDD_MAX; ++i) {
		fdd_eject_disk(&fdc->fdd[i]);
		if (fdc->fdd[i].path != NULL) {
			free(fdc->fdd[i].path);
			fdc->fdd[i].path = NULL;
		}
	}
}

void upd765_fdc_reset(FDC* fdc) {
	fdc->msr = 0;
	fdc->dor = 0;
	fdc->fdd_select = 0;

	fdc->st0 = 0;
	fdc->st1 = 0;
	fdc->st2 = 0;
	fdc->st3 = 0;

	fdc->command.byte = 0;
	fdc->command.param_count = 0;
	fdc->command.recieving = 0;
	fdc->command.received = 0;
	fdc->command.async = 0;
	fdc->command.error = 0;

	fdc->sector_size = 0;
	fdc->byte_index = 0;

	ring_buffer_reset(&fdc->data_register_out);
	ring_buffer_reset(&fdc->data_register_in);
}

uint8_t upd765_fdc_read_io_byte(FDC* fdc, uint8_t address) {
	switch (address) {
		case PORT_STATUS_REGISTER:
			return read_msr(fdc);
		case PORT_DATA_REGISTER:
			return read_data(fdc);
		default:
			dbg_print("[FDC] read byte %x\n", address);
			break;
	}
	return 0;
}
void upd765_fdc_write_io_byte(FDC* fdc, uint8_t address, uint8_t value) {
	switch (address) {
		case PORT_DIGITAL_OUTPUT: // (WO)
			write_dor(fdc, value);
			break;
		case PORT_DATA_REGISTER:
			write_data(fdc, value);
			break;
		default:
			dbg_print("[FDC] write byte %x\n", address);
			break;
	}
}

void upd765_fdc_update(FDC* fdc) {
	if (fdc->command.async) {
		command_execute_async(fdc);
	}
}
