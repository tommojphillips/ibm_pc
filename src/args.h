/* args.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 */

#ifndef ARGS_H
#define ARGS_H

typedef struct IBM_PC_CONFIG IBM_PC_CONFIG;
typedef struct DISPLAY_CONFIG DISPLAY_CONFIG;
typedef struct TOMI_VAR TOMI_VAR;

typedef struct ARGS {
	const char* config_filename;
	int dbg_ui;
	IBM_PC_CONFIG* pc_config;
	DISPLAY_CONFIG* display_config;
} ARGS;

void args_set_default(ARGS* args);
int args_parse_cli(int argc, char** argv, ARGS* args);
int args_parse_cli_for_config_file(int argc, char** argv, ARGS* args);
int args_parse_ini(TOMI_VAR* var_map, ARGS* args);

int args_create(TOMI_VAR** var_map);
void args_destroy(TOMI_VAR* var_map);

#endif
