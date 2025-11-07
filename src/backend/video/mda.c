/* mda.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * MDA - IBM Monochrome Display Adapter
 */

#include <stdint.h>

#include "mda.h"
#include "crtc_6845.h"

void mda_reset(MDA* mda) {
	crtc_6845_reset(&mda->crtc);
	mda->mode = 0;
	mda->status = 0;
	mda->color = 0;
	mda->blink = 0;
	mda->hcount = 0;
	mda->vcount = 0;
	mda->accum = 0;
}
uint8_t mda_read_io_byte(MDA* mda, uint8_t io_address) {
	switch (io_address) {

		case 0x1: // CRTC register (R/W)
		case 0x3:
		case 0x5:
		case 0x7:
			return crtc_6845_read_data(&mda->crtc);

		case 0xA: // MDA Status (read only)
			return mda->status;
	}
	return 0;
}

void mda_write_io_byte(MDA* mda, uint8_t port, uint8_t value) {
	switch (port) {

		case 0x0: // CRTC register index (WO)
		case 0x2:
		case 0x4:
		case 0x6:
			crtc_6845_write_index(&mda->crtc, value);
			break;

		case 0x1: // CRTC register (R/W)
		case 0x3:
		case 0x5:
		case 0x7:
			crtc_6845_write_data(&mda->crtc, value);
			break;

		case 0x8: // Mode Control Register (write only)
			mda->mode = value;
			break;

		case 0x9: // MDA Color Control Register (write only?)
			mda->color = value;
			break;
	}
}

void mda_update(MDA* mda) {

	uint8_t char_pixels = 9; /* 9 pixels per char */
	const uint8_t char_rows = mda->crtc.max_scanline + 1; /* x scanlines per char (usually 14) */

	const uint16_t htotal = (mda->crtc.htotal + 1) * char_pixels;
	const uint16_t hsync_pos = mda->crtc.hsync_pos * char_pixels;
	const uint16_t hsync_width = (mda->crtc.sync_width & 0x0F) * char_pixels;

	const uint16_t vtotal = ((mda->crtc.vtotal + 1) * char_rows) + mda->crtc.vtotal_adjust;
	const uint16_t vsync_pos = mda->crtc.vsync_pos * char_rows;

	uint16_t vsync_width = 0;
	if ((mda->crtc.sync_width & 0xF0) == 0) {
		vsync_width = 1 * char_rows;
	}
	else {
		vsync_width = ((mda->crtc.sync_width >> 4) & 0x0F) * char_rows;
	}

	mda->hcount++;
	if (mda->hcount >= htotal) {
		mda->hcount -= htotal;
		mda->vcount++;

		if (mda->vcount >= vtotal) {
			mda->vcount -= vtotal;
		}
	}

	mda->status &= ~(MDA_STATUS_HRETRACE | MDA_STATUS_VRETRACE);
	if (mda->hcount >= hsync_pos && mda->hcount < (hsync_pos + hsync_width)) {
		mda->status |= MDA_STATUS_HRETRACE;
	}
	if (mda->vcount >= vsync_pos && mda->vcount < (vsync_pos + vsync_width)) {
		mda->status |= MDA_STATUS_VRETRACE;
	}
}
