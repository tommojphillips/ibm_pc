/* ibm_pc.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * IBM PC Implementation
 */

#include <stdint.h>
#include <stdio.h>
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

#include "bit_utils.h"
#include "timing.h"
#include "input.h"

#define MEM_SIZE 0x100000
#define ADDRESS_MASK 0xFFFFF

/* PORTS */

#define MDA_BASE_ADDRESS MDA_IO_BASE_ADDRESS // Base port address of the MDA Card
#define MDA_MODE   (MDA_BASE_ADDRESS + 0x8)
#define MDA_STATUS (MDA_BASE_ADDRESS + 0xA)

#define CGA_BASE_ADDRESS CGA_IO_BASE_ADDRESS // Base port address of the MDA Card
#define CGA_MODE   (CGA_BASE_ADDRESS + 0x8)
#define CGA_COLOR  (CGA_BASE_ADDRESS + 0x9)
#define CGA_STATUS (CGA_BASE_ADDRESS + 0xA)

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

#define FDC_BASE_ADDRESS 0x3F0 // Base port address of the FDC (Floppy Disk Controller)
#define FDC_STATUS_A        (FDC_BASE_ADDRESS + 0) // (RO)
#define FDC_STATUS_B        (FDC_BASE_ADDRESS + 1) // (RO)
#define FDC_DIGITAL_OUTPUT  (FDC_BASE_ADDRESS + 2) // (RW)
#define FDC_TAPE_DRIVE      (FDC_BASE_ADDRESS + 3) // (RW)
#define FDC_MAIN_STATUS     (FDC_BASE_ADDRESS + 4) // (RO)
#define FDC_DATARATE_SELECT (FDC_BASE_ADDRESS + 4) // (RW)
#define FDC_DATA_FIFO       (FDC_BASE_ADDRESS + 5) // (RW)
#define FDC_DIGITAL_INPUT   (FDC_BASE_ADDRESS + 7) // (RO)
#define FDC_CONFIG_CONTROL  (FDC_BASE_ADDRESS + 7) // (RW)

/* System SW1 (PPI PORT A) */
#define SW1_DISKS_MASK 0xC0 // SW1 b6,b7 - Number of Floppy Disk drives mask (b6,b7)
#define SW1_DISKS_1    0x00 // SW1 b6,b7 - 1 drive
#define SW1_DISKS_2    0x40 // SW1 b6,b7 - 2 drives
#define SW1_DISKS_3    0x80 // SW1 b6,b7 - 3 drives
#define SW1_DISKS_4    0xC0 // SW1 b6,b7 - 4 drives

#define SW1_DISPLAY_MASK        0x30 // SW1 b4,b5 - Type of display mask
#define SW1_DISPLAY_RESERVED    0x00 // SW1 b4,b5 - Display reserved

#define SW1_DISPLAY_CGA_40X25    0x10 // SW1 b4,b5 - 40x25 Color Graphics display
#define SW1_DISPLAY_CGA_80X25    0x20 // SW1 b4,b5 - 80x25 Color Graphics display
#define SW1_DISPLAY_MDA_80X25    0x30 // SW1 b4,b5 - 80x25 Monochrome display

#define SW1_MEMORY_MASK 0x0C // SW1 b2,b3 - Amount of memory mask
#define SW1_MEMORY_16K  0x00 // SW1 b2,b3 - Amount of memory 16K
#define SW1_MEMORY_32K  0x04 // SW1 b2,b3 - Amount of memory 32K
#define SW1_MEMORY_48K  0x08 // SW1 b2,b3 - Amount of memory 48K
#define SW1_MEMORY_64K  0x0C // SW1 b2,b3 - Amount of memory 64K

#define SW1_HAS_FDC     0x01 // SW1 b1

#define SW2_MEMORY_MASK 0x1F // SW2 b1,b2,b3,b4,b5 - Amount of memory mask
#define SW2_MEMORY_16K  0x00 // SW2 b1,b2,b3,b4,b5 - Amount of memory 16K
#define SW2_MEMORY_32K  0x00 // SW2 b1,b2,b3,b4,b5 - Amount of memory 32K
#define SW2_MEMORY_48K  0x00 // SW2 b1,b2,b3,b4,b5 - Amount of memory 48K
#define SW2_MEMORY_64K  0x00 // SW2 b1,b2,b3,b4,b5 - Amount of memory 64K
#define SW2_MEMORY_96K  0x01 // SW2 b1,b2,b3,b4,b5 - Amount of memory 96K
#define SW2_MEMORY_128K 0x02 // SW2 b1,b2,b3,b4,b5 - Amount of memory 128K
#define SW2_MEMORY_160K 0x03 // SW2 b1,b2,b3,b4,b5 - Amount of memory 160K
#define SW2_MEMORY_192K 0x04 // SW2 b1,b2,b3,b4,b5 - Amount of memory 192K
#define SW2_MEMORY_224K 0x05 // SW2 b1,b2,b3,b4,b5 - Amount of memory 224K
#define SW2_MEMORY_256K 0x09 // SW2 b1,b2,b3,b4,b5 - Amount of memory 256K
#define SW2_MEMORY_288K 0x07 // SW2 b1,b2,b3,b4,b5 - Amount of memory 288K
#define SW2_MEMORY_320K 0x08 // SW2 b1,b2,b3,b4,b5 - Amount of memory 320K
#define SW2_MEMORY_352K 0x09 // SW2 b1,b2,b3,b4,b5 - Amount of memory 352K
#define SW2_MEMORY_384K 0x0A // SW2 b1,b2,b3,b4,b5 - Amount of memory 384K
#define SW2_MEMORY_416K 0x0B // SW2 b1,b2,b3,b4,b5 - Amount of memory 416K
#define SW2_MEMORY_448K 0x0C // SW2 b1,b2,b3,b4,b5 - Amount of memory 448K
#define SW2_MEMORY_480K 0x0D // SW2 b1,b2,b3,b4,b5 - Amount of memory 480K
#define SW2_MEMORY_512K 0x0E // SW2 b1,b2,b3,b4,b5 - Amount of memory 512K
#define SW2_MEMORY_544K 0x0F // SW2 b1,b2,b3,b4,b5 - Amount of memory 544K
#define SW2_MEMORY_640K 0x1F // SW2 b1,b2,b3,b4,b5 - Amount of memory 640K

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

void ibm_pc_change_video_adapter(uint8_t video_adapter) {
	ibm_pc->video_adapter = video_adapter;
	switch (video_adapter) {
		case VIDEO_ADAPTER_MDA:
			ibm_pc->sw1 = (ibm_pc->sw1 & ~SW1_DISPLAY_MASK) | SW1_DISPLAY_MDA_80X25; /* Set SW1 to MDA 80x25 */
			break;
		case VIDEO_ADAPTER_CGA:
			ibm_pc->sw1 = (ibm_pc->sw1 & ~SW1_DISPLAY_MASK) | SW1_DISPLAY_CGA_80X25; /* Set SW1 to CGA 80x25 */
			break;
		case VIDEO_ADAPTER_NONE:
			ibm_pc->sw1 = (ibm_pc->sw1 & ~SW1_DISPLAY_MASK);
			break;
	}
}

static uint16_t determine_base_memory(uint8_t sw) {
	uint16_t tmp = (uint16_t)sw; // IN AL, PORT_A ; GET SW1 
	tmp &= SW1_MEMORY_MASK;      // AND AL, 0CH   ; ISOLATE RAM SIZE SWS
	tmp += 4;                    // ADD AL, 4     ; CALCULATE MEMORY SIZE
	                      
	                             // MOV CL, 12 
	tmp <<= 12;                  // SHL AX, CL
	return tmp;
}

/* Memory Functions */
uint8_t read_mm_byte(uint20_t addr) {
	return memory_map_read_byte(&ibm_pc->mm, addr & ADDRESS_MASK);
}
void write_mm_byte(uint20_t addr, uint8_t value) {
	memory_map_write_byte(&ibm_pc->mm, addr & ADDRESS_MASK, value);
}

/* IO Functions */
uint8_t read_io_byte(uint16_t port) {
	
	uint8_t v = 0;
	if (isa_bus_read_io_byte(ibm_pc->isa_bus, port, &v)) {
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
		
		case 0x201: // Gamepad
			return 0x0;

		default:
			dbg_print("read byte from port: %04X\n", port);
			break;
	}
	return 0xFF;
}
void write_io_byte(uint16_t port, uint8_t value) {
	
	if (isa_bus_write_io_byte(ibm_pc->isa_bus, port, value)) {
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

		default:
			dbg_print("write byte to port: %04X = %02X\n", port, value);
			break;
	}
}
uint16_t read_io_word(uint16_t port) {
	switch (port) {
		default:
			printf("read word from port: %04X\n", port);
			break;
	}
	return 0xFFFF;
}
void write_io_word(uint16_t port, uint16_t value) {
	switch (port) {
		default:
			printf("write word to port: %04X = %04X\n", port, value);
			break;
	}
}

/* PPI Callbacks */
uint8_t ppi_port_a_read(I8255_PPI* ppi) {
	/* Port A (read keyboard input, system sw1) */
	
	if (ppi->port_b & PORTB_READ_SW1_KB) {
		// Read system sw1; suppress IRQ_KBD
		dbg_print("[PPI] porta read (SW1)\n");
		return ibm_pc->sw1;
	}
	else {
		// Read scan code
		if (ibm_pc->kb_clock_low) {
			return 0;
		}
		else {
			dbg_print("[PPI] porta read (kbd) (%04X)\n", ibm_pc->cpu.ip);
			if (input_has_input()) {
				uint8_t v = input_get_input();
				return v;
			}
			else {
				return 0;
			}
		}
	}
}
uint8_t ppi_port_b_read(I8255_PPI* ppi) {
	dbg_print("[PPI] portb read\n");
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

	ibm_pc->timer2_gate    = (value & PORTB_TIMER2_GATE);
	ibm_pc->speaker_data   = (value & PORTB_SPEAKER_DATA) >> 1;
	ibm_pc->cassette_data  = (value & PORTB_TIMER2_GATE);
	ibm_pc->cassette_motor = !((value & PORTB_CASSETTE_MOTOR_OFF) >> 3);
	ibm_pc->kb_clock_low =   !((value & PORTB_KB_ENABLE) >> 6);

	if (IS_RISING_EDGE(PORTB_KB_ENABLE, ppi->port_b, value)) {
		dbg_print("[PPI] kbd clk high\n");
		uint64_t ticks = timing_get_ticks_ms() - ibm_pc->kb_reset_elapsed;
		if (ticks > 10) {
			dbg_print("[PPI] kbd reset (%llums)\n", ticks);
			ibm_pc->kb_do_reset = 1;
		}
		ibm_pc->kb_reset_elapsed = 0;
	}
	else if (IS_FALLING_EDGE(PORTB_KB_ENABLE, ppi->port_b, value)) {
		dbg_print("[PPI] kbd clk low\n");
		ibm_pc->kb_reset_elapsed = timing_get_ticks_ms();
	}

	if (IS_RISING_EDGE(PORTB_READ_SW1_KB, ppi->port_b, value)) {
		ibm_pc->kb_reschedule = 1;
	}
	else if (IS_FALLING_EDGE(PORTB_READ_SW1_KB, ppi->port_b, value)) {
		ibm_pc->kb_reschedule = 0;
	}
}
uint8_t ppi_port_c_read(I8255_PPI* ppi) {
	/* PORT C (device output register, read spare key, system sw2) */
	
	uint8_t timer_bit = ibm_pc->pit.timer[2].out << 0x5;
	
	uint8_t cassete_bit = 0;
	if (!ibm_pc->cassette_motor) {
		// Loopback mode
		cassete_bit = ibm_pc->cassette_data << 0x4;
		//uint8_t speaker_loopback = (ibm_pc->speaker_data & ibm_pc->timer2_gate & ibm_pc->pit.timer[2].out);
		//cassete_bit = speaker_loopback << 0x4;
	}

	if (ppi->port_b & PORTB_READ_SW2_KEY) {
		return (ibm_pc->sw2 & 0x0F) | cassete_bit | timer_bit;
	}
	else {
		return ((ibm_pc->sw2 >> 4) & 0x01) | cassete_bit | timer_bit;
	}
}

static void pcspeaker_set(PC_SPEAKER* pc_speaker, uint8_t input) {
	pc_speaker->input = input;
}

static void kbd_update();

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
	/* PIT channel 1 is connected to the DRAM refresh circuit. */
	(void)timer;
	ibm_pc->ppi.port_b ^= 0x10; // DRAM refresh
	kbd_update();
}
static void pit_on_timer2(I8253_TIMER* timer) {
	/* PIT channel 2 is connected to the PC Speaker. */
		
	ibm_pc->timer2_out = timer->out;
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

/* ISA Card Callbacks */
static int isa_cga_write_io_byte(CGA* cga, uint16_t port, uint8_t value) {
	switch (port) {
		case CGA_BASE_ADDRESS + 0x0: // CGA Index
		case CGA_BASE_ADDRESS + 0x2: // CGA Index
		case CGA_BASE_ADDRESS + 0x4: // CGA Index
		case CGA_BASE_ADDRESS + 0x6: // CGA Index
		case CGA_BASE_ADDRESS + 0x1: // CGA Data
		case CGA_BASE_ADDRESS + 0x3: // CGA Data 
		case CGA_BASE_ADDRESS + 0x5: // CGA Data
		case CGA_BASE_ADDRESS + 0x7: // CGA Data
		case CGA_MODE:
		case CGA_STATUS:
		case CGA_COLOR:
			cga_write_io_byte(cga, (uint8_t)(port & ~CGA_BASE_ADDRESS), value);
			return 1;
	}
	return 0;
}
static int isa_cga_read_io_byte(CGA* cga, uint16_t port, uint8_t* v) {
	switch (port) {
		case CGA_BASE_ADDRESS + 0x0: // CGA Index
		case CGA_BASE_ADDRESS + 0x2: // CGA Index
		case CGA_BASE_ADDRESS + 0x4: // CGA Index
		case CGA_BASE_ADDRESS + 0x6: // CGA Index
		case CGA_BASE_ADDRESS + 0x1: // CGA Data
		case CGA_BASE_ADDRESS + 0x3: // CGA Data 
		case CGA_BASE_ADDRESS + 0x5: // CGA Data
		case CGA_BASE_ADDRESS + 0x7: // CGA Data
		case CGA_MODE:
		case CGA_STATUS:
			*v = cga_read_io_byte(cga, (uint8_t)(port & ~CGA_BASE_ADDRESS));
			return 1;
	}
	return 0;
}
static int isa_mda_write_io_byte(MDA* mda, uint16_t port, uint8_t value) {
	switch (port) {
		case MDA_BASE_ADDRESS + 0x0: // MDA Index
		case MDA_BASE_ADDRESS + 0x2: // MDA Index
		case MDA_BASE_ADDRESS + 0x4: // MDA Index
		case MDA_BASE_ADDRESS + 0x6: // MDA Index
		case MDA_BASE_ADDRESS + 0x1: // MDA Data
		case MDA_BASE_ADDRESS + 0x3: // MDA Data 
		case MDA_BASE_ADDRESS + 0x5: // MDA Data
		case MDA_BASE_ADDRESS + 0x7: // MDA Data
		case MDA_MODE:
		case MDA_STATUS:
			mda_write_io_byte(mda, (uint8_t)(port & ~MDA_BASE_ADDRESS), value);
			return 1;
	}
	return 0;
}
static int isa_mda_read_io_byte(MDA* mda, uint16_t port, uint8_t* v) {
	switch (port) {
		case MDA_BASE_ADDRESS + 0x0: // MDA Index
		case MDA_BASE_ADDRESS + 0x2: // MDA Index
		case MDA_BASE_ADDRESS + 0x4: // MDA Index
		case MDA_BASE_ADDRESS + 0x6: // MDA Index
		case MDA_BASE_ADDRESS + 0x1: // MDA Data
		case MDA_BASE_ADDRESS + 0x3: // MDA Data
		case MDA_BASE_ADDRESS + 0x5: // MDA Data
		case MDA_BASE_ADDRESS + 0x7: // MDA Data
		case MDA_MODE:
		case MDA_STATUS:
			*v = mda_read_io_byte(mda, (uint8_t)(port & ~MDA_BASE_ADDRESS));
			return 1;
	}
	return 0;
}
static int isa_fdc_write_io_byte(FDC* fdc, uint16_t port, uint8_t value) {
	switch (port) {
		
		case FDC_DIGITAL_OUTPUT:
		case FDC_TAPE_DRIVE:
		case FDC_DATARATE_SELECT:
		case FDC_CONFIG_CONTROL:
		case FDC_DATA_FIFO:
			fdc_write_io_byte(fdc, (uint8_t)(port & ~FDC_BASE_ADDRESS), value);
			return 1;
	}
	return 0;
}
static int isa_fdc_read_io_byte(FDC* fdc, uint16_t port, uint8_t* v) {
	switch (port) {
		case FDC_STATUS_A:
		case FDC_STATUS_B:
		case FDC_DIGITAL_OUTPUT:
		case FDC_TAPE_DRIVE:
		case FDC_MAIN_STATUS:
		case FDC_DIGITAL_INPUT:
		case FDC_DATA_FIFO:
			*v = fdc_read_io_byte(fdc, (uint8_t)(port & ~FDC_BASE_ADDRESS));
			return 1;
	}
	return 0;
}

/* FDC Callbacks */
static void fdc_do_irq(FDC* fdc) {
	(void)fdc;
	i8259_pic_request_interrupt(&ibm_pc->pic, IRQ_FDC);
}

/* Kbd */
static void kbd_reset_check() {
	if (ibm_pc->kb_do_reset) {
		ibm_pc->kb_do_reset = 0;
		input_reset_input();
		input_set_input(0xAA);
		i8259_pic_request_interrupt(&ibm_pc->pic, IRQ_KBD);
	}
}
static void kbd_update() {

	kbd_reset_check();

	if (ibm_pc->kb_reschedule) {
		//ibm_pc->kb_reschedule = 0;
		//input_reset_input();
		//i8259_pic_clear_interrupt(&ibm_pc->pic, IRQ_KBD);
		//printf("reschedule\n");
		//return;
	}

	if (!ibm_pc->kb_clock_low) {
		if (input_has_input()) {
			i8259_pic_request_interrupt(&ibm_pc->pic, IRQ_KBD);
		}
	}
}

static void pic_update() {
	if (ibm_pc->cpu.status.in) {
		uint8_t type = 0;
		if (i8259_pic_get_interrupt(&ibm_pc->pic, &type)) {
#ifdef DBG_PRINT
			if (type & ~0x8) {
				dbg_print("[PIC] IRQ %d\n", type);
			}
#endif
			i8086_intr(&ibm_pc->cpu, type);
		}
	}
}
static void pit_update() {
	/* pit cycles are 1/4 of cpu cycles */
	const uint64_t cycle_target = 4UL;
	ibm_pc->pit_accum += ibm_pc->cpu.cycles;
	while (ibm_pc->pit_accum >= cycle_target) {
		ibm_pc->pit_accum -= cycle_target;
		ibm_pc->pit_cycles++;
		i8253_pit_update(&ibm_pc->pit);
	}
}
static void cpu_update() {

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

#define bios5150_81
//#define basicc10
//#define supersoft_diag

#ifdef basicc10
	/* BASIC C v1.0 */
	switch ((ibm_pc->cpu.segments[1] << 4) + ibm_pc->cpu.ip) {
		case 0xFE996: // input char
			//ibm_pc->step = 1;
			break;

		case 0xFE9B7:
			//ibm_pc->step = 1;
			break;
	}
#endif

#ifdef bios5150_81
	/* BIOS_5150_24APR81_U33.BIN */
	switch ((ibm_pc->cpu.segments[1] << 4) + ibm_pc->cpu.ip) {
		//case 0xFE01A: // Initial Reliability Tests - phase 1
		//	dbg_print("stgtest\n");
		//	break;

		case 0xFE05B: // TEST.01
			dbg_print("reset\ntest.01\n");
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

		//case 0xFF2C3:
		//	dbg_print("INT 2 (NMI_INT)\n");
		//	break;
		//case 0xFFF54:
		//	dbg_print("INT 5 (PRINT_SCREEN)\n");
		//	break;
		//case 0xFFEA5:
		//	dbg_print("INT 8 (TIMER_INT)\n");
		//	break;
		//case 0xFF987:
		//	dbg_print("INT 9 (KB_INT)\n");
		//	break;
		//case 0xFEF57:
		//	dbg_print("INT E (DISK_INT)\n");
		//	break;
		//case 0xFF065:
		//	dbg_print("INT 10 (VIDEO_IO)\n");
		//	break;
		//case 0xFF1CE:
		//	dbg_print("INT 10 (VIDEO_IO) iret\n");
		//	break;
		case 0xFEC59:
			dbg_print("INT 13 (DISKETTE_IO) - ");
			switch (ibm_pc->cpu.registers[REG_AX].h) {
				case 0:
					dbg_print("AH=0; reset diskette system\n");
					break;
				case 1:
					dbg_print("AH=1; read sector/s\n");
					break;
				default:
					dbg_print("unknown: AH=%02X\n", ibm_pc->cpu.registers[REG_AX].h);
					break;
			}
			break;
		case 0xFE82E:
			dbg_print("INT 16 (KEYBOARD IO)\n");
			break;
		case 0xFE6F2:
			dbg_print("INT 19 (BOOT_STRAP)\n");
			break;

		case 0xFCE51: //fixed
			// ERROR: modrm addressing [bp], [bp + si], [bp + di], [bp + disp] does not use segement override correctly
			//ibm_pc->step = 1;
			break;

		case 0xFF98D:
			// LOOPE
			if (ibm_pc->cpu.status.zf == 0) {
				dbg_print("cass value changed\n");
			}
			else if (ibm_pc->cpu.registers[REG_CX].r16 == 1) { 
				// cx going to be 0 
				dbg_print("cass value time out\n");
			}
			break;

		case 0xFE545:
			if (ibm_pc->cpu.registers[REG_CX].r16 == 0) {
				dbg_print("cass err1 (JCXZ)\n");
			}
			break;

		case 0xFE54B:
			if (ibm_pc->cpu.status.cf == 0) {
				dbg_print("cass err2 (JNC)\n");
			}
			break;

		case 0xFF28A:
			dbg_print("hhhhhhhhh\n");
			ibm_pc->step = 1;
			break;
	}
#endif

#ifdef supersoft_diag
	// SUPERSOFT DIAG V1.2
	switch ((ibm_pc->cpu.segments[1] << 4) + ibm_pc->cpu.ip) {
		case 0xFF505: // PRINT STR end
			dbg_print("PRINT STR end\n");
			//ibm_pc->step = 1;
			break;
		
		case 0xFE825: // U3 CPU REGISTERS AND LOGIC test 1
			dbg_print("U3 CPU REGISTERS AND LOGIC test 1\n");
			//ibm_pc->step = 1;
			break;
					
		case 0xFE87E: // U3 CPU REGISTERS AND LOGIC test 2
			dbg_print("U3 CPU REGISTERS AND LOGIC test 2\n");
			//ibm_pc->step = 1;
			break;
		
		case 0xFE8A6: // U3 CPU REGISTERS AND LOGIC test 3
			dbg_print("U3 CPU REGISTERS AND LOGIC test 3\n");
			//ibm_pc->step = 1;
			break;
		
		case 0xFE8C8: // U3 CPU REGISTERS AND LOGIC test 4
			dbg_print("U3 CPU REGISTERS AND LOGIC test 4\n");
			//ibm_pc->step = 1;
			break;
		
		case 0xFEBC1: // Memory Refresh Test long wait loop
			//dbg_print("Memory Refresh Test Loop\n");
			ibm_pc->cpu.ip = 0xEBC6; // skip loop
			break;
				
		case 0xFEBD0: // Memory Refresh Test error condition
			ibm_pc->cpu.ip = 0xEBD8; // force correct condition
			break;
				
		case 0xFEC89: // Interrupt level 0
			//ibm_pc->step = 1;
			//ibm_pc->cpu.ip = 0xEBD8; 
			break;
	}
#endif
}

void ibm_pc_update() {
	/* IBM PC Update loop; */

	timing_new_frame(&ibm_pc->time);

	if (timing_check_frame(&ibm_pc->time)) {
	
		if (ibm_pc->step) {
			if (ibm_pc->step == 2) {
				ibm_pc->step = 1;
				pit_update();
				pic_update();
				cpu_update();
			}
		}
		else {
			ibm_pc->cpu_cycles = ibm_pc->cpu_extra_cycles;
			ibm_pc->pit_cycles = 0;
			ibm_pc->kbd_cycles = 0;
			while (ibm_pc->cpu_cycles < cpu_cycles_per_frame && !ibm_pc->step) {
				pit_update();
				pic_update();
				cpu_update();
			}
			if (ibm_pc->cpu_cycles >= cpu_cycles_per_frame) {
				ibm_pc->cpu_extra_cycles = ibm_pc->cpu_cycles - cpu_cycles_per_frame;
			}
		}
	}
}

void ibm_pc_reset() {
	/* IBM PC reset */

	ibm_pc->cassette_motor = 0;
	ibm_pc->speaker_data = 0;

	ibm_pc->kb_do_reset = 0;
	ibm_pc->kb_reschedule = 0;
	ibm_pc->kbd_cycles = 0;
	ibm_pc->kb_clock_low = 1;
	ibm_pc->kb_reset_elapsed = timing_get_ticks_ms();

	ibm_pc->cpu_cycles = 0;
	ibm_pc->cpu_extra_cycles = 0;

	ibm_pc->pit_cycles = 0;
	ibm_pc->pit_accum = 0;

	timing_reset_frame(&ibm_pc->time);

	i8086_reset(&ibm_pc->cpu);	
	i8237_dma_reset(&ibm_pc->dma);
	i8253_pit_reset(&ibm_pc->pit);
	i8255_ppi_reset(&ibm_pc->ppi);
	i8259_pic_reset(&ibm_pc->pic);
	fdc_reset(&ibm_pc->fdc);
	mda_reset(&ibm_pc->mda);
	cga_reset(&ibm_pc->cga);

	input_reset_input();

	memory_map_set_writeable_region(&ibm_pc->mm, 0);
}

int ibm_pc_init() {
	/* IBM PC Initialize */

	ibm_pc = (IBM_PC*)calloc(1, sizeof(IBM_PC));
	if (ibm_pc == NULL) {
		dbg_print("Failed to allocate memory for IBM_PC\n");
		return 1;
	}

	/* Setup 8086 CPU */
	i8086_init(&ibm_pc->cpu);
	ibm_pc->cpu.funcs.read_mem_byte  = read_mm_byte;
	ibm_pc->cpu.funcs.write_mem_byte = write_mm_byte;
	ibm_pc->cpu.funcs.read_io_byte   = read_io_byte;
	ibm_pc->cpu.funcs.read_io_word   = read_io_word;
	ibm_pc->cpu.funcs.write_io_byte  = write_io_byte;
	ibm_pc->cpu.funcs.write_io_word  = write_io_word;

	/* Setup 8086 Mnemonics */
	ibm_pc->mnem = (I8086_MNEM*)calloc(1, sizeof(I8086_MNEM));
	if (ibm_pc->mnem == NULL) {
		dbg_print("Failed to allocate memory for I8086_MNEM\n");
		return 1;
	}
	ibm_pc->mnem->state = &ibm_pc->cpu;

	/* Setup PPI */
	ibm_pc->ppi.port_a_read  = ppi_port_a_read;	
	ibm_pc->ppi.port_b_read  = ppi_port_b_read;
	ibm_pc->ppi.port_b_write = ppi_port_b_write;
	ibm_pc->ppi.port_c_read  = ppi_port_c_read;

	/* Setup PIT */
	i8253_pit_set_timer_cb(&ibm_pc->pit, 0, pit_on_timer0, NULL);
	i8253_pit_set_timer_cb(&ibm_pc->pit, 1, pit_on_timer1, NULL);
	i8253_pit_set_timer_cb(&ibm_pc->pit, 2, pit_on_timer2, &ibm_pc->timer2_gate);

	/* Setup FDC */
	ibm_pc->fdc.do_irq = fdc_do_irq;

	/* Setup Memory map */
	if (create_memory_map(&ibm_pc->mm, MEM_SIZE)) {
		dbg_print("Failed to create MEMORY_MAP\n");
		return 1;
	}

	/* Setup Memory Regions */

	/* SYSTEM RAM - 0x0000 - 0x10000 (64K) */
	add_memory_region(&ibm_pc->mm, 0x00000, 640*1024, 0xFFFFFFFF, 0);

	/* BASIC ROM */
	add_memory_region(&ibm_pc->mm, 0xF6000, 0x8000, 0x7FFF, MREGION_FLAG_WRITE_PROTECTED);

	/* IBM BIOS ROM */
	add_memory_region(&ibm_pc->mm, 0xFE000, 0x2000, 0x1FFF, MREGION_FLAG_WRITE_PROTECTED);

	/* Setup ISA Bus */
	
	if (isa_bus_create(&ibm_pc->isa_bus, &ibm_pc->mm)) {
		dbg_print("Failed to create ISA_BUS\n");
		return 1;
	}

	/* MDA Card */
	ibm_pc->mda_card_index = isa_bus_create_card(ibm_pc->isa_bus, 0, "MDA");
	/* MDA VIDEO RAM - B0000 - B0FFF (0x1000 4K) mirrored up to 0xB7FFF (x8)*/
	isa_card_add_memory_region(ibm_pc->isa_bus, ibm_pc->mda_card_index, MDA_MM_BASE_ADDRESS, 0x8000, MDA_MM_ADDRESS_MASK, 0);
	isa_card_add_io(ibm_pc->isa_bus, ibm_pc->mda_card_index, isa_mda_write_io_byte, isa_mda_read_io_byte, &ibm_pc->mda);
	isa_bus_enable_card(ibm_pc->isa_bus, ibm_pc->mda_card_index);

	/* CGA Card */
	ibm_pc->cga_card_index = isa_bus_create_card(ibm_pc->isa_bus, 0, "CGA");
	/* CGA VIDEO RAM - B8000 - BBFFF (0x4000 16K) mirrored up to 0xBFFFF (x2)*/
	isa_card_add_memory_region(ibm_pc->isa_bus, ibm_pc->cga_card_index, CGA_MM_BASE_ADDRESS, 0x8000, CGA_MM_ADDRESS_MASK, 0);
	isa_card_add_io(ibm_pc->isa_bus, ibm_pc->cga_card_index, isa_cga_write_io_byte, isa_cga_read_io_byte, &ibm_pc->cga);
	isa_bus_enable_card(ibm_pc->isa_bus, ibm_pc->cga_card_index);

	/* FDC Card */
	ibm_pc->fdc_card_index = isa_bus_create_card(ibm_pc->isa_bus, 0, "FDC");
	//isa_card_add_memory_region(ibm_pc->isa_bus, ibm_pc->fdc_card_index, FDC_MM_BASE_ADDRESS, 0x8000, FDC_MM_ADDRESS_MASK, 0);
	isa_card_add_io(ibm_pc->isa_bus, ibm_pc->fdc_card_index, isa_fdc_write_io_byte, isa_fdc_read_io_byte, &ibm_pc->fdc);
	isa_bus_enable_card(ibm_pc->isa_bus, ibm_pc->fdc_card_index);

	/* Set system switches (sw1, sw2) */
	//ibm_pc->sw1 = SW1_HAS_FDC | SW1_DISKS_1 | SW1_MEMORY_64K;
	ibm_pc->sw1 = SW1_MEMORY_64K;
	ibm_pc->sw2 = SW2_MEMORY_64K;

	uint20_t base_memory = determine_base_memory(ibm_pc->sw1);
	dbg_print("Base Memory: %d Kb\n", base_memory / 1024);

	/* setup timing; we base all timing off 60 HZ */
	ibm_pc->time.target_ms = HZ_TO_MS(FRAME_RATE_HZ);

	return 0; /* success */
}

void ibm_pc_destroy() {
	/* IBM PC Destroy */
	if (ibm_pc != NULL) {

		/* Free memory map */
		destroy_memory_map(&ibm_pc->mm);

		if (ibm_pc->isa_bus != NULL) {
			isa_bus_destroy(ibm_pc->isa_bus);
			ibm_pc->isa_bus = NULL;
		}

		fdc_remove_disks(&ibm_pc->fdc);

		free(ibm_pc);
		ibm_pc = NULL;
	}
}
