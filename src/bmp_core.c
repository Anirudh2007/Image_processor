/*
 * bmp_core.c — Implementation of the BMP parsing and processing library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmp_core.h"

/* Compile-time struct size assertions. */
_Static_assert(sizeof(BMPFileHeader) == 14,
               "BMPFileHeader must be exactly 14 bytes — check #pragma pack");
_Static_assert(sizeof(BMPInfoHeader) == 40,
               "BMPInfoHeader must be exactly 40 bytes — check #pragma pack");

#define BMP_SIGNATURE 0x4D42


int parse_bmp_headers(const char *filepath,
                      BMPFileHeader *file_hdr,
                      BMPInfoHeader *info_hdr)
{
    FILE *fp = NULL;

    if (!filepath || !file_hdr || !info_hdr) {
        fprintf(stderr, "Error: null argument passed to parse_bmp_headers.\n");
        return -1;
    }

    fp = fopen(filepath, "rb");
    if (!fp) {
        perror(filepath);
        return -1;
    }

    if (fread(file_hdr, sizeof(BMPFileHeader), 1, fp) != 1) {
        fprintf(stderr, "Error: failed to read BMP file header from '%s'.\n",
                filepath);
        fclose(fp);
        return -1;
    }

    if (file_hdr->signature != BMP_SIGNATURE) {
        fprintf(stderr,
                "Error: '%s' is not a valid BMP file (bad signature: 0x%04X).\n",
                filepath, file_hdr->signature);
        fclose(fp);
        return -1;
    }

    if (fread(info_hdr, sizeof(BMPInfoHeader), 1, fp) != 1) {
        fprintf(stderr, "Error: failed to read BMP info header from '%s'.\n",
                filepath);
        fclose(fp);
        return -1;
    }


    fclose(fp);
    return 0; /* Success */
}


void print_bmp_metadata(const BMPFileHeader *file_hdr,
                        const BMPInfoHeader *info_hdr)
{
    if (!file_hdr || !info_hdr) {
        fprintf(stderr, "Error: null pointer passed to print_bmp_metadata.\n");
        return;
    }

    /* Cast file_size to double before dividing to get a floating-point result. */
    double size_kb = (double)file_hdr->file_size / 1024.0;

    printf("\n");
    printf("  +-------------------------------------------------+\n");
    printf("  |             BMP Metadata Report                 |\n");
    printf("  +-------------------------------------------------+\n");
    printf("  | %-24s | %-20s |\n", "Property",          "Value");
    printf("  +--------------------------+---------------------+\n");
    printf("  | %-24s | %-20s |\n", "Signature",         "BM (0x4D42)");
    printf("  | %-24s | %-17.2f KB |\n", "File Size",    size_kb);
    printf("  | %-24s | %-20u |\n", "Pixel Data Offset",
           file_hdr->pixel_offset);
    printf("  | %-24s | %-20u |\n", "DIB Header Size",
           info_hdr->header_size);
    printf("  | %-24s | %-20d |\n", "Width (px)",        info_hdr->width);
    printf("  | %-24s | %-20d |\n", "Height (px)",
           /* Negative height means top-down; print absolute value with a note */
           info_hdr->height < 0 ? -info_hdr->height : info_hdr->height);
    printf("  | %-24s | %-20s |\n", "Row Order",
           info_hdr->height < 0 ? "Top-Down" : "Bottom-Up (standard)");
    printf("  | %-24s | %-20u |\n", "Bits Per Pixel",
           info_hdr->bits_per_pixel);
    printf("  | %-24s | %-20u |\n", "Compression",
           info_hdr->compression);
    printf("  | %-24s | %-20u |\n", "Raw Image Size (bytes)",
           info_hdr->image_size);
    printf("  | %-24s | %-20u |\n", "Colors in Table",
           info_hdr->colors_in_table);
    printf("  +--------------------------+---------------------+\n\n");
}
