/* mda.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * MDA
 */

#ifndef MDA_H
#define MDA_H

#include <stdint.h>

#include "crtc_6845.h"

#define MDA_STATUS_HRETRACE  0x01
#define MDA_STATUS_VRETRACE  0x08

#define MDA_MODE_LO_RES       0x00
#define MDA_MODE_HI_RES       0x01
#define MDA_MODE_VIDEO_ENABLE 0x08
#define MDA_MODE_BLINK_ENABLE 0x20

#define MDA_HI_RES_COLUMNS 80
#define MDA_HI_RES_ROWS    25
#define MDA_HI_RES_WIDTH   (720)
#define MDA_HI_RES_HEIGHT  (350)

#define MDA_ATTRIBUTE_BG          0x70 // background MASK b01110000
#define MDA_ATTRIBUTE_FG          0x07 // foreground MASK b00000111

#define MDA_ATTRIBUTE_NON_DISPLAY 0x00 // dont display text ?
#define MDA_ATTRIBUTE_UNDERLINE   0x01 // underline text
#define MDA_ATTRIBUTE_BW          0x07 // black text; white background
#define MDA_ATTRIBUTE_INTENSITY   0x10 // bold text
#define MDA_ATTRIBUTE_BLINK       0x80 // blink text

 /* MDA */
#define MDA_IO_BASE_ADDRESS 0x3B0   // IO base address of MDA card
#define MDA_MM_BASE_ADDRESS 0xB0000 // MM back address of MDA card
#define MDA_MM_ADDRESS_MASK 0x0FFF  // MM address mask

#define MDA_PHYS_ADDRESS(offset) (MDA_MM_BASE_ADDRESS + ((offset) & MDA_MM_ADDRESS_MASK))

/* MDA State */
typedef struct MDA {
	CRTC_6845 crtc;  /* cathode ray tube controller */
	uint8_t status;  /* status register */
	uint8_t mode;    /* mode control register */
	uint8_t blink;   /* blink variable */
	uint8_t color;   /* color control register */
	uint16_t hcount; /* horizontal pixel position */
	uint16_t vcount; /* vertical line position */
	uint64_t accum;  /* cycle accum */
} MDA;

/* hard reset mda */
void mda_reset(MDA* mda);

/* MDA Read IO */
uint8_t mda_read_io_byte(MDA* mda, uint8_t io_address);

/* MDA Write IO */
void mda_write_io_byte(MDA* mda, uint8_t io_address, uint8_t value);

/* MDA Update */
void mda_update(MDA* mda);

#endif
