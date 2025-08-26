/* crtc_6845.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Motorola 6845 cathode ray tube controller (CRTC)
 */

#ifndef CRTC_6845_H
#define CRTC_6845_H

#include <stdint.h>

#define CRTC_6845_REG_COUNT    18 // 18 indexable registers (19 including the index register)

/* R00 8bit write only
	The total displayed characters plus the non-displayed character times retrace 
	minus one */
#define CRTC_6845_HORIZONTAL_TOTAL        0x00 // R00

/* R01 8bit write only
	Determines the number of displayed characters per line. Any 8bit number may 
	be programmed as long as R0 greater than R1 */
#define CRTC_6845_HORIZONTAL_DISPLAYED    0x01 

/* R02 8bit write only
	When the programmed value of this register is increased, the display on the 
	CRT screen IS shifted to the left. When the programmed value is decreased the
	display is shifted to the right. Any 8bit value may be programmed as long as 
	the sum of R2 & R3 are less R0. R2 must be greater than R1 */
#define CRTC_6845_H_SYNC_POSITION         0x02 

/* R03 8bit write only 
	Determines the width of the horizontal sync Sync Width Register */	
#define CRTC_6845_SYNC_WIDTH              0x03

/* R04 7bit write only */
#define CRTC_6845_VERTICAL_TOTAL          0x04

/* R05 5bit write only */
#define CRTC_6845_V_TOTAL_ADJUST          0x05

/* R06 7bit write only
	Determines the number of displayed character rows. Any value less than R4 
	may be programmed */
#define CRTC_6845_VERTICAL_DISPLAYED      0x06

/* R07 7bit write only 
	Controls the position of vertical sync with respect to the reference. It is
	programmed in character row times.
	When increased, the display position is shifted up.
	When decreased, the display position is shifted down.
	Any value equal or less than R4 and greater than or	equal to R6 may be used. */
#define CRTC_6845_V_SYNC_POSITION         0x07

/* R08 2bit write only 
	b00 = normal sync mode (non-interlace)
	b10 = normal sync mode (non-interlace)
	b01 = interlace sync mode 
	b11 = interlace sync mode */
#define CRTC_6845_INTERLACE_MODE_AND_SKEW 0x08

/* R09 5bit write only
	Maximum Scan Line Address Register. determines the number of scan lines per
	character row including the spacing; thus, controlling operation of the row
	address counter. The programmed value is a maximum address and is one less
	than the number of scan lines */
#define CRTC_6845_MAX_SCAN_LINE_ADDRESS   0x09

/* R10 7bit write only
	Start scan line and the cursor blink rate. Bits 5 and 6 control the cursor
	operation. b00 = non-blink; b01 = cursor non-display; b10 = Blink 1/16; b11 = Blink 1/32 */
#define CRTC_6845_CURSOR_START 0x0A

/* R11 5bit write only 
	Defines the last scan line of the cursor */
#define CRTC_6845_CURSOR_END   0x0B

/* R12 6bit write only pair high 
	The start address register determines which portion of the RAM is displayed */
#define CRTC_6845_ADDRESS_HI   0x0C

/* R13 8bit write only pair low 
	The start address register determines which portion of the RAM is displayed */
#define CRTC_6845_ADDRESS_LO   0x0D

/* R14 6bit read/write pair high 
	The cursor register */
#define CRTC_6845_CURSOR_HI    0x0E

/* R15 8bit read/write pair low 
	The cursor register */
#define CRTC_6845_CURSOR_LO    0x0F

/* R16 6bit read-only */
#define CRTC_6845_LIGHT_PEN_HI 0x10

/* R17 8bit read-only */
#define CRTC_6845_LIGHT_PEN_LO 0x11

/* Cursor Attributes (R10) */
#define CRTC_6845_CURSOR_ATTR_MASK       0x60 /* Attribute mask */
#define CRTC_6845_CURSOR_ATTR_SOILD      0x00 /* enabled; non-blink */
#define CRTC_6845_CURSOR_ATTR_DISABLED   0x20 /* disabled; non-blink */
#define CRTC_6845_CURSOR_ATTR_BLINK_FAST 0x40 /* enabled; fast blink */
#define CRTC_6845_CURSOR_ATTR_BLINK_SLOW 0x60 /* enabled; slow blink */

 /* Cathode Ray Tube Controller 6845 */
typedef struct CRTC_6845 {
	uint8_t index; /* 5bit write only */
	uint8_t registers[CRTC_6845_REG_COUNT];
} CRTC_6845;

void crtc_6845_reset(CRTC_6845* crtc);
void crtc_6845_write_index(CRTC_6845* crtc, uint8_t value);
uint8_t crtc_6845_read_data(CRTC_6845* crtc);
void crtc_6845_write_data(CRTC_6845* crtc, uint8_t value);

#endif
