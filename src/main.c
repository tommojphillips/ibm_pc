/* main.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
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
#include "frontend/sdl/sdl3_ui.h"

#include "backend/ibm_pc.h"
#include "backend/timing.h"
#include "backend/input.h"

typedef struct ARGS {
	uint8_t dbg_ui;
	uint8_t video_adapter;
	uint8_t disks_loaded;
	uint32_t total_memory;
	uint8_t sw1_provided;
	uint8_t sw1;
	uint8_t sw2_provided;
	uint8_t sw2;
	uint8_t model;
} ARGS;

int next_arg(int argc, char** argv, int* i, const char** arg) {
	(*i)++;
	if (*i < argc) {
		*arg = argv[*i];
		return 1;
	}
	else {
		*arg = NULL;
		return 0;
	}
}

void set_default_args(ARGS* args) {
	args->dbg_ui = 0;
	args->video_adapter = VIDEO_ADAPTER_MDA_80X25;
	args->disks_loaded = 0;
	args->total_memory = 16 * 1024;
	args->sw1_provided = 0;
	args->sw1 = 0;
	args->sw2_provided = 0;
	args->sw2 = 0;
	args->model = MODEL_5150_16_64;
}

void str_to_num(const char* str, uint32_t* num) {
	/* Convert string to number. Supports decimal, hex, binary */
	size_t len = strlen(str);
	if (len >= 2 && (strncmp(str, "0x", 2) == 0 || strncmp(str, "0X", 2) == 0)) {
		str += 2;
		*num = strtol(str, NULL, 16) & 0xFFFFFFFF;
	}
	else if (len >= 1 && (strncmp(str, "x", 1) == 0 || strncmp(str, "X", 1) == 0)) {
		str += 1;
		*num = strtol(str, NULL, 16) & 0xFFFFFFFF;
	}
	else if (len >= 2 && (strncmp(str, "0b", 2) == 0 || strncmp(str, "0B", 2) == 0)) {
		str += 2;
		*num = strtol(str, NULL, 2) & 0xFFFFFFFF;
	}
	else if (len >= 1 && (strncmp(str, "b", 1) == 0 || strncmp(str, "B", 1) == 0)) {
		str += 1;
		*num = strtol(str, NULL, 2) & 0xFFFFFFFF;
	}
	else {
		*num = strtol(str, NULL, 10) & 0xFFFFFFFF;
	}
}

int parse_args(ARGS* args, int argc, char** argv) {
	uint32_t offset = 0;
	uint8_t disk = 0;
	for (int i = 1; i < argc; ++i) {
		const char* arg = argv[i];

		/* set dbg */
		if (strncmp("-dbg", arg, 5) == 0) {
			args->dbg_ui = 1;
			continue;
		}

		/* insert disk into A, B drive */
		if (strncmp("-d", arg, 3) == 0 || strncmp("-disk", arg, 6) == 0) {
			
			if (!next_arg(argc, argv, &i, &arg)) {
				break;
			}

			if (strlen(arg) >= 2 && arg[1] == ':') {
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
			}
			fdd_eject_disk(&ibm_pc->fdc.fdd[disk]);
			fdd_insert_disk(&ibm_pc->fdc.fdd[disk], arg);
			disk++; /* increment disk number so subsequent disk files load into the next disk. */
			args->disks_loaded++;
			continue;
		}

		/* write protect disk A, B drive */
		if (strncmp("-dwp", arg, 5) == 0 || strncmp("-disk-write-protect", arg, 20) == 0) {
			
			if (!next_arg(argc, argv, &i, &arg)) {
				break;
			}

			if (strlen(arg) >= 2 && arg[1] == ':') {
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
					printf("Unknown disk format. Expected [A-B]:\n");
					continue;
				}
				arg += 2; /* skip over '[A-B]:' */
			}
			ibm_pc->fdc.fdd[disk].status.write_protect = 1;
			continue;
		}

		/* set video adapter */
		if (strncmp("-v", arg, 3) == 0 || strncmp("-video", arg, 7) == 0) {

			if (!next_arg(argc, argv, &i, &arg)) {
				break;
			}

			if (strncmp("mda", arg, 4) == 0 || strncmp("MDA", arg, 4) == 0) {
				args->video_adapter = VIDEO_ADAPTER_MDA_80X25;
			}
			else if (strncmp("cga", arg, 4) == 0 || strncmp("CGA", arg, 4) == 0 || strncmp("cga80", arg, 6) == 0 || strncmp("CGA80", arg, 6) == 0) {
				args->video_adapter = VIDEO_ADAPTER_CGA_80X25;
			}
			else if (strncmp("cga40", arg, 6) == 0 || strncmp("CGA40", arg, 6) == 0) {
				args->video_adapter = VIDEO_ADAPTER_CGA_40X25;
			}
			else if (strncmp("none", arg, 4) == 0 || strncmp("NONE", arg, 4) == 0) {
				args->video_adapter = VIDEO_ADAPTER_NONE;
			}
			else {
				args->video_adapter = VIDEO_ADAPTER_NONE;
				printf("Unknown video adapter '%s'. Expected MDA, CGA, CGA40, CGA80, NONE\n", arg);
			}
			continue;
		}
		
		/* total ram */
		if (strncmp("-r", arg, 3) == 0 || strncmp("-ram", arg, 5) == 0) {

			if (!next_arg(argc, argv, &i, &arg)) {
				break;
			}

			uint32_t ram = 0;
			str_to_num(arg, &ram);

			switch (ram) {

				case 16:
					ram *= 1024;
				case 16 * 1024:
					args->total_memory = ram;
					break;

				case 32:
					ram *= 1024;
				case 32 * 1024:
					args->total_memory = ram;
					break;

				case 48:
					ram *= 1024;
				case 48 * 1024:
					args->total_memory = ram;
					break;

				case 64:
					ram *= 1024;
				case 64 * 1024:
					args->total_memory = ram;
					break;

				case 96:
					ram *= 1024;
				case 96 * 1024:
					args->total_memory = ram;
					break;

				case 128:
					ram *= 1024;
				case 128 * 1024:
					args->total_memory = ram;
					break;

				case 160:
					ram *= 1024;
				case 160 * 1024:
					args->total_memory = ram;
					break;

				case 192:
					ram *= 1024;
				case 192 * 1024:
					args->total_memory = ram;
					break;

				case 224:
					ram *= 1024;
				case 224 * 1024:
					args->total_memory = ram;
					break;

				case 256:
					ram *= 1024;
				case 256 * 1024:
					args->total_memory = ram;
					break;

				case 288:
					ram *= 1024;
				case 288 * 1024:
					args->total_memory = ram;
					break;

				case 320:
					ram *= 1024;
				case 320 * 1024:
					args->total_memory = ram;
					break;

				case 352:
					ram *= 1024;
				case 352 * 1024:
					args->total_memory = ram;
					break;

				case 384:
					ram *= 1024;
				case 384 * 1024:
					args->total_memory = ram;
					break;

				case 416:
					ram *= 1024;
				case 416 * 1024:
					args->total_memory = ram;
					break;

				case 448:
					ram *= 1024;
				case 448 * 1024:
					args->total_memory = ram;
					break;

				case 480:
					ram *= 1024;
				case 480 * 1024:
					args->total_memory = ram;
					break;

				case 512:
					ram *= 1024;
				case 512 * 1024:
					args->total_memory = ram;
					break;

				case 544:
					ram *= 1024;
				case 544 * 1024:
					args->total_memory = ram;
					break;

				case 576:
					ram *= 1024;
				case 576 * 1024:
					args->total_memory = ram;
					break;

				case 608:
					ram *= 1024;
				case 608 * 1024:
					args->total_memory = ram;
					break;

				case 640:
					ram *= 1024;
				case 640 * 1024:
					args->total_memory = ram;
					break;

				case 672:
					ram *= 1024;
				case 672 * 1024:
					args->total_memory = ram;
					break;

				case 704:
					ram *= 1024;
				case 704 * 1024:
					args->total_memory = ram;
					break;

				case 736:
					ram *= 1024;
				case 736 * 1024:
					args->total_memory = ram;
					break;

				case 768:
					ram *= 1024;
				case 768 * 1024:
					args->total_memory = ram;
					break;

				default:
					printf("Invalid total memory '%s'.\nVaild memory:\n 16\n 32\n 48\n 64\n", arg);

					for (int k = 64; k < 768; k += 32) {
						printf(" %d\n", k);
					}
					break;
			}
			continue;
		}

		/* sw1 */
		if (strncmp("-sw1", arg, 5) == 0) {

			if (!next_arg(argc, argv, &i, &arg)) {
				break;
			}

			uint32_t sw1 = 0;
			str_to_num(arg, &sw1);

			args->sw1 = sw1 & 0xFF;
			args->sw1_provided = 1;
			continue;
		}

		/* sw2 */
		if (strncmp("-sw2", arg, 5) == 0) {

			if (!next_arg(argc, argv, &i, &arg)) {
				break;
			}

			uint32_t sw2 = 0;
			str_to_num(arg, &sw2);

			args->sw2 = sw2 & 0xFF;
			args->sw2_provided = 1;
			continue;
		}

		/* sw2 */
		if (strncmp("-model", arg, 7) == 0) {

			if (!next_arg(argc, argv, &i, &arg)) {
				break;
			}

			if (strncmp("5150_16_64", arg, 11) == 0) {
				args->model = MODEL_5150_16_64;
			}
			else if (strncmp("5150_64_256", arg, 12) == 0) {
				args->model = MODEL_5150_64_256;
			}
			else {
				printf("Invalid model: %s\n", arg);
			}
			continue;
		}

		/* set load offset */
		if (strncmp("-o", arg, 3) == 0) {

			if (!next_arg(argc, argv, &i, &arg)) {
				break;
			}

			str_to_num(arg, &offset);
			continue;
		}

		/* print help */
		if (strncmp("-?", arg, 3) == 0) {

			printf("ibm_pc.exe [-o <offset>] <rom_file> <extra_flags>\n"
			       "-o <offset>                - Set load offset of the next ROM.\n"
				   "<rom_file>                 - Load ROM at offset; inc offset by ROM size.\n"
			       "-disk [A-B:]<disk_path>    - Load disk into drive A,B.\n"
			       "-disk-write-protect [A-B:] - Write protect disk in drive A,B.\n"
			       "-video <video_adapter>     - The video adapter to use MDA, CGA, CGA40, CGA80, NONE.\n"
			       "-ram <ram>                 - The amount of conventional ram. (16-64 in multiples of 16) or (64-768 in multiples of 32)\n"
			       "-sw1 <sw1>                 - Override sw1 setting.\n"
			       "-sw2 <sw2>                 - Override sw2 setting. \n"
			       "-dbg                       - Display debug window.\n"
			       "Numbers can be in decimal, hex or binary.\n");

			return 1; /* exit */
		}

		/* Default; read file into memory at offset */
		size_t file_size = 0;
		file_read_into_buffer(arg, ibm_pc->mm.mem, 0x100000, offset, &file_size, 0);
		if (file_size > 0xFFFFFFFF) {
			printf("File size too big: %s\n", arg);
		}
		else {
			offset += (uint32_t)(file_size & 0xFFFFFFFF); /* add filesize to offset so subsequent file reads are sequential in memory. */
		}
		continue;
	}

	return 0; /* no error */
}

#define dbg_gui_w  480
#define dbg_gui_h 350

#define gui_boarder_w_l 15 /* boarder width left */
#define gui_boarder_w_t 25 /* boarder width top */

#include "ui.h"

int main(int argc, char** argv) {
	
	/* Create SDL */
	if (sdl_create()) {
		exit(1);
	}

	/* Create Window Manager */
	if (window_manager_create(2)) {
		exit(1);
	}

	sdl_add_cb_on_process_event(window_manager_process_event);
	sdl_add_cb_on_update(window_manager_update);

	/* Create IBM PC */
	if (ibm_pc_create()) {
		exit(1);
	}

	/* Parse command-line args */
	ARGS args = { 0 };
	set_default_args(&args);
	if (parse_args(&args, argc, argv)) {
		exit(1);
	}

	WINDOW_INSTANCE* win1 = NULL;
	DISPLAY_INSTANCE* display = NULL;
	if (args.video_adapter != VIDEO_ADAPTER_NONE) {
		/* Create Main Window */
		if (window_instance_create(&win1)) {
			exit(1);
		}
		win1->title = "5150";
		window_instance_set_transform(win1, dbg_gui_w + gui_boarder_w_l, SDL_WINDOWPOS_CENTERED, 800, 580);
		window_instance_set_cb_on_process_event(win1, input_process_event);
		window_instance_open(win1);

		/* create display */
		if (display_create(&display, win1)) {
			exit(1);
		}
		display_on_video_adapter_changed(display, args.video_adapter);

		ui_create_renderer(win1->window, win1->renderer);
		window_instance_set_cb_on_render(win1, ui_update, display);
		window_instance_set_cb_on_process_event(win1, ui_process_event);
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
		sdl_timing_init_frame(&win2->time, HZ_TO_MS(60.0));
		window_instance_set_transform(win2, gui_boarder_w_l, SDL_WINDOWPOS_CENTERED, dbg_gui_w, dbg_gui_h);
		window_instance_set_cb_on_process_event(win2, input_process_event);
		window_instance_set_cb_on_render(win2, dbg_gui_render, &dbg_gui);
		window_instance_open(win2);
	}

	/* Setup timing callbacks for backend */
	timing_set_cb_get_ticks_ms(sdl_timing_get_ticks_ms);
	timing_set_cb_get_ticks_ns(sdl_timing_get_ticks_ns);
	timing_set_cb_init_frame(sdl_timing_init_frame);
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
	
	/* Get config */
	ibm_pc->config.total_memory = args.total_memory;
	ibm_pc->config.fdc_disks = args.disks_loaded;
	ibm_pc->config.video_adapter = args.video_adapter;
	ibm_pc->config.sw1_provided = args.sw1_provided;
	ibm_pc->config.sw2_provided = args.sw2_provided;	
	ibm_pc->config.sw1 = ~args.sw1; /* let the user enter the sws like on the planar. (inverted) */	
	ibm_pc->config.sw2 = ~args.sw2; /* let the user enter the sws like on the planar. (inverted) */
	ibm_pc->config.model = args.model;

	/* Initialize IBM PC */
	if (ibm_pc_init()) {
		exit(1);
	}

	/* Hard Reset IBM PC */
	ibm_pc_reset();

	while (!sdl->quit) {
		sdl_update();
		ibm_pc_update();
	}

	/* Clean up */
	ui_destroy();
	display_destroy(display);
	ibm_pc_destroy();
	window_manager_destroy();
	sdl_destroy();

	return 0;
}
