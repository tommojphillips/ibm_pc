/* sdl3_window.h
* GitHub: https:\\github.com\tommojphillips
*/

#ifndef WINDOW_SDL_H
#define WINDOW_SDL_H

#include <stdint.h>

#include "sdl3_timing.h"

#include "..\..\backend\ring_buffer.h"

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef union SDL_Event SDL_Event;
typedef struct TTF_TextEngine TTF_TextEngine;
typedef struct TTF_Font TTF_Font;

/* actual window width */
#define WINDOW_W (instance->transform.w)

/* actual window height */
#define WINDOW_H (instance->transform.h)

typedef struct WINDOW_INSTANCE WINDOW_INSTANCE;
typedef struct FONT_TEXTURE_DATA FONT_TEXTURE_DATA;

/* Window Transform */
typedef struct WINDOW_TRANSFORM {
	int x;
	int y;
	int h;
	int w;
} WINDOW_TRANSFORM;

/* Window Manager */
typedef struct WINDOW_MANAGER {
	WINDOW_INSTANCE* instances;
	int instance_count;
	int instance_index;
	TTF_TextEngine* text_engine;
} WINDOW_MANAGER;

typedef void(*WINDOW_INSTANCE_CB)(void* instance, void* param);

/* Window Instance */
typedef struct WINDOW_INSTANCE {
	/* Window */
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_WindowID window_id;
	TTF_TextEngine* text_engine;
	
	/* Font */
	/*FONT_TEXTURE_DATA* font_data;
	const char* font_path;
	int font_w;
	int font_h;
	int cell_w;
	int cell_h;*/

	int open;
	WINDOW_TRANSFORM transform;
	int window_state;
	const char* title;
	
	WINDOW_INSTANCE_CB on_render;
	void* on_render_param;
	WINDOW_INSTANCE_CB on_process_event;
	
	FRAME_STATE time;
	
} WINDOW_INSTANCE;

#ifdef __cplusplus
extern "C" {
#endif

/* Create New Window Instance; Add a window instance to the service_list
	Allocates memory for the window instance. */
int window_instance_create(WINDOW_INSTANCE** instance);

/* Open Window Instance; Open a window instance from the service_list
 Creates a new Window, renderer. */
void window_instance_open(WINDOW_INSTANCE* instance);

/* Close Window Instance; Close a window instance from the service_list
 Remove window instance from the service_list. Calls sdl_destroy_window_instance */
void window_instance_close(WINDOW_INSTANCE* instance);

/* Set Window Instance Transform */
void window_instance_set_transform(WINDOW_INSTANCE* instance, int x, int y, int w, int h);

/* Set Window Instance Min Size. Window must be open. */
void window_instance_set_min_size(WINDOW_INSTANCE* instance, int w, int h);

void window_instance_set_cb_on_process_event(WINDOW_INSTANCE* instance, WINDOW_INSTANCE_CB cb);
void window_instance_set_cb_on_render(WINDOW_INSTANCE* instance, WINDOW_INSTANCE_CB cb, void* cb_param);

/* Create Window Manager. 
 window_count: The amount of window instances to allocate memory for. 
 Returns: 1 if error, 0 if success */
int window_manager_create(int window_count);

/* Destroy Window Manager. */
void window_manager_destroy();

/* Process Event. */
void window_manager_process_event(SDL_Event* e);

/* Update Window Manager */
void window_manager_update();

#ifdef __cplusplus
};
#endif

#endif
