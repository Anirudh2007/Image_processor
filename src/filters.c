/*
 * filters.c — Format-agnostic pixel operations.
 */

#include "filters.h"
#include <stddef.h>


void filter_invert(Image *img)
{
    if (!img || !img->data) return;

    size_t total = (size_t)img->width
                 * (size_t)img->height
                 * (size_t)img->channels;

    for (size_t i = 0; i < total; i++) {
        img->data[i] = (uint8_t)(~img->data[i]);
    }
}
