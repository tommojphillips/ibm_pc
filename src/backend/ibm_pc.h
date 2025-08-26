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
#include "fdd/fdc.h"

#include "timing.h"

//#define MNEM_PRINT 1

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
#define DMA_CLOCK_DIVISOR 8.732575
#define DMA_CLOCK (CYSTRAL_14MHZ / DMA_CLOCK_DIVISOR) /* dma clock in Hz */

#define VIDEO_ADAPTER_NONE 0
#define VIDEO_ADAPTER_MDA  1
#define VIDEO_ADAPTER_CGA  2

#define ISA_CARD_MDA VIDEO_ADAPTER_MDA
#define ISA_CARD_CGA VIDEO_ADAPTER_CGA
#define ISA_CARD_FDC 3

static uint64_t const cpu_cycles_per_frame = (uint64_t)CYCLES_PER_FRAME(CPU_CLOCK);
static uint64_t const pit_cycles_per_frame = (uint64_t)CYCLES_PER_FRAME(PIT_CLOCK);
static uint64_t const dma_cycles_per_frame = (uint64_t)CYCLES_PER_FRAME(DMA_CLOCK);

typedef struct PC_SPEAKER {
	uint8_t input;
} PC_SPEAKER;

typedef struct IBM_PC {
	I8086 cpu;
	I8086_MNEM* mnem;
	
	uint64_t cpu_cycles;
	uint64_t cpu_extra_cycles;
	
	uint64_t pit_accum;
	uint64_t pit_cycles;

	uint64_t kbd_accum;
	uint64_t kbd_cycles;
	
	MEMORY_MAP mm;
	ISA_BUS* isa_bus;

	I8237_DMA dma;
	I8253_PIT pit;
	I8259_PIC pic;
	I8255_PPI ppi;
	FDC fdc;
	MDA mda;
	CGA cga;
	NMI nmi;
	PC_SPEAKER pc_speaker;

	FRAME_STATE time;

	uint8_t kb_reschedule;     /* keyboard reset */
	uint8_t kb_do_reset;       /* keyboard reset */
	uint8_t kb_clock_low;      /* keyboard reset */
	uint64_t kb_reset_elapsed; /* keyboard reset */
	uint64_t kb_reset_delay_elapsed; /* keyboard reset */

	uint8_t timer2_gate;       /* timer2 */
	uint8_t timer2_out;        /* timer2 */

	uint8_t speaker_data;      /* speaker data */
	uint8_t cassette_data;     /* cassette data */
	uint8_t cassette_motor;    /* cassette motor */
	
	uint8_t video_adapter;     /* VIDEO_ADAPTER_XXX */

	int cga_card_index;
	int mda_card_index;
	int fdc_card_index;

	uint8_t sw1;
	uint8_t sw2;

	int step;
} IBM_PC;

extern IBM_PC* ibm_pc;

void ibm_pc_reset();
int ibm_pc_init();
void ibm_pc_destroy();
void ibm_pc_update();

#endif
