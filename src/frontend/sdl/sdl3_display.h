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
	const char* font_path;
	int font_w;
	int font_h;
	int cell_w;
	int cell_h;
} DISPLAY_INSTANCE;

/* Updates the rendering settings and font based on the current video adapter type (MDA, CGA, or none). */
void display_on_video_adapter_changed(DISPLAY_INSTANCE* display, uint8_t video_adapter);

int display_create(DISPLAY_INSTANCE** instance, WINDOW_INSTANCE* window);
void display_destroy(DISPLAY_INSTANCE* instance);

void display_set_font(DISPLAY_INSTANCE* display, const char* font_path, int font_w, int font_h);
int display_generate_font_map(DISPLAY_INSTANCE* display);

#endif
