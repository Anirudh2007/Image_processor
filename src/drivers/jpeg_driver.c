/*
 * jpeg_driver.c — JPEG ImageDriver implementation.
 *
 * REQUIRES: libjpeg-turbo  (https://libjpeg-turbo.org)
 * Install (Ubuntu/Debian): sudo apt install libjpeg-turbo8-dev
 * Install (macOS/Homebrew): brew install jpeg-turbo
 * Install (MinGW/MSYS2):   pacman -S mingw-w64-x86_64-libjpeg-turbo
 */

#include "image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LIBJPEG
#include <jpeglib.h>
#endif

/* JPEG magic: SOI marker — always 0xFF 0xD8 0xFF */
static const uint8_t JPEG_MAGIC[] = { 0xFF, 0xD8, 0xFF };

/* Default quality for JPEG compression (0–100; 85 is a good balance). */
#ifndef JPEG_SAVE_QUALITY
#define JPEG_SAVE_QUALITY 85
#endif


static int jpeg_load(const char *path, Image *img)
{
#ifndef HAVE_LIBJPEG
    (void)path; (void)img;
    fprintf(stderr, "jpeg_load: library not available. "
                    "Recompile with -DHAVE_LIBJPEG -ljpeg.\n");
    return -1;
#else
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return -1; }

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr         jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, f);

    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
        fprintf(stderr, "jpeg_load: failed to read JPEG header in '%s'.\n", path);
        jpeg_destroy_decompress(&cinfo);
        fclose(f);
        return -1;
    }

    cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress(&cinfo);

    int32_t width    = (int32_t)cinfo.output_width;
    int32_t height   = (int32_t)cinfo.output_height;
    int     channels = (int)    cinfo.output_components; /* always 3 for RGB */

    size_t  row_bytes = (size_t)width * (size_t)channels;
    uint8_t *data     = malloc(row_bytes * (size_t)height);
    if (!data) {
        perror("jpeg_load: malloc");
        jpeg_destroy_decompress(&cinfo);
        fclose(f);
        return -1;
    }

    while (cinfo.output_scanline < cinfo.output_height) {
        JSAMPROW row = (JSAMPROW)(data +
                       (size_t)cinfo.output_scanline * row_bytes);
        jpeg_read_scanlines(&cinfo, &row, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(f);

    img->data     = data;
    img->width    = width;
    img->height   = height;
    img->channels = channels;
    return 0;
#endif /* HAVE_LIBJPEG */
}


static int jpeg_save(const char *path, const Image *img)
{
#ifndef HAVE_LIBJPEG
    (void)path; (void)img;
    fprintf(stderr, "jpeg_save: library not available. "
                    "Recompile with -DHAVE_LIBJPEG -ljpeg.\n");
    return -1;
#else
    if (img->channels == 4) {
        fprintf(stderr, "jpeg_save: JPEG does not support alpha channels. "
                        "Convert to RGB first.\n");
        return -1;
    }

    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return -1; }

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr       jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, f);

    cinfo.image_width      = (JDIMENSION)img->width;
    cinfo.image_height     = (JDIMENSION)img->height;
    cinfo.input_components = img->channels;
    cinfo.in_color_space   = (img->channels == 1) ? JCS_GRAYSCALE : JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, JPEG_SAVE_QUALITY, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    size_t row_bytes = (size_t)img->width * (size_t)img->channels;
    while (cinfo.next_scanline < cinfo.image_height) {
        JSAMPROW row = (JSAMPROW)(img->data +
                       (size_t)cinfo.next_scanline * row_bytes);
        jpeg_write_scanlines(&cinfo, &row, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(f);
    return 0;
#endif /* HAVE_LIBJPEG */
}


static void jpeg_free_data(Image *img)
{
    if (!img) return;
    free(img->data);
    img->data = NULL;
}


const ImageDriver jpeg_driver = {
    .name      = "JPEG",
    .extension = ".jpg",    /* Also register ".jpeg" separately if desired */
    .magic     = JPEG_MAGIC,
    .magic_len = sizeof(JPEG_MAGIC),
    .load      = jpeg_load,
    .save      = jpeg_save,
    .free_data = jpeg_free_data,
    .get_info  = NULL,
};
