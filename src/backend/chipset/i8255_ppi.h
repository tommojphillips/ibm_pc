/* i8255_ppi.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Intel 8255 Programmable Peripheral Interface (PPI)
 */

#ifndef I8255_PPI_H
#define I8255_PPI_H

#include <stdint.h>

typedef void(*port_write_func)(void* ppi, uint8_t value);
typedef uint8_t(*port_read_func)(void* ppi);

typedef struct I8255_PPI {
	uint8_t port_a;
	uint8_t port_b;
	uint8_t port_c;
	uint8_t control;

	port_read_func port_a_read;
	port_read_func port_b_read;
	port_read_func port_c_read;
	
	port_write_func port_a_write;
	port_write_func port_b_write;
	port_write_func port_c_write;
} I8255_PPI;

void i8255_ppi_reset(I8255_PPI* ppi);
uint8_t i8255_ppi_read_io_byte(I8255_PPI* ppi, uint8_t io_address);
void i8255_ppi_write_io_byte(I8255_PPI* ppi, uint8_t io_address, uint8_t value);

#endif
