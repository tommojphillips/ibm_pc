/* loadini.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "loadini.h"

#define LOADINI_MAX_LINE_SIZE 256
#define LOADINI_DELIM '='

#define DBG_PRINT
#define ERR_PRINT

#ifdef DBG_PRINT
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

#ifdef ERR_PRINT
#define err_print(x, ...) printf(x, __VA_ARGS__)
#else
#define err_print(x, ...)
#endif

typedef struct LOADINI_CONTEXT {
	FILE* stream;
	int line_num;
	char buffer[LOADINI_MAX_LINE_SIZE];
	char* buffer_ptr;
	char key[256];
	char val[256];
	char write_buf[32];
} LOADINI_CONTEXT;

static void ltrim(char** str, size_t* len) {
	/* Trim white space, from the beginning of the string */
	if (*str == NULL || len == NULL || *len == 0) {
		return;
	}
	while (**str == ' ' || **str == '\t' || **str == '\r'|| **str == '\n') {
		++(*str);
		--(*len);
	}
}
static void rtrim(char** str, size_t* len) {
	/* Trim white space from the end of the string */
	char* end = NULL;
	
	if (*str == NULL || len == NULL || *len == 0) {
		return;
	}
	
	end = *str + *len - 1;
	while (end > *str && (*end == ' ' || *end == '\t' || *end == '\r'|| *end == '\n')) {
		--end;
		--(*len);
	}
	*(end + 1) = '\0';
}
static void str_trim(char** str, size_t* len) {
	ltrim(str, len);
	rtrim(str, len);
}
static void str_cpy(char** dest, size_t dest_size, const char* src_start, const char* src_end, size_t* len) {
	*len = (size_t)(src_end - src_start);
	if (*len >= dest_size) {
		*len = dest_size - 1;
	}
	memcpy(*dest, src_start, *len);
	(*dest)[*len] = '\0';
}
static void trim_comment(char* buffer, size_t* len) {
	/* Trim comment from the end of the string */
	for (int i = 0; i < *len; ++i) {
		if (buffer[i] == ';' || strncmp(buffer + i, "//", 2) == 0) {
			buffer[i] = '\0';
			*len -= (*len - i);
			break;
		}
	}
}

static void str_to_int(const char* str, int* value, uint8_t* base) {
	/* Convert string to int. Supports hex, decimal, octal, binary */
	size_t len = strlen(str);
	if (len >= 2 && (strncmp(str, "0x", 2) == 0 || strncmp(str, "0X", 2) == 0)) {
		str += 2;
		*value = strtoul(str, NULL, 16) & 0xFFFFFFFF;
		if (base != NULL) {
			*base = 16;
		}
	}
	else if (len >= 2 && (strncmp(str, "0o", 2) == 0 || strncmp(str, "0O", 2) == 0)) {
		str += 2;
		*value = strtoul(str, NULL, 8) & 0xFFFFFFFF;
		if (base != NULL) {
			*base = 8;
		}
	}
	else if (len >= 2 && (strncmp(str, "0b", 2) == 0 || strncmp(str, "0B", 2) == 0)) {
		str += 2;
		*value = strtoul(str, NULL, 2) & 0xFFFFFFFF;
		if (base != NULL) {
			*base = 2;
		}
	}
	else if (len == 1 && (str[0] >= 'a' && str[0] <= 'z') || (str[0] >= 'A' && str[0] <= 'Z')) {
		*value = (char)str[0];
		if (base != NULL) {
			*base = 0;
		}
	}
	else {
		*value = strtol(str, NULL, 10) & 0xFFFFFFFF;
		if (base != NULL) {
			*base = 10;
		}
	}
}
static void int_to_str(int value, int base, char* buffer) {
	/* Convert int to string. Supports hex, decimal, octal, binary */
	int32_t i = 30;
	uint32_t bits = 1;
	uint32_t temp = value;
	const char ascii[] = "0123456789ABCDEF";

	if (base < 2 || base > 16) {
		return;
	}

	if (value == 0) {
		buffer[i--] = ascii[0];
	}
	else {
		for (; value && i; --i) {
			buffer[i] = ascii[value % base];
			value /= base;
		}
	}

	switch (base) {
		case 2:
			/* Get number of bits */
			while (temp >>= 1) {
				bits++;
			}
			
			/* Round up to the nearest 8 bits */
			bits = (bits + 0x7) & ~0x7;

			/* Get number of bits we havent printed yet */
			bits -= 30 - i;

			/* Pad bits */
			for (; bits; --bits) {
				buffer[i--] = '0';
			}

			buffer[i--] = 'b';
			buffer[i--] = '0';
			break;

		case 8:
			buffer[i--] = 'o';
			buffer[i--] = '0';
			break;

		case 16:
			buffer[i--] = 'x';
			buffer[i--] = '0';
			break;
	}

	/* Shift to beginning of buffer */
	int j = 0;
	while (buffer[i + 1]) {
		buffer[j++] = buffer[++i];
	}
}

static void set_bool_value(LOADINI_VAR* ptr, const char* value) {
	if (strcmp(value, "f") == 0 || strcmp(value, "F") == 0 || strcmp(value, "false") == 0 || strcmp(value, "FALSE") == 0 || strcmp(value, "0") == 0) {
		*(bool*)ptr->var = false;
	}
	else if(strcmp(value, "t") == 0 || strcmp(value, "T") == 0 || strcmp(value, "true") == 0 || strcmp(value, "TRUE") == 0 || strcmp(value, "1") == 0) {
		*(bool*)ptr->var = true;
	} 
	else {
		*(bool*)ptr->var = false;
	}
	
}
static void set_u32_value(LOADINI_VAR* ptr, const char* value) {
	int number = 0;
	str_to_int(value, &number, NULL);
	*(uint32_t*)ptr->var = number & 0xFFFFFFFF;
}
static void set_u16_value(LOADINI_VAR* ptr, const char* value) {
	int number = 0;
	str_to_int(value, &number, NULL);
	*(uint16_t*)ptr->var = number & 0xFFFF;
}
static void set_u8_value(LOADINI_VAR* ptr, const char* value) {
	int number = 0;
	str_to_int(value, &number, NULL);
	*(uint8_t*)ptr->var = number & 0xFF;
}
static void set_float_value(LOADINI_VAR* ptr, const char* value) {
	*(float*)ptr->var = strtof(value, NULL);
}
static void set_double_value(LOADINI_VAR* ptr, const char* value) {
	*(double*)ptr->var = strtod(value, NULL);
}
static void set_str_value(const LOADINI_SETTING* setting, LOADINI_VAR* ptr, const char* value, size_t len) {
	if (setting->option & LOADINI_OPTION_STR_ALLOC) {
		/* With an ALLOCATED string; the var pointer is a pointer to a pointer */
		*(char**)ptr->var = calloc(1, len + 1);
		if (*(char**)ptr->var == NULL) {
			err_print("[LOADINI] Error: Out of memory\n");
			return;
		}
		if (len > 0) {
			strncpy_s(*(char**)ptr->var, len + 1, value, len);
		}
	}
	else {
		/* With an UNALLOCATED string; the var pointer is just a pointer */
		if ((char*)ptr->var == NULL) {
			err_print("[LOADINI] Error: Unallocated string var is NULL\n");
			return;
		}
		if (len > 0) {
			strncpy_s((char*)ptr->var, setting->u.string_info.len, value, setting->u.string_info.len - 1);
		}
	}
}
static void set_enum_value(const LOADINI_SETTING* setting, LOADINI_VAR* ptr, const char* value) {
	if (setting == NULL) {
		return;
	}
	if (setting->u.enum_info.def == NULL) {
		err_print("[LOADINI] Error: Enum def is NULL for key: %s\n", setting->key);
		return;
	}
	if (setting->u.enum_info.count == 0) {
		err_print("[LOADINI] Error: Enum count is 0 for key: %s\n", setting->key);
		return;
	}
	
	for (int i = 0; i < setting->u.enum_info.count; ++i) {
		if (strcmp(setting->u.enum_info.def[i].str, value) == 0) {
			*(uint8_t*)ptr->var = setting->u.enum_info.def[i].id;
			return;
		}
	}
	for (int i = 0; i < setting->u.enum_info.count; ++i) {
		int number = 0;
		str_to_int(value, &number, NULL);
		if (setting->u.enum_info.def[i].id == number) {			
			*(uint8_t*)ptr->var = number & 0xFF;
			return;
		}
	}
	*(uint8_t*)ptr->var = setting->u.enum_info.def[0].id;
}

static void get_bool_value(LOADINI_VAR* ptr, char* value) {
	if (*(bool*)ptr->var == false) {
		strcpy(value, "false");
	}
	else {
		strcpy(value, "true");
	}
}
static void get_u32_value(LOADINI_VAR* ptr, char* value) {
	int_to_str(*(uint32_t*)ptr->var, 16, value);
}
static void get_u16_value(LOADINI_VAR* ptr, char* value) {
	int_to_str(*(uint16_t*)ptr->var, 16, value);
}
static void get_u8_value(LOADINI_VAR* ptr, char* value) {
	int_to_str(*(uint8_t*)ptr->var, 16, value);
}
static void get_float_value(LOADINI_VAR* ptr, char* value) {
	sprintf(value, "%f", *(float*)ptr->var);
}
static void get_double_value(LOADINI_VAR* ptr, char* value) {
	sprintf(value, "%lf", *(double*)ptr->var);
}
static void get_str_value(const LOADINI_SETTING* setting, LOADINI_VAR* ptr, char* value) {
	if (setting == NULL) {
		return;
	}
	if (setting->option & LOADINI_OPTION_STR_ALLOC) {
		/* With an ALLOCATED string; the var pointer is a pointer to a pointer; this is so the variable pointed to by var is updated. */
		strcpy(value, *(char**)ptr->var);
	}
	else {
		/* With an UNALLOCATED string; the var pointer is just a pointer; this is because you cant get an address of an address. */
		strcpy(value, (char*)ptr->var);
	}
}
static void get_enum_value(const LOADINI_SETTING* setting, LOADINI_VAR* ptr, char* value) {
	if (setting == NULL) {
		return;
	}
	if (setting->u.enum_info.def == NULL) {
		err_print("[LOADINI] Error: Enum def is NULL for key: %s\n", setting->key);
		return;
	}
	if (setting->u.enum_info.count == 0) {
		err_print("[LOADINI] Error: Enum count is 0 for key: %s\n", setting->key);
		return;
	}

	for (int i = 0; i < setting->u.enum_info.count; ++i) {
		if (setting->u.enum_info.def[i].id == *(uint8_t*)ptr->var) {
			strcpy(value, setting->u.enum_info.def[i].str);
			return;
		}
	}
	strcpy(value, setting->u.enum_info.def[0].str);
}

static int new_block(const LOADINI_SETTING* setting, LOADINI_VAR* ptr, void* temp_block) {
	void** structure = (void**)((uint8_t*)ptr->var + setting->u.struct_info.ptr_offset);
	size_t count = 0;
	size_t index = 0;

	switch (setting->type) {
		case LOADINI_TYPE_STRUCT_ARRAY: { /* Array of structs */
			size_t* structure_count = (size_t*)((uint8_t*)ptr->var + setting->u.struct_info.count_offset);
			index = *structure_count;
			count = index + 1;
			*structure_count = count;
		} break;
		case LOADINI_TYPE_STRUCT: /* Only 1 struct */
			index = 0;
			count = 1;
			break;
	}

	void* new_block = realloc(*structure, setting->u.struct_info.def->size * count);
	if (new_block == NULL) {
		err_print("[LOADINI] Error: Out of memory\n");
		return LOADINI_ERROR_ALLOC;
	}

	dbg_print("[LOADINI] new_block: %p (%s)\n", new_block, setting->key);

	memcpy((uint8_t*)new_block + (setting->u.struct_info.def->size * index), temp_block, setting->u.struct_info.def->size);

	*structure = new_block;

	return LOADINI_ERROR_SUCCESS;
}

static int get_key_token(char** dest, size_t dest_size, const char** src, size_t* len, int* line_num) {
	char* delim = strchr(*src, LOADINI_DELIM);
	if (!delim) {
		err_print("[LOADINI] Error: Missing '=' on line %d\n", *line_num);
		return LOADINI_ERROR_INVALID_KEY;
	}
	str_cpy(dest, dest_size, *src, delim, len);
	str_trim(dest, len);
	*src = delim + 1;

	if (*len == 0) {
		err_print("[LOADINI] Error: Key is empty on line %d\n", *line_num);
		return LOADINI_ERROR_INVALID_KEY;
	}
	return LOADINI_ERROR_SUCCESS;
}
static int get_val_token(char** dest, size_t dest_size, char** src, size_t* len, int* line_num) {
	char* val_start = *src;
	*len = strlen(val_start);
	str_trim(&val_start, len);

	char* val_end = val_start;
	char* next = val_start;
	if (*val_start == '\'' || *val_start == '\"') {
		char quote = *val_start++;
		val_end = val_start;
		while (*val_end && *val_end != quote) {
			val_end++;
		}
		next = val_end;
		if (*val_end != quote) {
			err_print("[LOADINI] Error: Data is missing matching quote %c on line %d\n", quote, *line_num);
			return LOADINI_ERROR_INVALID_DATA;
		}
		else {
			next++; /* consume end quote */
		}
	}
	else {
		while (*val_end && *val_end != ',' && *val_end != ']' && *val_end != '\r' && *val_end != '\n') {
			val_end++;
		}
		next = val_end;
	}
	str_cpy(dest, dest_size, val_start, val_end, len);
	*src = next;

	return LOADINI_ERROR_SUCCESS;
}

static int write_var(LOADINI_CONTEXT* context, const LOADINI_SETTING* setting, LOADINI_VAR* ptr) {
	if (ptr->var == NULL) {
		err_print("[LOADINI] Error: Key: '%s' pointer not set\n", setting->key);
		return LOADINI_ERROR_INVALID_DATA;
	}

	memset(context->write_buf, 0, sizeof(context->write_buf));

	switch (setting->type) {
		case LOADINI_TYPE_BOOL:
			get_bool_value(ptr, context->write_buf);
			break;
		case LOADINI_TYPE_STR:
			get_str_value(setting, ptr, context->write_buf);
			break;
		case LOADINI_TYPE_U8:
			get_u8_value(ptr, context->write_buf);
			break;
		case LOADINI_TYPE_U16:
			get_u16_value(ptr, context->write_buf);
			break;
		case LOADINI_TYPE_U32:
			get_u32_value(ptr, context->write_buf);
			break;
		case LOADINI_TYPE_FLOAT:
			get_float_value(ptr, context->write_buf);
			break;
		case LOADINI_TYPE_DOUBLE:
			get_double_value(ptr, context->write_buf);
			break;
		case LOADINI_TYPE_ENUM:
			get_enum_value(setting, ptr, context->write_buf);
			break;
		default:
			err_print("[LOADINI] Error: Unknown setting type\n");
			return LOADINI_ERROR_INVALID_TYPE;
	}

	fprintf(context->stream, "%s = '%s'", setting->key, context->write_buf);
	dbg_print("[LOADINI] %s = '%s'\n", setting->key, context->write_buf);
	return LOADINI_ERROR_SUCCESS;
}
static int write_struct(LOADINI_CONTEXT* context, const LOADINI_SETTING* setting, LOADINI_VAR* ptr, int depth) {
		
	void** structure = (void**)((uint8_t*)ptr->var + setting->u.struct_info.ptr_offset);

	if (*structure == NULL) {
		return LOADINI_ERROR_SUCCESS;
	}

	size_t count = 0;
	switch (setting->type) {
		case LOADINI_TYPE_STRUCT_ARRAY: /* Array of structs */
			count = *(size_t*)((uint8_t*)ptr->var + setting->u.struct_info.count_offset);
			break;
		case LOADINI_TYPE_STRUCT: /* Only 1 struct */
			count = 1;
			break;
	}

	for (size_t i = 0; i < count; ++i) {
		
		fprintf(context->stream, "%s = [\n", setting->key);
		for (size_t j = 0; j < setting->u.struct_info.def->field_count; ++j) {

			if (j > 0) {
				fputc(',', context->stream);
				fputc('\n', context->stream);
			}

			for (int k = 0; k < depth+1; ++k) {
				fputc('\t', context->stream);
			}

			LOADINI_VAR field_var = { 0 };
			int r = 0;
			switch (setting->u.struct_info.def->fields[j].setting.type) {
				case LOADINI_TYPE_STRUCT:
				case LOADINI_TYPE_STRUCT_ARRAY:
					/* Write the struct to the stream */
					field_var.var = (uint8_t*)*structure + (setting->u.struct_info.def->size * i);
					r = write_struct(context, &setting->u.struct_info.def->fields[j].setting, &field_var, depth+1);
					break;
				default:
					/* Write the var to the stream */
					field_var.var = (uint8_t*)*structure + (setting->u.struct_info.def->size * i) + setting->u.struct_info.def->fields[j].offset;
					r = write_var(context, &setting->u.struct_info.def->fields[j].setting, &field_var);
					break;
			}
						
			if (r != LOADINI_ERROR_SUCCESS) {
				return r;
			}		
		}

		fputc('\n', context->stream);
		for (int k = 0; k < depth; ++k) {
			fputc('\t', context->stream);
		}
		fputc(']', context->stream);

		if (depth == 0) {
			fputc('\n', context->stream);
		}
	}
	return LOADINI_ERROR_SUCCESS;
}

static int parse_var(LOADINI_CONTEXT* context, const LOADINI_SETTING* setting, LOADINI_VAR* var) {
	if (var->var == NULL) {
		err_print("[LOADINI] key: '%s' pointer not set\n", setting->key);
		return LOADINI_ERROR_INVALID_DATA;
	}

	char* val_ptr = context->val;
	size_t val_len = 0;

	/* Get Value */
	int r = get_val_token(&val_ptr, sizeof(context->val), &context->buffer_ptr, &val_len, &context->line_num);
	if (r != LOADINI_ERROR_SUCCESS) {
		return r;
	}

	switch (setting->type) {
		case LOADINI_TYPE_STR:
			set_str_value(setting, var, val_ptr, val_len);
			break;
		case LOADINI_TYPE_BOOL:
			set_bool_value(var, val_ptr);
			break;
		case LOADINI_TYPE_U8:
			set_u8_value(var, val_ptr);
			break;
		case LOADINI_TYPE_U16:
			set_u16_value(var, val_ptr);
			break;
		case LOADINI_TYPE_U32:
			set_u32_value(var, val_ptr);
			break;
		case LOADINI_TYPE_FLOAT:
			set_float_value(var, val_ptr);
			break;
		case LOADINI_TYPE_DOUBLE:
			set_double_value(var, val_ptr);
			break;
		case LOADINI_TYPE_ENUM:
			set_enum_value(setting, var, val_ptr);
			break;
		default:
			err_print("[LOADINI] Unknown setting type\n");
			return LOADINI_ERROR_INVALID_TYPE;
	}

	dbg_print("[LOADINI] %s = '%s'\n", setting->key, val_ptr);

	return LOADINI_ERROR_SUCCESS;
}
static int parse_struct(LOADINI_CONTEXT* context, const LOADINI_SETTING* setting, LOADINI_VAR* ptr) {
	size_t i = 0;
	int r = 0;
	void* temp_structure = calloc(1, setting->u.struct_info.def->size);
	if (!temp_structure) {
		err_print("[LOADINI] Error: Out of memory\n");
		return LOADINI_ERROR_ALLOC;
	}

	dbg_print("[LOADINI] tmp_block: %p (%s)\n", temp_structure, setting->key);

	size_t buffer_len = strlen(context->buffer_ptr);
	trim_comment(context->buffer_ptr, &buffer_len);
	str_trim(&context->buffer_ptr, &buffer_len);

	if (context->buffer_ptr[0] == '[') {
		context->buffer_ptr++;
		buffer_len--;
	}
	else {
		err_print("[LOADINI] Error: Missing '[' in struct %s on line %d\n", setting->key, context->line_num);
		free(temp_structure);
		return LOADINI_ERROR_INVALID_DATA;
	}

	while (1) {

		while (*context->buffer_ptr) {

			buffer_len = strlen(context->buffer_ptr);
			trim_comment(context->buffer_ptr, &buffer_len);
			str_trim(&context->buffer_ptr, &buffer_len);

			if (buffer_len == 0) {
				break;
			}

			if (context->buffer_ptr[0] == ']') {
				context->buffer_ptr++;
				buffer_len--;
				r = new_block(setting, ptr, temp_structure);
				free(temp_structure);
				return r;
			}

			char* key_ptr = context->key;
			size_t key_len = 0;

			/* Get Key */
			r = get_key_token(&key_ptr, sizeof(context->key), &context->buffer_ptr, &key_len, &context->line_num);
			if (r != LOADINI_ERROR_SUCCESS) {
				return r;
			}

			for (i = 0; i < setting->u.struct_info.def->field_count; ++i) {
				if (strcmp(setting->u.struct_info.def->fields[i].setting.key, key_ptr) != 0) {
					continue;
				}
				
				LOADINI_VAR field_var = { 0 };
				
				switch (setting->u.struct_info.def->fields[i].setting.type) {
					case LOADINI_TYPE_STRUCT:
					case LOADINI_TYPE_STRUCT_ARRAY:
						/* Parse struct from the stream */
						field_var.var = temp_structure;
						r = parse_struct(context, &setting->u.struct_info.def->fields[i].setting, &field_var);
						break;
					default:
						/* Parse the var from the stream */
						field_var.var = (uint8_t*)temp_structure + setting->u.struct_info.def->fields[i].offset;
						r = parse_var(context, &setting->u.struct_info.def->fields[i].setting, &field_var);
						break;
				}

				if (r != LOADINI_ERROR_SUCCESS) {
					return r;
				}
				break;
			}

			if (i == setting->u.struct_info.def->field_count) {
				err_print("[LOADINI] Error: Unknown key in struct '%s': '%s' on line %d\n", setting->key, key_ptr, context->line_num);
				free(temp_structure);
				return LOADINI_ERROR_INVALID_KEY;
			}

			/* consume comma */
			while (*context->buffer_ptr == ',') {
				context->buffer_ptr++;
			}
		}
		
		if (!fgets(context->buffer, LOADINI_MAX_LINE_SIZE, context->stream)) {
			break;
		}

		context->buffer_ptr = context->buffer;
		context->line_num++;
	}

	err_print("[LOADINI] Error: Missing ']' in struct %s on line %d\n", setting->key, context->line_num);
	free(temp_structure);
	return LOADINI_ERROR_INVALID_DATA;
}
static int parse_next_line(LOADINI_CONTEXT* context, const LOADINI_SETTING* settings_map, LOADINI_VAR* var_map, const size_t count) {
	size_t i = 0;
	int r = 0;

	char* key_ptr = context->key;
	size_t key_len = 0;

	context->buffer_ptr = context->buffer;
	while (*context->buffer_ptr) {

		size_t buffer_len = strlen(context->buffer_ptr);
		trim_comment(context->buffer_ptr, &buffer_len);
		str_trim(&context->buffer_ptr, &buffer_len);

		if (buffer_len == 0) {
			return LOADINI_ERROR_SUCCESS;
		}

		/* Get Key */
		r = get_key_token(&key_ptr, sizeof(context->key), &context->buffer_ptr, &key_len, &context->line_num);
		if (r != LOADINI_ERROR_SUCCESS) {
			return r;
		}

		/* Compare key and set value */
		for (i = 0; i < count; ++i) {
			if (strcmp(key_ptr, settings_map[i].key) != 0) {
				continue;
			}

			switch (settings_map[i].type) {
				case LOADINI_TYPE_STRUCT:
				case LOADINI_TYPE_STRUCT_ARRAY:
					/* Parse struct from the stream */
					r = parse_struct(context, &settings_map[i], &var_map[i]);
					break;
				default:
					/* Parse the var from the stream */
					r = parse_var(context, &settings_map[i], &var_map[i]);
					break;
			}

			if (r != LOADINI_ERROR_SUCCESS) {
				return r;
			}
			break;
		}

		if (i == count) {
			err_print("[LOADINI] Error: Unknown key '%s' on line %d\n", key_ptr, context->line_num);
			return LOADINI_ERROR_INVALID_KEY;
		}

		/* consume comma */
		while (*context->buffer_ptr == ',') {
			context->buffer_ptr++;
		}
	}
	return LOADINI_ERROR_SUCCESS;
}

static int loadini_load_from_stream(LOADINI_CONTEXT* context, const LOADINI_SETTING* settings_map, LOADINI_VAR* var_map, const size_t count) {
	while (fgets(context->buffer, LOADINI_MAX_LINE_SIZE, context->stream) != NULL) {
		context->line_num++;
		int r = parse_next_line(context, settings_map, var_map, count);
		if (r != LOADINI_ERROR_SUCCESS) {
			return r;
		}
	}
	return LOADINI_ERROR_SUCCESS;
}
static int loadini_save_to_stream(LOADINI_CONTEXT* context, const LOADINI_SETTING* settings_map, LOADINI_VAR* var_map, const size_t count) {
	int r = 0;
	for (uint32_t i = 0; i < count; ++i) {
		switch (settings_map[i].type) {
			case LOADINI_TYPE_STRUCT:
			case LOADINI_TYPE_STRUCT_ARRAY:
				/* Write struct to the stream */
				r = write_struct(context, &settings_map[i], &var_map[i], 0);
				if (r != LOADINI_ERROR_SUCCESS) {
					return r;
				}
				break;
			default:
				/* Write the var to the stream */
				r = write_var(context, &settings_map[i], &var_map[i]);
				if (r != LOADINI_ERROR_SUCCESS) {
					return r;
				}
				fprintf(context->stream, "\n");
				break;
		}
	}
	return LOADINI_ERROR_SUCCESS;
}

int loadini_load_from_file(const char* path, const LOADINI_SETTING* settings_map, LOADINI_VAR* var_map, const size_t count) {
	LOADINI_CONTEXT context = { 0 };

	if (var_map == NULL) {
		err_print("[LOADINI] Error: var_map was NULL\n");
		return LOADINI_ERROR_FILE;
	}

	if (path == NULL) {
		err_print("[LOADINI] Error: Path was NULL\n");
		return LOADINI_ERROR_FILE;
	}

	context.stream = fopen(path, "rb");
	if (context.stream == NULL) {
		char* err_msg = strerror(errno);
		err_print("[LOADINI] %s: %s (%d)\n", path, err_msg, errno);
		return LOADINI_ERROR_FILE;
	}

	int r = loadini_load_from_stream(&context, settings_map, var_map, count);
	fclose(context.stream);
	return r;
}
int loadini_save_to_file(const char* const path, const LOADINI_SETTING* settings_map, LOADINI_VAR* var_map, const size_t count) {
	LOADINI_CONTEXT context = { 0 };

	if (var_map == NULL) {
		err_print("[LOADINI] Error: var_map was NULL\n");
		return LOADINI_ERROR_FILE;
	}

	if (path == NULL) {
		err_print("[LOADINI] Error: Path was NULL\n");
		return LOADINI_ERROR_FILE;
	}

	context.stream = fopen(path, "wb");
	if (context.stream == NULL) {
		char* err_msg = strerror(errno);
		err_print("[LOADINI] %s: %s (%d)\n", path, err_msg, errno);
		return LOADINI_ERROR_FILE;
	}

	int r = loadini_save_to_stream(&context, settings_map, var_map, count);
	fclose(context.stream);
	return r;
}

int loadini_create_var_map(LOADINI_VAR** var_map, const size_t count) {
	*var_map = calloc(count, sizeof(LOADINI_VAR));
	if (*var_map == NULL) {
		err_print("[LOADINI] Error: Out of memory\n");
		return LOADINI_ERROR_ALLOC;
	}
	
	for (int i = 0; i < count; ++i) {
		(*var_map)[i].var = NULL;
	}

	return 0;
}

static void destroy_str(const LOADINI_SETTING* setting, LOADINI_VAR* ptr) {
	if ((setting->option & LOADINI_OPTION_STR_ALLOC) && ptr->var != NULL) {
		free(ptr->var);
		ptr->var = NULL;
	}
}
static void destroy_struct(const LOADINI_SETTING* setting, LOADINI_VAR* ptr) {
	void** structure = (void**)((uint8_t*)ptr->var + setting->u.struct_info.ptr_offset);

	if (*structure == NULL) {
		return;
	}

	size_t count = 0;
	switch (setting->type) {
		case LOADINI_TYPE_STRUCT_ARRAY: /* Array of structs */
			count = *(size_t*)((uint8_t*)ptr->var + setting->u.struct_info.count_offset);
			break;
		case LOADINI_TYPE_STRUCT: /* Only 1 struct */
			count = 1;
			break;
	}

	/* iterate over each struct element */
	for (size_t j = 0; j < count; ++j) {
		void* element = (uint8_t*)*structure + (setting->u.struct_info.def->size * j);

		/* iterate over fields */
		for (size_t k = 0; k < setting->u.struct_info.def->field_count; ++k) {
			const LOADINI_FIELD* field = &setting->u.struct_info.def->fields[k];
			LOADINI_VAR field_var = { 0 };
			switch (field->setting.type) {
				case LOADINI_TYPE_STR:
					field_var.var = (uint8_t*)element + field->offset;
					destroy_str(&field->setting, &field_var);
					break;

				case LOADINI_TYPE_STRUCT:
				case LOADINI_TYPE_STRUCT_ARRAY:
					field_var.var = element;
					destroy_struct(&field->setting, &field_var);
					break;
			}
		}
	}

	dbg_print("[LOADINI] destroy_block: %p (%s)\n", *structure, setting->key);
	free(*structure);
	*structure = NULL;	
}
void loadini_destroy_var_map(const LOADINI_SETTING* settings_map, LOADINI_VAR* var_map, const size_t count) {
	if (var_map == NULL) {
		return;
	}

	for (int i = 0; i < count; ++i) {
		switch (settings_map[i].type) {
			case LOADINI_TYPE_STR:
				destroy_str(&settings_map[i], &var_map[i]);
				break;

			case LOADINI_TYPE_STRUCT:
			case LOADINI_TYPE_STRUCT_ARRAY:
				destroy_struct(&settings_map[i], &var_map[i]);
				break;
		}
	}
	free(var_map);
}
