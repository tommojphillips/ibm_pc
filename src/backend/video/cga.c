/* cga.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Color Graphics Adapter
 */

#include <stdint.h>

#include "cga.h"
#include "crtc_6845.h"

void cga_reset(CGA* cga) {
	crtc_6845_reset(&cga->crtc);
	cga->mode = 0;
	cga->status = 0;
	cga->blink = 0;
	cga->color = 0;
	cga->hcount = 0;
	cga->vcount = 0;
	cga->accum = 0;
}
uint8_t cga_read_io_byte(CGA* cga, uint8_t io_address) {
	switch (io_address) {

		case 0x1: // CRTC register (R/W)
		case 0x3:
		case 0x5:
		case 0x7:
			return crtc_6845_read_data(&cga->crtc);

		case 0xA: // CGA Status (read only)
			return cga->status;
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
			cga->mode = value;
			break;

		case 0x9: // CGA Color Control Register (write only?)
			cga->color = value;
			break;
	}
}

void cga_update(CGA* cga) {

	uint8_t char_pixels = 0; /* x pixels per char */

	if (cga->mode & CGA_MODE_GRAPHICS) {
		if (cga->mode & CGA_MODE_GRAPHICS_RES_HI) {
			char_pixels = 8;
		}
		else {
			char_pixels = 4;
		}
	}
	else {
		if (cga->mode & CGA_MODE_TEXT_RES_HI) {
			char_pixels = 8;
		}
		else {
			char_pixels = 16;
		}
	}

	const uint8_t char_rows = cga->crtc.max_scanline + 1; /* x scanlines per char (usually 8) */

	const uint16_t htotal = (cga->crtc.htotal + 1) * char_pixels;
	const uint16_t hsync_pos = cga->crtc.hsync_pos * char_pixels;
	const uint16_t hsync_width = (cga->crtc.sync_width & 0x0F) * char_pixels;

	const uint16_t vtotal = ((cga->crtc.vtotal + 1) * char_rows) + cga->crtc.vtotal_adjust;
	const uint16_t vsync_pos = cga->crtc.vsync_pos * char_rows;
	
	uint16_t vsync_width = 0;
	if ((cga->crtc.sync_width & 0xF0) == 0) {
		vsync_width = 1 * char_rows;
	}
	else {
		vsync_width = ((cga->crtc.sync_width >> 4) & 0x0F) * char_rows;
	}

	cga->hcount++;
	if (cga->hcount >= htotal) {
		cga->hcount -= htotal;
		cga->vcount++;

		if (cga->vcount >= vtotal) {
			cga->vcount -= vtotal;
		}
	}

	// Update status bits
	cga->status &= ~(CGA_STATUS_HRETRACE | CGA_STATUS_VRETRACE);	
	if (cga->hcount >= hsync_pos && cga->hcount < (hsync_pos + hsync_width)) {
		cga->status |= CGA_STATUS_HRETRACE;
	}
	if (cga->vcount >= vsync_pos && cga->vcount < (vsync_pos + vsync_width)) {
		cga->status |= CGA_STATUS_VRETRACE;
	}	
}
