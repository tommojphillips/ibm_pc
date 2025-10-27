/* sdl3_window.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
*/

#ifndef WINDOW_SDL_H
#define WINDOW_SDL_H

#include <stdint.h>

#include "sdl3_timing.h"

#include "backend/ring_buffer.h"

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef union SDL_Event SDL_Event;
typedef struct TTF_TextEngine TTF_TextEngine;
typedef struct TTF_Font TTF_Font;

/* actual window width */
#define WINDOW_W (instance->transform.w)

/* actual window height */
#define WINDOW_H (instance->transform.h)

#define WINDOW_INSTANCE_STATE_DESTROYED   0x0
#define WINDOW_INSTANCE_STATE_FULL_SCREEN 0x1
#define WINDOW_INSTANCE_STATE_OPEN        0x2
#define WINDOW_INSTANCE_STATE_CREATED     0x4

typedef struct WINDOW_INSTANCE WINDOW_INSTANCE;
typedef struct FONT_TEXTURE_DATA FONT_TEXTURE_DATA;

/* Window Transform */
typedef struct WINDOW_TRANSFORM {
	int32_t x;
	int32_t y;
	int32_t h;
	int32_t w;
} WINDOW_TRANSFORM;

/* Window Manager */
typedef struct WINDOW_MANAGER {
	WINDOW_INSTANCE* instances;
	TTF_TextEngine* text_engine;
	uint16_t instance_count;
	uint16_t instance_index;
	uint16_t instances_open;
} WINDOW_MANAGER;

typedef void(*WINDOW_INSTANCE_CB)(void* instance, void* param);

/* Window Instance */
typedef struct WINDOW_INSTANCE {
	/* Window */
	SDL_Window* window;
	SDL_Renderer* renderer;
	TTF_TextEngine* text_engine;
	SDL_WindowID window_id;
	uint32_t window_state;
	WINDOW_TRANSFORM transform;
	const char* title;
	
	WINDOW_INSTANCE_CB* on_render;
	void** on_render_param;
	int32_t on_render_count;
	int32_t on_render_index;

	WINDOW_INSTANCE_CB* on_process_event;
	int32_t on_process_event_count;
	int32_t on_process_event_index;
	
	FRAME_STATE time;
	
} WINDOW_INSTANCE;

#ifdef __cplusplus
extern "C" {
#endif

/* Create New Window Instance.
 Allocates memory for the window instance. */
int window_instance_create(WINDOW_INSTANCE** instance);

/* Open Window Instance.
 Creates the window and renderer. */
int window_instance_open(WINDOW_INSTANCE* instance);

/* Close Window Instance.
 Destroys the window and renderer. */
int window_instance_close(WINDOW_INSTANCE* instance);

/* Destroy Window Instance.
 Frees memory for the window instance. */
int window_instance_destroy(WINDOW_INSTANCE* instance);

/* Set Window Instance Transform */
void window_instance_set_transform(WINDOW_INSTANCE* instance, int32_t x, int32_t y, int32_t w, int32_t h);

/* Set Window Instance Min Size. Window must be open. */
void window_instance_set_min_size(WINDOW_INSTANCE* instance, int32_t w, int32_t h);

int window_instance_is_full_screen(WINDOW_INSTANCE* instance);
void window_instance_set_full_screen(WINDOW_INSTANCE* instance, int fullscreen);
void window_instance_toggle_full_screen(WINDOW_INSTANCE* instance);

int window_instance_set_cb_on_process_event(WINDOW_INSTANCE* instance, WINDOW_INSTANCE_CB cb);
int window_instance_set_cb_on_render(WINDOW_INSTANCE* instance, WINDOW_INSTANCE_CB cb, void* cb_param);

/* Create Window Manager. 
 window_count: The amount of window instances to allocate memory for. 
 Returns: 1 if error, 0 if success */
int window_manager_create(uint16_t window_count);

/* Destroy Window Manager. */
void window_manager_destroy(void);

/* Process Event. */
int window_manager_process_event(SDL_Event* e);

/* Update Window Manager */
void window_manager_update(void);

#ifdef __cplusplus
};
#endif

#endif
