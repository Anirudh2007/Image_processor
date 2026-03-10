/*
 * bmp_core.c — Implementation of the BMP parsing and processing library.
 */

#include <stdio.h>
#include <stdlib.h>   /* malloc(), free() */
#include <string.h>   /* memset() (not used directly, but good practice to include) */

#include "bmp_core.h"

/*
 * Compile-time struct size assertions.
 * If packing is wrong (e.g., #pragma pack was ignored by the compiler), this
 * will produce a hard build error rather than a subtle runtime bug. This is a
 * defensive programming technique worth mentioning in interviews.
 */
_Static_assert(sizeof(BMPFileHeader) == 14,
               "BMPFileHeader must be exactly 14 bytes — check #pragma pack");
_Static_assert(sizeof(BMPInfoHeader) == 40,
               "BMPInfoHeader must be exactly 40 bytes — check #pragma pack");

/* The BMP magic number: ASCII 'B' = 0x42, 'M' = 0x4D.
 * On a little-endian system, reading 2 bytes "BM" into a uint16_t gives 0x4D42. */
#define BMP_SIGNATURE 0x4D42


/* ============================================================
 *  parse_bmp_headers()
 * ============================================================ */
int parse_bmp_headers(const char *filepath,
                      BMPFileHeader *file_hdr,
                      BMPInfoHeader *info_hdr)
{
    FILE *fp = NULL;

    /* Validate input pointers before we do anything else. */
    if (!filepath || !file_hdr || !info_hdr) {
        fprintf(stderr, "Error: null argument passed to parse_bmp_headers.\n");
        return -1;
    }

    /*
     * Open in "rb" (read binary) mode.
     * "rb" prevents the C runtime from performing any newline translation
     * (e.g., \r\n -> \n on Windows), which would silently corrupt binary data.
     */
    fp = fopen(filepath, "rb");
    if (!fp) {
        /*
         * perror() prints the string we provide followed by a colon and the
         * system error message for the current value of errno. This gives the
         * user an exact OS-level reason (e.g., "No such file or directory").
         */
        perror(filepath);
        return -1;
    }

    /*
     * Read the File Header.
     * fread(buffer, element_size, element_count, stream)
     * Returns the number of elements successfully read. We expect exactly 1.
     * Checking the return value guards against truncated or empty files.
     */
    if (fread(file_hdr, sizeof(BMPFileHeader), 1, fp) != 1) {
        fprintf(stderr, "Error: failed to read BMP file header from '%s'.\n",
                filepath);
        fclose(fp);
        return -1;
    }

    /*
     * Validate the magic number IMMEDIATELY after reading the file header.
     * This is the canonical way to identify a BMP file. If this check fails,
     * we stop — there is no point reading further into a non-BMP file.
     */
    if (file_hdr->signature != BMP_SIGNATURE) {
        fprintf(stderr,
                "Error: '%s' is not a valid BMP file (bad signature: 0x%04X).\n",
                filepath, file_hdr->signature);
        fclose(fp);
        return -1;
    }

    /*
     * Read the DIB / Info Header.
     * The file pointer is already positioned right after the file header
     * because fread() advances it by the number of bytes it read.
     */
    if (fread(info_hdr, sizeof(BMPInfoHeader), 1, fp) != 1) {
        fprintf(stderr, "Error: failed to read BMP info header from '%s'.\n",
                filepath);
        fclose(fp);
        return -1;
    }

    /*
     * Always close the file handle before returning.
     * Not closing is a resource leak. In long-running programs or loops,
     * this exhausts the OS file descriptor table.
     */
    fclose(fp);
    return 0; /* Success */
}


/* ============================================================
 *  print_bmp_metadata()
 * ============================================================ */
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


/* ============================================================
 *  apply_negative_filter()
 * ============================================================ */
int apply_negative_filter(const char *input_path, const char *output_path)
{
    FILE           *fp_in  = NULL;
    FILE           *fp_out = NULL;
    BMPFileHeader   file_hdr;
    BMPInfoHeader   info_hdr;
    uint8_t        *pixel_buf = NULL;
    uint32_t        pixel_data_size;
    size_t          bytes_read;
    uint32_t        i;

    if (!input_path || !output_path) {
        fprintf(stderr, "Error: null argument passed to apply_negative_filter.\n");
        return -1;
    }

    /* --- Open input for reading in binary mode --- */
    fp_in = fopen(input_path, "rb");
    if (!fp_in) {
        perror(input_path);
        return -1;
    }

    /* Read and validate both headers using the same approach as parse_bmp_headers */
    if (fread(&file_hdr, sizeof(BMPFileHeader), 1, fp_in) != 1) {
        fprintf(stderr, "Error: could not read file header from '%s'.\n",
                input_path);
        fclose(fp_in);
        return -1;
    }

    if (file_hdr.signature != BMP_SIGNATURE) {
        fprintf(stderr, "Error: '%s' is not a valid BMP file.\n", input_path);
        fclose(fp_in);
        return -1;
    }

    if (fread(&info_hdr, sizeof(BMPInfoHeader), 1, fp_in) != 1) {
        fprintf(stderr, "Error: could not read info header from '%s'.\n",
                input_path);
        fclose(fp_in);
        return -1;
    }

    /*
     * Calculate the size of the pixel data block.
     * The BMP spec says image_size can be 0 for uncompressed (BI_RGB) images,
     * so we fall back to: file_size - pixel_offset.
     * This is the byte count from the pixel start to end of file.
     */
    if (info_hdr.image_size != 0) {
        pixel_data_size = info_hdr.image_size;
    } else {
        pixel_data_size = file_hdr.file_size - file_hdr.pixel_offset;
    }

    if (pixel_data_size == 0) {
        fprintf(stderr, "Error: image has no pixel data.\n");
        fclose(fp_in);
        return -1;
    }

    /*
     * Allocate a heap buffer for the pixel data.
     *
     * Why malloc() and not a stack array?
     * 1. The size is only known at runtime.
     * 2. Stack space is limited (typically 1-8 MB). A 4K BMP can be >40 MB.
     * 3. Variable-length arrays (VLAs) are optional in C99 and forbidden with
     *    -pedantic for standard C. malloc() is always the right call here.
     *
     * We cast the return of malloc() — not strictly required in C, but it
     * makes the type explicit and is good practice in a project also compiled
     *  as C++.
     */
    pixel_buf = (uint8_t *)malloc(pixel_data_size);
    if (!pixel_buf) {
        fprintf(stderr, "Error: malloc failed for %u bytes of pixel data.\n",
                pixel_data_size);
        fclose(fp_in);
        return -1;
    }

    /*
     * Seek to the pixel data offset before reading.
     * SEEK_SET means the offset is relative to the beginning of the file.
     * This correctly skips over any optional color table that may exist
     * between the headers and the pixel data.
     */
    if (fseek(fp_in, (long)file_hdr.pixel_offset, SEEK_SET) != 0) {
        fprintf(stderr, "Error: fseek failed on '%s'.\n", input_path);
        free(pixel_buf);
        fclose(fp_in);
        return -1;
    }

    /* Read all pixel bytes into the heap buffer in one call. */
    bytes_read = fread(pixel_buf, 1, pixel_data_size, fp_in);
    if (bytes_read != (size_t)pixel_data_size) {
        fprintf(stderr,
                "Warning: expected %u bytes, read %zu. File may be truncated.\n",
                pixel_data_size, bytes_read);
        /* We continue with what we have — a partial invert is still useful. */
    }

    fclose(fp_in);
    fp_in = NULL; /* Null the pointer after close to prevent double-free accidents */

    /*
     * Apply the negative filter: bitwise NOT (~) on every byte.
     *
     * For a 24-bit BMP (3 bytes per pixel: B, G, R):
     *   Original pixel: R=200, G=100, B=50  -> ~200=55, ~100=155, ~50=205
     * This is a true color negative — equivalent to (255 - channel) per channel.
     *
     * Bitwise NOT on the raw bytes works regardless of bit depth because we
     * are not interpreting the bytes, just flipping all bits.
     */
    for (i = 0; i < (uint32_t)bytes_read; i++) {
        pixel_buf[i] = ~pixel_buf[i];
    }

    /* --- Open output for writing in binary mode --- */
    fp_out = fopen(output_path, "wb");
    if (!fp_out) {
        perror(output_path);
        free(pixel_buf);
        return -1;
    }

    /*
     * Write the headers UNTOUCHED to the output file.
     * The metadata (size, dimensions, offsets) remains valid — only the
     * pixel bytes have changed.
     */
    if (fwrite(&file_hdr, sizeof(BMPFileHeader), 1, fp_out) != 1 ||
        fwrite(&info_hdr, sizeof(BMPInfoHeader), 1, fp_out) != 1) {
        fprintf(stderr, "Error: failed to write headers to '%s'.\n",
                output_path);
        free(pixel_buf);
        fclose(fp_out);
        return -1;
    }

    /*
     * Seek output to the pixel offset. This correctly pads any gap between
     * the headers and the pixel data (e.g., a color table) with zero bytes,
     * preserving file structure integrity.
     */
    if (fseek(fp_out, (long)file_hdr.pixel_offset, SEEK_SET) != 0) {
        fprintf(stderr, "Error: fseek failed on output '%s'.\n", output_path);
        free(pixel_buf);
        fclose(fp_out);
        return -1;
    }

    /* Write the inverted pixel buffer to the output file. */
    if (fwrite(pixel_buf, 1, bytes_read, fp_out) != bytes_read) {
        fprintf(stderr, "Error: failed to write pixel data to '%s'.\n",
                output_path);
        free(pixel_buf);
        fclose(fp_out);
        return -1;
    }

    /*
     * Release the heap buffer. Every malloc() MUST have a matching free().
     * Tools like Valgrind or AddressSanitizer (-fsanitize=address) will
     * catch leaks if you forget this. Nulling after free is a defensive habit
     * — it turns a use-after-free into an immediate null-pointer crash.
     */
    free(pixel_buf);
    pixel_buf = NULL;

    fclose(fp_out);

    printf("Success: inverted image written to '%s'.\n", output_path);
    return 0;
}
