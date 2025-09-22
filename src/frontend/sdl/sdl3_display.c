/* sdl3_display.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Implements draw display buffer function
 */

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "sdl3_display.h"
#include "sdl3_window.h"
#include "sdl3_font.h"

#include "../../backend/video/mda.h"
#include "../../backend/video/cga.h"
#include "../../backend/ibm_pc.h"

#define sx(x)    ((x) ? 0xBF : 0)
#define se(x)    sx(x)

#define max(x, y) (x > y ? x : y)
#define min(x, y) (y > x ? x : y)

#define CENTER(t, x, y) (((t - (x)) / 2) + (y))

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

static void display_update_cell_position(DISPLAY_INSTANCE* display, SDL_FRect* rect, int max_rows, int max_columns, int row, int column) {
	rect->x = (float)CENTER(display->window->transform.w, max_columns * display->cell_w, column * display->cell_w);
	rect->y = (float)CENTER(display->window->transform.h, max_rows * display->cell_h, row * display->cell_h);
	rect->w = (float)display->cell_w;
	rect->h = (float)display->cell_h;
}

static void display_update_cursor_position(CRTC_6845* crtc, uint8_t bytes_per_row, int* row, int* column) {
	/* calculate cursor position */	
	const uint16_t cursor_address_register = (crtc->registers[CRTC_6845_CURSOR_HI]) << 8 | crtc->registers[CRTC_6845_CURSOR_LO];	
	uint32_t offset = cursor_address_register;
	*row = offset / bytes_per_row;
	*column = offset % bytes_per_row;
}

/* Dummy display */
static void disabled_draw_screen(DISPLAY_INSTANCE* display, int w, int h, const char* msg) {
	/* Card is not enabled */
	SDL_FRect rect = { .x = (float)CENTER(display->window->transform.w, w, 0), .y = (float)CENTER(display->window->transform.h, h, 0), .w = (float)w, .h = (float)h };
	SDL_SetRenderDrawColor(display->window->renderer, 0, 0, 0, 0xFF);
	SDL_RenderFillRect(display->window->renderer, &rect);
	SDL_SetRenderDrawColor(display->window->renderer, 0xFF, 0, 0, 0xFF);
	SDL_RenderDebugText(display->window->renderer, (float)CENTER(display->window->transform.w, w, w / 2) - (80), (float)CENTER(display->window->transform.h,h, h / 2), msg);
}
void dummy_draw_screen(WINDOW_INSTANCE* window, DISPLAY_INSTANCE* display) {
	disabled_draw_screen(display, window->transform.w, window->transform.h, "DUMMY");
}

/* MDA */
#define MDA_BYTES_PER_ROW 80
static void mda_draw_background(DISPLAY_INSTANCE* display, SDL_FRect* rect, uint8_t attribute) {

	//if (attribute & MDA_ATTRIBUTE_NON_DISPLAY) return;

	/* render background */
	if (attribute & MDA_ATTRIBUTE_BW) {
		SDL_SetRenderDrawColor(display->window->renderer, 0, 0, 0, 0xFF);
	}
	else {
		SDL_SetRenderDrawColor(display->window->renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	}
	SDL_RenderFillRect(display->window->renderer, rect);
}
static void mda_draw_character(DISPLAY_INSTANCE* display, SDL_FRect* rect, uint8_t character, uint8_t attribute, int blink) {

	if (character == 0) return;
	//if (attribute & MDA_ATTRIBUTE_NON_DISPLAY) return;
		
	/* only render character if not blinking or blink < 15 (half time) */
	if ((attribute & MDA_ATTRIBUTE_BLINK) && blink < 15) {
		return;
	}

	/* render text */
	if (attribute & MDA_ATTRIBUTE_BW) {
		SDL_SetTextureColorMod(display->font_data->textures[character], 0xFF, 0xFF, 0xFF);
	}
	else {
		SDL_SetTextureColorMod(display->font_data->textures[character], 0, 0, 0);
	}
	SDL_RenderTexture(display->window->renderer, display->font_data->textures[character], NULL, rect);
}
static void mda_text_draw_screen(DISPLAY_INSTANCE* display) {

	/* get cell dimensions */
	display->cell_w = display->window->transform.w / ibm_pc->mda.columns;
	display->cell_h = display->window->transform.h / ibm_pc->mda.rows;

	/* increment blink; count to 31 */
	ibm_pc->mda.blink = (ibm_pc->mda.blink++) & 0x1F;

	SDL_FRect rect = { 0 }; 
	int row = 0;
	int column = 0;
	uint8_t attribute = 0;
	for (row = 0; row < ibm_pc->mda.rows; row++) {
		for (column = 0; column < ibm_pc->mda.columns; column++) {
			const uint16_t address_register = (ibm_pc->mda.crtc.registers[CRTC_6845_ADDRESS_HI]) << 8 | ibm_pc->mda.crtc.registers[CRTC_6845_ADDRESS_LO];
			const uint32_t offset = ((row * (ibm_pc->mda.columns * 2)) + column * 2);
			const uint32_t address = MDA_MM_BASE_ADDRESS + ((address_register + offset) & MDA_MM_ADDRESS_MASK);
			const uint8_t character = ibm_pc->cpu.funcs.read_mem_byte(address);
			attribute = ibm_pc->cpu.funcs.read_mem_byte(address + 1);
			display_update_cell_position(display, &rect, ibm_pc->mda.rows, ibm_pc->mda.columns, row, column);
			mda_draw_background(display, &rect, attribute);
			mda_draw_character(display, &rect, character, attribute, ibm_pc->mda.blink);
		}
	}

	/* draw cursor */
	display_update_cursor_position(&ibm_pc->mda.crtc, MDA_BYTES_PER_ROW, &row, &column);
	display_update_cell_position(display, &rect, ibm_pc->mda.rows, ibm_pc->mda.columns, row, column);
	attribute = (MDA_ATTRIBUTE_BW | MDA_ATTRIBUTE_BLINK); /* invert BG/FG, force blink */
	mda_draw_character(display, &rect, '_', attribute, ibm_pc->mda.blink);
}
static void mda_draw_screen(WINDOW_INSTANCE* instance, DISPLAY_INSTANCE* display) {
	(void)instance;
	if (!(ibm_pc->mda.mode & MDA_MODE_VIDEO_ENABLE)) {
		disabled_draw_screen(display, ibm_pc->mda.width, ibm_pc->mda.height, "ADAPTER NOT ENABLED");
		return;
	}
	
	mda_text_draw_screen(display);
}

/* CGA */
#define CGA_BYTES_PER_ROW 80
typedef struct COLOR_PALETTE {
	uint8_t r;
	uint8_t b;
	uint8_t g;
	uint8_t br;
} COLOR_PALETTE;

static void cga_graphics_draw_lo_res(DISPLAY_INSTANCE* display) {
	/* 4 colors @ 320px. 1byte = 4px. b00 = col1, b01 = col2, b10 = col3, b11 = col4 */

		/* color palettes */
	const COLOR_PALETTE palette0[4] = {
		{ .r = 0xFF, /* red */
		  .b = 0,
		  .g = 0,
		  .br = 0x80 },

		{ .r = 0,    /* green */
		  .b = 0,
		  .g = 0xFF,
		  .br = 0x80 },

		{ .r = 0xFF, /* yellow */
		  .b = 0,
		  .g = 0xFF,
		  .br = 0x80 },

		{ .r = 0,    /* black? */
		  .b = 0,
		  .g = 0,
		  .br = 0x80 },
	};
	const COLOR_PALETTE palette1[4] = {
		{ .r = 0xFF, /* magenta */
		  .b = 0xFF,
		  .g = 0,
		  .br = 0 },

		{ .r = 0,   /* cyan */
		  .b = 0xFF,
		  .g = 0xFF,
		  .br = 0x80 },

		{ .r = 0xFF, /* white */
		  .b = 0xFF,
		  .g = 0xFF,
		  .br = 0x80 },

		{ .r = 0,   /* black? */
		  .b = 0,
		  .g = 0,
		  .br = 0x80 },
	};
	const COLOR_PALETTE palette2[4] = {
		{ .r = 0,    /* black */
		  .b = 0,
		  .g = 0,
		  .br = 0x80 },

		{ .r = 0xFF, /* red */
		  .b = 0,
		  .g = 0,
		  .br = 0x80 },

		{ .r = 0,    /* cyan */
		  .b = 0xFF,
		  .g = 0xFF,
		  .br = 0x80 },

		{ .r = 0xFF, /* white */
		  .b = 0xFF,
		  .g = 0xFF,
		  .br = 0x80 },
	};

#if 1
	/* get boarder color */
	uint8_t b = se(ibm_pc->cga.color & CGA_COLOR_BOADER_B);
	uint8_t g = se(ibm_pc->cga.color & CGA_COLOR_BOADER_G);
	uint8_t r = se(ibm_pc->cga.color & CGA_COLOR_BOADER_R);
	uint8_t br = se(ibm_pc->cga.color & CGA_COLOR_BOADER_BR);

	/* draw boarder/background */
	SDL_SetRenderDrawColor(display->window->renderer, r, g, b, (0x80 + br) & 0xFF);
	SDL_FRect rect = { .x = 0, .y = 0, .w = (float)display->window->transform.w, .h = (float)display->window->transform.h };
	SDL_RenderFillRect(display->window->renderer, &rect);
#endif

	if (ibm_pc->cga.color & CGA_COLOR_BRIGHT_FG) {
		/*foreground colours display in high intensity */
	}

	if (ibm_pc->cga.color & CGA_COLOR_PALETTE) {
		/* If set, the fixed colours in the palette are magenta, cyan and white. If clr, they're red, green and yellow
		   Bit 2 in the mode control register (if set) overrides this bit */
	}

	for (int y = 0; y < CGA_HI_RES_GRAPHICS_HEIGHT; ++y) {
		for (int x = 0; x < CGA_BYTES_PER_ROW; ++x) {
			const uint16_t address_register = (ibm_pc->cga.crtc.registers[CRTC_6845_ADDRESS_HI]) << 8 | ibm_pc->cga.crtc.registers[CRTC_6845_ADDRESS_LO];
			const uint32_t offset = (y * CGA_BYTES_PER_ROW) + x;
			const uint32_t address = CGA_MM_BASE_ADDRESS + ((address_register + offset) & CGA_MM_ADDRESS_MASK);
			uint8_t pixel_data = ibm_pc->cpu.funcs.read_mem_byte(address);
			for (int p = 0; p < 4; ++p) {
				uint8_t sel = pixel_data & 0x3;
				if (ibm_pc->cga.mode & CGA_MODE_BW) {
					SDL_SetRenderDrawColor(display->window->renderer, palette2[sel].r, palette2[sel].g, palette2[sel].b, palette2[sel].br);
				}
				else {
					if (ibm_pc->cga.color & CGA_COLOR_PALETTE) {
						SDL_SetRenderDrawColor(display->window->renderer, palette1[sel].r, palette1[sel].g, palette1[sel].b, palette1[sel].br);
					}
					else {
						SDL_SetRenderDrawColor(display->window->renderer, palette0[sel].r, palette0[sel].g, palette0[sel].b, palette0[sel].br);
					}
				}
				pixel_data >>= 2;

				SDL_FRect rect1 = {
					.x = (float)CENTER(display->window->transform.w, ibm_pc->cga.width, (offset * 4) + p),
					.y = (float)CENTER(display->window->transform.h, ibm_pc->cga.height, (offset / 4) + p),
					.w = (float)display->cell_w,
					.h = (float)display->cell_h
				};
				SDL_RenderFillRect(display->window->renderer, &rect1);
			}
		}
	}
}

static void cga_graphics_draw_hi_res(DISPLAY_INSTANCE* display) {
	/* 2 colors @ 640px. 1byte = 8px. b0 = col1, b1 = col2 */

		/* color palette */
	const COLOR_PALETTE palette[2] = {
		{.r = 0xFF, /* white */
		  .b = 0xFF,
		  .g = 0xFF,
		  .br = 0x80 },

		{.r = 0,    /* black */
		  .b = 0,
		  .g = 0,
		  .br = 0x80 },
	};

	for (int y = 0; y < CGA_HI_RES_GRAPHICS_HEIGHT; ++y) {
		for (int x = 0; x < CGA_BYTES_PER_ROW; ++x) {
			const uint16_t address_register = (ibm_pc->cga.crtc.registers[CRTC_6845_ADDRESS_HI]) << 8 | ibm_pc->cga.crtc.registers[CRTC_6845_ADDRESS_LO];
			const uint32_t offset = (y * CGA_BYTES_PER_ROW) + x;
			const uint32_t address = CGA_MM_BASE_ADDRESS + ((address_register + offset) & CGA_MM_ADDRESS_MASK);
			uint8_t byte = ibm_pc->cpu.funcs.read_mem_byte(address);
			for (int p = 0; p < 8; ++p) {
				SDL_SetRenderDrawColor(display->window->renderer, palette[byte & 0x1].r, palette[byte & 0x1].g, palette[byte & 0x1].b, palette[byte & 0x1].br);
				byte >>= 1;

				SDL_FRect rect = {
					.x = (float)CENTER(display->window->transform.w, ibm_pc->cga.width, (offset * 8) + p),
					.y = (float)CENTER(display->window->transform.h, ibm_pc->cga.height, (offset / 8) + p),
					.w = (float)display->cell_w,
					.h = (float)display->cell_h
				};
				SDL_RenderFillRect(display->window->renderer, &rect);
			}
		}
	}
}
static void cga_draw_background(DISPLAY_INSTANCE* display, SDL_FRect* rect, uint8_t attribute) {

	/* render background */
	uint8_t b = sx(attribute & CGA_ATTRIBUTE_B_BG);
	uint8_t g = sx(attribute & CGA_ATTRIBUTE_G_BG);
	uint8_t r = sx(attribute & CGA_ATTRIBUTE_R_BG);
	uint8_t br = (attribute & CGA_ATTRIBUTE_BR_BG) >> 1;
	SDL_SetRenderDrawColor(display->window->renderer, r + br, g + br, b + br, 0xFF);
	SDL_RenderFillRect(display->window->renderer, rect);
}
static void cga_draw_character(DISPLAY_INSTANCE* display, SDL_FRect* rect, uint8_t character, uint8_t attribute, int blink) {
	if (character == 0) {
		return;
	}
	/* only render character if not blinking or blink < 15 (half time) */
	if (ibm_pc->cga.mode & CGA_MODE_BLINK_ENABLE) {
		if ((attribute & CGA_ATTRIBUTE_BLINK) && blink < 15) {
			return;
		}
	}

	/* render text */
	uint8_t b = sx(attribute & CGA_ATTRIBUTE_B_FG);
	uint8_t g = sx(attribute & CGA_ATTRIBUTE_G_FG);
	uint8_t r = sx(attribute & CGA_ATTRIBUTE_R_FG);
	uint8_t br = (attribute & CGA_ATTRIBUTE_BR_FG) << 3;
	SDL_SetTextureColorMod(display->font_data->textures[character], r+br, g+br, b+br);
	SDL_RenderTexture(display->window->renderer, display->font_data->textures[character], NULL, rect);
}
static void cga_text_draw_screen(DISPLAY_INSTANCE* display) {

	/* get cell dimensions */
	display->cell_w = display->window->transform.w / ibm_pc->cga.columns;
	display->cell_h = display->window->transform.h / ibm_pc->cga.rows;
	
	/* increment blink; count to 31 */
	ibm_pc->cga.blink = (ibm_pc->cga.blink++) & 0x1F;

#if 0
	/* get boarder color */
	uint8_t b  = se(ibm_pc->cga.color & CGA_COLOR_BOADER_B);
	uint8_t g  = se(ibm_pc->cga.color & CGA_COLOR_BOADER_G);
	uint8_t r  = se(ibm_pc->cga.color & CGA_COLOR_BOADER_R);
	uint8_t br = se(ibm_pc->cga.color & CGA_COLOR_BOADER_BR);
	
	if (ibm_pc->cga.mode & CGA_MODE_HI_RES_GRAPH) {
		/* In text mode, setting this bit has the following effects:
		   - The border is always black.
		   - The characters displayed are missing columns - as if the bit pattern has 
		     been ANDed with another value. According to reenigne.org, the value is the
		     equivalent bit pattern from the 640x200 graphics mode. */
		r = 0;
		b = 0;
		g = 0;
	}

	/* draw boarder */
	SDL_SetRenderDrawColor(window_state->renderer, r, g, b, (0x80 + br) & 0xFF);
	SDL_FRect rect = { .x = 0, .y = 0, .w = (float)window_state->win.w, .h = (float)window_state->win.h };
	SDL_RenderFillRect(window_state->renderer, &rect);
#endif

	/* draw text */
	SDL_FRect rect = { 0 };
	uint8_t attribute = 0;
	int row = 0;
	int column = 0;
	for (row = 0; row < ibm_pc->cga.rows; row++) {
		for (column = 0; column < ibm_pc->cga.columns; column++) {
			const uint16_t address_register = (ibm_pc->cga.crtc.registers[CRTC_6845_ADDRESS_HI]) << 8 | ibm_pc->cga.crtc.registers[CRTC_6845_ADDRESS_LO];
			const uint32_t offset = ((row * (ibm_pc->cga.columns * 2)) + column * 2);
			const uint32_t address = CGA_MM_BASE_ADDRESS + ((address_register + offset) & CGA_MM_ADDRESS_MASK);
			const uint8_t character = ibm_pc->cpu.funcs.read_mem_byte(address);
			attribute = ibm_pc->cpu.funcs.read_mem_byte(address + 1);
			display_update_cell_position(display, &rect, ibm_pc->cga.rows, ibm_pc->cga.columns, row, column);
			cga_draw_background(display, &rect, attribute);
			cga_draw_character(display, &rect, character, attribute, ibm_pc->cga.blink);
		}
	}

	/* draw cursor */
	display_update_cursor_position(&ibm_pc->cga.crtc, CGA_BYTES_PER_ROW, &row, &column);
	display_update_cell_position(display, &rect, ibm_pc->cga.rows, ibm_pc->cga.columns, row, column);
	attribute = 0x77 | CGA_ATTRIBUTE_BLINK; /* invert BG/FG, force blink */
	cga_draw_character(display, &rect, '_', attribute, ibm_pc->cga.blink);
}
static void cga_draw_screen(WINDOW_INSTANCE* instance, DISPLAY_INSTANCE* display) {
	(void)instance;
	/*if (!(ibm_pc->cga.mode & CGA_MODE_VIDEO_ENABLE)) {
		disabled_draw_screen(display, ibm_pc->cga.width, ibm_pc->cga.height, "ADAPTER NOT ENABLED");
		return;
	}*/

	if (ibm_pc->cga.mode & CGA_MODE_GRAPHICS) {
		if (ibm_pc->cga.mode & CGA_MODE_GRAPHICS_RES_HI) {
			cga_graphics_draw_hi_res(display);
		}
		else {
			cga_graphics_draw_lo_res(display);
		}
	}
	else {
		cga_text_draw_screen(display);
	}
}

void display_on_video_adapter_changed(DISPLAY_INSTANCE* display, uint8_t video_adapter) {
	if (display->window != NULL) {
		switch (video_adapter) {
			case VIDEO_ADAPTER_MDA_80X25:
				sdl_timing_init_frame(&display->window->time, HZ_TO_MS(50.0));
				display->window->on_render = mda_draw_screen;
				display->window->on_render_param = display;

				display_set_font(display, "Bm437_IBM_MDA.FON", 9, 14);
				if (display_generate_font_map(display)) {
					exit(1);
				}
				dbg_print("[DISPLAY] Video adapter: MDA\n");
				break;
			case VIDEO_ADAPTER_CGA_40X25:
			case VIDEO_ADAPTER_CGA_80X25:
				sdl_timing_init_frame(&display->window->time, HZ_TO_MS(60.0));
				display->window->on_render = cga_draw_screen;
				display->window->on_render_param = display;

				display_set_font(display, "Bm437_IBM_CGA.FON", 8, 8);
				if (display_generate_font_map(display)) {
					exit(1);
				}
				dbg_print("[DISPLAY] Video adapter: CGA\n");
				break;
			case VIDEO_ADAPTER_NONE:
				sdl_timing_init_frame(&display->window->time, HZ_TO_MS(60.0));
				display->window->on_render = dummy_draw_screen;
				display->window->on_render_param = display;
				dbg_print("[DISPLAY] Video adapter: DUMMY\n");
				break;
		}
	}
	else {
		dbg_print("[DISPLAY] Video adapter: HEADLESS\n");
	}
}

int display_create(DISPLAY_INSTANCE** display, WINDOW_INSTANCE* window) {
	if (display == NULL) {
		dbg_print("[DISPLAY] display is NULL\n");
		return 1;
	}

	*display = (DISPLAY_INSTANCE*)calloc(1, sizeof(DISPLAY_INSTANCE));
	if (*display == NULL) {
		dbg_print("[DISPLAY] Failed to allocate memory\n");
		return 1;
	}

	(*display)->window = window;

	/* Create font texture map */
	if (font_create_map(&(*display)->font_data)) {
		return 1;
	}
	
	return 0;
}

void display_set_font(DISPLAY_INSTANCE* display, const char* font_path, int font_w, int font_h) {
	display->font_path = font_path;
	display->font_w = font_w;
	display->font_h = font_h;
}
int display_generate_font_map(DISPLAY_INSTANCE* display) {
	if (display == NULL) {
		dbg_print("[DISPLAY] display is NULL\n");
		return 1;
	}

	/* Create font texture map */
	if (display->font_data->ttf == NULL) {
		if (font_open_font(display->font_data, display->font_path)) {
			return 1;
		}
	}

	if (font_create_textures(display->window->renderer, display->window->text_engine, display->font_w, display->font_h, display->font_data)) {
		return 1;
	}

	return 0;
}
void display_destroy(DISPLAY_INSTANCE* instance) {
	if (instance != NULL) {
		
		if (instance->font_data != NULL) {
			font_destroy_map(instance->font_data);
			instance->font_data = NULL;
		}

		free(instance);
	}
}
