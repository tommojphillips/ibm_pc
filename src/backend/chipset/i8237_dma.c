/* i8237_dma.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * PROGRAMMABLE DMA CONTROLLER
 */

#include <stdint.h>

#include "i8237_dma.h"
#include "backend/utility/bit_utils.h"

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

#define PORT_CHANNEL0_ADDRESS     0x00 /* RW */
#define PORT_CHANNEL1_ADDRESS     0x02 /* RW */
#define PORT_CHANNEL2_ADDRESS     0x04 /* RW */
#define PORT_CHANNEL3_ADDRESS     0x06 /* RW */

#define PORT_CHANNEL0_WC          0x01 /* RW */
#define PORT_CHANNEL1_WC          0x03 /* RW */
#define PORT_CHANNEL2_WC          0x05 /* RW */
#define PORT_CHANNEL3_WC          0x07 /* RW */

#define PORT_CHANNEL0_PAGE        0x87 /* RW */
#define PORT_CHANNEL1_PAGE        0x83 /* RW */
#define PORT_CHANNEL2_PAGE        0x81 /* RW */
#define PORT_CHANNEL3_PAGE        0x82 /* RW */

#define PORT_READ_STATUS_REGISTER  0x08 /* RO */
#define PORT_COMMAND_REGISTER      0x08 /* WO */
#define PORT_WRITE_REQ             0x09 /* WO */
#define PORT_CHANNEL_MASK_REGISTER 0x0A /* RW */
#define PORT_CHANNEL_MODE_REGISTER 0x0B /* RW */
#define PORT_CLEAR_FLIPFLOP        0x0C /* WO */

#define PORT_READ_TEMP_REGISTER    0x0D /* RO */
#define PORT_CLEAR_MASTER_REGISTER 0x0D /* WO */
#define PORT_CLEAR_MASK_REGISTER   0x0E /* WO */
#define PORT_WRITE_MASK_REGISTER   0x0F /* WO */

#define COMMAND_MEM_TO_MEM         0x01
#define COMMAND_CHANNEL0_HOLD      0x02
#define COMMAND_DISABLE            0x04
#define COMMAND_TIMING             0x08
#define COMMAND_PRIORITY           0x10

#define MODE_TRANSFER_TYPE         0x0C
#define MODE_AUTO_INIT             0x10
#define MODE_ADDRESS_MODE          0x20
#define MODE_SERVICE_MODE          0xC0

#define TRANSFER_TYPE_VERFIY       0x00
#define TRANSFER_TYPE_WRITE        0x04
#define TRANSFER_TYPE_READ         0x08
#define TRANSFER_TYPE_ILLEGAL      0x0C

#define AUTO_INIT_OFF              0x00
#define AUTO_INIT_ON               0x10

#define ADDRESS_MODE_INC           0x00
#define ADDRESS_MODE_DEC           0x20

#define SERVICE_MODE_DEMAND        0x00
#define SERVICE_MODE_SINGLE        0x40
#define SERVICE_MODE_BLOCK         0x80
#define SERVICE_MODE_CASCADE       0xC0

uint8_t address_read(I8237_DMA* dma, uint8_t channel) {
	uint8_t address = 0;
	if (dma->flipflop) {
		address = (dma->channels[channel].current_address >> 8) & 0xFF;
	}
	else {
		address = dma->channels[channel].current_address & 0xFF;
	}

	dma->flipflop = !dma->flipflop;

	return address;
}

uint8_t wc_read(I8237_DMA* dma, uint8_t channel) {
	uint8_t word_count = 0;
	if (dma->flipflop) {
		word_count = (dma->channels[channel].current_word_count >> 8) & 0xFF;
	}
	else {
		word_count = dma->channels[channel].current_word_count & 0xFF;
	}

	dma->flipflop = !dma->flipflop;

	return word_count;
}

uint8_t page_read(I8237_DMA* dma, uint8_t channel) {
	return dma->channels[channel].page;
}

uint8_t status_read(I8237_DMA* dma) {
	return dma->status;
}

uint8_t temp_read(I8237_DMA* dma) {
	return dma->temp;
}

void address_write(I8237_DMA* dma, uint8_t channel, uint8_t value) {
	if (dma->flipflop) {
		dma->channels[channel].latched_address = (dma->channels[channel].latched_address & 0xFF) | ((uint16_t)value << 8);
	}
	else {
		dma->channels[channel].latched_address = (dma->channels[channel].latched_address & 0xFF00) | value;
	}

	dma->channels[channel].current_address = dma->channels[channel].latched_address;

	dma->flipflop = !dma->flipflop;
}

void wc_write(I8237_DMA* dma, uint8_t channel, uint8_t value) {
	if (dma->flipflop) {
		dma->channels[channel].latched_word_count = (dma->channels[channel].latched_word_count & 0xFF) | ((uint16_t)value << 8);
	}
	else {
		dma->channels[channel].latched_word_count = (dma->channels[channel].latched_word_count & 0xFF00) | value;
	}

	dma->channels[channel].current_word_count = dma->channels[channel].latched_word_count;

	dma->flipflop = !dma->flipflop;
}

void page_write(I8237_DMA* dma, uint8_t channel, uint8_t value) {
	dma->channels[channel].page = value;
}

void command_write(I8237_DMA* dma, uint8_t value) {

#ifdef DBG_PRINT
	if (IS_RISING_EDGE(COMMAND_DISABLE, dma->command, value)) {
		dbg_print("[DMA] DMA Disabled\n");
	}
	else if (IS_FALLING_EDGE(COMMAND_DISABLE, dma->command, value)) {
		dbg_print("[DMA] DMA Enabled\n");
	}
#endif

	/* Mem to Mem Command not implemented */
	/* Channel 0 Hold Command not implemented */
	/* Timing Command not implemented */
	/* Priority Command not implemented */

	dma->command = value;
}

void mask_write(I8237_DMA* dma, uint8_t value) {
	for (int i = 0; i < DMA_CHANNEL_COUNT; ++i) {
		dma->channels[i].masked = value & 0x01;
		value >>= 1;
	}
}

void mask_clear(I8237_DMA* dma) {
	for (int i = 0; i < DMA_CHANNEL_COUNT; ++i) {
		dma->channels[i].masked = 0;
	}
}

void channel_mask_write(I8237_DMA* dma, uint8_t value) {
	dma->channels[value & 0x03].masked = (value >> 0x02) & 0x01;
}

void channel_mode_write(I8237_DMA* dma, uint8_t value) {
	dma->channels[value & 0x03].mode = value;
	dma->channels[value & 0x03].terminal_count = 0;
}

void flipflop_clear(I8237_DMA* dma) {
	dma->flipflop = 0;
}

void master_clear(I8237_DMA* dma) {
	for (int i = 0; i < DMA_CHANNEL_COUNT; ++i) {
		dma->channels[i].masked = 1;
	}
	dma->command = 0;
	dma->status = 0;
	dma->temp = 0;
	dma->flipflop = 0;
}

void req_write(I8237_DMA* dma, uint8_t value) {
	(void)dma;
	(void)value;
	dbg_print("[DMA] REQ WRITE not implemented\n");
}

void i8237_dma_reset(I8237_DMA* dma) {
	for (int i = 0; i < DMA_CHANNEL_COUNT; ++i) {
		dma->channels[i].latched_address = 0;
		dma->channels[i].current_address = 0;
		dma->channels[i].latched_word_count = 0;
		dma->channels[i].current_word_count = 0;
		dma->channels[i].masked = 0;
		dma->channels[i].mode = 0;
		dma->channels[i].page = 0;
		dma->channels[i].request = 0;
		dma->channels[i].terminal_count = 0;
		dma->channels[i].terminal_count_reached = 0;
	}

	dma->command = 0;
	dma->request = 0;
	dma->status = 0;
	dma->temp = 0;
}
uint8_t i8237_dma_read_io_byte(I8237_DMA* dma, uint8_t io_address) {
	switch (io_address) {
		case PORT_CHANNEL0_ADDRESS:
			return address_read(dma, 0);
		case PORT_CHANNEL1_ADDRESS:
			return address_read(dma, 1);
		case PORT_CHANNEL2_ADDRESS:
			return address_read(dma, 2);
		case PORT_CHANNEL3_ADDRESS:
			return address_read(dma, 3);
		
		case PORT_CHANNEL0_WC:
			return wc_read(dma, 0);
		case PORT_CHANNEL1_WC:
			return wc_read(dma, 1);
		case PORT_CHANNEL2_WC:
			return wc_read(dma, 2);
		case PORT_CHANNEL3_WC:
			return wc_read(dma, 3);
		
		case PORT_CHANNEL0_PAGE:
			return page_read(dma, 0);
		case PORT_CHANNEL1_PAGE:
			return page_read(dma, 1);
		case PORT_CHANNEL2_PAGE:
			return page_read(dma, 2);
		case PORT_CHANNEL3_PAGE:
			return page_read(dma, 3);

		case PORT_READ_STATUS_REGISTER:
			return status_read(dma);
		case PORT_READ_TEMP_REGISTER:
			return status_read(dma);
		
		default:
			dbg_print("[DMA] reading unimplemented IO\n");
			return 0;
	}
}
void i8237_dma_write_io_byte(I8237_DMA* dma, uint8_t io_address, uint8_t value) {	
	switch (io_address) {
		case PORT_CHANNEL0_ADDRESS:
			address_write(dma, 0, value);
			break;
		case PORT_CHANNEL1_ADDRESS:
			address_write(dma, 1, value);
			break;
		case PORT_CHANNEL2_ADDRESS:
			address_write(dma, 2, value);
			break;
		case PORT_CHANNEL3_ADDRESS:
			address_write(dma, 3, value);
			break;
		
		case PORT_CHANNEL0_WC:
			wc_write(dma, 0, value);
			break;
		case PORT_CHANNEL1_WC:
			wc_write(dma, 1, value);
			break;
		case PORT_CHANNEL2_WC:
			wc_write(dma, 2, value);
			break;
		case PORT_CHANNEL3_WC:
			wc_write(dma, 3, value);
			break;

		case PORT_CHANNEL0_PAGE:
			page_write(dma, 0, value);
			break;
		case PORT_CHANNEL1_PAGE:
			page_write(dma, 1, value);
			break;
		case PORT_CHANNEL2_PAGE:
			page_write(dma, 2, value);
			break;
		case PORT_CHANNEL3_PAGE:
			page_write(dma, 3, value);
			break;
		
		case PORT_COMMAND_REGISTER:
			command_write(dma, value);
			break;
		
		case PORT_WRITE_REQ:
			req_write(dma, value);
			break;
				
		case PORT_CHANNEL_MASK_REGISTER:
			channel_mask_write(dma, value);
			break;
				
		case PORT_CHANNEL_MODE_REGISTER:
			channel_mode_write(dma, value);
			break;
		
		case PORT_CLEAR_FLIPFLOP:
			flipflop_clear(dma);
			break;
		
		case PORT_CLEAR_MASTER_REGISTER:
			master_clear(dma);
			break;
		
		case PORT_CLEAR_MASK_REGISTER:
			mask_clear(dma);
			break;
		
		case PORT_WRITE_MASK_REGISTER:
			mask_write(dma, value);
			break;

		default:
			dbg_print("[DMA] writing unimplemented IO\n");
			break;
	}
}

void i8237_dma_init(I8237_DMA* dma) {
	dma->read_mem_byte = NULL;
	dma->write_mem_byte = NULL;
}

void i8237_dma_update(I8237_DMA* dma) {
	(void)dma;
	/* Nothing to do */
}

uint32_t i8237_dma_get_transfer_address(I8237_DMA* dma, uint8_t channel) {
	return ((uint32_t)dma->channels[channel].page << 16) + dma->channels[channel].current_address;
}

uint32_t i8237_dma_get_transfer_size(I8237_DMA* dma, uint8_t channel) {
	return (uint32_t)dma->channels[channel].current_word_count + 1;
}

void i8237_dma_write_byte(I8237_DMA* dma, uint8_t channel, uint8_t value) {

	uint32_t transfer_address = i8237_dma_get_transfer_address(dma, channel);
	
	switch (dma->channels[channel].mode & MODE_ADDRESS_MODE) {
		case ADDRESS_MODE_INC:
			if (dma->channels[channel].current_word_count > 0) {

				if ((dma->channels[channel].mode & MODE_TRANSFER_TYPE) == TRANSFER_TYPE_WRITE) {
					dma->write_mem_byte(transfer_address, value);
				}

				dma->channels[channel].current_address += 1;
				dma->channels[channel].current_word_count -= 1;
			}
			else if (dma->channels[channel].terminal_count == 0) {

				if ((dma->channels[channel].mode & MODE_TRANSFER_TYPE) == TRANSFER_TYPE_WRITE) {
					dma->write_mem_byte(transfer_address, value);
				}

				if ((dma->channels[channel].mode & MODE_AUTO_INIT) == AUTO_INIT_ON) {
					/* Reload */
					dma->channels[channel].current_address = dma->channels[channel].latched_address;
					dma->channels[channel].current_word_count = dma->channels[channel].latched_word_count;
				}
				else {
					dma->channels[channel].terminal_count = 1;
				}

				/* Set TC */
				dma->channels[channel].terminal_count_reached = 1;
			}
			else {
				/* tried to write on terminal count */
			}
			break;
		
		case ADDRESS_MODE_DEC:
			/* Not Implemented */
			dbg_print("[DMA] DEC ADDRESS MODE not implemented\n");
			break;
	}
}
uint8_t i8237_dma_read_byte(I8237_DMA* dma, uint8_t channel) {

	if (dma->command & COMMAND_DISABLE) {
		return 0;
	}

	uint32_t transfer_address = i8237_dma_get_transfer_address(dma, channel);
	uint8_t data = 0;

	switch (dma->channels[channel].mode & MODE_ADDRESS_MODE) {
		case ADDRESS_MODE_INC:
			if (dma->channels[channel].current_word_count > 0) {
				data = dma->read_mem_byte(transfer_address);

				dma->channels[channel].current_address += 1;
				dma->channels[channel].current_word_count -= 1;
			}
			else if (dma->channels[channel].terminal_count == 0) {
				data = dma->read_mem_byte(transfer_address);

				if ((dma->channels[channel].mode & MODE_AUTO_INIT) == AUTO_INIT_ON) {
					/* Reload */
					dma->channels[channel].current_address = dma->channels[channel].latched_address;
					dma->channels[channel].current_word_count = dma->channels[channel].latched_word_count;
				}
				else {
					dma->channels[channel].terminal_count = 1;
				}

				/* Set TC */
				dma->channels[channel].terminal_count_reached = 1;
			}
			else {
				/* tried to read on terminal count */
			}
			break;
		
		case ADDRESS_MODE_DEC:
			/* Not Implemented */
			dbg_print("[DMA] DEC ADDRESS MODE not implemented\n");
			break;
	}

	return data;
}

uint8_t i8237_dma_channel_ready(I8237_DMA* dma, uint8_t channel) {
	return !dma->channels[channel].masked;
}

uint8_t i8237_dma_terminal_count(I8237_DMA* dma, uint8_t channel) {
	return dma->channels[channel].terminal_count;
}
