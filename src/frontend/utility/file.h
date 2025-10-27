/* file.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 */

#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include <stdint.h>

int file_read_into_buffer(const char* filename, void* buff, const size_t buff_size, const size_t offset, size_t* file_size, const size_t expected_size);
int file_read_alloc_buffer(const char* filename, void** buff, size_t* file_size);
const char* file_get_filename(const char* path);
#endif