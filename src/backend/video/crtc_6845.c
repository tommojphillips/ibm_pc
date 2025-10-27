/* crtc_6845.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Motorola 6845 cathode ray tube controller (CRTC)
 */

/* Sources
 * 6845 datasheet: https://bitsavers.trailing-edge.com/components/motorola/_dataSheets/6845.pdf 
*/

#include <stdint.h>

#include "crtc_6845.h"

void crtc_6845_reset(CRTC_6845* crtc) {
	crtc->index = 0;
	crtc->htotal = 0;
	crtc->hdisp = 0;
	crtc->hsync_pos = 0;
	crtc->sync_width = 0;
	crtc->vtotal = 0;
	crtc->vtotal_adjust = 0;
	crtc->vdisp = 0;
	crtc->vsync_pos = 0;
	crtc->interlace_mode = 0;
	crtc->max_scanline = 0;
	crtc->cursor_start = 0;
	crtc->cursor_end = 0;
	crtc->start_address = 0;
	crtc->cursor_address = 0;
	crtc->lightpen_address = 0;
}

void crtc_6845_write_index(CRTC_6845* crtc, uint8_t value) {
	crtc->index = (value & 0x1F); /* 5bit */
}
uint8_t crtc_6845_read_data(CRTC_6845* crtc) {	
	switch (crtc->index) {
		case CRTC_6845_CURSOR_HI:
			return (crtc->cursor_address >> 8) & 0x3F; /* 6bit */
		case CRTC_6845_CURSOR_LO:
			return crtc->cursor_address & 0xFF; /* 8bit */
		case CRTC_6845_LIGHT_PEN_HI:
			return (crtc->lightpen_address >> 8) & 0x3F; /* 6bit */
		case CRTC_6845_LIGHT_PEN_LO:
			return crtc->lightpen_address & 0xFF; /* 8bit */
	}
	return 0;
}
void crtc_6845_write_data(CRTC_6845* crtc, uint8_t value) {
	switch (crtc->index) {
		case CRTC_6845_HORIZONTAL_TOTAL:
			crtc->htotal = value; /* 8bit */
			break;
		case CRTC_6845_HORIZONTAL_DISPLAYED:
			crtc->hdisp = value; /* 8bit */
			break;
		case CRTC_6845_H_SYNC_POSITION:
			crtc->hsync_pos = value; /* 8bit */
			break;
		case CRTC_6845_SYNC_WIDTH:
			crtc->sync_width = value; /* 8bit */
			break;
		case CRTC_6845_VERTICAL_TOTAL:
			crtc->vtotal = value & 0x7F; /* 7bit */
			break;
		case CRTC_6845_V_TOTAL_ADJUST:
			crtc->vtotal_adjust = value & 0x1F; /* 5bit */
			break;
		case CRTC_6845_VERTICAL_DISPLAYED:
			crtc->vdisp = value & 0x7F; /* 7bit */
			break;
		case CRTC_6845_V_SYNC_POSITION:
			crtc->vsync_pos = value & 0x7F; /* 7bit */
			break;
		case CRTC_6845_INTERLACE_MODE_AND_SKEW:
			crtc->interlace_mode = value & 0x03; /* 6bit but only the lower 2bits are used */
			break;
		case CRTC_6845_MAX_SCAN_LINE_ADDRESS:
			crtc->max_scanline = value & 0x1F; /* 5bit */
			break;
		case CRTC_6845_CURSOR_START:
			crtc->cursor_start = value & 0x7F; /* 7bit */
			break;
		case CRTC_6845_CURSOR_END:
			crtc->cursor_end = value & 0x1F; /* 5bit */
			break;
		case CRTC_6845_ADDRESS_HI:
			crtc->start_address = (crtc->start_address & 0x00FF) | ((value & 0x3F) << 8); /* 6bit */
			break;
		case CRTC_6845_ADDRESS_LO:
			crtc->start_address = (crtc->start_address & 0xFF00) | value; /* 8bit */
			break;
		case CRTC_6845_CURSOR_HI:
			crtc->cursor_address = (crtc->cursor_address & 0x00FF) | ((value & 0x3F) << 8); /* 6bit */
			break;
		case CRTC_6845_CURSOR_LO:
			crtc->cursor_address = (crtc->cursor_address & 0xFF00) | value; /* 8bit */
			break;
	}
}
