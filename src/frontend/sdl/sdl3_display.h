/* sdl3_display.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Display
 */

#ifndef SDL_DISPLAY_H
#define SDL_DISPLAY_H

typedef struct FONT_TEXTURE_DATA FONT_TEXTURE_DATA;
typedef struct WINDOW_INSTANCE WINDOW_INSTANCE;

typedef struct DISPLAY_INSTANCE {
	WINDOW_INSTANCE* window; /* assigned window */
	/* Font */
	FONT_TEXTURE_DATA* font_data;
	float cell_w;
	float cell_h;
	int on_render_index;
	float offset_y;
	int scanline_emu;
	int correct_aspect_ratio;
	int scale_mode;
} DISPLAY_INSTANCE;

/* Updates the rendering settings and font based on the current video adapter type (MDA, CGA, or none). */
void display_on_video_adapter_changed(DISPLAY_INSTANCE* display, const uint8_t video_adapter);

int display_create(DISPLAY_INSTANCE** instance, WINDOW_INSTANCE* window);
void display_destroy(DISPLAY_INSTANCE* instance);

int display_generate_font_map(DISPLAY_INSTANCE* display, const char* font_path);

#endif
