/* sdl3_ui.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 */

#ifndef SDL3_UI
#define SDL3_UI

typedef struct DISPLAY_INSTANCE DISPLAY_INSTANCE;
typedef struct SDL_DialogFileFilter SDL_DialogFileFilter;

typedef struct UI_FILE_DIAG_CONTEXT {
	void* userparam;
	int index;
	int flag;
} UI_FILE_DIAG_CONTEXT;

typedef struct UI_CONTEXT {
	float menu_slide;
	float slide_offset;
	UI_FILE_DIAG_CONTEXT diag_context;
	SDL_PropertiesID diag_properties;
	char* current_directory;
	char* disk_directory;
	char* hdd_directory;
	int dbg;
	char buffer[32];
} UI_CONTEXT;

void ui_context_create(UI_CONTEXT* ui_context);
void ui_context_destroy(UI_CONTEXT* ui_context);

void ui_update(UI_CONTEXT* ui_context, DISPLAY_INSTANCE* display);

#endif
