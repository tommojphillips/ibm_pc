/* mda.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * MDA - IBM Monochrome Display Adapter
 */

#include <stdint.h>

#include "mda.h"
#include "crtc_6845.h"
#include "../bit_utils.h"

uint8_t mda_status(MDA* mda) {
	mda->status ^= MDA_STATUS_HRETRACE; /* fake h retrace */
	mda->status ^= MDA_STATUS_VRETRACE; /* fake v retrace */
	return mda->status;
}
void mda_mode(MDA* mda, uint8_t value) {
	if (value & MDA_MODE_HI_RES) {
		mda->columns = MDA_HI_RES_COLUMNS;
		mda->rows = MDA_HI_RES_ROWS;
		mda->width = MDA_HI_RES_WIDTH;
		mda->height = MDA_HI_RES_HEIGHT;
	}

	mda->mode = value;
}

void mda_reset(MDA* mda) {
	mda_mode(mda, MDA_MODE_LO_RES);
}
uint8_t mda_read_io_byte(MDA* mda, uint8_t io_address) {
	switch (io_address) {

		case 0x1: // CRTC register (R/W)
		case 0x3:
		case 0x5:
		case 0x7:
			return crtc_6845_read_data(&mda->crtc);

		case 0xA: // MDA Status (read only)
			return mda_status(mda);
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
			mda_mode(mda, value);
			break;
	}
}
