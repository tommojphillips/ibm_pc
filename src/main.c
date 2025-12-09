/* main.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <SDL3/SDL_main.h>

#include "tomi.h"

#include "frontend/sdl/sdl3_common.h"
#include "frontend/sdl/sdl3_window.h"
#include "frontend/sdl/sdl3_input.h"
#include "frontend/sdl/sdl3_display.h"
#include "frontend/sdl/dbg_gui.h"
#include "frontend/sdl/sdl3_ui.h"

#include "backend/ibm_pc.h"
#include "backend/timing.h"

#include "ui.h"
#include "args.h"

#define dbg_gui_w 480
#define dbg_gui_h 350

#define gui_boarder_w_l 15 /* boarder width left */
#define gui_boarder_w_t 25 /* boarder width top */

int main(int argc, char** argv) {
	
	SDL* sdl = NULL;
	WINDOW_MANAGER* window_manager = NULL;
	WINDOW_INSTANCE* win1 = NULL;
	DISPLAY_INSTANCE* display = NULL;
	TOMI_VAR* var_map = NULL;
	
	UI_CONTEXT ui_context = { 0 };
	DBG_GUI dbg_gui = { 0 };

	/* Create SDL */
	if (sdl_create(&sdl)) {
		exit(1);
	}

	/* Create Window Manager */
	if (window_manager_create(&window_manager, 2)) {
		exit(1);
	}

	sdl_add_cb_on_process_event(sdl, window_manager_process_event, window_manager);
	sdl_add_cb_on_update(sdl, window_manager_update, window_manager);

	/* Create Display */
	if (display_create(&display, NULL)) {
		exit(1);
	}

	/* Create IBM PC */
	if (ibm_pc_create()) {
		exit(1);
	}

	/* Parse command-line/config-file args */
	ARGS args = { .pc_config = &ibm_pc->config, .display_config = &display->config };
	args_set_default(&args);

	/* I want the command-line args to overwrite the config-file args. So parse command-line for the config file now */
	if (args_parse_cli_for_config_file(argc, argv, &args)) {
		exit(1);
	}

	/* Parse config-file args */
	if (args.config_filename != NULL) {
		args_create(&var_map);
		args_parse_ini(var_map, &args);
	}

	/* Parse the rest of the command-line args */
	if (args_parse_cli(argc, argv, &args)) {
		exit(1);
	}

	if (ibm_pc->config.video_adapter != VIDEO_ADAPTER_NONE) {
		/* Create Main Window */
		if (window_instance_create(window_manager, &win1)) {
			exit(1);
		}
		win1->title = "5150";
		window_instance_set_transform(win1, dbg_gui_w + gui_boarder_w_l, SDL_WINDOWPOS_CENTERED, 800, 580);
		window_instance_add_cb_on_process_event(win1, input_process_event);
		window_instance_open(win1);

		display_set_window(display, win1);

		/* Change adapter */
		display_on_video_adapter_changed(display, ibm_pc->config.video_adapter);
		
		/* Create UI */
		ui_context_create(&ui_context);
		ui_create_renderer(win1->window, win1->renderer);
		window_instance_add_cb_on_render(win1, ui_update, &ui_context, display);
		window_instance_add_cb_on_process_event(win1, ui_process_event);
	}

	if (args.dbg_ui) {
		/* Create Debug Window */
		dbg_gui.win = win1;
		WINDOW_INSTANCE* win2 = NULL;
		if (window_instance_create(window_manager, &win2)) {
			exit(1);
		}
		win2->title = "dbg";
		sdl_timing_init_frame(&win2->time, HZ_TO_MS(60.0));
		window_instance_set_transform(win2, gui_boarder_w_l, SDL_WINDOWPOS_CENTERED, dbg_gui_w, dbg_gui_h);
		window_instance_add_cb_on_process_event(win2, input_process_event);
		window_instance_add_cb_on_render(win2, dbg_gui_render, NULL, &dbg_gui);
		window_instance_open(win2);
	}

	/* Setup timing callbacks for backend */
	timing_set_cb_get_ticks_ms(sdl_timing_get_ticks_ms);
	timing_set_cb_get_ticks_ns(sdl_timing_get_ticks_ns);
	timing_set_cb_init_frame(sdl_timing_init_frame);
	timing_set_cb_reset_frame(sdl_timing_reset_frame);
	timing_set_cb_new_frame(sdl_timing_new_frame);
	timing_set_cb_check_frame(sdl_timing_check_frame);

	/* Setup audio callbacks for backend */
	//audio_set_cb_(sdl_audio_);
	
	/* Initialize IBM PC */
	ibm_pc_init();

	/* Hard Reset IBM PC */
	ibm_pc_reset();

	while (!sdl->quit) {
		sdl_update(sdl);
		ibm_pc_update();
	}

	/* Clean up */
	ui_destroy();
	ui_context_destroy(&ui_context);

	args_destroy(var_map);
	ibm_pc_destroy();
	display_destroy(display);
	window_manager_destroy(window_manager);
	sdl_destroy(sdl);

	return 0;
}
