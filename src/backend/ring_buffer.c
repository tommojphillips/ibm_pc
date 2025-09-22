/* ring_buffer.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Implements A ring buffer
 */

#include <stdint.h>
#include <malloc.h>

#include "ring_buffer.h"

void ring_buffer_reset(RING_BUFFER* rb) {
	if (rb != NULL) {		
		rb->count = 0;
		rb->head = 0;
		rb->tail = 0;
	}
}

int ring_buffer_init(RING_BUFFER* rb, int buffer_size) {
	if (rb != NULL) {

		rb->buffer = (uint8_t*)calloc(1, buffer_size);
		if (rb->buffer == NULL) {
			return 1;
		}

		rb->buffer_size = buffer_size;
		ring_buffer_reset(rb);
		return 0;
	}
	return 1;
}

void ring_buffer_destroy(RING_BUFFER* rb) {
	if (rb != NULL) {

		if (rb->buffer != NULL) {
			free(rb->buffer);
			rb->buffer = NULL;
		}

		rb->buffer_size = 0; 
		ring_buffer_reset(rb);
	}
}
void ring_buffer_push(RING_BUFFER* rb, uint8_t item) {
	rb->buffer[rb->tail] = item;
	rb->tail = (rb->tail + 1) % rb->buffer_size;
	if (rb->count < rb->buffer_size) {
		rb->count++;
	}
	else {
		/* We have overrun the head; inc the head so pop reads the oldest values first. */
		rb->head = (rb->head + 1) % rb->buffer_size;
	}
}
uint8_t ring_buffer_pop(RING_BUFFER* rb) {
	if (rb->count == 0) {
		return 0;
	}
	uint8_t item = rb->buffer[rb->head];
	rb->head = (rb->head + 1) % rb->buffer_size;
	rb->count--;
	return item;
}
int ring_buffer_peek(RING_BUFFER* rb, int head_offset, uint8_t* out) {
	if (head_offset < rb->count) {
		if (out != NULL) {
			*out = rb->buffer[(rb->head + head_offset) % rb->buffer_size];
		}
		return 0;
	}
	return 1;
}
