﻿/* ibm_pc.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * IBM PC Implementation
 */

#include <stdint.h>
#include <stdlib.h>
#include <memory.h>

#include "ibm_pc.h"

#include "i8086.h"
#include "i8086_mnem.h"
#include "io/memory_map.h"
#include "io/isa_bus.h"
#include "chipset/i8253_pit.h"
#include "chipset/i8255_ppi.h"
#include "chipset/i8259_pic.h"
#include "chipset/nmi.h"
#include "fdd/fdc.h"
#include "video/mda.h"
#include "video/cga.h"

#include "isa_cards/mda_isa_card.h"
#include "isa_cards/cga_isa_card.h"

#include "bit_utils.h"

#define MEM_SIZE 0x100000

/* PORTS */

#define NMI_BASE_ADDRESS 0xA0 // Base port address of the NMI Logic Controller
#define NMI_ENABLE_INT (NMI_BASE_ADDRESS + 0) // Enable/Disable interrupt

#define PIC_BASE_ADDRESS 0x20 // Base port address of the PIC
#define PIC_PORT_A (PIC_BASE_ADDRESS + 0) // Port A
#define PIC_PORT_B (PIC_BASE_ADDRESS + 1) // Port B

#define PIT_BASE_ADDRESS 0x40  // Base port address of the PIT
#define PIT_PORT_A    (PIT_BASE_ADDRESS + 0) // Port A
#define PIT_PORT_B    (PIT_BASE_ADDRESS + 1) // Port C
#define PIT_PORT_C    (PIT_BASE_ADDRESS + 2) // Port C
#define PIT_PORT_CTRL (PIT_BASE_ADDRESS + 3) // Command Port

#define PPI_BASE_ADDRESS 0x60  // Base port address of the PPI
#define PPI_PORT_A   (PPI_BASE_ADDRESS + 0) // Port A
#define PPI_PORT_B   (PPI_BASE_ADDRESS + 1) // Port B
#define PPI_PORT_C   (PPI_BASE_ADDRESS + 2) // Port C
#define PPI_CONTROL  (PPI_BASE_ADDRESS + 3) // Control Port

#define FDC_BASE_ADDRESS 0x3F0 // Base port address of the FDC
#define FDC_DIGITAL_OUTPUT  (FDC_BASE_ADDRESS + 2) // (RW)
#define FDC_MAIN_STATUS     (FDC_BASE_ADDRESS + 4) // (RO)
#define FDC_DATA_FIFO       (FDC_BASE_ADDRESS + 5) // (RW)

/* PPI Port B */
#define PORTB_TIMER2_GATE        0x01 // b0 - Turn on timer2; on = 1, off = 0
#define PORTB_SPEAKER_DATA       0x02 // b1 - Turn on speaker; on = 1, off = 0 
#define PORTB_READ_SW2_KEY       0x04 // b2 - Enable read SW2 or scan key; enable SW2 read = 1, enable scan key read = 0
#define PORTB_CASSETTE_MOTOR_OFF 0x08 // b3 - Turn on LED; on = 1, off = 0 
#define PORTB_LED_ON             0x08 // b3 - Turn on LED; on = 1, off = 0 
#define PORTB_KB_ENABLE          0x40 // b6 - hold keyboard low
#define PORTB_READ_SW1_KB        0x80 // b7 - Enable read SW1 or scan code; enable SW1 read = 1, enable scan code read = 0

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

/* PIT IRQ */
enum {
	IRQ_TIMER0 = 0x00,
	IRQ_KBD    = 0x01,
	IRQ_VID    = 0x02, // EGA vertical retrace (arrives via IRQ 9 on MODEL_5170)
	IRQ_SLAVE  = 0x02, // MODEL_5170
	IRQ_COM2   = 0x03,
	IRQ_COM1   = 0x04,
	IRQ_XTC    = 0x05, // MODEL_5160 uses IRQ 5 for HDC (XTC version)
	IRQ_LPT2   = 0x05, // MODEL_5170 uses IRQ 5 for LPT2
	IRQ_FDC    = 0x06,
	IRQ_LPT1   = 0x07,
	IRQ_RTC    = 0x08, // MODEL_5170
	IRQ_IRQ2   = 0x09, // MODEL_5170
	IRQ_FPU    = 0x0D, // MODEL_5170
	IRQ_ATC1   = 0x0E, // MODEL_5170 uses IRQ 14 for primary ATC controller interrupts
	IRQ_ATC2   = 0x0F  // MODEL_5170 *can* use IRQ 15 for secondary ATC controller interrupts
};

/* IBM PC global variable */
IBM_PC* ibm_pc = NULL;

uint8_t determine_planar_ram_sw(uint20_t planar_ram) {
	// Convert planar ram amount to sw1
	uint8_t sw = 0;
	switch (ibm_pc->config.model) {
		case MODEL_5150_16_64:
			sw = (planar_ram >> 12) & 0xFF;
			break;
		case MODEL_5150_64_256:
			sw = (planar_ram >> 14) & 0xFF;
			break;
	}

	sw -= 4;
	sw &= 0x0C;
	return sw;
}
uint20_t determine_planar_ram_size(uint8_t sw1) {
	// Convert sw1 to planar ram amount
	// FE169 (6025005 AUG81)
	// FE167 (6322507 APR84)
	
	uint20_t planar_ram = 0;
	planar_ram = sw1;             // in al, PORT_A ; DETERMINE BASE RAM SIZE
	planar_ram &= 0x0C;           // and al, 0CH   ; ISOLATE RAM SIZE SWS
	planar_ram += 4;              // add al, 4     ; CALCULATE MEMORY SIZE
	
	switch (ibm_pc->config.model) {
		case MODEL_5150_16_64:
			planar_ram <<= 12;
			break;
		case MODEL_5150_64_256:
			planar_ram <<= 14;
			break;
	}
	return planar_ram;
}

uint8_t determine_io_ram_sw(uint20_t planar_ram, uint20_t io_ram) {
	// Convert io ram amount to sw2
	uint8_t sw = 0;
	switch (ibm_pc->config.model) {
		case MODEL_5150_16_64:
			sw = (io_ram / 1024 / 32) & 0x1F;
			break;
		case MODEL_5150_64_256:
			if (planar_ram >= 64 * 1024) {
				planar_ram -= 64 * 1024;
			}
			sw = ((io_ram + planar_ram) / 1024 / 32) & 0x1F;
			break;
	}
	return sw;
}
uint20_t determine_io_ram_size(uint8_t sw1, uint8_t sw2) {
	// Convert sw2 to io ram amount
	uint20_t io_ram = 0;
	uint20_t planar_ram = 0;
	switch (ibm_pc->config.model) {
		case MODEL_5150_16_64:
			io_ram = (sw2 & 0x1F) * 1024 * 32;
			break;
		case MODEL_5150_64_256:
			planar_ram = determine_planar_ram_size(sw1);
			if (planar_ram > 64 * 1024) {
				planar_ram -= 64 * 1024;
			}
			else {
				planar_ram = 0;
			}
			io_ram = (sw2 & 0x1F) * 1024 * 32;
			if (io_ram >= planar_ram) {
				io_ram -= planar_ram;
			}
			break;
	}
	return io_ram;
}

void cal_planar_io_ram_from_total(void) {
	/* Split total ram into planar ram and io channel ram */

	uint32_t max_planar_memory = 64 * 1024;
	if (ibm_pc->config.total_memory >= max_planar_memory) {
		ibm_pc->config.base_memory = max_planar_memory;
		ibm_pc->config.extended_memory = ibm_pc->config.total_memory - max_planar_memory;
	}
	else {
		ibm_pc->config.base_memory = ibm_pc->config.total_memory;
		ibm_pc->config.extended_memory = 0;
	}
}

void ibm_pc_set_sw1(void) {
	/* Set sw1 based on PC Config */
	if (!ibm_pc->config.sw1_provided) {
		ibm_pc->config.sw1 = 0;
		ibm_pc->config.sw1 |= determine_planar_ram_sw(ibm_pc->config.base_memory);
		ibm_pc->config.sw1 |= ibm_pc->config.video_adapter & SW1_DISPLAY_MASK;

		if (ibm_pc->config.fdc_disks > 0) {
			ibm_pc->config.sw1 |= SW1_HAS_FDC;

			if (ibm_pc->config.fdc_disks <= 4) {
				ibm_pc->config.sw1 |= ((ibm_pc->config.fdc_disks - 1) & 0x03) << 6; /* SW1_DISKS_X */
			}
		}
	}
}
void ibm_pc_set_sw2(void) {
	/* Set sw2 based on PC Config */
	if (!ibm_pc->config.sw2_provided) {
		ibm_pc->config.sw2 = determine_io_ram_sw(ibm_pc->config.base_memory, ibm_pc->config.extended_memory);
	}
}

void ibm_pc_set_config(void) {
	/* Set PC Config based on sw1,sw2  */
	if (ibm_pc->config.model == MODEL_5150_16_64) {
		dbg_print("Model: 5150 16-64KB\n");
	}
	else if (ibm_pc->config.model == MODEL_5150_64_256) {
		dbg_print("Model: 5150 64-256KB\n");
	}

	if (ibm_pc->config.sw1 & SW1_HAS_FDC) {
		ibm_pc->config.fdc_disks = ((ibm_pc->config.sw1 & SW1_DISKS_MASK) >> 6) + 1;
	}
	else {
		ibm_pc->config.fdc_disks = 0;
	}

	ibm_pc->config.base_memory = determine_planar_ram_size(ibm_pc->config.sw1);
	ibm_pc->config.extended_memory = determine_io_ram_size(ibm_pc->config.sw1, ibm_pc->config.sw2);
	ibm_pc->config.total_memory = ibm_pc->config.base_memory + ibm_pc->config.extended_memory;

	dbg_print("Planar RAM: %u Kb\n", ibm_pc->config.base_memory / 1024);
	dbg_print("IO RAM:     %u Kb\n", ibm_pc->config.extended_memory / 1024);
	dbg_print("Total RAM:  %u Kb\n", ibm_pc->config.total_memory / 1024);

	MEMORY_REGION* region = memory_map_get_mregion(&ibm_pc->mm, ibm_pc->ram_mregion_index);
	region->size = ibm_pc->config.total_memory;
}

/* I8086 Callbacks */
uint8_t read_mm_byte(uint20_t addr) {
	return memory_map_read_byte(&ibm_pc->mm, addr);
}
void write_mm_byte(uint20_t addr, uint8_t value) {
	memory_map_write_byte(&ibm_pc->mm, addr, value);
}
uint8_t read_io_byte(uint16_t port) {
	
	uint8_t v = 0;
	if (isa_bus_read_io_byte(&ibm_pc->isa_bus, port, &v)) {
		return v;
	}

	switch (port) {
		
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0A:
		case 0x0B:
		case 0x0C:
		case 0x0D:
		case 0x0E:
		case 0x0F:
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x87:
			return i8237_dma_read_io_byte(&ibm_pc->dma, (uint8_t)(port & 0xFF));

		case NMI_ENABLE_INT:
			return nmi_read_io_byte(&ibm_pc->nmi, (uint8_t)(port & ~NMI_BASE_ADDRESS));

		case PIC_PORT_A:
		case PIC_PORT_B:
			return i8259_pic_read_io_byte(&ibm_pc->pic, port & ~PIC_BASE_ADDRESS);

		case PIT_PORT_A:
		case PIT_PORT_B:
		case PIT_PORT_C:
		case PIT_PORT_CTRL:
			return i8253_pit_read(&ibm_pc->pit, port & ~PIT_BASE_ADDRESS);

		case PPI_PORT_A:
		case PPI_PORT_B:
		case PPI_PORT_C:
			return i8255_ppi_read_io_byte(&ibm_pc->ppi, port & ~PPI_BASE_ADDRESS);
		
		case FDC_MAIN_STATUS:
		case FDC_DATA_FIFO:
			return upd765_fdc_read_io_byte(&ibm_pc->fdc, (uint8_t)(port & ~FDC_BASE_ADDRESS));

		case 0x201: // Gamepad
			return 0x0;

		default:
			dbg_print("read byte from port: %04X\n", port);
			break;
	}
	return 0xFF;
}
void write_io_byte(uint16_t port, uint8_t value) {
	
	if (isa_bus_write_io_byte(&ibm_pc->isa_bus, port, value)) {
		return;
	}

	switch (port) {
		
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0A:
		case 0x0B:
		case 0x0C:
		case 0x0D:
		case 0x0E:
		case 0x0F:
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x87:
			i8237_dma_write_io_byte(&ibm_pc->dma, (uint8_t)(port & 0xFF), value);
			break;

		case NMI_ENABLE_INT:
			nmi_write_io_byte(&ibm_pc->nmi, (uint8_t)(port & ~NMI_BASE_ADDRESS), value);
			break;

		case PIC_PORT_A:
		case PIC_PORT_B:
			i8259_pic_write_io_byte(&ibm_pc->pic, (uint8_t)(port & ~PIC_BASE_ADDRESS), value);
			break;

		case PIT_PORT_A:
		case PIT_PORT_B:
		case PIT_PORT_C:
		case PIT_PORT_CTRL:
			i8253_pit_write(&ibm_pc->pit, (uint8_t)(port & ~PIT_BASE_ADDRESS), value);
			break;

		case PPI_PORT_A:
		case PPI_PORT_B:
		case PPI_PORT_C:
		case PPI_CONTROL:
			i8255_ppi_write_io_byte(&ibm_pc->ppi, (uint8_t)(port & ~PPI_BASE_ADDRESS), value);
			break;

		case FDC_DIGITAL_OUTPUT:
		case FDC_DATA_FIFO:
			upd765_fdc_write_io_byte(&ibm_pc->fdc, (uint8_t)(port & ~FDC_BASE_ADDRESS), value);
			break;

		default:
			dbg_print("write byte to port: %04X = %02X\n", port, value);
			break;
	}
}
uint16_t read_io_word(uint16_t port) {
	return (read_io_byte(port) | (read_io_byte(port + 1) << 8));
}
void write_io_word(uint16_t port, uint16_t value) {
	write_io_byte(port, value & 0xFF);
	write_io_byte(port+1, (value >> 8) & 0xFF);
}

/* PPI Callbacks */
uint8_t ppi_port_a_read(I8255_PPI* ppi) {
	/* Port A (read keyboard input, system sw1) */
	
	if (ppi->port_b & PORTB_READ_SW1_KB) {
		return ibm_pc->config.sw1;
	}
	else {
		return kbd_get_data(&ibm_pc->kbd);
	}
}
uint8_t ppi_port_b_read(I8255_PPI* ppi) {
	//dbg_print("[PPI] portb read\n");
	return ppi->port_b;
}
void ppi_port_b_write(I8255_PPI* ppi, uint8_t value) {
	/* Port B (device control register)

		PORTB-b7 0 enable keyboard read
				 1 clear keyboard and enable sense of SW1
		
		PORTB-b6 0 hold keyboard clock low, no shift reg. shifts
				 1 enable keyboard clock signal
		
		PORTB-b5 0 enable i/o check
				 1 disable i/o check
		
		PORTB-b4 0 enable r/w memory parity check
				 1 disable r/w parity check
		
		PORTB-b3 0 turn off LED
				 1 turn on LED (old cassettee motor off)
		
		PORTB-b2 0 read spare key
				 1 read r/w memory size (from Port C)
		
		PORTB-b1 0 turn off speaker
				 1 enable speaker data
		
		PORTB-b0 0 turn off timer 2
				 1 turn on timer 2, gate speaker with square wave */

	ibm_pc->timer2_gate  = (value & PORTB_TIMER2_GATE);

	if (IS_RISING_EDGE(PORTB_KB_ENABLE, ppi->port_b, value)) {
		kbd_set_clk(&ibm_pc->kbd, 1);
	}
	else if (IS_FALLING_EDGE(PORTB_KB_ENABLE, ppi->port_b, value)) {
		kbd_set_clk(&ibm_pc->kbd, 0);
	}

	if (IS_RISING_EDGE(PORTB_READ_SW1_KB, ppi->port_b, value)) {
		kbd_set_enable(&ibm_pc->kbd, 0);
	}
	if (IS_FALLING_EDGE(PORTB_READ_SW1_KB, ppi->port_b, value)) {
		kbd_set_enable(&ibm_pc->kbd, 1);
	}
}
uint8_t ppi_port_c_read(I8255_PPI* ppi) {
	/* PORT C (device output register, read spare key, system sw2) */
	
	if (ppi->port_b & PORTB_CASSETTE_MOTOR_OFF) {
		// Loopback mode
	}

	if (ppi->port_b & PORTB_READ_SW2_KEY) {
		return (ibm_pc->config.sw2 & 0x0F);
	}
	else {
		return ((ibm_pc->config.sw2 >> 4) & 0x01);
	}
}

static void pcspeaker_set(PC_SPEAKER* pc_speaker, uint8_t input) {
	pc_speaker->input = input;
}

/* PIT Callbacks */
static void pit_on_timer0(I8253_TIMER* timer) {
	/* PIT channel 0 is connected to the system timer interrupt line (IRQ0) on the i8259 PIC */

	switch (timer->ctrl & I8253_PIT_CTRL_MODE) {
		case I8253_PIT_MODE0: // interrupt on terminal count
			i8259_pic_request_interrupt(&ibm_pc->pic, IRQ_TIMER0);
			break;

		case I8253_PIT_MODE2: // rate generator
		case I8253_PIT_MODE6: // rate generator
			if (timer->out == 0) {
				i8259_pic_request_interrupt(&ibm_pc->pic, IRQ_TIMER0);
			}
			break;

		case I8253_PIT_MODE3: // square wave generator
		case I8253_PIT_MODE7: // square wave generator
			if (timer->out == 0) {
				i8259_pic_request_interrupt(&ibm_pc->pic, IRQ_TIMER0);
			}
			break;
	}
}
static void pit_on_timer1(I8253_TIMER* timer) {
	(void)timer;
}
static void pit_on_timer2(I8253_TIMER* timer) {
	/* PIT channel 2 is connected to the PC Speaker. */
	
	switch (timer->ctrl & I8253_PIT_CTRL_MODE) {
		case I8253_PIT_MODE0: // interrupt on terminal count
			pcspeaker_set(&ibm_pc->pc_speaker, 0);
			break;

		case I8253_PIT_MODE2: // rate generator
		case I8253_PIT_MODE6: // rate generator
			pcspeaker_set(&ibm_pc->pc_speaker, 0);
			break;

		case I8253_PIT_MODE3: // square wave generator
		case I8253_PIT_MODE7: // square wave generator
			pcspeaker_set(&ibm_pc->pc_speaker, timer->out);
			break;
	}
}

/* FDC Callbacks */
static void fdc_request_irq(FDC* fdc) {
	(void)fdc;
	i8259_pic_request_interrupt(&ibm_pc->pic, IRQ_FDC);
}

/* KBD Callbacks */
static void kbd_request_irq(KBD* kbd) {
	(void)kbd;
	i8259_pic_request_interrupt(&ibm_pc->pic, IRQ_KBD);
}
static void kbd_clear_irq(KBD* kbd) {
	(void)kbd;
	i8259_pic_clear_interrupt(&ibm_pc->pic, IRQ_KBD);
}

/* DMA Callbacks */
void dma_write_byte(uint8_t channel, uint8_t value) {
	i8237_dma_write_byte(&ibm_pc->dma, channel, value);
}
uint8_t dma_read_byte(uint8_t channel) {
	return i8237_dma_read_byte(&ibm_pc->dma, channel);
}

/* I8086 INTR Callbacks */
void i8086_assert_intr(uint8_t type) {
	ibm_pc->cpu.intr = 1;
	ibm_pc->cpu.intr_type = type;
}
void i8086_deassert_intr(void) {
	ibm_pc->cpu.intr = 0;
	ibm_pc->cpu.intr_type = 0;
}

/* Kbd */
static void kbd_update(void) {
	const uint64_t cycle_target = 35400;
	ibm_pc->kbd_accum += ibm_pc->cpu.cycles;
	while (ibm_pc->kbd_accum >= cycle_target) {
		ibm_pc->kbd_accum -= cycle_target;
		ibm_pc->kbd_cycles++;
		kbd_tick(&ibm_pc->kbd);
	}
}
static void dma_update(void) {
	/* dma cycles are 3/2 of cpu cycles */
	const uint64_t cycle_target = 2; // CPU cycles
	const uint64_t cycle_factor = 3; // factor

	ibm_pc->dma_accum += ibm_pc->cpu.cycles * cycle_factor;
	while (ibm_pc->dma_accum >= cycle_target) {
		ibm_pc->dma_accum -= cycle_target;
		ibm_pc->dma_cycles++;
		i8237_dma_update(&ibm_pc->dma);
	}
}
static void fdc_update(void) {
	/* fdc cycles are 3/14 of cpu cycles */
	const uint64_t cycle_target = 14; // CPU cycles
	const uint64_t cycle_factor = 3;  // factor

	ibm_pc->fdc_accum += ibm_pc->cpu.cycles * cycle_factor;
	while (ibm_pc->fdc_accum >= cycle_target) {
		ibm_pc->fdc_accum -= cycle_target;
		ibm_pc->fdc_cycles++;
		upd765_fdc_update(&ibm_pc->fdc);
	}
}
static void pic_update(void) {
	i8259_pic_get_interrupt(&ibm_pc->pic);
}
static void pit_update(void) {
	/* pit cycles are 1/4 of cpu cycles */
	const uint64_t cycle_target = 4; // CPU cycles
	const uint64_t cycle_factor = 1; // factor

	ibm_pc->pit_accum += ibm_pc->cpu.cycles * cycle_factor;
	while (ibm_pc->pit_accum >= cycle_target) {
		ibm_pc->pit_accum -= cycle_target;
		ibm_pc->pit_cycles++;
		i8253_pit_update(&ibm_pc->pit);
	}
}
static void cpu_update(void) {

	ibm_pc->cpu.cycles = 0;
	if (i8086_execute(&ibm_pc->cpu) == I8086_DECODE_UNDEFINED) {
		dbg_print("ERROR: undef op: %02X", ibm_pc->cpu.opcode);
		if (ibm_pc->cpu.modrm.byte != 0) {
			dbg_print(" /%02X", ibm_pc->cpu.modrm.reg);
		}
		dbg_print("\n");
		return;
	}
	ibm_pc->cpu_cycles += ibm_pc->cpu.cycles;

//#define bios5150_81

#ifdef bios5150_81
	/* BIOS_5150_24APR81_U33.BIN */
	switch ((ibm_pc->cpu.segments[1] << 4) + ibm_pc->cpu.ip) {

		case 0xFE05B: // TEST.01
			dbg_print("test.01\n");
			break;

		case 0xFE0F9: //
			dbg_print("timer1 failure (01) (bits did not turn off)\n");
			break;

		case 0xFE10D: //
			dbg_print("timer1 failure (02) (bits did not stay on)\n");
			break;

		case 0xFE0B0: // TEST.02
			dbg_print("test.02\n");
			break;

		case 0xFE0DA: // TEST.03 (dma test)
			dbg_print("test.03\n");
			//ibm_pc->cpu.ip = 0xE158; // skip test 3; goto test 4.
			break;

		case 0xFE132: //
			dbg_print("dma error; pattern not read as written\n");
			break;

		case 0xFE158: // TEST.04
			dbg_print("test.04\n");
			break;

		case 0xFE1EF: //
			dbg_print("stgtest error\n");
			break;

		case 0xFE27D: // 
			dbg_print("error beep\n");
			break;

		case 0xFE2DC: // 
			dbg_print("error msg\n");
			break;

		case 0xFE33B: // TEST.05
			dbg_print("test.05\n");
			break;

		case 0xFE235: // TEST.06
			dbg_print("test.06\n");
			break;

		case 0xFE250: // TEST.06
			if (!ibm_pc->cpu.status.zf)
				dbg_print("test.06 error imr not 0\n");
			break;

		case 0xFE25A: // TEST.06
			if (!ibm_pc->cpu.status.zf)
				dbg_print("test.06 error imr not 0xFF\n");
			break;

		case 0xFE27B: // TEST.06
			if (!ibm_pc->cpu.status.zf)
				dbg_print("test.06 error INT occcured\n");
			else
				dbg_print("test.06 passed\n");
			break;

		case 0xFE270: // TEST.06
			//ibm_pc->step = 1;
			break;

		case 0xFE279: // TEST.06
			//ibm_pc->step = 1;
			break;

		case 0xFE285: // TEST.07 (timer test)
			dbg_print("test.07\n");
			//ibm_pc->cpu.ip = 0xE2B3; // skip test 7; goto test 8.
			break;

		case 0xFE29E: // TEST.07 (timer too slow)
			dbg_print("test.07 error timer too slow\n");
			break;

		case 0xFE2AF: // TEST.07 (timer too fast)
			if (!ibm_pc->cpu.status.zf)
				dbg_print("test.07 error timer too fast\n");
			break;

		case 0xFE2B3: // TEST.07 (passed)
			dbg_print("test.07 timer speed passed\n");
			break;

		case 0xFE34C: //
			dbg_print("ROS II error checksum\n");
			break;

		case 0xFE352: // TEST.08
			dbg_print("test.08\n");
			break;

		case 0xFE3AF: // TEST.09
			dbg_print("test.09\n");
			break;

		case 0xFE3C0: // TEST.10
			dbg_print("test.10\n"); 
			//ibm_pc->cpu.ip = 0xE3F8; // skip test 10; TEST 11
			break;

		case 0xFE3F0: //
			dbg_print("test.10 err beep\n"); 
			break;

		case 0xFE3F8: // TEST.11
			dbg_print("test.11\n");
			break;

		case 0xFE42B: // TEST.11
			dbg_print("test.11 (skip mem test)\n");
			ibm_pc->cpu.ip = 0xE47A; // skip mem test; goto TEST 12
			break;

		case 0xFE4C7: // TEST.12
			dbg_print("test.12\n");
			break;

		case 0xFE51E: // TEST.13
			dbg_print("test.13\n");
			//ibm_pc->cpu.status.cf = 0;
			//ibm_pc->cpu.ip = 0xE551; // skip test 13; TEST 14. (JNC)
			break;

		case 0xFE553: // TEST.13 ERR
			dbg_print("test.13 ERR E553\n");
			break;

		case 0xFE59D: // TEST.13 ERR
			dbg_print("test.13 ERR E59D\n");
			break;

		case 0xFE55C: // TEST.14
			dbg_print("test.14\n");
			break;

		case 0xFE5A6: // JMP BOOT
			dbg_print("jmp boot\n");
			break;
		case 0xFE620: // JMP BOOT
			dbg_print("jmp boot_strap\n");
			break;
		case 0xFE722: // JMP BOOT
			dbg_print("jmp basic_boot\n");
			break;
		case 0xFE724: // JMP BOOT
			dbg_print("jmp boot_locn\n");
			break;

		//case 0xFF98D:
		//	// LOOPE
		//	if (ibm_pc->cpu.status.zf == 0) {
		//		dbg_print("cass value changed\n");
		//	}
		//	else if (ibm_pc->cpu.registers[REG_CX].r16 == 1) { 
		//		// cx going to be 0 
		//		dbg_print("cass value time out\n");
		//	}
		//	break;

		//case 0xFE545:
		//	if (ibm_pc->cpu.registers[REG_CX].r16 == 0) {
		//		dbg_print("cass err1 (JCXZ)\n");
		//	}
		//	break;

		case 0xFF065:
			dbg_print("INT 10 (VIDEO_IO) ah=%x\n", ibm_pc->cpu.registers[REG_AX].h);
			break;
		 
		//case 0xFE54B:
		//	if (ibm_pc->cpu.status.cf == 0) {
		//		dbg_print("cass err2 (JNC)\n");
		//	}
		//	break;

		/*case 0xFEE4E:
			dbg_print("FDC check direction bit (0x40) - ");
			if (ibm_pc->cpu.status.zf) {
				dbg_print("OK\n");
			}
			else {
				dbg_print("ERR\n");
			}
			break;

		case 0xFEE61:
			dbg_print("FDC check ready bit (0x80) - ");
			if (ibm_pc->cpu.status.zf) {
				dbg_print("ERR\n");
			}
			else {
				dbg_print("OK\n");
			}
			break;

		case 0xFEF7C:
			dbg_print("FDC check ready bit (0x80) - ");
			if (ibm_pc->cpu.status.zf) {
				dbg_print("ERR\n");
			}
			else {
				dbg_print("OK\n");
			}
			break;

		case 0xFEF8D:
			dbg_print("FDC check direction bit (0x40) - ");
			if (ibm_pc->cpu.status.zf) {
				dbg_print("ERR\n");
			}
			else {
				dbg_print("OK\n");
			}
			break;*/
	}
#endif
}

void ibm_pc_update(void) {
	/* IBM PC Update loop; */

	timing_new_frame(&ibm_pc->time);

	if (timing_check_frame(&ibm_pc->time)) {
	
		if (ibm_pc->step) {
			if (ibm_pc->step == 2) {
				ibm_pc->step = 1;
				isa_bus_update(&ibm_pc->isa_bus, ibm_pc->cpu.cycles);
				dma_update();
				fdc_update();
				pit_update();
				kbd_update();
				pic_update();
				cpu_update();
			}
		}
		else {
			ibm_pc->cpu_cycles = ibm_pc->cpu_accum;
			ibm_pc->dma_cycles = 0;
			ibm_pc->fdc_cycles = 0;
			ibm_pc->pit_cycles = 0;
			ibm_pc->kbd_cycles = 0;
			while (ibm_pc->cpu_cycles < cpu_cycles_per_frame && !ibm_pc->step) {
				isa_bus_update(&ibm_pc->isa_bus, ibm_pc->cpu.cycles);

				dma_update();
				fdc_update();
				pit_update();
				kbd_update();
				pic_update();
				cpu_update();
			}
			if (ibm_pc->cpu_cycles >= cpu_cycles_per_frame) {
				ibm_pc->cpu_accum = ibm_pc->cpu_cycles - cpu_cycles_per_frame;
			}
		}
	}
}

void ibm_pc_reset(void) {
	/* IBM PC reset */

	ibm_pc->cpu_cycles = 0;
	ibm_pc->cpu_accum = 0;

	ibm_pc->pit_cycles = 0;
	ibm_pc->pit_accum = 0;

	ibm_pc->fdc_cycles = 0;
	ibm_pc->fdc_accum = 0;

	ibm_pc->dma_cycles = 0;
	ibm_pc->dma_accum = 0;

	ibm_pc->kbd_cycles = 0;
	ibm_pc->kbd_accum = 0;

	timing_reset_frame(&ibm_pc->time);

	i8086_reset(&ibm_pc->cpu);	
	i8237_dma_reset(&ibm_pc->dma);
	i8253_pit_reset(&ibm_pc->pit);
	i8255_ppi_reset(&ibm_pc->ppi);
	i8259_pic_reset(&ibm_pc->pic);
	upd765_fdc_reset(&ibm_pc->fdc);
	kbd_reset(&ibm_pc->kbd);

	isa_bus_reset(&ibm_pc->isa_bus);

	memory_map_set_writeable_region(&ibm_pc->mm, 0);
}

int ibm_pc_init(void) {
	/* IBM PC Initialize */

	/* Calculate planar ram and io ram */
	cal_planar_io_ram_from_total();

	/* Setup PC Config (system switches sw1, sw2)*/
	ibm_pc_set_sw1();
	ibm_pc_set_sw2();

	/* Setup 8086 CPU */
	i8086_init(&ibm_pc->cpu);
	ibm_pc->cpu.funcs.read_mem_byte  = read_mm_byte;
	ibm_pc->cpu.funcs.write_mem_byte = write_mm_byte;
	ibm_pc->cpu.funcs.read_io_byte   = read_io_byte;
	ibm_pc->cpu.funcs.read_io_word   = read_io_word;
	ibm_pc->cpu.funcs.write_io_byte  = write_io_byte;
	ibm_pc->cpu.funcs.write_io_word  = write_io_word;

	/* Setup 8086 Mnemonics */	
	ibm_pc->mnem.state = &ibm_pc->cpu;

	/* Setup PPI callbacks */
	ibm_pc->ppi.port_a_read  = ppi_port_a_read;	
	ibm_pc->ppi.port_b_read  = ppi_port_b_read;
	ibm_pc->ppi.port_b_write = ppi_port_b_write;
	ibm_pc->ppi.port_c_read  = ppi_port_c_read;

	/* Setup PIT callbacks */
	i8253_pit_set_timer_cb(&ibm_pc->pit, 0, pit_on_timer0, NULL);
	i8253_pit_set_timer_cb(&ibm_pc->pit, 1, pit_on_timer1, NULL);
	i8253_pit_set_timer_cb(&ibm_pc->pit, 2, pit_on_timer2, &ibm_pc->timer2_gate);

	/* Setup PIC callbacks */
	ibm_pc->pic.assert_intr = i8086_assert_intr;
	ibm_pc->pic.deassert_intr = i8086_deassert_intr;

	/* Setup FDC callbacks */
	ibm_pc->fdc.do_irq = fdc_request_irq;
	ibm_pc->fdc.read_mem_byte = read_mm_byte;
	ibm_pc->fdc.write_mem_byte = write_mm_byte;
	ibm_pc->fdc.dma_p = &ibm_pc->dma;

	/* Setup KBD callbacks */
	ibm_pc->kbd.request_irq = kbd_request_irq;
	ibm_pc->kbd.clear_irq = kbd_clear_irq;

	/* Setup DMA */
	i8237_dma_init(&ibm_pc->dma);
	ibm_pc->dma.read_mem_byte = read_mm_byte;
	ibm_pc->dma.write_mem_byte = write_mm_byte;

	/* Setup Memory Map Regions */

	/* PLANAR/IO RAM - placeholder; need ram mregion index for ibm_pc_set_config() */
	ibm_pc->ram_mregion_index = memory_map_add_mregion(&ibm_pc->mm, 0x00000, 16*1024, 0xFFFFF, MREGION_FLAG_NONE);

	/* BIOS ROM - 0xFE000 - 0xFFFFF (0x2000 08K) */
	memory_map_add_mregion(&ibm_pc->mm, 0xFE000, 0x2000, 0xFFFFF, MREGION_FLAG_WRITE_PROTECTED);

	/* BASIC ROM - 0xF6000 - 0xFDFFF (0x8000 32K) */
	memory_map_add_mregion(&ibm_pc->mm, 0xF6000, 0x8000, 0xFFFFF, MREGION_FLAG_WRITE_PROTECTED);
	
	/* EXPANSION ROM - 0xC0000 - 0xF5FFF (0x36000 216K) */
	memory_map_add_mregion(&ibm_pc->mm, 0xC0000, 0x36000, 0xFFFFF, MREGION_FLAG_WRITE_PROTECTED);

	/* Setup ISA Bus, ISA Cards */

	/* For now; just add both cga, mda cards to the bus. */

	/* MDA Card; VIDEO RAM - B0000 - B0FFF (0x1000 04K) mirrored up to 0xB7FFF (0x8000 32K) x8 */
	isa_card_add_mda(&ibm_pc->isa_bus, &ibm_pc->mda);

	/* CGA Card; VIDEO RAM - B8000 - BBFFF (0x4000 16K) mirrored up to 0xBFFFF (0x8000 32K) x2 */
	isa_card_add_cga(&ibm_pc->isa_bus, &ibm_pc->cga);

	/* After all mregions are set; validate the memory map */
	memory_map_validate(&ibm_pc->mm);
	
	ibm_pc_set_config();

	/* Setup timing; we base all timing off 60 HZ */
	timing_init_frame(&ibm_pc->time, HZ_TO_MS(FRAME_RATE_HZ));

	return 0; /* success */
}

int ibm_pc_create(void) {
	/* IBM PC Create */

	ibm_pc = calloc(1, sizeof(IBM_PC));
	if (ibm_pc == NULL) {
		dbg_print("Failed to allocate memory for IBM_PC\n");
		return 1;
	}

	/* Create Memory Map; 6 MRegions */
	if (memory_map_create(&ibm_pc->mm, MEM_SIZE, 6)) {
		return 1; /* memory_map_create() reports errors to console */
	}
	
	/* Create ISA Bus; 4 ISA Card slots */
	if (isa_bus_create(&ibm_pc->isa_bus, &ibm_pc->mm, ISA_BUS_SLOTS)) {
		return 1; /* isa_bus_create() reports errors to console */
	}

	/* Create fdc */
	if (upd765_fdc_create(&ibm_pc->fdc)) {
		return 1; /* upd765_fdc_create() reports errors to console */
	}

	return 0; /* success */
}
void ibm_pc_destroy(void) {
	/* IBM PC Destroy */
	if (ibm_pc != NULL) {

		/* Destroy fdc */
		upd765_fdc_destroy(&ibm_pc->fdc);

		/* Destroy isa bus */
		isa_bus_destroy(&ibm_pc->isa_bus);

		/* Destroy memory map */
		memory_map_destroy(&ibm_pc->mm);

		free(ibm_pc);
		ibm_pc = NULL;
	}
}
