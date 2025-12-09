/* args.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <SDL3/SDL.h>

#include "args.h"

#include "tomi.h"
#include "frontend/utility/file.h"
#include "frontend/sdl/sdl3_display.h"

#include "backend/ibm_pc.h"

static const TOMI_ENUM model_def[] = {
	{ "5150_16_64",  MODEL_5150_16_64 },
	{ "5150_64_256", MODEL_5150_64_256 },
};

static const TOMI_ENUM video_adapter_def[] = {
	{ "MDA",   VIDEO_ADAPTER_MDA_80X25 },
	{ "CGA",   VIDEO_ADAPTER_CGA_80X25 },
	{ "CGA80", VIDEO_ADAPTER_CGA_80X25 },
	{ "CGA40", VIDEO_ADAPTER_CGA_40X25 },
};

static const TOMI_ENUM texture_scale_def[] = {
	{ "Nearest", SDL_SCALEMODE_NEAREST },
	{ "Linear",  SDL_SCALEMODE_LINEAR  },
};

static const TOMI_ENUM display_scale_def[] = {
	{ "Fit",     DISPLAY_SCALE_FIT     },
	{ "Stretch", DISPLAY_SCALE_STRETCH },
};

static const TOMI_ENUM display_view_def[] = {
	{ "Cropped", DISPLAY_SCALE_FIT     },
	{ "Full",    DISPLAY_SCALE_STRETCH },
};

static const TOMI_FIELD rom_fields[] = {
	TOMI_FIELD_STR("path", ROM, path),
	TOMI_FIELD_U32("address", ROM, address)
};

static const TOMI_STRUCT rom_def = {
	TOMI_STRUCT_DEF(rom_fields, ROM)
};

static const TOMI_FIELD disk_fields[] = {
	TOMI_FIELD_STR("path", DISK, path),
	TOMI_FIELD_U8("drive", DISK, drive),
	TOMI_FIELD_U8("write_protect", DISK, write_protect)
};

static const TOMI_STRUCT disk_def = {
	TOMI_STRUCT_DEF(disk_fields, DISK)
};

static const TOMI_FIELD chs_fields[] = {
	TOMI_FIELD_U16("c", CHS, c),
	TOMI_FIELD_U8("h", CHS, h),
	TOMI_FIELD_U8("s", CHS, s),
};

static const TOMI_STRUCT chs_def = {
	TOMI_STRUCT_DEF(chs_fields, CHS)
};

static const TOMI_ENUM hdd_type_def[] = {
	{ "None",   XEBEC_HDD_TYPE_NONE },
	{ "Type1",  XEBEC_HDD_TYPE_1    },
	{ "Type2",  XEBEC_HDD_TYPE_2    },
	{ "Type13", XEBEC_HDD_TYPE_13   },
	{ "Type16", XEBEC_HDD_TYPE_16   },
};

static const TOMI_FIELD hdd_fields[] = {
	TOMI_FIELD_STR("path", HDD, path),
	TOMI_FIELD_U8("drive", HDD, drive),
	TOMI_FIELD_STRUCT("geometry", HDD, geometry, &chs_def),
	TOMI_FIELD_ENUM_U32("type", HDD, type, hdd_type_def),
};

static const TOMI_STRUCT hdd_def = {
	TOMI_STRUCT_DEF(hdd_fields, HDD)
};

static const TOMI_SETTING setting_map[] = {
	TOMI_SETTING_BOOL("dbg_ui"),

	/* IBM PC */
	TOMI_SETTING_ENUM_U8("model", model_def),
	TOMI_SETTING_ENUM_U8("video_adapter", video_adapter_def),
	TOMI_SETTING_U32("conventional_ram"),
	TOMI_SETTING_U8("num_floppies"),
	TOMI_SETTING_U8("sw1_override"),
	TOMI_SETTING_U8("sw2_override"),
	TOMI_SETTING_U8("sw1"),
	TOMI_SETTING_U8("sw2"),
	TOMI_SETTING_STRUCT_ARRAY("disk", IBM_PC_CONFIG, &disk_def, disks, disk_count),
	TOMI_SETTING_STRUCT_ARRAY("rom", IBM_PC_CONFIG, &rom_def, roms, rom_count),
	TOMI_SETTING_STRUCT_ARRAY("hdd", IBM_PC_CONFIG, &hdd_def, hdds, hdd_count),

	/* DISPLAY */
	TOMI_SETTING_ENUM_U8("texture_scale_mode", texture_scale_def),
	TOMI_SETTING_ENUM_U8("display_scale_mode", display_scale_def),
	TOMI_SETTING_ENUM_U8("display_view_mode", display_view_def),
	TOMI_SETTING_BOOL("correct_aspect_ratio"),
	TOMI_SETTING_BOOL("emulate_max_scanline"),
	TOMI_SETTING_BOOL("allow_display_disable"),
	TOMI_SETTING_BOOL("delay_display_disable"),
	TOMI_SETTING_U64("delay_display_disable_time"),
	TOMI_SETTING_STR("mda_font", TOMI_FIELD_SIZE(DISPLAY_CONFIG, mda_font)),
	TOMI_SETTING_STR("cga_font", TOMI_FIELD_SIZE(DISPLAY_CONFIG, cga_font)),
};

static const int settings_map_count = TOMI_ARRAY_COUNT(TOMI_SETTING, setting_map);

static int next_arg(int argc, char** argv, int* i, const char** arg) {
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

static void str_to_num(const char* str, uint32_t* num) {
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

void args_set_default(ARGS* args) {
	args->dbg_ui = 0;
	args->config_filename = "ibm_pc.ini";

	args->pc_config->video_adapter = VIDEO_ADAPTER_MDA_80X25;
	args->pc_config->fdc_disks = 2;
	args->pc_config->total_memory = 16 * 1024;
	args->pc_config->sw1_provided = 0;
	args->pc_config->sw1 = 0;
	args->pc_config->sw2_provided = 0;
	args->pc_config->sw2 = 0;
	args->pc_config->model = MODEL_5150_16_64;
	args->pc_config->roms = NULL;
	args->pc_config->rom_count = 0;
	args->pc_config->disks = NULL;
	args->pc_config->disk_count = 0;
	args->pc_config->hdds = NULL;
	args->pc_config->hdd_count = 0;

	args->display_config->correct_aspect_ratio = 1;
	args->display_config->scanline_emu = 1;
	args->display_config->texture_scale_mode = SDL_SCALEMODE_NEAREST;
	args->display_config->display_scale_mode = DISPLAY_SCALE_FIT;
	args->display_config->display_view_mode = DISPLAY_VIEW_CROPPED;
	args->display_config->allow_display_disable = 1;
	args->display_config->delay_display_disable = 1;
	args->display_config->delay_display_disable_time = 200; // 200 ms
	strcpy(args->display_config->mda_font, "Bm437_IBM_MDA.FON");
	strcpy(args->display_config->cga_font, "Bm437_IBM_CGA.FON");
}

int args_parse_cli(int argc, char** argv, ARGS* args) {
	DISK disk = { 0 };
	ROM rom = { 0 };
	
	for (int i = 1; i < argc; ++i) {
		const char* arg = argv[i];

		/* set dbg */
		if (strncmp("-dbg", arg, 5) == 0) {
			args->dbg_ui = 1;
			continue;
		}

		/* set config file */
		if (strncmp("-c", arg, 3) == 0 || strncmp("-config", arg, 8) == 0) {

			if (!next_arg(argc, argv, &i, &arg)) {
				break;
			}

			args->config_filename = arg; /* command-line arguments (argv) are valid for the lifetime of the program. */
			continue;
		}

		/* set the amount of disk drives 0-4 */
		if (strncmp("-ds", arg, 4) == 0 || strncmp("-disks", arg, 7) == 0) {

			if (!next_arg(argc, argv, &i, &arg)) {
				break;
			}
			
			uint32_t disks = 0;
			str_to_num(arg, &disks);

			if (disks > 4) {
				printf("To many disks. Expected 0-4\n");
				continue;
			}

			args->pc_config->fdc_disks = disks & 0xFF;
			continue;
		}

		/* Insert disk into A, B, C, D drive using '[A-D]:' as a switch */
		if (strlen(arg) == 2 && arg[1] == ':') {
			/* format: [A-D]: <path> */

			/* convert to disk number */
			char_to_drive(arg[0], &disk.drive);

			if (!next_arg(argc, argv, &i, &arg)) {
				break;
			}
			
			strncpy_s(disk.path, sizeof(disk.path), arg, sizeof(disk.path) - 1);
			ibm_pc_add_disk(&disk);
			disk.write_protect = 0; /* reset write_protect flag */
			continue;
		}

		/* Insert disk into A, B, C, D drive using '[-d]/[-disk]' as a switch */
		if (strncmp("-d", arg, 3) == 0 || strncmp("-disk", arg, 6) == 0) {
			/* format: -disk [A-D]:<path> */
			if (strlen(arg) >= 2 && arg[1] == ':') {
				/* convert to disk number */
				char_to_drive(arg[0], &disk.drive);
				arg += 2; /* skip over '[A-D]:' */
			}

			strncpy_s(disk.path, sizeof(disk.path), arg, sizeof(disk.path) - 1);
			ibm_pc_add_disk(&disk);
			disk.write_protect = 0; /* reset write_protect flag */
			continue;
		}

		/* Write protect the next loaded disk */
		if (strncmp("-dwp", arg, 5) == 0 || strncmp("-disk-write-protect", arg, 20) == 0) {
			/* format: -disk-write-protect */
			disk.write_protect = 1;
			continue;
		}

		/* set video adapter */
		if (strncmp("-v", arg, 3) == 0 || strncmp("-video", arg, 7) == 0) {

			if (!next_arg(argc, argv, &i, &arg)) {
				break;
			}

			if (strncmp("mda", arg, 4) == 0 || strncmp("MDA", arg, 4) == 0) {
				args->pc_config->video_adapter = VIDEO_ADAPTER_MDA_80X25;
			}
			else if (strncmp("cga", arg, 4) == 0 || strncmp("CGA", arg, 4) == 0 || strncmp("cga80", arg, 6) == 0 || strncmp("CGA80", arg, 6) == 0) {
				args->pc_config->video_adapter = VIDEO_ADAPTER_CGA_80X25;
			}
			else if (strncmp("cga40", arg, 6) == 0 || strncmp("CGA40", arg, 6) == 0) {
				args->pc_config->video_adapter = VIDEO_ADAPTER_CGA_40X25;
			}
			else if (strncmp("none", arg, 4) == 0 || strncmp("NONE", arg, 4) == 0) {
				args->pc_config->video_adapter = VIDEO_ADAPTER_NONE;
			}
			else {
				args->pc_config->video_adapter = VIDEO_ADAPTER_NONE;
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

			uint32_t k;
			for (k = 16; k <= 736;) {
				if (ram == k || ram == k * 1024) {
					args->pc_config->total_memory = k * 1024;
					break;
				}

				if (k < 64) {
					k += 16;
				}
				else {
					k += 32;
				}
			}

			if (k > 736) {
				printf("Invalid total memory '%s'.\nVaild memory:\n", arg);

				for (k = 16; k <= 736;) {
					printf(" %d\n", k);

					if (k < 64) {
						k += 16;
					}
					else {
						k += 32;
					}
				}
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

			args->pc_config->sw1 = ~sw1 & 0xFF; /* let the user enter the sws like on the planar. (inverted) */
			args->pc_config->sw1_provided = 1;
			continue;
		}

		/* sw2 */
		if (strncmp("-sw2", arg, 5) == 0) {

			if (!next_arg(argc, argv, &i, &arg)) {
				break;
			}

			uint32_t sw2 = 0;
			str_to_num(arg, &sw2);

			args->pc_config->sw2 = ~sw2 & 0xFF; /* let the user enter the sws like on the planar. (inverted) */
			args->pc_config->sw2_provided = 1;
			continue;
		}

		/* sw2 */
		if (strncmp("-model", arg, 7) == 0) {

			if (!next_arg(argc, argv, &i, &arg)) {
				break;
			}

			if (strncmp("5150_16_64", arg, 11) == 0) {
				args->pc_config->model = MODEL_5150_16_64;
			}
			else if (strncmp("5150_64_256", arg, 12) == 0) {
				args->pc_config->model = MODEL_5150_64_256;
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

			str_to_num(arg, &rom.address);
			continue;
		}

		/* print help */
		if (strncmp("-?", arg, 3) == 0) {

			printf("ibm_pc.exe [-c <config_file>] [-o <offset>] <rom_file> <extra_flags>\n"
			       "-c <config_file>           - Set config file.\n"
			       "-o <offset>                - Load offset of the next ROM.\n"
				   "<rom_file>                 - Load ROM at offset; inc offset by ROM size.\n"
			       "<A-D>:                     - Load next disk into drive A,B,C,D.\n"
			       "-disks <0-4>               - Amount of disk drives. 0-4.\n"
			       "-disk [A-D:]<disk_path>    - Load disk into drive A,B,C,D.\n"
			       "-disk-write-protect [A-D:] - Write protect the next loaded disk.\n"
			       "-video <video_adapter>     - The video adapter to use 'MDA', 'CGA', NONE.\n"
			       "-ram <ram>                 - The amount of conventional ram. (16-64 in multiples of 16) or (64-768 in multiples of 32)\n"
			       "-sw1 <sw1>                 - Override sw1 setting.\n"
			       "-sw2 <sw2>                 - Override sw2 setting. \n"
			       "-model <model>             - Motherboard model. Primarily use to set and report the correct amount of RAM. use '5150_16_64', '5150_64_256'\n"
			       "-dbg                       - Display debug window.\n"
			       "# Numbers can be in decimal, hex or binary.\n");

			return 1; /* exit */
		}

		/* Default; Load ROM */
		size_t file_size = 0;
		strncpy_s(rom.path, sizeof(rom.path), arg, sizeof(rom.path) - 1);
		ibm_pc_add_rom(&rom);

		if (file_get_file_size(arg, &file_size)) {
			rom.address += (uint32_t)(file_size & 0xFFFFFFFF); /* add filesize to offset so subsequent file reads are sequential in memory. */
		}
		continue;
	}

	return 0; /* no error */
}

int args_parse_cli_for_config_file(int argc, char** argv, ARGS* args) {

	for (int i = 1; i < argc; ++i) {
		const char* arg = argv[i];

		/* set config file */
		if (strncmp("-c", arg, 3) == 0 || strncmp("-config", arg, 8) == 0) {

			if (!next_arg(argc, argv, &i, &arg)) {
				break;
			}

			args->config_filename = arg; /* command-line arguments (argv) are valid for the lifetime of the program. */
			continue;
		}
	}

	return 0; /* no error */
}

int args_parse_ini(TOMI_VAR* var_map, ARGS* args) {
	int i = 0;

	#define set_var(address) (var_map[i++].var = address)

	set_var(&args->dbg_ui);

	set_var(&args->pc_config->model);
	set_var(&args->pc_config->video_adapter);
	set_var(&args->pc_config->total_memory);
	set_var(&args->pc_config->fdc_disks);
	set_var(&args->pc_config->sw1_provided);
	set_var(&args->pc_config->sw2_provided);
	set_var(&args->pc_config->sw1);
	set_var(&args->pc_config->sw2);
	set_var(args->pc_config); /* disk struct array */
	set_var(args->pc_config); /* rom struct array */
	set_var(args->pc_config); /* hdd struct array */

	set_var(&args->display_config->texture_scale_mode);
	set_var(&args->display_config->display_scale_mode);
	set_var(&args->display_config->display_view_mode);
	set_var(&args->display_config->correct_aspect_ratio);
	set_var(&args->display_config->scanline_emu);
	set_var(&args->display_config->allow_display_disable);
	set_var(&args->display_config->delay_display_disable);
	set_var(&args->display_config->delay_display_disable_time);
	set_var(&args->display_config->mda_font);
	set_var(&args->display_config->cga_font);

	if (tomi_load_from_file(args->config_filename, setting_map, var_map, settings_map_count) != TOMI_ERROR_SUCCESS) {
		return 1;
	}
	return 0;
}

int args_create(TOMI_VAR** var_map) {
	if (tomi_create_var_map(var_map, settings_map_count) != TOMI_ERROR_SUCCESS) {
		return 1;
	}
	else {
		return 0;
	}
}

void args_destroy(TOMI_VAR* var_map) {
	tomi_save_to_file("output.ini", setting_map, var_map, settings_map_count);
	tomi_destroy_var_map(setting_map, var_map, settings_map_count);
}
