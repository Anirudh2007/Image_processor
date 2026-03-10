#ifndef BMP_CORE_H
#define BMP_CORE_H

/*
 * bmp_core.h — Public interface for the BMP parser library.
 *
 * stdint.h provides fixed-width integer types (uint8_t, uint16_t, uint32_t,
 * int32_t, etc.). These are critical here because the BMP specification defines
 * fields by an exact number of bytes. Using 'int' or 'long' would be wrong:
 * their sizes are platform-dependent. uint16_t is ALWAYS 2 bytes, everywhere.
 */
#include <stdint.h>

/* ============================================================
 *  STRUCT PACKING — WHY THIS MATTERS FOR BINARY I/O
 * ============================================================
 * By default, the C compiler inserts invisible "padding" bytes between struct
 * fields to align them to natural CPU word boundaries (usually 4 or 8 bytes).
 * This makes field access faster, but it DESTROYS our ability to do a direct
 * fread() into the struct, because the struct layout in memory would no longer
 * match the layout on disk.
 *
 * Example without packing (hypothetical):
 *   struct { uint16_t a; uint32_t b; }
 *   Memory: [a][a][PAD][PAD][b][b][b][b]  <- 8 bytes
 *   On disk: [a][a][b][b][b][b]           <- 6 bytes  *** MISMATCH ***
 *
 * #pragma pack(push, 1) saves the current alignment state and sets the new
 * alignment to 1 byte (no padding). #pragma pack(pop) restores normal alignment
 * for all structs defined after it, so we don't accidentally break other code.
 * ============================================================ */
#pragma pack(push, 1)

/*
 * BMPFileHeader — The first 14 bytes of every valid BMP file.
 * Layout is dictated by the Windows BMP specification.
 */
typedef struct {
    uint16_t signature;     /* "Magic number": must be 0x4D42 ('BM' in ASCII).
                             * This is what every BMP parser checks first to
                             * confirm the file is actually a BMP. */
    uint32_t file_size;     /* Total size of the file in bytes. */
    uint16_t reserved1;     /* Reserved; must be 0. Set by the creating app. */
    uint16_t reserved2;     /* Reserved; must be 0. Set by the creating app. */
    uint32_t pixel_offset;  /* Byte offset from the start of the file to the
                             * first byte of pixel data. Skips both headers
                             * and any optional color table. */
} BMPFileHeader;            /* sizeof(BMPFileHeader) must be exactly 14 bytes. */

/*
 * BMPInfoHeader — The DIB (Device Independent Bitmap) header.
 * This is the BITMAPINFOHEADER variant, which is exactly 40 bytes.
 * It immediately follows the BMPFileHeader in the file.
 */
typedef struct {
    uint32_t header_size;   /* Size of this header in bytes (must be 40). */
    int32_t  width;         /* Image width in pixels. Signed per spec. */
    int32_t  height;        /* Image height in pixels. Positive = bottom-up
                             * (most common). Negative = top-down storage. */
    uint16_t color_planes;  /* Number of color planes; must be 1. */
    uint16_t bits_per_pixel;/* Color depth: 1, 4, 8, 16, 24, or 32. */
    uint32_t compression;   /* Compression method. 0 = BI_RGB (uncompressed). */
    uint32_t image_size;    /* Size of the raw pixel data in bytes.
                             * Can be 0 for BI_RGB uncompressed images. */
    int32_t  x_pixels_per_m; /* Horizontal print resolution (pixels/metre). */
    int32_t  y_pixels_per_m; /* Vertical print resolution (pixels/metre). */
    uint32_t colors_in_table; /* Number of colors in the color table (0 = max). */
    uint32_t important_colors;/* Number of important colors (0 = all). */
} BMPInfoHeader;             /* sizeof(BMPInfoHeader) must be exactly 40 bytes. */

#pragma pack(pop)
/* All structs defined after this line use the compiler's default alignment. */


/* ============================================================
 *  FUNCTION PROTOTYPES
 * ============================================================ */

/*
 * parse_bmp_headers()
 *   Opens the file at `filepath`, reads the two headers via fread(), and
 *   validates the BMP magic number.
 *
 *   Parameters:
 *     filepath  — path to the BMP file to read.
 *     file_hdr  — caller-allocated struct to populate with file header data.
 *     info_hdr  — caller-allocated struct to populate with DIB header data.
 *
 *   Returns:  0 on success, -1 on any error (bad path, not a BMP, I/O fail).
 */
int parse_bmp_headers(const char *filepath,
                      BMPFileHeader *file_hdr,
                      BMPInfoHeader *info_hdr);

/*
 * print_bmp_metadata()
 *   Formats and prints the contents of both headers to stdout in a human-
 *   readable table. Takes const pointers — it promises not to modify the data.
 */
void print_bmp_metadata(const BMPFileHeader *file_hdr,
                        const BMPInfoHeader *info_hdr);

/*
 * apply_negative_filter()
 *   Reads a BMP from `input_path`, inverts every pixel byte with bitwise NOT,
 *   and writes the result to `output_path`. Headers are copied byte-for-byte.
 *
 *   Returns:  0 on success, -1 on any error.
 */
int apply_negative_filter(const char *input_path, const char *output_path);

#endif /* BMP_CORE_H */
