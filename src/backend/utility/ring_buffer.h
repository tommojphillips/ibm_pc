/* ring_buffer.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Implements A ring buffer
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

/* Ring Buffer Struct */
typedef struct RING_BUFFER {
	uint8_t* buffer;
	int buffer_size;
	int head;
	int tail;
	int count;
} RING_BUFFER;

/* Init the buffer; Allocs the buffer of <buffer_size>
	rb: The ring buffer struct
	buffer_size: the buffer size
	Returns: 0 if success. Otherwise returns 1. (malloc error, rb NULL) */
int ring_buffer_create(RING_BUFFER* rb, int buffer_size);

/* Destroy the buffer 
	rb: The ring buffer struct */
void ring_buffer_destroy(RING_BUFFER* rb);

/* Reset the buffer 
	rb: The ring buffer struct */
void ring_buffer_reset(RING_BUFFER* rb);

/* Push a value to the tail
	If the tail overruns the head; increments the head so pop reads the oldest values first.
	rb: The ring buffer struct
	item: the value to push. */
void ring_buffer_push(RING_BUFFER* rb, uint8_t item);

/* Pop the value off of the head 
	rb: The ring buffer struct 
	Returns: the value popped. */
uint8_t ring_buffer_pop(RING_BUFFER* rb);

/* Peek at the value at head + offset 
	rb: The ring buffer struct 
	head_offset: the offset from the head to peek at. 
	out: the value at the offset. Can be NULL ie; Dont return the value.
	Returns: 0 if the offset is in the vaild range of the queue. (head_offset < queue_count). Otherwise returns 1. */
int ring_buffer_peek(RING_BUFFER* rb, int head_offset, uint8_t* out);

/* Is the ring buffer empty
	rb: The ring buffer struct 
	Returns: 0 if the ring buffer has any bytes. Otherwise returns 1. */
int ring_buffer_is_empty(RING_BUFFER* rb);

/* Discard x amount of elements in the buffer.
	rb: The ring buffer struct
	amount: Amount of elements to discard from the head. 
	Returns: 0 if success. Otherwise returns 1. (rb NULL, amount < 0) */
int ring_buffer_discard(RING_BUFFER* rb, int amount);

#endif
