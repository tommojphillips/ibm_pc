/* crtc_6845.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Motorola 6845 cathode ray tube controller (CRTC)
 */

/* Sources
 * 6845 datasheet: https://bitsavers.trailing-edge.com/components/motorola/_dataSheets/6845.pdf 
*/

#include <stdint.h>

#include "crtc_6845.h"

static const uint8_t masks[CRTC_6845_REG_COUNT] = {
	0xFF, // R00 8bit
	0xFF, // R01 8bit
	0xFF, // R02 8bit
	0xFF, // R03 8bit
	0x7F, // R04 7bit
	0x1F, // R05 5bit
	0x7F, // R06 7bit
	0x7F, // R07 7bit
	0x03, // R08 2bit
	0x1F, // R09 5bit
	0x7F, // R10 7bit
	0x1F, // R11 5bit
	0x3F, // R12 6bit
	0xFF, // R13 8bit
	0x3F, // R14 6bit
	0xFF, // R15 8bit
	0x3F, // R16 6bit
	0xFF, // R17 8bit
};

void crtc_6845_reset(CRTC_6845* crtc) {
	crtc->index = 0;
	for (int i = 0; i < CRTC_6845_REG_COUNT; ++i) {
		crtc->registers[i] = 0;
	}
}

void crtc_6845_write_index(CRTC_6845* crtc, uint8_t value) {
	crtc->index = (value & 0x1F); /* 5bit */
}
uint8_t crtc_6845_read_data(CRTC_6845* crtc) {
	/* read registers start from R14 inclusive onwards */
	if (crtc->index > CRTC_6845_ADDRESS_LO && crtc->index < CRTC_6845_REG_COUNT) {
		return crtc->registers[crtc->index];
	}
	return 0;
}
void crtc_6845_write_data(CRTC_6845* crtc, uint8_t value) {
	/* write registers start from R0 inclusive and end at R15 inclusive */
	if (crtc->index < CRTC_6845_LIGHT_PEN_HI) {
		crtc->registers[crtc->index] = (value & masks[crtc->index]);
	}
}
