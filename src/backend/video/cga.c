/* cga.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Color Graphics Adapter
 */

#include <stdint.h>

#include "cga.h"
#include "crtc_6845.h"
#include "../bit_utils.h"

static uint8_t cga_status(CGA* cga) {
	cga->status ^= CGA_STATUS_HRETRACE; /* fake h retrace */
	cga->status ^= CGA_STATUS_VRETRACE; /* fake v retrace */
	return cga->status;
}
static void cga_color(CGA* cga, uint8_t value) {
	cga->color = value;
}
static void cga_mode(CGA* cga, uint8_t value) {
	if (value & CGA_MODE_GRAPHICS) {		
		if (value & CGA_MODE_GRAPHICS_RES_HI) {
			/* high res graphics mode */
			cga->columns = 0;
			cga->rows    = 0;
			cga->width   = CGA_HI_RES_GRAPHICS_WIDTH;
			cga->height  = CGA_HI_RES_GRAPHICS_HEIGHT;
		}
		else {
			/* low res graphics mode */
			cga->columns = 0;
			cga->rows    = 0;
			cga->width   = CGA_LO_RES_GRAPHICS_WIDTH;
			cga->height  = CGA_LO_RES_GRAPHICS_HEIGHT;
		}
	}
	else {
		if (value & CGA_MODE_TEXT_RES_HI) {
			/* high res text mode */
			cga->columns = CGA_HI_RES_TEXT_COLUMNS;
			cga->rows    = CGA_HI_RES_TEXT_ROWS;
			cga->width   = CGA_HI_RES_TEXT_WIDTH;
			cga->height  = CGA_HI_RES_TEXT_HEIGHT;
		}
		else {
			/* low res text mode */
			cga->columns = CGA_LO_RES_TEXT_COLUMNS;
			cga->rows    = CGA_LO_RES_TEXT_ROWS;
			cga->width   = CGA_LO_RES_TEXT_WIDTH;
			cga->height  = CGA_LO_RES_TEXT_HEIGHT;
		}
	}
	
	cga->mode = value;
}

void cga_reset(CGA* cga) {
	crtc_6845_reset(&cga->crtc);
	cga_mode(cga, CGA_MODE_TEXT | CGA_MODE_TEXT_RES_LO);
}
uint8_t cga_read_io_byte(CGA* cga, uint8_t io_address) {
	switch (io_address) {

		case 0x1: // CRTC register (R/W)
		case 0x3:
		case 0x5:
		case 0x7:
			return crtc_6845_read_data(&cga->crtc);

		case 0xA: // CGA Status (read only)
			return cga_status(cga);
	}
	return 0;
}
void cga_write_io_byte(CGA* cga, uint8_t io_address, uint8_t value) {
	switch (io_address) {

		case 0x0: // CRTC register index (WO)
		case 0x2:
		case 0x4:
		case 0x6:
			crtc_6845_write_index(&cga->crtc, value);
			break;

		case 0x1: // CRTC register (R/W)
		case 0x3:
		case 0x5:
		case 0x7:
			crtc_6845_write_data(&cga->crtc, value);
			break;

		case 0x8: // CGA Mode Control Register (write only)
			cga_mode(cga, value);
			break;

		case 0x9: // CGA Color Control Register (write only?)
			cga_color(cga, value);
			break;
	}
}
