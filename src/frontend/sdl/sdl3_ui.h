
#ifndef SDL3_UI
#define SDL3_UI

typedef struct DISPLAY_INSTANCE DISPLAY_INSTANCE;

typedef struct UI_CONTEXT {
	float menu_slide;
	float slide_offset;
} UI_CONTEXT;

void ui_update(UI_CONTEXT* ui_context, DISPLAY_INSTANCE* display);

#endif
