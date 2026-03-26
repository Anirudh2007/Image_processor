/*
 * bmp_driver.c — BMP ImageDriver implementation.
 */

#include "image.h"
#include "bmp_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* "BM" in ASCII — the BMP magic number. */
static const uint8_t BMP_MAGIC[] = { 0x42, 0x4D };


static int bmp_load(const char *path, Image *img)
{
    BMPFileHeader fhdr;
    BMPInfoHeader ihdr;

    if (parse_bmp_headers(path, &fhdr, &ihdr) != 0)
        return -1;

    int32_t width    = (ihdr.width  < 0) ? -ihdr.width  : ihdr.width;
    int32_t height   = (ihdr.height < 0) ? -ihdr.height : ihdr.height;
    int     channels = ihdr.bits_per_pixel / 8;

    if (width == 0 || height == 0 || channels == 0) {
        fprintf(stderr, "bmp_load: invalid dimensions (%dx%d, %d ch) in '%s'.\n",
                width, height, channels, path);
        return -1;
    }

    int32_t unpadded_row = width * channels;
    int32_t padded_row   = (unpadded_row + 3) & ~3; /* round up to multiple of 4 */

    uint8_t *data = malloc((size_t)unpadded_row * (size_t)height);
    if (!data) {
        perror("bmp_load: malloc");
        return -1;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        perror(path);
        free(data);
        return -1;
    }

    if (fseek(f, (long)fhdr.pixel_offset, SEEK_SET) != 0) {
        perror("bmp_load: fseek");
        fclose(f);
        free(data);
        return -1;
    }

    uint8_t *row_buf = malloc((size_t)padded_row);
    if (!row_buf) {
        perror("bmp_load: row_buf malloc");
        fclose(f);
        free(data);
        return -1;
    }

    for (int32_t row = 0; row < height; row++) {
        if (fread(row_buf, 1, (size_t)padded_row, f) != (size_t)padded_row) {
            fprintf(stderr, "bmp_load: short read at row %d of '%s'.\n", row, path);
            free(row_buf);
            fclose(f);
            free(data);
            return -1;
        }
        memcpy(data + (size_t)row * (size_t)unpadded_row,
               row_buf,
               (size_t)unpadded_row);
    }

    free(row_buf);
    fclose(f);

    img->data     = data;
    img->width    = width;
    img->height   = height;
    img->channels = channels;
    return 0;
}


static int bmp_save(const char *path, const Image *img)
{
    if (!img || !img->data) {
        fprintf(stderr, "bmp_save: NULL image or data.\n");
        return -1;
    }

    int32_t unpadded_row     = img->width * img->channels;
    int32_t padded_row       = (unpadded_row + 3) & ~3;
    uint32_t pixel_data_size = (uint32_t)padded_row * (uint32_t)img->height;
    uint32_t file_size       = 14u + 40u + pixel_data_size;

    BMPFileHeader fhdr;
    memset(&fhdr, 0, sizeof(fhdr));
    fhdr.signature    = 0x4D42;
    fhdr.file_size    = file_size;
    fhdr.pixel_offset = 14u + 40u;  /* No colour table */

    BMPInfoHeader ihdr;
    memset(&ihdr, 0, sizeof(ihdr));
    ihdr.header_size    = 40u;
    ihdr.width          = img->width;
    ihdr.height         = img->height; /* Positive = bottom-up (standard) */
    ihdr.color_planes   = 1;
    ihdr.bits_per_pixel = (uint16_t)(img->channels * 8);
    ihdr.compression    = 0;           /* BI_RGB — uncompressed */
    ihdr.image_size     = pixel_data_size;

    FILE *f = fopen(path, "wb");
    if (!f) {
        perror(path);
        return -1;
    }

    if (fwrite(&fhdr, sizeof(fhdr), 1, f) != 1 ||
        fwrite(&ihdr, sizeof(ihdr), 1, f) != 1) {
        fprintf(stderr, "bmp_save: failed to write headers to '%s'.\n", path);
        fclose(f);
        return -1;
    }

    static const uint8_t PAD[3] = {0, 0, 0};
    int32_t pad_len = padded_row - unpadded_row;

    for (int32_t row = 0; row < img->height; row++) {
        const uint8_t *rowp = img->data + (size_t)row * (size_t)unpadded_row;
        if (fwrite(rowp, 1, (size_t)unpadded_row, f) != (size_t)unpadded_row) {
            fprintf(stderr, "bmp_save: pixel write failed at row %d.\n", row);
            fclose(f);
            return -1;
        }
        if (pad_len > 0) {
            if (fwrite(PAD, 1, (size_t)pad_len, f) != (size_t)pad_len) {
                fprintf(stderr, "bmp_save: padding write failed at row %d.\n", row);
                fclose(f);
                return -1;
            }
        }
    }

    fclose(f);
    return 0;
}


static void bmp_free_data(Image *img)
{
    if (!img) return;
    free(img->data);
    img->data = NULL;
}


static void bmp_get_info(const char *path)
{
    BMPFileHeader fhdr;
    BMPInfoHeader ihdr;
    if (parse_bmp_headers(path, &fhdr, &ihdr) == 0)
        print_bmp_metadata(&fhdr, &ihdr);
}


const ImageDriver bmp_driver = {
    .name      = "BMP",
    .extension = ".bmp",
    .magic     = BMP_MAGIC,
    .magic_len = sizeof(BMP_MAGIC),
    .load      = bmp_load,
    .save      = bmp_save,
    .free_data = bmp_free_data,
    .get_info  = bmp_get_info,
};
