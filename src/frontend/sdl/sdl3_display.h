/* sdl3_display.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Display
 */

#ifndef SDL_DISPLAY_H
#define SDL_DISPLAY_H

typedef struct FONT_TEXTURE_DATA FONT_TEXTURE_DATA;
typedef struct WINDOW_INSTANCE WINDOW_INSTANCE;

#define DISPLAY_VIEW_CROPPED 0
#define DISPLAY_VIEW_FULL    1

#define DISPLAY_SCALE_FIT     0
#define DISPLAY_SCALE_STRETCH 1

#define FONT_PATH_LEN 256

typedef struct DISPLAY_CONFIG {
	int scanline_emu;
	int correct_aspect_ratio;
	int display_view_mode;
	int display_scale_mode;
	int texture_scale_mode;
	char mda_font[FONT_PATH_LEN];
	char cga_font[FONT_PATH_LEN];
} DISPLAY_CONFIG;

typedef struct DISPLAY_INSTANCE {
	WINDOW_INSTANCE* window; /* assigned window */
	/* Font */
	FONT_TEXTURE_DATA* font_data;
	float cell_w;
	float cell_h;
	int on_render_index;
	float offset_y;
	DISPLAY_CONFIG config;
} DISPLAY_INSTANCE;

/* Updates the rendering settings and font based on the current video adapter type (MDA, CGA, or none). */
void display_on_video_adapter_changed(DISPLAY_INSTANCE* display, const uint8_t video_adapter);

int display_create(DISPLAY_INSTANCE** instance, WINDOW_INSTANCE* window);
void display_destroy(DISPLAY_INSTANCE* instance);

int display_generate_font_map(DISPLAY_INSTANCE* display, const char* font_path);

int display_set_window(DISPLAY_INSTANCE* display, WINDOW_INSTANCE* window);

#endif
