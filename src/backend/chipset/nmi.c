/* nmi.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Non Maskable Interrupt Logic Controller
 */

#include <stdint.h>

#include "nmi.h"

uint8_t nmi_read_io_byte(NMI* nmi, uint8_t io_address) {
	(void)io_address;
	return nmi->status;
}
void nmi_write_io_byte(NMI* nmi, uint8_t io_address, uint8_t value) {
	(void)io_address;
	nmi->status = value;
}
