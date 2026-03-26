/*
 * main.c — CLI entry point for the image processing tool.
 *
 * Subcommands:
 *   info   <input>                  Print image metadata.
 *   invert <input> <output>         Apply colour-negative filter and save.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image.h"
#include "driver_registry.h"
#include "filters.h"

extern const ImageDriver bmp_driver;
extern const ImageDriver png_driver;
extern const ImageDriver jpeg_driver;

static void build_registry(DriverRegistry *reg)
{
    registry_init(reg);
    registry_register(reg, &bmp_driver);
    registry_register(reg, &png_driver);
    registry_register(reg, &jpeg_driver);
}


static void print_generic_info(const char *path, const Image *img,
                                const ImageDriver *drv)
{
    printf("\n");
    printf("  +----------------------------------------------+\n");
    printf("  |           Image Information                  |\n");
    printf("  +----------------------------------------------+\n");
    printf("  | %-22s | %-19s |\n", "Property", "Value");
    printf("  +------------------------+--------------------+\n");
    printf("  | %-22s | %-19s |\n", "Format",      drv->name);
    printf("  | %-22s | %-19s |\n", "File",        path);
    printf("  | %-22s | %-19d |\n", "Width (px)",  img->width);
    printf("  | %-22s | %-19d |\n", "Height (px)", img->height);
    printf("  | %-22s | %-19d |\n", "Channels",    img->channels);
    printf("  +------------------------+--------------------+\n\n");
}


static int cmd_info(const DriverRegistry *reg, const char *path)
{
    const ImageDriver *drv = registry_detect(reg, path);
    if (!drv) {
        fprintf(stderr,
                "Error: could not detect format of '%s'.\n"
                "  Make sure the file exists and is a supported format.\n",
                path);
        return EXIT_FAILURE;
    }

    if (drv->get_info) {
        drv->get_info(path);
        return EXIT_SUCCESS;
    }

    Image img = {0};
    if (drv->load(path, &img) != 0)
        return EXIT_FAILURE;

    print_generic_info(path, &img, drv);
    drv->free_data(&img);
    return EXIT_SUCCESS;
}


static int cmd_invert(const DriverRegistry *reg,
                      const char *in_path,
                      const char *out_path)
{
    if (strcmp(in_path, out_path) == 0) {
        fprintf(stderr, "Error: input and output paths must be different.\n");
        return EXIT_FAILURE;
    }

    const ImageDriver *in_drv = registry_detect(reg, in_path);
    if (!in_drv) {
        fprintf(stderr,
                "Error: unsupported input format for '%s'.\n", in_path);
        return EXIT_FAILURE;
    }

    const char *out_ext = strrchr(out_path, '.');
    const ImageDriver *out_drv = registry_find_by_extension(reg, out_ext);
    if (!out_drv) {
        fprintf(stderr,
                "Error: unsupported output format for '%s'.\n"
                "  Supported extensions:", out_path);
        for (size_t i = 0; i < reg->count; i++)
            fprintf(stderr, " %s", reg->drivers[i]->extension);
        fprintf(stderr, "\n");
        return EXIT_FAILURE;
    }

    Image img = {0};
    printf("Loading  '%s' via %s driver...\n", in_path,  in_drv->name);
    if (in_drv->load(in_path, &img) != 0)
        return EXIT_FAILURE;

    filter_invert(&img);

    printf("Saving   '%s' via %s driver...\n", out_path, out_drv->name);
    int result = out_drv->save(out_path, &img);

    in_drv->free_data(&img);

    if (result == 0)
        printf("Done. Written to '%s'.\n", out_path);
    return (result == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


static void print_usage(const char *prog)
{
    fprintf(stderr,
            "Universal Image Processing Library\n"
            "\n"
            "Usage:\n"
            "  %s info   <input>              Print image metadata\n"
            "  %s invert <input> <output>     Apply colour-negative filter\n"
            "\n"
            "Supported formats (compiled in):\n"
            "  BMP  -- always available\n"
            "  PNG  -- requires -DHAVE_LIBPNG  and -lpng\n"
            "  JPEG -- requires -DHAVE_LIBJPEG and -ljpeg\n"
            "\n"
            "Examples:\n"
            "  %s info   photo.bmp\n"
            "  %s invert photo.bmp photo_neg.bmp\n"
            "  %s invert photo.bmp photo_neg.png   (cross-format)\n",
            prog, prog, prog, prog, prog);
}


int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    DriverRegistry reg;
    build_registry(&reg);

    if (strcmp(argv[1], "info") == 0) {
        if (argc != 3) {
            fprintf(stderr,
                    "Error: 'info' requires exactly one file argument.\n"
                    "Usage: %s info <file>\n", argv[0]);
            return EXIT_FAILURE;
        }
        return cmd_info(&reg, argv[2]);
    }

    if (strcmp(argv[1], "invert") == 0) {
        if (argc != 4) {
            fprintf(stderr,
                    "Error: 'invert' requires an input and an output file.\n"
                    "Usage: %s invert <input> <output>\n", argv[0]);
            return EXIT_FAILURE;
        }
        return cmd_invert(&reg, argv[2], argv[3]);
    }

    fprintf(stderr, "Error: unknown command '%s'.\n\n", argv[1]);
    print_usage(argv[0]);
    return EXIT_FAILURE;
}
