/* nmi.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Non Maskable Interrupt Logic Controller
 */

#ifndef NMI_H
#define NMI_H

#define NMI_ENABLE_INTERRUPTS 0x80

typedef struct NMI {
	uint8_t status;
} NMI;

uint8_t nmi_read_io_byte(NMI* nmi, uint8_t io_address);
void nmi_write_io_byte(NMI* nmi, uint8_t io_address, uint8_t value);

#endif
