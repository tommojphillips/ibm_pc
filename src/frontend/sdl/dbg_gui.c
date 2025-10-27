/* dbg_gui.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * DBG GUI
 */

#include <stdint.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "sdl3_common.h"
#include "sdl3_window.h"
#include "sdl3_timing.h"

#include "dbg_gui.h"

#include "backend/ibm_pc.h"

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

 /* Get 20bit address SEG:ADDR */
#define PHYS_ADDRESS(seg, offset) ((((uint20_t)seg << 4) + ((offset) & 0xFFFF)) & 0xFFFFF)

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

void avg_frame_timer_add(AVG_FRAME_TIMER* t, double ms) {
	// Subtract the old value from the sum
	t->sum -= t->frame_times[t->index];

	// Add the new value
	t->frame_times[t->index] = ms;
	t->sum += ms;

	// Advance the index
	t->index = (t->index + 1) % FRAME_HISTORY;

	// Track number of valid frames
	if (t->count < FRAME_HISTORY) t->count++;
}
double avg_frame_timer_get(const AVG_FRAME_TIMER* t) {
	return (t->count > 0) ? (t->sum / t->count) : 0.0;
}

void print_timer(int i, I8253_TIMER* timer, char* str);

static void print_video_adapter(WINDOW_INSTANCE* instance, DBG_GUI* gui, float x, float y) {
	// print display adapter and mode
	if (ibm_pc->config.video_adapter == VIDEO_ADAPTER_MDA_80X25) {
		sprintf(gui->str, "MDA %dx%d", ibm_pc->mda.crtc.hdisp, ibm_pc->mda.crtc.vdisp);
	}
	else if (ibm_pc->config.video_adapter == VIDEO_ADAPTER_CGA_80X25 || ibm_pc->config.video_adapter == VIDEO_ADAPTER_CGA_40X25) {
		if (ibm_pc->cga.mode & CGA_MODE_GRAPHICS) {
			sprintf(gui->str, "CGA Graphics %dx%d", ibm_pc->cga.width, ibm_pc->cga.height);
		}
		else {
			sprintf(gui->str, "CGA %dx%d", ibm_pc->cga.crtc.hdisp, ibm_pc->cga.crtc.vdisp);
		}
	}
	else if (ibm_pc->config.video_adapter == VIDEO_ADAPTER_NONE) {
		sprintf(gui->str, "HEADLESS");
	}

	// get avg fps
	avg_frame_timer_add(&gui->win_avg_fps, gui->win->time.last_ms);

	if (gui->win != NULL) {
		sprintf(gui->str + strlen(gui->str), " @ %.2fhz", HZ_TO_MS(avg_frame_timer_get(&gui->win_avg_fps)));
	}
	SDL_RenderDebugText(instance->renderer, x, y, gui->str);
}
void dbg_gui_render(WINDOW_INSTANCE* instance, DBG_GUI* gui) {

	// update stats
	SDL_SetRenderDrawColor(instance->renderer, 0xFF, 0, 0, 0xFF);
	
	// get avg fps
	avg_frame_timer_add(&gui->emu_avg_fps, ibm_pc->time.last_ms);

	// print cpu/pit info	
		
	sprintf(gui->str, "KBD %6llu cycles", ibm_pc->kbd_cycles);
	SDL_RenderDebugText(instance->renderer, 10.0f, instance->transform.h - 70.0f, gui->str);
	
	sprintf(gui->str, "DMA %6llu cycles", ibm_pc->dma_cycles);
	SDL_RenderDebugText(instance->renderer, 10.0f, instance->transform.h - 60.0f, gui->str);
	
	sprintf(gui->str, "FDC %6llu cycles", ibm_pc->fdc_cycles);
	SDL_RenderDebugText(instance->renderer, 10.0f, instance->transform.h - 50.0f, gui->str);
	
	sprintf(gui->str, "PIT %6llu cycles", ibm_pc->pit_cycles);
	SDL_RenderDebugText(instance->renderer, 10.0f, instance->transform.h - 40.0f, gui->str);
	
	sprintf(gui->str, "CPU %6llu cycles @ %.2fhz", ibm_pc->cpu_cycles, HZ_TO_MS(avg_frame_timer_get(&gui->emu_avg_fps)));
	SDL_RenderDebugText(instance->renderer, 10.0f, instance->transform.h - 30.0f, gui->str);
		
	print_video_adapter(instance, gui, 10.0f, (instance->transform.h - 10.0f));
	
	float h = 10;

	// print cpu registers/instuction
	uint16_t ip = ibm_pc->cpu.ip;
	for (int i = 0; i < 5; ++i) {
		i8086_mnem_at(&ibm_pc->mnem, ibm_pc->cpu.segments[SEG_CS], ip);
		sprintf(gui->str, "%04X.%04X: %s", ibm_pc->mnem.segment, ip, ibm_pc->mnem.str);
		SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str);
		
		for (int j = 0; j < ibm_pc->mnem.counter; ++j) {
			sprintf(gui->str + (j * 3), " %02X", ibm_pc->cpu.funcs.read_mem_byte(PHYS_ADDRESS(ibm_pc->cpu.segments[SEG_CS], ip + j)));
		}
		SDL_RenderDebugText(instance->renderer, 280.0f, h, gui->str);
		
		h += 10;
		ip += ibm_pc->mnem.counter;
	}

	h += 5;
	sprintf(gui->str, "AX %04X BX %04X",
		ibm_pc->cpu.registers[REG_AX].r16, ibm_pc->cpu.registers[REG_BX].r16);
	SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str);
	h += 10;

	sprintf(gui->str, "CX %04X DX %04X",
		ibm_pc->cpu.registers[REG_CX].r16, ibm_pc->cpu.registers[REG_DX].r16);
	SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str);
	h += 10;

	sprintf(gui->str, "SI %04X DI %04X",
		ibm_pc->cpu.registers[REG_SI].r16, ibm_pc->cpu.registers[REG_DI].r16);
	SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str);
	h += 10;

	sprintf(gui->str, "SP %04X BP %04X",
		ibm_pc->cpu.registers[REG_SP].r16, ibm_pc->cpu.registers[REG_BP].r16);
	SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str);
	h += 10;

	h += 5;
	sprintf(gui->str, "ES %04X CS %04X",
		ibm_pc->cpu.segments[SEG_ES], ibm_pc->cpu.segments[SEG_CS]);
	SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str);
	h += 10;

	sprintf(gui->str, "DS %04X SS %04X",
		ibm_pc->cpu.segments[SEG_DS], ibm_pc->cpu.segments[SEG_SS]);
	SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str);
	h += 10;

	gui->str[0] = '\0';
	if (ibm_pc->cpu.status.cf) {
		strcat(gui->str, "C ");
	}
	else {
		strcat(gui->str, "- ");
	}
	if (ibm_pc->cpu.status.pf) {
		strcat(gui->str, "P ");
	}
	else {
		strcat(gui->str, "- ");
	}
	if (ibm_pc->cpu.status.af) {
		strcat(gui->str, "A ");
	}
	else {
		strcat(gui->str, "- ");
	}
	if (ibm_pc->cpu.status.zf) {
		strcat(gui->str, "Z ");
	}
	else {
		strcat(gui->str, "- ");
	}
	if (ibm_pc->cpu.status.sf) {
		strcat(gui->str, "S ");
	}
	else {
		strcat(gui->str, "- ");
	}
	if (ibm_pc->cpu.status.of) {
		strcat(gui->str, "O ");
	}
	else {
		strcat(gui->str, "- ");
	}
	if (ibm_pc->cpu.status.df) {
		strcat(gui->str, "D ");
	}
	else {
		strcat(gui->str, "- ");
	}
	if (ibm_pc->cpu.status.in) {
		strcat(gui->str, "I ");
	}
	else {
		strcat(gui->str, "- ");
	}
	if (ibm_pc->cpu.status.tf) {
		strcat(gui->str, "T ");
	}
	else {
		strcat(gui->str, "- ");
	}

	if (ibm_pc->cpu.int_latch) {
		strcat(gui->str, "IF ");
	}
	else {
		strcat(gui->str, "   ");
	}

	if (ibm_pc->cpu.tf_latch) {
		strcat(gui->str, "TF ");
	}
	else {
		strcat(gui->str, "   ");
	}
	SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str);
	h += 10;

	float tmp_h = h;
	if (ibm_pc->pic.irr & (1 << IRQ_TIMER0)) {
		sprintf(gui->str, "IRQ_TIMER0 ");
		SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str);
		h += 10;
	}
	if (ibm_pc->pic.irr & (1 << IRQ_KBD)) {
		sprintf(gui->str, "IRQ_KBD ");
		SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str);
		h += 10;
	}
	if (ibm_pc->pic.irr & (1 << IRQ_FDC)) {
		sprintf(gui->str, "IRQ_FDC ");
		SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str);
		h += 10;
	}

	float tmp2_h = h;
	h = tmp_h;
	if (ibm_pc->cpu.intr) {
		sprintf(gui->str, "INTR ");
		SDL_RenderDebugText(instance->renderer, 50.0f, h, gui->str);
		h += 10;
	}
	if (ibm_pc->cpu.nmi) {
		sprintf(gui->str, "NMI ");
		SDL_RenderDebugText(instance->renderer, 50.0f, h, gui->str);
		h += 10;
	}

	if (tmp2_h > h) {
		h = tmp2_h;
	}
	
	h += 5;	

#if 1
	int i = 0;
	uint8_t c = 0;
	while (ring_buffer_peek(&sdl->key_state, i, &c) == 0) {
		sprintf(gui->str, "(%x) KEY: %02X", i, c);
		i++;
		SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str);
		h += 10;
	}

	h += 5;
	sprintf(gui->str, "Tail: %d, Head: %d, Count: %d ", sdl->key_state.tail, sdl->key_state.head, sdl->key_state.count);
	SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str); 
	h += 15;
#endif

	for (int t = 0; t < 3; ++t) {
		print_timer(t, &ibm_pc->pit.timer[t], gui->str);
		SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str);
		h += 10;
	}
}

void print_timer(int i, I8253_TIMER* timer, char* str) {
	
	double clk = (double)PIT_CLOCK;
	double freq = 0;
	if (timer->reload != 0) {
		freq = clk / timer->reload;
	}
	else {
		freq = clk / 0x10000;
	}

	sprintf(str, "Timer%d: %04X %04X %.2fhz - ", i, timer->counter, timer->reload, freq);
	switch (timer->channel_state) {
		case I8253_TIMER_STATE_COUNTING:
			strcat(str, "Counting");
			break;
		case I8253_TIMER_STATE_DELAY_LOAD_CYCLE:
			strcat(str, "Delay Load Cycle");
			break;
		case I8253_TIMER_STATE_WAITING_FOR_GATE:
			strcat(str, "Waiting for Gate");
			break;
		case I8253_TIMER_STATE_WAITING_FOR_RELOAD:
			strcat(str, "Waiting for Reload");
			break;
		case I8253_TIMER_STATE_WAITING_LOAD_CYCLE:
			strcat(str, "Waiting for Load Cycle");
			break;
	}
}
