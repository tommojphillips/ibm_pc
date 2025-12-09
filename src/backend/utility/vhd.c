/* vhd.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * 
 * https://download.microsoft.com/download/f/f/e/ffef50a5-07dd-4cf8-aaa3-442c0673a029/Virtual%20Hard%20Disk%20Format%20Spec_10_18_06.doc
*/

#include <stdint.h>
#include <malloc.h>

#include "vhd.h"
#include "lba.h"

#define VHD_FEATURE_NONE      0x00000000 /* The hard disk image has no special features enabled in it. */
#define VHD_FEATURE_TEMPORARY 0x00000001 /* A temporary disk designation indicates to an application that this disk is a candidate for deletion on shutdown */
#define VHD_FEATURE_RESERVED  0x00000002 /* This bit must always be set to 1. All other bits are also reserved and should be set to 0. */

#define VHD_HOST_OS_WINDOWS   0x5769326B /* 'Wi2K' */ 
#define VHD_HOST_OS_MACINTOSH 0x4D616320 /* 'MAC ' */

#define VHD_DISK_TYPE_NONE             0x00000000
#define VHD_DISK_TYPE_REV0             0x00000001
#define VHD_DISK_TYPE_FIXED_HDD        0x00000002
#define VHD_DISK_TYPE_DYNAMIC_HDD      0x00000003
#define VHD_DISK_TYPE_DIFFERENCING_HDD 0x00000004
#define VHD_DISK_TYPE_REV1             0x00000005
#define VHD_DISK_TYPE_REV2             0x00000006

#define VHD_COOKIE      0x636F6E6563746978 /* 'conectix' */

#define VHD_FORMAT_VER  0x00010000 /* 1.0 */

#define VHD_CREATOR_APP 0x544F4D4F /* 'TOMO' */
#define VHD_CREATOR_VER 0x00010000 /* 1.0 */

typedef struct VHD_FOOTER {
    uint64_t cookie;              /* Used to uniquely identify the original creator of the hard disk image. */
    uint32_t features;            /* Used to indicate specific feature support. */
    uint32_t file_format_version; /* Must be initialized to 0x00010000. */
    uint64_t data_offset;         /* For fixed disks, this field should be set to 0xFFFFFFFFFFFFFFFF. */
    uint32_t timestamp;
    uint32_t creator_application; /* Used to document which application created the hard disk. */
    uint32_t creator_version;     /* Stores the major/minor version of the application that created the hard disk image.*/
    uint32_t creator_host_os;     /* Stores the type of host operating system this disk image is created on. */
    uint64_t orginal_size;        /* Stores the size of the hard disk in bytes. */
    uint64_t current_size;        /* Stores the current size of the hard disk, in bytes. */
    CHS disk_geometry;            /* Stores the cylinder, heads, and sectors per track value for the hard disk. */
    uint32_t disk_type;           /* */
    uint32_t checksum;            /* Stores a checksum of the hard disk footer. */
    char unique_id[16];           /* Used to identify the hard disk. */
    uint8_t saved_state;          /* Stores a flag that describes whether the system is in saved state. If the hard disk is in the saved state the value is set to 1. */
    char reserved[427];           /* Reserved. Contains all zeroes. */
} VHD_FOOTER;

static_assert(sizeof(VHD_FOOTER) == 512, "VHD_FOOTER not 512 bytes");

static uint8_t byte_swap8(uint8_t v) {
    return v;
}

static uint16_t byte_swap16(uint16_t v) {
    return (v << 8) | (v >> 8);
}

static uint32_t byte_swap32(uint32_t v) {
    v = ((v << 8) & 0xFF00FF00) | ((v >> 8) & 0x00FF00FF);
    return (v << 16) | (v >> 16);
}

static uint64_t byte_swap64(uint64_t v) {
    v = ((v << 8) & 0xFF00FF00FF00FF00) | ((v >> 8) & 0x00FF00FF00FF00FF);
    v = ((v << 16) & 0xFFFF0000FFFF0000) | ((v >> 16) & 0x0000FFFF0000FFFF);
    return (v << 32) | (v >> 32);
}

static uint32_t calculate_checksum(const VHD_FOOTER* footer) {
    uint32_t checksum = 0;
    for (int i = 0; i < sizeof(VHD_FOOTER); ++i) {
        if (i >= 0x40 && i < 0x44) {
            continue; /* ignore checksum bits */
        }
        checksum += ((uint8_t*)footer)[i];
    }
    return ~checksum;
}

int vhd_create(CHS geometry, uint8_t** buffer, size_t* buffer_size) {

    size_t total_sectors = (size_t)geometry.c * geometry.h * geometry.s;
    size_t total_bytes = total_sectors * 512;
    size_t total_size = total_bytes + sizeof(VHD_FOOTER);

    uint8_t* vhd = calloc(1, total_size);
    if (vhd == NULL) {
        return 1;
    }

    VHD_FOOTER* footer = (VHD_FOOTER*)(vhd + total_bytes);

    /* VHD Footer is written in big-endian */
    footer->cookie = byte_swap64(VHD_COOKIE);                   // 'conectix'
    footer->features = byte_swap32(VHD_FEATURE_RESERVED);
    footer->file_format_version = byte_swap32(VHD_FORMAT_VER);  // 1.0
    footer->data_offset = byte_swap64(0xFFFFFFFFFFFFFFFF);
    footer->timestamp = byte_swap32(0);
    footer->creator_application = byte_swap32(VHD_CREATOR_APP); // 'TOMO'
    footer->creator_version = byte_swap32(VHD_CREATOR_VER);     // 1.0
    footer->creator_host_os = byte_swap32(VHD_HOST_OS_WINDOWS); // 'Wi2K'
    footer->orginal_size = byte_swap64(total_bytes);
    footer->current_size = byte_swap64(total_bytes);
    footer->disk_geometry.c = byte_swap16(geometry.c);
    footer->disk_geometry.h = byte_swap8(geometry.h);
    footer->disk_geometry.s = byte_swap8(geometry.s);
    footer->disk_type = byte_swap32(VHD_DISK_TYPE_FIXED_HDD);
    footer->saved_state = byte_swap8(0);
    footer->checksum = byte_swap32(calculate_checksum(footer));

    *buffer = vhd;
    *buffer_size = total_size;
    return 0;
}

void vhd_destroy(uint8_t* buffer) {
    if (buffer != NULL) {
        free(buffer);
    }
}

int vhd_verify(uint8_t* buffer, size_t buffer_size) {

    if (buffer_size < sizeof(VHD_FOOTER)) {
        return 1;
    }

    VHD_FOOTER* footer = (VHD_FOOTER*)(buffer + buffer_size - sizeof(VHD_FOOTER));

    if (footer->checksum != byte_swap32(calculate_checksum(footer))) {
        return 1;
    }

    if (footer->cookie != byte_swap64(VHD_COOKIE)) {
        return 1;
    }

    if (footer->disk_type != byte_swap32(VHD_DISK_TYPE_FIXED_HDD)) {
        return 1;
    }

    if (footer->data_offset != byte_swap64(0xFFFFFFFFFFFFFFFF)) {
        return 1;
    }

    size_t total_sectors = (size_t)byte_swap16(footer->disk_geometry.c) * byte_swap8(footer->disk_geometry.h) * byte_swap8(footer->disk_geometry.s);
    size_t total_bytes = total_sectors * 512;
    size_t total_size = total_bytes + sizeof(VHD_FOOTER);

    if (buffer_size != total_size) {
        return 1;
    }

    return 0;
}
CHS vhd_get_geometry(uint8_t* buffer, size_t buffer_size) {
    CHS chs = { 0 };
    if (buffer_size < sizeof(VHD_FOOTER)) {
        return chs;
    }

    VHD_FOOTER* footer = (VHD_FOOTER*)(buffer + buffer_size - sizeof(VHD_FOOTER));

    chs.c = byte_swap16(footer->disk_geometry.c);
    chs.h = byte_swap8(footer->disk_geometry.h);
    chs.s = byte_swap8(footer->disk_geometry.s);
    return chs;
}
size_t vhd_get_file_size(uint8_t* buffer, size_t buffer_size) {
    if (buffer_size < sizeof(VHD_FOOTER)) {
        return 0;
    }

    VHD_FOOTER* footer = (VHD_FOOTER*)(buffer + buffer_size - sizeof(VHD_FOOTER));

    return byte_swap64(footer->current_size);
}
