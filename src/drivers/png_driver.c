/*
 * png_driver.c — PNG ImageDriver implementation.
 *
 * REQUIRES: libpng  (https://libpng.org)
 * Install (Ubuntu/Debian): sudo apt install libpng-dev
 * Install (macOS/Homebrew): brew install libpng
 * Install (MinGW/MSYS2):   pacman -S mingw-w64-x86_64-libpng
 */

#include "image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LIBPNG
#include <png.h>
#endif

/* PNG magic: \x89 P N G \r \n \x1a \n  (8 bytes) */
static const uint8_t PNG_MAGIC[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
};


static int png_load(const char *path, Image *img)
{
#ifndef HAVE_LIBPNG
    (void)path; (void)img;
    fprintf(stderr, "png_load: library not available. "
                    "Recompile with -DHAVE_LIBPNG -lpng.\n");
    return -1;
#else
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return -1; }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                              NULL, NULL, NULL);
    if (!png) {
        fclose(f);
        fprintf(stderr, "png_load: png_create_read_struct failed.\n");
        return -1;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(f);
        return -1;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(f);
        return -1;
    }

    png_init_io(png, f);
    png_read_info(png, info);

    png_set_strip_16(png);
    png_set_packing(png);
    png_set_expand(png);
    png_set_gray_to_rgb(png);
    png_read_update_info(png, info);
    int32_t width    = (int32_t)png_get_image_width (png, info);
    int32_t height   = (int32_t)png_get_image_height(png, info);
    int     channels = (int)    png_get_channels    (png, info);

    size_t row_bytes = (size_t)width * (size_t)channels;
    uint8_t *data = malloc(row_bytes * (size_t)height);
    if (!data) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(f);
        perror("png_load: malloc");
        return -1;
    }

    png_bytep *row_ptrs = malloc((size_t)height * sizeof(png_bytep));
    if (!row_ptrs) {
        free(data);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(f);
        return -1;
    }
    for (int32_t r = 0; r < height; r++)
        row_ptrs[r] = (png_bytep)(data + (size_t)r * row_bytes);

    png_read_image(png, row_ptrs);
    png_read_end(png, NULL);

    free(row_ptrs);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(f);

    img->data     = data;
    img->width    = width;
    img->height   = height;
    img->channels = channels;
    return 0;
#endif /* HAVE_LIBPNG */
}


static int png_save(const char *path, const Image *img)
{
#ifndef HAVE_LIBPNG
    (void)path; (void)img;
    fprintf(stderr, "png_save: library not available. "
                    "Recompile with -DHAVE_LIBPNG -lpng.\n");
    return -1;
#else
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return -1; }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                               NULL, NULL, NULL);
    if (!png) { fclose(f); return -1; }

    png_infop info = png_create_info_struct(png);
    if (!info) { png_destroy_write_struct(&png, NULL); fclose(f); return -1; }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(f);
        return -1;
    }

    png_init_io(png, f);

    int color_type;
    switch (img->channels) {
        case 1: color_type = PNG_COLOR_TYPE_GRAY;       break;
        case 3: color_type = PNG_COLOR_TYPE_RGB;        break;
        case 4: color_type = PNG_COLOR_TYPE_RGB_ALPHA;  break;
        default:
            fprintf(stderr, "png_save: unsupported channel count %d.\n",
                    img->channels);
            png_destroy_write_struct(&png, &info);
            fclose(f);
            return -1;
    }

    png_set_IHDR(png, info,
                 (png_uint_32)img->width,
                 (png_uint_32)img->height,
                 8,               /* bit depth */
                 color_type,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);

    size_t row_bytes = (size_t)img->width * (size_t)img->channels;
    png_bytep *row_ptrs = malloc((size_t)img->height * sizeof(png_bytep));
    if (!row_ptrs) {
        png_destroy_write_struct(&png, &info);
        fclose(f);
        return -1;
    }
    for (int32_t r = 0; r < img->height; r++)
        row_ptrs[r] = (png_bytep)(img->data + (size_t)r * row_bytes);

    png_write_image(png, row_ptrs);
    png_write_end(png, NULL);

    free(row_ptrs);
    png_destroy_write_struct(&png, &info);
    fclose(f);
    return 0;
#endif /* HAVE_LIBPNG */
}


static void png_free_data(Image *img)
{
    if (!img) return;
    free(img->data);
    img->data = NULL;
}


const ImageDriver png_driver = {
    .name      = "PNG",
    .extension = ".png",
    .magic     = PNG_MAGIC,
    .magic_len = sizeof(PNG_MAGIC),
    .load      = png_load,
    .save      = png_save,
    .free_data = png_free_data,
    .get_info  = NULL,   /* Falls back to generic info in main.c */
};
