/*
 * driver_registry.c — Implementation of the DriverRegistry.
 */

#include "driver_registry.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>   /* tolower() */



void registry_init(DriverRegistry *reg)
{
    memset(reg, 0, sizeof(*reg));
}



int registry_register(DriverRegistry *reg, const ImageDriver *driver)
{
    if (!reg || !driver) {
        fprintf(stderr, "registry_register: NULL argument.\n");
        return -1;
    }
    if (reg->count >= MAX_DRIVERS) {
        fprintf(stderr,
                "registry_register: registry is full (%d/%d). "
                "Increase MAX_DRIVERS in driver_registry.h.\n",
                (int)reg->count, MAX_DRIVERS);
        return -1;
    }
    reg->drivers[reg->count++] = driver;
    return 0;
}


static const char *last_dot(const char *path)
{
    return strrchr(path, '.');
}

static int icase_eq(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0');
}



const ImageDriver *registry_find_by_extension(const DriverRegistry *reg,
                                               const char *ext)
{
    if (!reg || !ext) return NULL;
    for (size_t i = 0; i < reg->count; i++) {
        const ImageDriver *d = reg->drivers[i];
        if (d->extension && icase_eq(d->extension, ext))
            return d;
    }
    return NULL;
}



const ImageDriver *registry_find_by_magic(const DriverRegistry *reg,
                                           const char *path)
{
    if (!reg || !path) return NULL;

    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    uint8_t buf[16];
    size_t  n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    for (size_t i = 0; i < reg->count; i++) {
        const ImageDriver *d = reg->drivers[i];
        if (!d->magic || d->magic_len == 0) continue;
        if (d->magic_len > n)               continue;
        if (memcmp(buf, d->magic, d->magic_len) == 0)
            return d;
    }
    return NULL;
}



const ImageDriver *registry_detect(const DriverRegistry *reg, const char *path)
{
    const ImageDriver *d = registry_find_by_magic(reg, path);
    if (d) return d;
    return registry_find_by_extension(reg, last_dot(path));
}
