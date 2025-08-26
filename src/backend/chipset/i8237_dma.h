/* i8237_dma.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * PROGRAMMABLE DMA CONTROLLER
 */

#ifndef I8237_DMA_H
#define I8237_DMA_H

#include <stdint.h>

#define DMA_NUM_REG 16

typedef struct I8237_DMA {
	uint8_t reg[DMA_NUM_REG];
} I8237_DMA;

void i8237_dma_reset(I8237_DMA* dma);
uint8_t i8237_dma_read_io_byte(I8237_DMA* dma, uint8_t io_address);
void i8237_dma_write_io_byte(I8237_DMA* dma, uint8_t io_address, uint8_t value);

#endif
