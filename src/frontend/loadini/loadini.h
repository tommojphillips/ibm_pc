/* loadini.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 */

#ifndef LOAD_INI_H
#define LOAD_INI_H

#include <stdint.h>

/* LOADINI Type */
typedef uint8_t LOADINI_TYPE;
#define LOADINI_TYPE_BOOL          0 /* boolean */
#define LOADINI_TYPE_STR           1 /* null terminated string */
#define LOADINI_TYPE_U8            2 /* unsigned 8 bit number */
#define LOADINI_TYPE_U16           3 /* unsigned 16 bit number */
#define LOADINI_TYPE_U32           4 /* unsigned 32 bit number */
#define LOADINI_TYPE_FLOAT         8 /* float */
#define LOADINI_TYPE_DOUBLE        9 /* double */
#define LOADINI_TYPE_ENUM         10 /* string-ed identifier */
#define LOADINI_TYPE_STRUCT       11 /* struct */
#define LOADINI_TYPE_STRUCT_ARRAY 12 /* struct array */

/* LOADINI Option */
typedef uint8_t LOADINI_OPTION;
#define LOADINI_OPTION_NONE      0 /* none */
#define LOADINI_OPTION_STR_ALLOC 1 /* type str allocate buffer */
#define LOADINI_OPTION_CB        2 /* cb function */

/* LOADINI Error */
typedef int LOADINI_ERROR;
#define LOADINI_ERROR_SUCCESS      0 /* success */
#define LOADINI_ERROR_INVALID_DATA 1 /* data error */
#define LOADINI_ERROR_INVALID_KEY  2 /* key error */
#define LOADINI_ERROR_FILE         3 /* file error */
#define LOADINI_ERROR_INVALID_TYPE 4 /* type error */
#define LOADINI_ERROR_ALLOC        5 /* alloc error */

/* Get struct field offset */
#define LOADINI_FIELD_OFFSET(s, f) ((size_t)&(((s*)0)->f))

/* Get struct field size */
#define LOADINI_FIELD_SIZE(s, f)   (sizeof(((s*)0)->f))

/* Get array count */
#define LOADINI_ARRAY_COUNT(s, m)  (sizeof(m) / sizeof(s))

/* Setting bool def
 name: The name of the setting */
#define LOADINI_SETTING_BOOL(name) { name, LOADINI_TYPE_BOOL, LOADINI_OPTION_NONE }

/* Setting u32 def
 name: The name of the setting */
#define LOADINI_SETTING_U32(name) { name, LOADINI_TYPE_U32, LOADINI_OPTION_NONE }

/* Setting u16 def
 name: The name of the setting */
#define LOADINI_SETTING_U16(name) { name, LOADINI_TYPE_U16, LOADINI_OPTION_NONE }

/* Setting u8 def
 name: The name of the setting */
#define LOADINI_SETTING_U8(name) { name, LOADINI_TYPE_U8, LOADINI_OPTION_NONE }

/* Setting string alloc def
 name: The name of the setting */
#define LOADINI_SETTING_STRING_ALLOC(name) { name, LOADINI_TYPE_STR, LOADINI_OPTION_STR_ALLOC, .u.string_info = { 0 } }

/* Setting string pre-alloc def
 name: The name of the setting
 len:  The size of the pre-allocated array */
#define LOADINI_SETTING_STRING(name, len) { name, LOADINI_TYPE_STR, LOADINI_OPTION_NONE, .u.string_info = { len } }

/* Setting enum def
 name: The name of the setting
 def:  The LOADINI_ENUM definition */
#define LOADINI_SETTING_ENUM(name, def) { name, LOADINI_TYPE_ENUM, LOADINI_OPTION_NONE, .u.enum_info = { def, LOADINI_ARRAY_COUNT(LOADINI_ENUM, def) } }

/* Setting struct def
 name: The name of the setting
 type: The struct type
 def:  The LOADINI_STRUCT definition
 ptr:  The pointer to the struct */
#define LOADINI_SETTING_STRUCT(name, type, def, ptr) { name, LOADINI_TYPE_STRUCT, LOADINI_OPTION_NONE, .u.struct_info = { def, LOADINI_FIELD_OFFSET(type, ptr), 0 } }

/* Setting struct array def 
 name:  The name of the setting
 type:  The struct type
 def:   The LOADINI_STRUCT definition
 ptr:   The pointer to the struct
 count: The count var. Holds the amount of structs in the ptr */
#define LOADINI_SETTING_STRUCT_ARRAY(name, type, def, ptr, count) { name, LOADINI_TYPE_STRUCT_ARRAY, LOADINI_OPTION_NONE, .u.struct_info = { def, LOADINI_FIELD_OFFSET(type, ptr), LOADINI_FIELD_OFFSET(type, count) } }

 /* Field bool def
  name:  The name of the setting
  type:  The fields parent struct type
  field: The field */
#define LOADINI_FIELD_BOOL(name, type, field) { LOADINI_SETTING_BOOL(name), LOADINI_FIELD_OFFSET(type, field) }

/* Field u32 def
 name:  The name of the setting
 type:  The fields parent struct type
 field: The field */
#define LOADINI_FIELD_U32(name, type, field) { LOADINI_SETTING_U32(name), LOADINI_FIELD_OFFSET(type, field) }

/* Field u16 def
 name:  The name of the setting
 type:  The fields parent struct type
 field: The field */
#define LOADINI_FIELD_U16(name, type, field) { LOADINI_SETTING_U16(name), LOADINI_FIELD_OFFSET(type, field) }

/* Field u8 def
 name:  The name of the setting
 type:  The fields parent struct type
 field: The field */
#define LOADINI_FIELD_U8(name, type, field) { LOADINI_SETTING_U8(name), LOADINI_FIELD_OFFSET(type, field) }

/* Field string alloc def
 name:  The name of the setting
 type:  The fields parent struct type
 field: The field */
#define LOADINI_FIELD_STR_ALLOC(name, type, field) { LOADINI_SETTING_STRING_ALLOC(name), LOADINI_FIELD_OFFSET(type, field) }

/* Field string pre-alloc def
 name:  The name of the setting
 type:  The fields parent struct type
 field: The field */
#define LOADINI_FIELD_STR(name, type, field) { LOADINI_SETTING_STRING(name, LOADINI_FIELD_SIZE(type, field)), LOADINI_FIELD_OFFSET(type, field), }

/* Field enum def
 name:  The name of the setting
 type:  The fields parent struct type
 field: The field 
 def:   The LOADINI_ENUM definition */
#define LOADINI_FIELD_ENUM(name, type, field, def) { LOADINI_SETTING_ENUM(name, def), LOADINI_FIELD_OFFSET(type, field) }

/* Field struct def
 name:  The name of the setting
 type:  The fields parent struct type
 field: The field 
 def:   The LOADINI_STRUCT definition */
#define LOADINI_FIELD_STRUCT(name, type, field, def) { LOADINI_SETTING_STRUCT(name, type, def, field), LOADINI_FIELD_OFFSET(type, field) }

/* Field struct array def
 name:  The name of the setting
 type:  The fields parent struct type
 field: The field
 count: The count var. Holds the amount of structs in the ptr
 def:   The LOADINI_STRUCT definition */
#define LOADINI_FIELD_STRUCT_ARRAY(name, type, field, count, def) { LOADINI_SETTING_STRUCT_ARRAY(name, type, def, field, count), LOADINI_FIELD_OFFSET(type, field) }

#define LOADINI_STRUCT_DEF(fields, type) sizeof(type), fields, LOADINI_ARRAY_COUNT(LOADINI_FIELD, fields)

/* LOADINI Enum */
typedef struct LOADINI_ENUM {
	const char* str;
	uint8_t id;
} LOADINI_ENUM;

typedef struct LOADINI_FIELD LOADINI_FIELD;

/* LOADINI Struct */
typedef struct LOADINI_STRUCT {
	size_t size;                 /* sizeof(target struct) */
	const LOADINI_FIELD* fields; /* Array of fields */
	size_t field_count;          /* Array field count*/
} LOADINI_STRUCT;

typedef struct LOADINI_ENUM_INFO {
	const LOADINI_ENUM* def;
	size_t count;
} LOADINI_ENUM_INFO;

typedef struct LOADINI_STRUCT_INFO {
	const LOADINI_STRUCT* def;
	size_t ptr_offset;
	size_t count_offset;
} LOADINI_STRUCT_INFO;

typedef struct LOADINI_STRING_INFO {
	size_t len;
} LOADINI_STRING_INFO;

/* LOADINI Setting */
typedef struct LOADINI_SETTING {
	const char* key;       /* The key for this setting */
	LOADINI_TYPE type;     /* The type of VAR. LOADINI_TYPE_* */
	LOADINI_OPTION option; /* The options for this setting */
	union {
		LOADINI_STRUCT_INFO struct_info;
		LOADINI_ENUM_INFO enum_info;
		LOADINI_STRING_INFO string_info;
	} u;
} LOADINI_SETTING;

/* LOADINI Struct Field */
typedef struct LOADINI_FIELD {
	LOADINI_SETTING setting;
	size_t offset;
} LOADINI_FIELD;

/* LOADINI Var */
typedef struct LOADINI_VAR {
	void* var;    /* The value */
} LOADINI_VAR;

// Load map from a file
// path:         The file path to the save file (.INI)
// settings_map: The settings map 
// var_map:      The var map to load settings into
// count:        Total settings in map
// returns:      LOADINI_ERROR*
int loadini_load_from_file(const char* path, const LOADINI_SETTING* settings_map, LOADINI_VAR* var_map, const size_t count);

// Save map to a file
// path:         The file path to the save file (.INI)
// settings_map: The settings map 
// var_map:      The var map to save settings from
// count:        Total settings in map
// returns:      LOADINI_ERROR*
int loadini_save_to_file(const char* const path, const LOADINI_SETTING* settings_map, LOADINI_VAR* var_map, const size_t count);

// Create a var map
// var_map:      The var map to create
// count:        Total settings in map
// returns:      LOADINI_ERROR*
int loadini_create_var_map(LOADINI_VAR** var_map, const size_t count);

// Destroy a var map
// settings_map: The settings map 
// var_map:      The var map to destroy
// count:        Total settings in map
void loadini_destroy_var_map(const LOADINI_SETTING* settings_map, LOADINI_VAR* var_map, const size_t count);

#endif
