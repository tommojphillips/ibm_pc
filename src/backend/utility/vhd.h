/* vhd.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
*/

#include <stdint.h>

typedef struct CHS CHS;

int vhd_create(CHS geometry, uint8_t** buffer, size_t* buffer_size);
void vhd_destroy(uint8_t* buffer);
int vhd_verify(uint8_t* buffer, size_t buffer_size);
CHS vhd_get_geometry(uint8_t* buffer, size_t buffer_size);
size_t vhd_get_file_size(uint8_t* buffer, size_t buffer_size);
