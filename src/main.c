/* main.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Intel 8086 CPU test main
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#include "frontend/utility/file.h"
#include "frontend/sdl/sdl3_common.h"
#include "frontend/sdl/sdl3_window.h"
#include "frontend/sdl/sdl3_input.h"
#include "frontend/sdl/sdl3_display.h"
#include "frontend/sdl/dbg_gui.h"

#include "backend/ibm_pc.h"
#include "backend/timing.h"
#include "backend/input.h"

typedef struct ARGS {
	int dbg_ui;
	uint8_t video_adapter;
} ARGS;

void parse_args(ARGS* args, int argc, char** argv) {
	size_t offset = 0;
	for (int i = 1; i < argc; ++i) {
		const char* arg = argv[i];

		/* set dbg */
		if (strcmp("-dbg", arg) == 0) {
			args->dbg_ui = 1;
		}

		/* insert disk into A, B drive */
		if (strncmp("-d", arg, 2) == 0) {
			arg += 2;
			uint8_t disk = 0;
			if (arg[1] == ':') {
				/* convert A-Z, a-z to disk number */
				if ((arg[0] >= 'A' && arg[0] <= 'Z')) {
					disk = arg[0] - 'A';
				}
				else if (arg[0] >= 'a' && arg[0] <= 'z') {
					disk = arg[0] - 'a';
				}
				else if (arg[0] >= '0' && arg[0] <= '9') {
					/* already a number; convert to int (0-255) */
					disk = strtol(arg, NULL, 10) & 0xFF;
				}
				else {
					/* unknown disk format. expected
					[A-B]:<path> or
					[0-1]:<path> */
					printf("Unknown disk format. Expected [A-B]:<path>\n");
					continue;
				}
				arg += 2; /* skip over '[A-B]:' */
				disk &= 0x1; /* map disk A - B(0 - 1) */
			}
			fdc_insert_disk(&ibm_pc->fdc, disk, arg);
			continue;
		}

		/* set video adapter */
		if (strncmp("-v", arg, 2) == 0) {
			arg += 2;

			if (strncmp("mda", arg, 3) == 0 || strncmp("MDA", arg, 3) == 0) {
				args->video_adapter = VIDEO_ADAPTER_MDA;
			}
			else if (strncmp("cga", arg, 3) == 0 || strncmp("CGA", arg, 3) == 0) {
				args->video_adapter = VIDEO_ADAPTER_CGA;
			}
			else {
				args->video_adapter = VIDEO_ADAPTER_NONE;
			}
			continue;
		}

		/* set load offset */
		if (strncmp("-o", arg, 2) == 0) {
			arg += 2;
			offset = strtol(arg, NULL, 16);
			continue;
		}

		/* Default; read file into memory at offset */
		size_t file_size = 0;
		file_read_into_buffer(arg, ibm_pc->mm.mem, 0x100000, offset, &file_size, 0);
		offset += file_size; /* add filesize to offset so subsequent file reads are sequential in memory. */
		continue;
	}
}

#define dbg_gui_w  480
#define dbg_gui_h 350

#define gui_boarder_w_l 15 /* boarder width left */
#define gui_boarder_w_t 25 /* boarder width top */

void ibm_pc_change_video_adapter(uint8_t video_adapter);

int main(int argc, char** argv) {
	
	/* Create SDL */
	if (sdl_create()) {
		exit(1);
	}

	/* Create IBM PC */
	if (ibm_pc_init()) {
		exit(1);
	}
	
	/* Parse command-line args */
	ARGS args = { 0 };
	parse_args(&args, argc, argv);
		
	/* Create Window Manager */
	if (window_manager_create(2)) {
		exit(1);
	}

	WINDOW_INSTANCE* win1 = NULL;
	if (args.video_adapter != VIDEO_ADAPTER_NONE) {
		/* Create Main Window */
		if (window_instance_create(&win1)) {
			exit(1);
		}
		win1->title = "5150";
		window_instance_set_transform(win1, dbg_gui_w + gui_boarder_w_l, SDL_WINDOWPOS_CENTERED, 800, 550);
		window_instance_set_cb_on_process_event(win1, input_process_event);
		window_instance_open(win1);		
	}

	if (args.dbg_ui) {
		/* Create Debug Window */
		DBG_GUI dbg_gui = { 0 };
		dbg_gui.win = win1;
		WINDOW_INSTANCE* win2 = NULL;
		if (window_instance_create(&win2)) {
			exit(1);
		}
		win2->title = "dbg";
		win2->time.target_ms = HZ_TO_MS(60.0);
		window_instance_set_transform(win2, gui_boarder_w_l, SDL_WINDOWPOS_CENTERED, dbg_gui_w, dbg_gui_h);
		window_instance_set_cb_on_process_event(win2, input_process_event);
		window_instance_set_cb_on_render(win2, dbg_gui_render, &dbg_gui);
		window_instance_open(win2);
	}

	/* create display */
	DISPLAY_INSTANCE* display = NULL;
	if (display_create(&display, win1)) {
		exit(1);
	}

	/* change to selected video adapter */
	ibm_pc_change_video_adapter(args.video_adapter);
	display_on_video_adapter_changed(display, args.video_adapter);

	/* Setup timing callbacks for backend */
	timing_set_cb_get_ticks_ms(sdl_timing_get_ticks_ms);
	timing_set_cb_get_ticks_ns(sdl_timing_get_ticks_ns);
	timing_set_cb_reset_frame(sdl_timing_reset_frame);
	timing_set_cb_new_frame(sdl_timing_new_frame);
	timing_set_cb_check_frame(sdl_timing_check_frame);

	/* Setup input callbacks for backend */
	input_set_cb_get_input(sdl_input_get_input);
	input_set_cb_set_input(sdl_input_set_input);
	input_set_cb_has_input(sdl_input_has_input);
	input_set_cb_reset_input(sdl_input_reset_input);

	/* Setup audio callbacks for backend */
	//audio_set_cb_(sdl_audio_);
		
	/* Hard Reset PC */
	ibm_pc_reset();

	while (!sdl->quit) {
		sdl_update();
		ibm_pc_update();
	}

	/* Clean up */
	ibm_pc_destroy();
	display_destroy(display);
	sdl_destroy();

	return 0;
}
