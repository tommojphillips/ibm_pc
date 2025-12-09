/* i8237_dma.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * PROGRAMMABLE DMA CONTROLLER
 */

#ifndef I8237_DMA_H
#define I8237_DMA_H

#include <stdint.h>

#define DMA_CHANNEL_COUNT 4

typedef struct I8237_DMA_CHANNEL {
	uint16_t latched_address;
	uint16_t current_address;
	uint16_t current_word_count;
	uint16_t latched_word_count;
	uint8_t mode;
	uint8_t terminal_count;
	uint8_t request;
	uint8_t masked;
	uint8_t page;
} I8237_DMA_CHANNEL;

typedef struct I8237_DMA {
	uint8_t command;
	uint8_t request;
	uint8_t status;
	uint8_t temp;
	uint8_t flipflop; /* 1 = HI byte; 0 = LO byte */
	I8237_DMA_CHANNEL channels[DMA_CHANNEL_COUNT];

	uint8_t(*read_mem_byte)(uint32_t);         // read mem byte
	void(*write_mem_byte)(uint32_t, uint8_t);  // write mem byte
} I8237_DMA;

void i8237_dma_init(I8237_DMA* dma, uint8_t(*read_mem_byte)(uint32_t), void(*write_mem_byte)(uint32_t, uint8_t));
void i8237_dma_reset(I8237_DMA* dma);
void i8237_dma_update(I8237_DMA* dma);

uint8_t i8237_dma_read_io_byte(I8237_DMA* dma, uint8_t io_address);
void i8237_dma_write_io_byte(I8237_DMA* dma, uint8_t io_address, uint8_t value);

uint32_t i8237_dma_get_transfer_address(I8237_DMA* dma, uint8_t channel);
uint32_t i8237_dma_get_transfer_size(I8237_DMA* dma, uint8_t channel);

void i8237_dma_write_byte(I8237_DMA* dma, uint8_t channel, uint8_t value);
uint8_t i8237_dma_read_byte(I8237_DMA* dma, uint8_t channel);

uint8_t i8237_dma_channel_ready(I8237_DMA* dma, uint8_t channel);
uint8_t i8237_dma_terminal_count(I8237_DMA* dma, uint8_t channel);

void i8237_dma_request_service(I8237_DMA* dma, uint8_t channel);
void i8237_dma_clear_service(I8237_DMA* dma, uint8_t channel);

#endif
