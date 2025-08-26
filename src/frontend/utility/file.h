/* file.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 */

#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include <stdint.h>

#include <stdio.h>
#define file_tell  ftell
#define file_open  fopen_s
#define file_close fclose
#define file_seek  fseek
#define file_read  fread
#define file_write fwrite
#define file_t     FILE

int file_read_into_buffer(const char* filename, void* buff, const size_t buff_size, const size_t offset, size_t* file_size, const size_t expected_size);

#endif