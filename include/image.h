#ifndef IMAGE_H
#define IMAGE_H

/*
 * image.h — Core types for the image processing library.
 */

#include <stdint.h>
#include <stddef.h>


/* Decoded image with tightly-packed pixels. */
/* Pixel (x, y) is at data[(y * width + x) * channels]. */
typedef struct {
    uint8_t *data;      /* Heap-allocated pixel buffer (see layout above) */
    int32_t  width;     /* Image width  in pixels (always positive)       */
    int32_t  height;    /* Image height in pixels (always positive)       */
    int      channels;  /* Bytes per pixel                                */
} Image;


/*
 * ImageDriver — the plugin interface for image format handlers.
 */
typedef struct {
    const char    *name;       /* Human-readable name,  e.g. "BMP"  */
    const char    *extension;  /* Canonical extension,  e.g. ".bmp" */
    const uint8_t *magic;      /* Magic byte sequence for detection  */
    size_t         magic_len;  /* Byte length of magic sequence      */

    int  (*load     )(const char *path, Image *img);
    int  (*save     )(const char *path, const Image *img);
    void (*free_data)(Image *img);
    void (*get_info )(const char *path); /* Optional; may be NULL   */
} ImageDriver;


#endif /* IMAGE_H */
