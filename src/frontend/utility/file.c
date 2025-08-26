/* file.c 
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ ) 
 */
#include <stdint.h>

#include "file.h"

#define DBG_PRINT
#ifdef DBG_PRINT
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

int file_read_into_buffer(const char* filename, void* buff, const size_t buff_size, const size_t offset, size_t* file_size, const size_t expected_size) {
	file_t* file = NULL;
	size_t size = 0;
	if (filename == NULL)
		return 1;

	file_open(&file, filename, "rb");
	if (file == NULL) {
		dbg_print("Error: could not open file: %s\n", filename);
		return 1;
	}

	file_seek(file, 0, SEEK_END);
	size = file_tell(file);
	file_seek(file, 0, SEEK_SET);

	if (file_size != NULL) {
		*file_size = size;
	}

	if (expected_size != 0 && size != expected_size) {
		dbg_print("Error: invalid file size. Expected %zu bytes. Got %zu bytes\n", expected_size, size);
		file_close(file);
		return 1;
	}

	if (offset + size > buff_size) {
		dbg_print("Error: file is too big for buffer. Offset: %zx. File size: %zu bytes. Buffer size: %zu bytes\n", offset, size, buff_size);
		file_close(file);
		return 1;
	}

	size_t bytes_read = file_read((uint8_t*)buff + offset, 1, size, file);
	dbg_print("0x%05zX -> %s (%zu bytes)\n", offset, filename, bytes_read);
	file_close(file);
	return 0;
}
