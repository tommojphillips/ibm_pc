/* sdl3_window.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
*/

#ifndef WINDOW_SDL_H
#define WINDOW_SDL_H

#include <stdint.h>

#include "sdl3_timing.h"

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef union SDL_Event SDL_Event;
typedef struct TTF_TextEngine TTF_TextEngine;
typedef struct TTF_Font TTF_Font;

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

typedef void(*WINDOW_INSTANCE_CB_ON_PROCESS_EVENT)(WINDOW_INSTANCE* instance, SDL_Event* e);

typedef void(*WINDOW_INSTANCE_CB)(void* param1, void* param2);

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
	void** on_render_param1;
	void** on_render_param2;
	int32_t on_render_count;
	int32_t on_render_index;

	WINDOW_INSTANCE_CB_ON_PROCESS_EVENT* on_process_event;
	int32_t on_process_event_count;
	int32_t on_process_event_index;
	
	FRAME_STATE time;
	
	WINDOW_MANAGER* manager;
} WINDOW_INSTANCE;

#ifdef __cplusplus
extern "C" {
#endif

/* Create New Window Instance. Allocates memory for the window instance. 
 manager:  The window manager
 instance: The window instance 
 Returns:  -1 if error, The index of the window if success */
int window_instance_create(WINDOW_MANAGER* manager, WINDOW_INSTANCE** instance);

/* Open Window Instance. Creates the window and renderer. 
 instance: The window instance 
 Returns:  1 if error, 0 if success */
int window_instance_open(WINDOW_INSTANCE* instance);

/* Close Window Instance. Destroys the window and renderer. 
 instance: The window instance 
 Returns:  1 if error, 0 if success */
int window_instance_close(WINDOW_INSTANCE* instance);

/* Destroy Window Instance.
 Frees memory for the window instance. 
 instance: The window instance 
 Returns:  1 if error, 0 if success */
int window_instance_destroy(WINDOW_INSTANCE* instance);

/* Set Window Instance Transform 
 instance: The window instance */
void window_instance_set_transform(WINDOW_INSTANCE* instance, int32_t x, int32_t y, int32_t w, int32_t h);

/* Set Window Instance Min Size. Window must be open. 
 instance: The window instance */
void window_instance_set_min_size(WINDOW_INSTANCE* instance, int32_t w, int32_t h);

/* Get Window Instance fullscreen. Window must be open.
 instance: The window instance 
 Returns:  1 if window is fullscreen, 0 if not */
int window_instance_is_full_screen(WINDOW_INSTANCE* instance);

/* Set Window Instance fullscreen. Window must be open. 
 instance: The window instance */
void window_instance_set_full_screen(WINDOW_INSTANCE* instance, int fullscreen);

/* Toggle Window Instance fullscreen. Window must be open.
 instance: The window instance */
void window_instance_toggle_full_screen(WINDOW_INSTANCE* instance);

/* Add a on_process_event callback. WINDOW_INSTANCE* will be passed to the callback as parameter1. SDL_Event* will be passed to the callback as parameter2
 instance:  The window instance
 cb:        The callback function
 Returns:   -1 if error, The index of the callback if success */
int window_instance_add_cb_on_process_event(WINDOW_INSTANCE* instance, WINDOW_INSTANCE_CB_ON_PROCESS_EVENT cb);

/* Add a on_render callback. 
 instance:  The window instance
 cb:        The callback function
 cb_param1: The callback param1. If NULL, the window instance will be passed to the callback function.
 cb_param2: The callback param2. If NULL, NULL will be passed to the callback function.
 Returns:   -1 if error, The index of the callback if success */
int window_instance_add_cb_on_render(WINDOW_INSTANCE* instance, WINDOW_INSTANCE_CB cb, void* cb_param1, void* cb_param2);


/* Set a on_process_event callback of a pre-existing callback. WINDOW_INSTANCE* will be passed to the callback as parameter1. SDL_Event* will be passed to the callback as parameter2
 instance:  The window instance to add a callback to.
 index:     The index of the pre-existing callback.
 cb:        The callback function
 Returns:   1 if error, 0 if success */
int window_instance_set_cb_on_process_event(WINDOW_INSTANCE* instance, int index, WINDOW_INSTANCE_CB_ON_PROCESS_EVENT cb);

/* Set a on_render callback of a pre-existing callback.
 instance:  The window instance to add a callback to.
 index:     The index of the pre-existing callback.
 cb:        The callback function
 cb_param1: The callback param1. If NULL, the window instance will be passed to the callback function.
 cb_param2: The callback param2. If NULL, NULL will be passed to the callback function.
 Returns:   1 if error, 0 if success */
int window_instance_set_cb_on_render(WINDOW_INSTANCE* instance, int index, WINDOW_INSTANCE_CB cb, void* cb_param1, void* cb_param2);

/* Create Window Manager.
 manager:      The window manager
 window_count: The amount of window instances to allocate memory for. 
 Returns:      1 if error, 0 if success */
int window_manager_create(WINDOW_MANAGER** manager, uint16_t window_count);

/* Destroy Window Manager. 
 manager: The window manager */
void window_manager_destroy(WINDOW_MANAGER* manager);

/* Process Event.
 manager: The window manager
 e:       The SDL_Event to process 
 Returns: Non-zero if window_manager wants to quit the program, 0 if not. */
int window_manager_process_event(WINDOW_MANAGER* manager, SDL_Event* e);

/* Update Window Manager.
 manager: The window manager */
void window_manager_update(WINDOW_MANAGER* manager);

#ifdef __cplusplus
};
#endif

#endif
