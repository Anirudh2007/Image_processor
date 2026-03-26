#ifndef DRIVER_REGISTRY_H
#define DRIVER_REGISTRY_H

/*
 * driver_registry.h — Registry for image format drivers.
 */

#include "image.h"
#include <stddef.h>

#define MAX_DRIVERS 16



typedef struct {
    const ImageDriver *drivers[MAX_DRIVERS]; /* Pointers to static driver objects */
    size_t             count;                /* Number of registered drivers       */
} DriverRegistry;




/*
 * registry_init()
 */
void registry_init(DriverRegistry *reg);

/*
 * registry_register()
 */
int registry_register(DriverRegistry *reg, const ImageDriver *driver);

/*
 * registry_find_by_extension()
 */
const ImageDriver *registry_find_by_extension(const DriverRegistry *reg,
                                               const char *ext);

/*
 * registry_find_by_magic()
 */
const ImageDriver *registry_find_by_magic(const DriverRegistry *reg,
                                           const char *path);

/*
 * registry_detect()
 */
const ImageDriver *registry_detect(const DriverRegistry *reg,
                                   const char *path);


#endif /* DRIVER_REGISTRY_H */