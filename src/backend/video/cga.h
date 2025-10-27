/* cga.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Color Graphics Adapter
 */

#ifndef CGA_H
#define CGA_H

#include <stdint.h>

#include "crtc_6845.h"

 /* CGA */

/* CGA MM/IO Base Addresses */

#define CGA_IO_BASE_ADDRESS      0x3D0   // IO base address of CGA card
#define CGA_MM_BASE_ADDRESS      0xB8000 // MM base address of CGA card
#define CGA_MM_ADDRESS_MASK      0x3FFF  // MM address mask

#define CGA_PHYS_ADDRESS(offset) (CGA_MM_BASE_ADDRESS + ((offset) & CGA_MM_ADDRESS_MASK))

/* CGA Status Register */

#define CGA_STATUS_HRETRACE       0x01 /* Set when the screen is in a horizontal retrace interval. While this bit is set, video RAM can be accessed without "snow". */
#define CGA_STATUS_VRETRACE       0x08 /* Set when the screen is in a vertical retrace interval. While this bit is set, video RAM can be accessed without "snow". */

/* CGA Mode Control Register */

#define CGA_MODE_TEXT_RES_MASK   0x01 /* Text mode bit mask */
#define CGA_MODE_TEXT_RES_HI     0x01 /* Text mode 80x25 */
#define CGA_MODE_TEXT_RES_LO     0x00 /* Text mode 40x25 */

#define CGA_MODE_MASK            0x02 /* Mode bit mask */
#define CGA_MODE_GRAPHICS        0x02 /* Graphics mode*/
#define CGA_MODE_TEXT            0x00 /* Text mode */

/* If the card is displaying on a composite monitor, this disables the NTSC 
   color(sic) burst, giving black and white output. On an RGB monitor it has no
   effect except in the 320x200 graphics mode, when it selects a third palette
   (black/red/cyan/white). This palette is not documented, and not all of IBM's
   later CGA-compatible cards support it. */
#define CGA_MODE_BW              0x04

#define CGA_MODE_VIDEO_ENABLE    0x08 /* If set, screen output will not be shown (exactly as if the video RAM contained all zeroes) */

#define CGA_MODE_GRAPHICS_RES_MASK 0x10 /* Graphics mode bit mask */
#define CGA_MODE_GRAPHICS_RES_HI   0x10 /* 2-colour graphics @ 640px */
#define CGA_MODE_GRAPHICS_RES_LO   0x00 /* 4-colour graphics @ 320px */

#define CGA_MODE_BLINK_ENABLE    0x20 /* If set, characters with attribute bit 7 set will blink. If not, they will have high intensity background <Text Mode> */

#define CGA_MODE_CHANGED_MASK 0x13

/* Text High Res */
#define CGA_HI_RES_TEXT_WIDTH    640
#define CGA_HI_RES_TEXT_HEIGHT   200
#define CGA_HI_RES_TEXT_COLUMNS  80
#define CGA_HI_RES_TEXT_ROWS     25

/* Text Low Res */
#define CGA_LO_RES_TEXT_WIDTH    320
#define CGA_LO_RES_TEXT_HEIGHT   200
#define CGA_LO_RES_TEXT_COLUMNS  40
#define CGA_LO_RES_TEXT_ROWS     25

/* Graphics High Res */
#define CGA_HI_RES_GRAPHICS_WIDTH  640
#define CGA_HI_RES_GRAPHICS_HEIGHT 200

/* Graphics Low Res */
#define CGA_LO_RES_GRAPHICS_WIDTH  320
#define CGA_LO_RES_GRAPHICS_HEIGHT 200

/* CGA Text Mode Attribute Byte Masks */

#define CGA_ATTRIBUTE_FG     0x0F
#define CGA_ATTRIBUTE_B_FG   0x01 /* blue text */
#define CGA_ATTRIBUTE_G_FG   0x02 /* green text */
#define CGA_ATTRIBUTE_R_FG   0x04 /* red text */
#define CGA_ATTRIBUTE_BR_FG  0x08 /* bright text */

#define CGA_ATTRIBUTE_BG     0xF0
#define CGA_ATTRIBUTE_B_BG   0x10 /* blue background */
#define CGA_ATTRIBUTE_G_BG   0x20 /* green background */
#define CGA_ATTRIBUTE_R_BG   0x40 /* red background */
#define CGA_ATTRIBUTE_BR_BG  0x80 /* if mode bit.5 clr. bright background */
#define CGA_ATTRIBUTE_BLINK  0x80 /* if mode bit.5 set. blink text */

/* CGA Color Control Register */

/* selects one of the 16 CGA colours 
   (bit 0 = blue, bit 1 = green, bit 2 = red, bit 3 = intensity). 
   text mode:             border (overscan)
   320x200 graphics mode: background and border
   640x200 graphics mode: foreground colour */
#define CGA_COLOR_MASK 0x0F
#define CGA_COLOR_FG   CGA_COLOR_MASK
#define CGA_COLOR_BG   CGA_COLOR_MASK
#define CGA_COLOR_B    0x01 /* blue */
#define CGA_COLOR_G    0x02 /* green */
#define CGA_COLOR_R    0x04 /* red */
#define CGA_COLOR_BR   0x08 /* intensity */

/* 320x200 graphics mode only. If set, the foreground colours display in high intensity. */
#define CGA_COLOR_BRIGHT_FG 0x10

/* 320x200 graphics mode only. If set, the fixed colours in the palette are 
   magenta, cyan and white. If clr, they're red, green and yellow. Bit 2 in
   the mode control register (if set) overrides this bit. */
#define CGA_COLOR_PALETTE   0x20

/* CGA State */
typedef struct CGA {
	CRTC_6845 crtc;         /* cathode ray tube controller */
	uint8_t status;         /* status register */
	uint8_t mode;           /* mode control register */
	uint8_t blink;          /* blink variable */
	uint8_t color;          /* color control register */
	uint16_t width;         /* display output width */
	uint16_t height;        /* display output height */
	uint16_t hcount;        /* horizontal pixel position */
	uint16_t vcount;        /* vertical line position */
} CGA;

/* hard reset CGA */
void cga_reset(CGA* cga);

/* CGA Read IO */
uint8_t cga_read_io_byte(CGA* cga, uint8_t io_address);

/* CGA Write IO */
void cga_write_io_byte(CGA* cga, uint8_t io_address, uint8_t value);

void cga_tick(CGA* cga);

#endif
