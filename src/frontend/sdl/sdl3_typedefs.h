/* graphics_typedefs.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
*/

#ifndef GRAPHICS_TYPEDEFS_H
#define GRAPHICS_TYPEDEFS_H

#include <stdint.h>

typedef struct COLOR_RGBA {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} COLOR_RGBA;

typedef struct COLOR_RGB {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} COLOR_RGB;

#endif
