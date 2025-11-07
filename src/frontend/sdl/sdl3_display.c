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
#include "sdl3_typedefs.h"

#include "backend/video/mda.h"
#include "backend/video/cga.h"
#include "backend/ibm_pc.h"

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

static void get_cell_position(const DISPLAY_INSTANCE* const display, const float offset_x, const float offset_y, const int x, const int y, SDL_FRect* const rect) {
	rect->x = offset_x + (x * display->cell_w);
	rect->y = offset_y + (y * display->cell_h);
	rect->w = display->cell_w;
	rect->h = display->cell_h;
}
static void get_cell_dimensions(DISPLAY_INSTANCE* display, const int w, const int h, float* const offset_x, float* const offset_y) {
	// Compute available drawable area
	const float window_w = (float)display->window->transform.w;
	const float window_h = (float)display->window->transform.h - display->offset_y;
		
	switch (display->config.display_scale_mode) {
		case DISPLAY_SCALE_FIT: {
			float aspect_correction_y = 1.0f;
			if (display->config.correct_aspect_ratio) {
				/* 1.2 = K * (4/3) * (w/h)
				 * 1.2 = K * 1.3333 * 1.6
				 * 1.2 = K * 2.1333
				 * K = 1.2 / 2.1333 = 0.5625 */
				const float aspect_correction_factor = 0.5625f;
				const float physical_aspect = 4.0f / 3.0f;
				const float actual_aspect = (float)w / (float)h; 
				aspect_correction_y = aspect_correction_factor * physical_aspect * actual_aspect;
			}

			// Compute the *effective* display height
			const float effective_height = h * aspect_correction_y;

			// Determine scale to fit both width and height
			const float scale_x = window_w / w;
			const float scale_y = window_h / effective_height;
			const float scale = (scale_x < scale_y) ? scale_x : scale_y;

			// Set cell size
			display->cell_w = scale;
			display->cell_h = scale * aspect_correction_y;
		} break;

		case DISPLAY_SCALE_STRETCH: {
			const float scale_x = window_w / w;
			const float scale_y = window_h / h;

			// Set cell size
			display->cell_w = scale_x;
			display->cell_h = scale_y;
		} break;
	}

	// Compute offsets to center image
	float drawn_w = w * display->cell_w;
	float drawn_h = h * display->cell_h;

	*offset_x = (window_w - drawn_w) / 2.0f;
	*offset_y = display->offset_y + (window_h - drawn_h) / 2.0f;
}

static void get_cursor_position(const CRTC_6845* const crtc, uint8_t* row, uint8_t* column) {
	/* calculate cursor position */
	if (crtc->hdisp > 0) {
		*row = (crtc->cursor_address / crtc->hdisp) & 0xFF;
		*column = (crtc->cursor_address % crtc->hdisp) & 0xFF;
	}
}

static void fill_screen(const DISPLAY_INSTANCE* const display, const COLOR_RGB color) {
	const SDL_FRect rect = {
		.x = 0,
		.y = 0,
		.w = (float)display->window->transform.w,
		.h = (float)display->window->transform.h,
	};
	SDL_SetRenderDrawColor(display->window->renderer, color.r, color.g, color.b, 0xFF);
	SDL_RenderFillRect(display->window->renderer, &rect);
}

static void draw_overscan(const WINDOW_INSTANCE* const window, const COLOR_RGB color, const float offset_x, const float offset_y) {
	const SDL_FRect rect = {
		.x = offset_x,
		.y = offset_y,
		.w = (float)window->transform.w - (offset_x * 2),
		.h = (float)window->transform.h - (offset_y * 2),
	};
	SDL_SetRenderDrawColor(window->renderer, color.r, color.g, color.b, 0xFF);
	SDL_RenderFillRect(window->renderer, &rect);
}

/* Dummy display */
static void disabled_draw_screen(const DISPLAY_INSTANCE* const display) {
	const COLOR_RGB col = { 0, 0, 0 };
	fill_screen(display, col);
}
static void dummy_draw_screen(DISPLAY_INSTANCE* display, void* param2) {
	(void)param2;
	disabled_draw_screen(display);
}

/* MDA */
static void mda_draw_background(DISPLAY_INSTANCE* display, const SDL_FRect* rect, const uint8_t attribute) {
	/* render background */
	if (attribute & MDA_ATTRIBUTE_BW) {
		SDL_SetRenderDrawColor(display->window->renderer, 0, 0, 0, 0xFF);
	}
	else {
		SDL_SetRenderDrawColor(display->window->renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	}
	SDL_RenderFillRect(display->window->renderer, rect);
}
static void mda_draw_character(DISPLAY_INSTANCE* display, const SDL_FRect* rect, const uint8_t character, const uint8_t attribute) {
	/* only render character if not blinking or blink < blink_count (half time) */
	if ((ibm_pc->mda.mode & MDA_MODE_BLINK_ENABLE) && (attribute & MDA_ATTRIBUTE_BLINK) && ibm_pc->mda.blink < 0x0F) {
		return;
	}

	/* render text */
	if (attribute & MDA_ATTRIBUTE_BW) {
		SDL_SetTextureColorMod(display->font_data->textures[character], 0xFF, 0xFF, 0xFF);
	}
	else {
		SDL_SetTextureColorMod(display->font_data->textures[character], 0, 0, 0);
	}
	SDL_SetTextureScaleMode(display->font_data->textures[character], display->config.texture_scale_mode);
	SDL_RenderTexture(display->window->renderer, display->font_data->textures[character], NULL, rect);
}
static void mda_draw_cursor(DISPLAY_INSTANCE* display, const SDL_FRect* rect, const uint8_t attribute) {

	if ((ibm_pc->mda.crtc.cursor_start & CRTC_6845_CURSOR_ATTR_MASK) == CRTC_6845_CURSOR_ATTR_DISABLED) {
		return;
	}

	/* dont render cursor if blink rate < half time */
	if ((ibm_pc->mda.blink & 0x1F) < 0x0F) {
		return;
	}

	/* render cursor */
	if (attribute & MDA_ATTRIBUTE_BW) {
		SDL_SetTextureColorMod(display->font_data->textures['_'], 0xFF, 0xFF, 0xFF);
	}
	else {
		SDL_SetTextureColorMod(display->font_data->textures['_'], 0, 0, 0);
	}
	SDL_SetTextureScaleMode(display->font_data->textures['_'], display->config.texture_scale_mode);
	SDL_RenderTexture(display->window->renderer, display->font_data->textures['_'], NULL, rect);
}
static void mda_text_draw_screen(DISPLAY_INSTANCE* display, MDA* mda) {

	/* get cell dimensions */
	float offset_x;
	float offset_y;
	get_cell_dimensions(display, mda->crtc.hdisp, mda->crtc.vdisp, &offset_x, &offset_y);

	/* increment blink; count to 31 */
	mda->blink = (mda->blink++) & 0x1F;

	for (uint8_t row = 0; row < mda->crtc.vdisp; ++row) {
		for (uint8_t column = 0; column < mda->crtc.hdisp; ++column) {
			const uint16_t char_index = mda->crtc.start_address + row * mda->crtc.hdisp + column;
			const uint32_t char_address = MDA_PHYS_ADDRESS(char_index * 2);
			const uint8_t character = ibm_pc->cpu.funcs.read_mem_byte(char_address);
			const uint8_t attribute = ibm_pc->cpu.funcs.read_mem_byte(char_address + 1);
			
			/* get cell position */
			SDL_FRect rect;
			get_cell_position(display, offset_x, offset_y, column, row, &rect);
			
			/* draw text */
			mda_draw_background(display, &rect, attribute);
			mda_draw_character(display, &rect, character, attribute);

			/* draw cursor */
			if (char_index == mda->crtc.cursor_address) {
				mda_draw_cursor(display, &rect, attribute);
			}
		}
	}
}
static void mda_draw_screen(DISPLAY_INSTANCE* display, MDA* mda) {
	if (!(mda->mode & MDA_MODE_VIDEO_ENABLE)) {
		disabled_draw_screen(display);
		return;
	}
	
	mda_text_draw_screen(display, mda);
}

/* CGA */
#define CGA_COLOR_BLACK          0  /* black */
#define CGA_COLOR_BLUE           1  /* blue */
#define CGA_COLOR_GREEN          2  /* green */
#define CGA_COLOR_CYAN           3  /* cyan */
#define CGA_COLOR_RED            4  /* red */
#define CGA_COLOR_MAGENTA        5  /* magenta */
#define CGA_COLOR_BROWN          6  /* brown */
#define CGA_COLOR_WHITE          7  /* white */
#define CGA_COLOR_BRIGHT_BLACK   8  /* bright black */
#define CGA_COLOR_BRIGHT_BLUE    9  /* bright blue */
#define CGA_COLOR_BRIGHT_GREEN   10 /* bright green */
#define CGA_COLOR_BRIGHT_CYAN    11 /* bright cyan */
#define CGA_COLOR_BRIGHT_RED     12 /* bright red */
#define CGA_COLOR_BRIGHT_MAGENTA 13 /* bright magenta */
#define CGA_COLOR_BRIGHT_YELLOW  14 /* bright yellow */
#define CGA_COLOR_BRIGHT_WHITE   15 /* bright white */

static const COLOR_RGB cga_colors[16] = {
		{ 0x00, 0x00, 0x00 }, /* black */
		{ 0x00, 0x00, 0xAA }, /* blue */
		{ 0x00, 0xAA, 0x00 }, /* green */
		{ 0x00, 0xAA, 0xAA }, /* cyan */
		{ 0xAA, 0x00, 0x00 }, /* red */
		{ 0xAA, 0x00, 0xAA }, /* magenta */
		{ 0xAA, 0x55, 0x00 }, /* brown */
		{ 0xAA, 0xAA, 0xAA }, /* white */

		{ 0x55, 0x55, 0x55 }, /* bright black */
		{ 0x55, 0x55, 0xFF }, /* bright blue */
		{ 0x55, 0xFF, 0x55 }, /* bright green */
		{ 0x55, 0xFF, 0xFF }, /* bright cyan */
		{ 0xFF, 0x55, 0x55 }, /* bright red */
		{ 0xFF, 0x55, 0xFF }, /* bright magenta */
		{ 0xFF, 0xFF, 0x55 }, /* bright yellow */
		{ 0xFF, 0xFF, 0xFF }, /* bright white */
};

static void cga_graphics_draw_lo_res(DISPLAY_INSTANCE* display, CGA* cga) {
	const uint8_t palette0[8] = {
		(cga->color & CGA_COLOR_BG),
		CGA_COLOR_GREEN,
		CGA_COLOR_RED,
		CGA_COLOR_BROWN,
		(cga->color & CGA_COLOR_BG),
		CGA_COLOR_BRIGHT_GREEN,
		CGA_COLOR_BRIGHT_RED,
		CGA_COLOR_BRIGHT_YELLOW,
	};
	const uint8_t palette1[8] = {
		(cga->color & CGA_COLOR_BG),
		CGA_COLOR_CYAN,
		CGA_COLOR_MAGENTA,
		CGA_COLOR_WHITE,
		(cga->color & CGA_COLOR_BG),
		CGA_COLOR_BRIGHT_CYAN,
		CGA_COLOR_BRIGHT_MAGENTA,
		CGA_COLOR_BRIGHT_WHITE,
	};
	const uint8_t palette2[8] = {
		(cga->color & CGA_COLOR_BG),
		CGA_COLOR_CYAN,
		CGA_COLOR_RED,
		CGA_COLOR_WHITE,
		(cga->color & CGA_COLOR_BG),
		CGA_COLOR_BRIGHT_CYAN,
		CGA_COLOR_BRIGHT_RED,
		CGA_COLOR_BRIGHT_WHITE,
	};
		
	int width = 320;
	int height = 200;

	const int pixels_per_byte = 4;
	const int bytes_per_row = width / pixels_per_byte;

	/* get cell dimensions */
	float offset_x;
	float offset_y;
	get_cell_dimensions(display, width, height, &offset_x, &offset_y);

#if 0
	/* draw boarder */
	fill_screen(display, cga_colors[(cga->color & CGA_COLOR_BG)]);
#endif

	// choose color palette
	const uint8_t* palette;
	if (cga->mode & CGA_MODE_BW) {
		palette = &palette2[0];
	}
	else if (cga->color & CGA_COLOR_PALETTE) {
		palette = &palette1[0];
	} 
	else {
		palette = &palette0[0];
	}

	for (int y = 0; y < height; ++y) {
		uint16_t base = (y & 1) ? 0x2000 : 0x0000;
		uint32_t row_offset = (y >> 1) * bytes_per_row;

		for (int x = 0; x < bytes_per_row; ++x) {
			uint32_t address = CGA_PHYS_ADDRESS(base + row_offset + x);
			uint8_t byte = ibm_pc->cpu.funcs.read_mem_byte(address);

			for (int bit = 6; bit >= 0; bit -= 2) {
				const uint8_t color_index = ((byte >> bit) & 0x3) | (cga->color & CGA_COLOR_BRIGHT_FG) >> 2;
				const COLOR_RGB c = cga_colors[palette[color_index]];
				SDL_SetRenderDrawColor(display->window->renderer, c.r, c.g, c.b, 0xFF);

				/* get cell position */
				SDL_FRect rect;
				get_cell_position(display, offset_x, offset_y, ((x * pixels_per_byte) + (3 - bit / 2)), y, &rect);

				SDL_RenderFillRect(display->window->renderer, &rect);
			}
		}
	}
}
static void cga_graphics_draw_hi_res(DISPLAY_INSTANCE* display, CGA* cga) {

	const int width = 640;
	const int height = 200;

	const int pixels_per_byte = 8;
	const int bytes_per_row = width / pixels_per_byte;

	/* get cell dimensions */
	float offset_x;
	float offset_y;
	get_cell_dimensions(display, width,height, &offset_x, &offset_y);

#if 0
	/* draw boarder */
	fill_screen(display, cga_colors[CGA_COLOR_BLACK]);
#endif

	for (int y = 0; y < height; ++y) {
		uint16_t base = (y & 1) ? 0x2000 : 0x0000;
		uint32_t row_offset = (y >> 1) * bytes_per_row;

		for (int x = 0; x < bytes_per_row; ++x) {
			uint32_t address = CGA_PHYS_ADDRESS(base + row_offset + x);
			uint8_t byte = ibm_pc->cpu.funcs.read_mem_byte(address);

			for (int bit = 7; bit >= 0; --bit) {
				const uint8_t color_index = (byte >> bit) & 0x1;
				const COLOR_RGB* c = &cga_colors[(cga->color & CGA_COLOR_FG) * color_index];
				SDL_SetRenderDrawColor(display->window->renderer, c->r, c->g, c->b, 0xFF);

				/* get cell position */
				SDL_FRect rect;
				get_cell_position(display, offset_x, offset_y, ((x * pixels_per_byte) + (7 - bit)), y, &rect);

				SDL_RenderFillRect(display->window->renderer, &rect);
			}
		}
	}
}

static void cga_draw_background(DISPLAY_INSTANCE* display, const SDL_FRect* rect, const uint8_t attribute) {
	/* render background */
	uint8_t index = (attribute & CGA_ATTRIBUTE_BG) >> 4;
	if (ibm_pc->cga.mode & CGA_MODE_BLINK_ENABLE) {
		index &= 0x07; /* if MODE bit5 = 1, then ignore intensity bit (bit7) in attribute */
	}
	const COLOR_RGB col = cga_colors[index];
	SDL_SetRenderDrawColor(display->window->renderer, col.r, col.g, col.b, 0xFF);
	SDL_RenderFillRect(display->window->renderer, rect);
}
static void cga_draw_character(DISPLAY_INSTANCE* display, const SDL_FRect* rect, const uint8_t character, const uint8_t attribute) {
	
	/* only render char if MODE bit5 = 1 and ATTRIBUTE bit7 = 1 and blink interval is half time */
	if ((ibm_pc->cga.mode & CGA_MODE_BLINK_ENABLE) && (attribute & CGA_ATTRIBUTE_BLINK) && ibm_pc->cga.blink < 0x0F) {
		return;
	}

	float scanline_ratio = 1.0;
	if (display->config.scanline_emu) {
		scanline_ratio = (ibm_pc->cga.crtc.max_scanline + 1) / 8.0f;
	}

	// Get the actual texture dimensions
	float tex_w;
	float tex_h;
	SDL_GetTextureSize(display->font_data->textures[character], &tex_w, &tex_h);

	SDL_FRect src = {
		.x = 0,
		.y = 0,
		.w = tex_w,
		.h = tex_h * scanline_ratio,
	};

	/* render text */
	const COLOR_RGB col = cga_colors[(attribute & CGA_ATTRIBUTE_FG)];
	SDL_SetTextureColorMod(display->font_data->textures[character], col.r, col.g, col.b);
	SDL_SetTextureScaleMode(display->font_data->textures[character], display->config.texture_scale_mode);
	SDL_RenderTexture(display->window->renderer, display->font_data->textures[character], &src, rect);
}
static void cga_draw_cursor(DISPLAY_INSTANCE* display, const SDL_FRect* rect, const uint8_t attribute) {

	/* The IBM CGA Adapter ignores the CRTCs internal blink rate logic.
	 Bits 5,6 = b01 disables the cursor (The CRTC stops asserting CURSOR).
	 Bits 5,6 = b00, b10, b11 all display the cursor and blinks at the same fixed hardware rate. */
	
	if ((ibm_pc->cga.crtc.cursor_start & CRTC_6845_CURSOR_ATTR_MASK) == CRTC_6845_CURSOR_ATTR_DISABLED) {
		return;
	}

	/* dont render cursor if blink rate < half time */
	if ((ibm_pc->cga.blink & 0x1F) < 0x0F) {
		return;
	}

	float scanline_ratio = 1.0;
	if (display->config.scanline_emu) {
		scanline_ratio = (ibm_pc->cga.crtc.max_scanline + 1) / 8.0f;
	}

	SDL_FRect src = {
		.x = 0,
		.y = 0,
		.w = rect->w,
		.h = rect->h * scanline_ratio,
	};

	/* render text */
	const COLOR_RGB  col = cga_colors[(attribute & CGA_ATTRIBUTE_FG)];
	SDL_SetTextureColorMod(display->font_data->textures['_'], col.r, col.g, col.b);
	SDL_SetTextureScaleMode(display->font_data->textures['_'], display->config.texture_scale_mode);
	SDL_RenderTexture(display->window->renderer, display->font_data->textures['_'], &src, rect);
}
static void cga_text_draw_screen(DISPLAY_INSTANCE* display, CGA* cga) {

	/* get cell dimensions */
	float offset_x;
	float offset_y;
	get_cell_dimensions(display, cga->crtc.hdisp, cga->crtc.vdisp, &offset_x, &offset_y);

	/* increment blink; count to 31 */
	cga->blink = (cga->blink++) & 0x1F;
	
#if 0
	/* draw boarder */
	if (cga->mode & CGA_MODE_GRAPHICS_RES_HI) {
		/* In text mode, setting this bit has the following effects:
		   - The border is always black.
		   - The characters displayed are missing columns - as if the bit pattern has 
		     been ANDed with another value. According to reenigne.org, the value is the
		     equivalent bit pattern from the 640x200 graphics mode. */
		fill_screen(display, cga_colors[CGA_COLOR_BLACK]);
	}
	else {
		fill_screen(display, cga_colors[cga->color & CGA_COLOR_BG]);
	}
#endif

	for (uint8_t row = 0; row < cga->crtc.vdisp; ++row) {
		for (uint8_t column = 0; column < cga->crtc.hdisp; ++column) {
			const uint16_t char_index = cga->crtc.start_address + row * cga->crtc.hdisp + column;
			const uint32_t char_address = CGA_PHYS_ADDRESS(char_index * 2);
			const uint8_t character = ibm_pc->cpu.funcs.read_mem_byte(char_address);
			const uint8_t attribute = ibm_pc->cpu.funcs.read_mem_byte(char_address + 1);
			
			/* get cell position */
			SDL_FRect rect;
			get_cell_position(display, offset_x, offset_y, column, row, &rect);

			/* draw text */
			cga_draw_background(display, &rect, attribute);
			cga_draw_character(display, &rect, character, attribute);

			/* draw cursor */
			if (char_index == cga->crtc.cursor_address) {
				cga_draw_cursor(display, &rect, attribute);
			}
		}
	}
}

static void cga_draw_screen(DISPLAY_INSTANCE* display, CGA* cga) {
	if (!(cga->mode & CGA_MODE_VIDEO_ENABLE)) {
		disabled_draw_screen(display);
		return;
	}

	if (cga->mode & CGA_MODE_GRAPHICS) {
		if (cga->mode & CGA_MODE_GRAPHICS_RES_HI) {
			cga_graphics_draw_hi_res(display, cga);
		}
		else {
			cga_graphics_draw_lo_res(display, cga);
		}
	}
	else {
		cga_text_draw_screen(display, cga);
	}
}

void display_on_video_adapter_changed(DISPLAY_INSTANCE* display, const uint8_t video_adapter) {
	if (display->window != NULL) {
		switch (video_adapter) {
			case VIDEO_ADAPTER_MDA_80X25:
				sdl_timing_init_frame(&display->window->time, HZ_TO_MS(50.0));
				window_instance_set_cb_on_render(display->window, display->on_render_index, mda_draw_screen, display, &ibm_pc->mda);

				if (display_generate_font_map(display, display->config.mda_font)) {
					exit(1);
				}
				dbg_print("[DISPLAY] Video adapter: MDA\n");
				break;
			case VIDEO_ADAPTER_CGA_40X25:
			case VIDEO_ADAPTER_CGA_80X25:
				sdl_timing_init_frame(&display->window->time, HZ_TO_MS(60.0));
				window_instance_set_cb_on_render(display->window, display->on_render_index, cga_draw_screen, display, &ibm_pc->cga);

				if (display_generate_font_map(display, display->config.cga_font)) {
					exit(1);
				}
				dbg_print("[DISPLAY] Video adapter: CGA\n");
				break;
			case VIDEO_ADAPTER_NONE:
				sdl_timing_init_frame(&display->window->time, HZ_TO_MS(60.0));
				window_instance_set_cb_on_render(display->window, display->on_render_index, dummy_draw_screen, display, NULL);
				dbg_print("[DISPLAY] Video adapter: DUMMY\n");
				break;
		}
	}
	else {
		dbg_print("[DISPLAY] Video adapter: HEADLESS\n");
	}
}
int display_generate_font_map(DISPLAY_INSTANCE* display, const char* font_path) {
	if (display == NULL) {
		dbg_print("[DISPLAY] display is NULL\n");
		return 1;
	}

	if (font_open_font(display->font_data, font_path)) {
		return 1;
	}

	/* Create font texture map */
	font_destroy_textures(display->font_data);
	if (font_create_textures(display->window->renderer, display->window->text_engine, display->font_data)) {
		return 1;
	}

	font_close_font(display->font_data);
	return 0;
}

int display_set_window(DISPLAY_INSTANCE* display, WINDOW_INSTANCE* window) {
	if (display->on_render_index == -1) {
		display->window = window;
		display->on_render_index = window_instance_add_cb_on_render(window, dummy_draw_screen, display, NULL);
		return 0;
	}
	else {
		dbg_print("[DISPLAY] Failed to set window on_render cb. CB already set.\n");
		return 1;
	}
}

int display_create(DISPLAY_INSTANCE** display, WINDOW_INSTANCE* window) {
	if (display == NULL) {
		dbg_print("[DISPLAY] display is NULL\n");
		return 1;
	}

	*display = calloc(1, sizeof(DISPLAY_INSTANCE));
	if (*display == NULL) {
		dbg_print("[DISPLAY] Failed to allocate memory\n");
		return 1;
	}
	
	(*display)->on_render_index = -1;
	if (window != NULL) {
		display_set_window(*display, window);
	}

	/* Create font texture map */
	if (font_create_map(&(*display)->font_data)) {
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
