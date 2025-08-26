/* dbg_gui.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * DBG GUI
 */

#include <stdint.h>
#include <stdio.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "sdl3_common.h"
#include "sdl3_window.h"
#include "sdl3_timing.h"

#include "dbg_gui.h"

#include "../../backend/ibm_pc.h"

#define DBG_PRINT
#ifdef DBG_PRINT
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

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

void print_timer(int i, I8253_TIMER* timer, char* str);

static void print_video_adapter(WINDOW_INSTANCE* instance, DBG_GUI* gui, float x, float y) {
	// print display adapter and mode
	if (ibm_pc->video_adapter == VIDEO_ADAPTER_MDA) {
		sprintf(gui->str, "MDA 80x25");
	}
	else if (ibm_pc->video_adapter == VIDEO_ADAPTER_CGA) {
		if (ibm_pc->cga.mode & CGA_MODE_GRAPHICS) {
			if (ibm_pc->cga.mode & CGA_MODE_GRAPHICS_RES_HI) {
				sprintf(gui->str, "CGA Graphics 640x200");
			}
			else {
				sprintf(gui->str, "CGA Graphics 320x200");
			}
		}
		else {
			if (ibm_pc->cga.mode & CGA_MODE_TEXT_RES_HI) {
				sprintf(gui->str, "CGA 80x25");
			}
			else {
				sprintf(gui->str, "CGA 40x25");
			}
		}
	}
	sprintf(gui->str + strlen(gui->str), " @ %.0fhz", 1000.0 / gui->win->time.last_ms);
	SDL_RenderDebugText(instance->renderer, x, y, gui->str);
}
void dbg_gui_render(WINDOW_INSTANCE* instance, DBG_GUI* gui) {

	// update stats
	SDL_SetRenderDrawColor(instance->renderer, 0xFF, 0, 0, 0xFF);
	
	// print cpu/pit info	
	
	sprintf(gui->str, "PIT %6llu cycles", ibm_pc->pit_cycles);
	SDL_RenderDebugText(instance->renderer, 10.0f, instance->transform.h - 40.0f, gui->str);
	
	sprintf(gui->str, "CPU %6llu cycles @ %.0fhz", ibm_pc->cpu_cycles, 1000.0 / ibm_pc->time.last_ms);
	SDL_RenderDebugText(instance->renderer, 10.0f, instance->transform.h - 30.0f, gui->str);
	
	print_video_adapter(instance, gui, 10.0f, (instance->transform.h - 10.0f));
	
	float h = 10;

	// print cpu registers/instuction
	uint16_t ip = ibm_pc->cpu.ip;
	for (int i = 0; i < 5; ++i) {
		i8086_mnem_at(ibm_pc->mnem, ibm_pc->cpu.segments[SEG_CS], ip);
		sprintf(gui->str, "%04X.%04X: %s", ibm_pc->mnem->segment, ip, ibm_pc->mnem->str);
		SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str);
		
		for (int j = 0; j < ibm_pc->mnem->counter; ++j) {
			sprintf(gui->str + (j * 3), " %02X", ibm_pc->cpu.funcs.read_mem_byte((ibm_pc->cpu.segments[SEG_CS] << 4) + ip + j));
		}
		SDL_RenderDebugText(instance->renderer, 280.0f, h, gui->str);
		
		h += 10;
		ip += ibm_pc->mnem->counter;
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
	if (ibm_pc->cpu.pins.intr) {
		sprintf(gui->str, "INTR ");
		SDL_RenderDebugText(instance->renderer, 50.0f, h, gui->str);
		h += 10;
	}
	if (ibm_pc->cpu.pins.nmi) {
		sprintf(gui->str, "NMI ");
		SDL_RenderDebugText(instance->renderer, 50.0f, h, gui->str);
		h += 10;
	}

	if (tmp2_h > h) {
		h = tmp2_h;
	}
	
	h += 5;	

#if 0
	int i = sdl->key_state.head;
	for (int c = 0; c < sdl->key_state.count; ++c) {
		sprintf(gui->str, "(%x) KEY: %02X", i, sdl->key_state.buffer[i]);
		i = (i + 1) % KEYS_SIZE;
		SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str);
		h += 10;
	}

	h += 5;
	sprintf(gui->str, "Tail: %d, Head: %d, Count: %d ", sdl->key_state.tail, sdl->key_state.head, sdl->key_state.count);
	SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str); 
	h += 10;

	for (int t = 0; t < sdl->key_state.count; ++t) {
		SDL_RenderDebugText(instance->renderer, 50.0f, h, gui->str);
		h += 10;
	}
#endif

	for (int t = 0; t < 3; ++t) {
		print_timer(t, &ibm_pc->pit.timer[t], gui->str);
		SDL_RenderDebugText(instance->renderer, 10.0f, h, gui->str);
		h += 10;
	}
}

void print_timer(int i, I8253_TIMER* timer, char* str) {
	
	double freq = 0;
	if (timer->reload != 0) {
		freq = 1193182.0 / timer->reload;
	}

	sprintf(str, "Timer%d: %04X %05X %.2fhz - ", i, timer->counter, timer->reload, freq);
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
