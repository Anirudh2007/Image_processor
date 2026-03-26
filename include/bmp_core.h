#ifndef BMP_CORE_H
#define BMP_CORE_H

/*
 * bmp_core.h — BMP parser library interface.
 */
#include <stdint.h>

#pragma pack(push, 1)

typedef struct {
    uint16_t signature;
    uint32_t file_size;     /* Total size of the file in bytes. */
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t pixel_offset;
} BMPFileHeader;

typedef struct {
    uint32_t header_size;
    int32_t  width;
    int32_t  height;
    uint16_t color_planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t  x_pixels_per_m;
    int32_t  y_pixels_per_m;
    uint32_t colors_in_table;
    uint32_t important_colors;
} BMPInfoHeader;

#pragma pack(pop)


int parse_bmp_headers(const char *filepath,
                      BMPFileHeader *file_hdr,
                      BMPInfoHeader *info_hdr);

void print_bmp_metadata(const BMPFileHeader *file_hdr,
                        const BMPInfoHeader *info_hdr);

#endif /* BMP_CORE_H */
