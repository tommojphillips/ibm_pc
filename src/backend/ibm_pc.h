/* ibm_pc.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * IBM PC Implementation
 */

#ifndef IBM_PC_H
#define IBM_PC_H

#include <stdint.h>

#include "i8086.h"
#include "i8086_mnem.h"
#include "io/memory_map.h"
#include "io/isa_bus.h"
#include "chipset/i8237_dma.h"
#include "chipset/i8253_pit.h"
#include "chipset/i8255_ppi.h"
#include "chipset/i8259_pic.h"
#include "chipset/nmi.h"
#include "video/mda.h"
#include "video/cga.h"
#include "fdc/fdc.h"
#include "keyboard.h"

#include "timing.h"

/* CLOCK */

/* 1 MHz in Hz */
#define MHZ2HZ 1000000.0

/* 14Mhz Cystral in Hz */
#define CYSTRAL_14MHZ (15.75 / 1.1 * MHZ2HZ)

/* Display Frame Rate in Hz */
#define FRAME_RATE_HZ 60.0

/* cycles per frame */
#define CYCLES_PER_FRAME(clock_hz) (clock_hz / FRAME_RATE_HZ)

/* CPU Clock */
#define CPU_CLOCK_DIVISOR 3.0                         /* cpu clock divsor */
#define CPU_CLOCK (CYSTRAL_14MHZ / CPU_CLOCK_DIVISOR) /* cpu clock in Hz */

/* PIT Clock */
#define PIT_CLOCK_DIVISOR 12.0
#define PIT_CLOCK (CYSTRAL_14MHZ / PIT_CLOCK_DIVISOR) /* pit clock in Hz */

/* DMA Clock */
#define DMA_CLOCK_DIVISOR 2.0
#define DMA_CLOCK (CYSTRAL_14MHZ / DMA_CLOCK_DIVISOR) /* dma clock in Hz */

/* FDC Clock */
#define FDC_CLOCK_DIVISOR 14.0
#define FDC_CLOCK (CYSTRAL_14MHZ / FDC_CLOCK_DIVISOR) /* fdc clock in Hz */

/* System SW1 (PPI PORT A) */
#define SW1_DISKS_MASK 0xC0 // SW1 b6,b7 - Number of Floppy Disk drives mask (b6,b7)
#define SW1_DISKS_1    0x00 // SW1 b6,b7 - 1 drive
#define SW1_DISKS_2    0x40 // SW1 b6,b7 - 2 drives
#define SW1_DISKS_3    0x80 // SW1 b6,b7 - 3 drives
#define SW1_DISKS_4    0xC0 // SW1 b6,b7 - 4 drives

#define SW1_DISPLAY_MASK        0x30 // SW1 b4,b5 - Type of display mask
#define SW1_DISPLAY_RESERVED    0x00 // SW1 b4,b5 - Display reserved
#define SW1_DISPLAY_CGA_40X25   0x10 // SW1 b4,b5 - 40x25 Color Graphics display
#define SW1_DISPLAY_CGA_80X25   0x20 // SW1 b4,b5 - 80x25 Color Graphics display
#define SW1_DISPLAY_MDA_80X25   0x30 // SW1 b4,b5 - 80x25 Monochrome display

#define SW1_MEMORY_MASK 0x0C // SW1 b2,b3 - Amount of memory mask
#define SW1_MEMORY_16K  0x00 // SW1 b2,b3 - Amount of memory 16K
#define SW1_MEMORY_32K  0x04 // SW1 b2,b3 - Amount of memory 32K
#define SW1_MEMORY_48K  0x08 // SW1 b2,b3 - Amount of memory 48K
#define SW1_MEMORY_64K  0x0C // SW1 b2,b3 - Amount of memory 64K

#define SW1_HAS_FDC     0x01 // SW1 b1
#define SW1_HAS_FPU     0x02 // SW1 b2

#define VIDEO_ADAPTER_NONE      0x00
#define VIDEO_ADAPTER_CGA_40X25 SW1_DISPLAY_CGA_40X25
#define VIDEO_ADAPTER_CGA_80X25 SW1_DISPLAY_CGA_80X25
#define VIDEO_ADAPTER_MDA_80X25 SW1_DISPLAY_MDA_80X25
#define VIDEO_ADAPTER_RESERVED  SW1_DISPLAY_RESERVED

#define MODEL_5150_16_64  0 /* 5150 16-64 KB Motherboard */
#define MODEL_5150_64_256 1 /* 5150 64-256 KB Motherboard */

#define ISA_BUS_SLOTS 4 /* number of Card Slots on ISA BUS */

static uint64_t const cpu_cycles_per_frame = (uint64_t)CYCLES_PER_FRAME(CPU_CLOCK);
static uint64_t const pit_cycles_per_frame = (uint64_t)CYCLES_PER_FRAME(PIT_CLOCK);
static uint64_t const dma_cycles_per_frame = (uint64_t)CYCLES_PER_FRAME(DMA_CLOCK);
static uint64_t const fdc_cycles_per_frame = (uint64_t)CYCLES_PER_FRAME(FDC_CLOCK);

typedef struct PC_SPEAKER {
	uint8_t input;
} PC_SPEAKER;

#define PATH_LEN 256

typedef struct {
	char path[PATH_LEN];
	uint32_t address;
} ROM;

typedef struct {
	char path[PATH_LEN];
	uint8_t drive;
	uint8_t write_protect;
} DISK;

typedef struct IBM_PC_CONFIG {
	uint8_t video_adapter;
	uint8_t fdc_disks;
	uint8_t sw1_provided;
	uint8_t sw1;
	uint8_t sw2_provided;
	uint8_t sw2;
	uint8_t model;
	uint32_t total_memory;
	DISK* disks;
	size_t disk_count;
	ROM* roms;
	size_t rom_count;
} IBM_PC_CONFIG;

typedef struct IBM_PC {
	I8086 cpu;
	I8086_MNEM mnem;
			
	MEMORY_MAP mm;
	ISA_BUS isa_bus;

	I8237_DMA dma;
	I8253_PIT pit;
	I8259_PIC pic;
	I8255_PPI ppi;
	FDC fdc;
	MDA mda;
	CGA cga;

	FRAME_STATE time;

	uint64_t cpu_accum;
	uint64_t cpu_cycles;
	
	uint64_t pit_accum;
	uint64_t pit_cycles;
	
	uint64_t dma_accum;
	uint64_t dma_cycles;
	
	uint64_t kbd_accum;
	uint64_t kbd_cycles;

	KBD kbd;

	NMI nmi;
	PC_SPEAKER pc_speaker;

	uint8_t timer2_gate;       /* timer2 gate */
	
	IBM_PC_CONFIG config;

	int ram_mregion_index;

	uint8_t step;
} IBM_PC;

extern IBM_PC* ibm_pc;

int ibm_pc_create(void);
void ibm_pc_destroy(void);

void ibm_pc_init(void);
void ibm_pc_reset(void);
void ibm_pc_update(void);

void ibm_pc_add_rom(ROM* rom);
void ibm_pc_load_roms(void);
void ibm_pc_add_disk(DISK* disk);
void ibm_pc_load_disks(void);
void ibm_pc_set_config(void);

uint8_t determine_planar_ram_sw(uint20_t planar_ram);
uint8_t determine_io_ram_sw(uint20_t planar_ram, uint20_t io_ram);
uint20_t determine_planar_ram_size(uint8_t sw1);
uint20_t determine_io_ram_size(uint8_t sw1, uint8_t sw2);

#endif
