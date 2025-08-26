/* i8255_ppi.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Intel 8255 Programmable Peripheral Interface (PPI)
 */

#include <stdint.h>

#include "i8255_ppi.h"

#define MODE_SELECT  0x80 // 

#define GROUP_A_MODE 0x60 // b00 = MODE0; b01 = MODE1; b1X = MODE2
#define PORT_A_MODE  0x10 // 0  = OUTPUT; 1 = INPUT
#define PORT_CH_MODE 0x08 // 0  = OUTPUT; 1 = INPUT

#define GROUP_B_MODE 0x04 // b0 = MODE0; b1 = MODE1
#define PORT_B_MODE  0x02 // 0  = OUTPUT; 1 = INPUT
#define PORT_CL_MODE 0x01 // 0  = OUTPUT; 1 = INPUT

#define GROUP_A_MODE0 0x00 /* Basic io */
#define GROUP_A_MODE1 0x20 /* Strobed io */
#define GROUP_A_MODE2 0x40 /* Strobed bi-directional io */

#define GROUP_B_MODE0 0x00
#define GROUP_B_MODE1 0x04

#define PORT_A    0x0
#define PORT_B    0x1
#define PORT_C    0x2
#define PORT_CTRL 0x3

void i8255_ppi_reset(I8255_PPI* ppi) {
	ppi->control = 0x0;
}

uint8_t i8255_ppi_read_io_byte(I8255_PPI* ppi, uint8_t io_address) {
	switch (io_address) {
		case PORT_A:
			if (ppi->port_a_read != NULL) {
				return ppi->port_a_read(ppi);
			}
			break;

		case PORT_B:
			if (ppi->port_b_read != NULL) {
				return ppi->port_b_read(ppi);
			}
			break;

		case PORT_C:
			if (ppi->port_c_read != NULL) {
				return ppi->port_c_read(ppi);
			}
			break;
	}
	return 0;
}

void i8255_ppi_write_io_byte(I8255_PPI* ppi, uint8_t io_address, uint8_t value) {
	switch (io_address) {
		case PORT_A:
			if (ppi->port_a_write != NULL) {
				ppi->port_a_write(ppi, value);
			}
			ppi->port_a = value;
			break;

		case PORT_B:
			if (ppi->port_b_write != NULL) {
				ppi->port_b_write(ppi, value);
			}
			ppi->port_b = value;
			break;

		case PORT_C:
			if (ppi->port_c_write != NULL) {
				ppi->port_c_write(ppi, value);
			}
			ppi->port_c = value;
			break;

		case PORT_CTRL:
			ppi->control = value;
			break;
	}
}
