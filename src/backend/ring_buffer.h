/* ring_buffer.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Implements A ring buffer
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

typedef struct RING_BUFFER {
	uint8_t* buffer;
	int buffer_size;
	int head;
	int tail;
	int count;
} RING_BUFFER;

int ring_buffer_init(RING_BUFFER* rb, int buffer_size);
void ring_buffer_destroy(RING_BUFFER* rb);
void ring_buffer_reset(RING_BUFFER* rb);

void ring_buffer_push(RING_BUFFER* rb, uint8_t item);
uint8_t ring_buffer_pop(RING_BUFFER* rb);

#endif
