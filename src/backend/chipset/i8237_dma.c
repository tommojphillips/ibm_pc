/* i8237_dma.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * PROGRAMMABLE DMA CONTROLLER
 */

#include <stdint.h>

#include "i8237_dma.h"

void i8237_dma_reset(I8237_DMA* dma) {
	for (int i = 0; i < DMA_NUM_REG; ++i) {
		dma->reg[i] = 0;
	}
}
uint8_t i8237_dma_read_io_byte(I8237_DMA* dma, uint8_t io_address) {
	return dma->reg[io_address & 0xF];
}
void i8237_dma_write_io_byte(I8237_DMA* dma, uint8_t io_address, uint8_t value) {
	dma->reg[io_address & 0xF] = value;
}
