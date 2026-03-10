/*
 * main.c — CLI entry point for the BMP tool.
 *
 * Architecture note:
 *   This file is intentionally thin. Its only jobs are:
 *     1. Parse argc/argv and validate argument counts.
 *     2. Dispatch to the appropriate library function in bmp_core.c.
 *     3. Return a meaningful exit code to the shell.
 *
 *   All BMP logic lives in bmp_core.c. Keeping main.c as a pure "dispatcher"
 *   means bmp_core.c can be reused as a library without modification.
 */

#include <stdio.h>
#include <stdlib.h>   /* EXIT_SUCCESS, EXIT_FAILURE */
#include <string.h>   /* strcmp() */

#include "bmp_core.h"

/* ============================================================
 *  Forward declaration of the usage helper.
 *  Defined at the bottom to keep main() at the top for readability.
 * ============================================================ */
static void print_usage(const char *program_name);


/* ============================================================
 *  main()
 * ============================================================ */
int main(int argc, char *argv[])
{
    /*
     * argc is the count of command-line tokens (including the program name).
     * argv is an array of C-strings:
     *   argv[0] = program name (e.g., "./bmp_tool")
     *   argv[1] = subcommand  (e.g., "info" or "invert")
     *   argv[2] = first file argument
     *   argv[3] = second file argument (invert only)
     *
     * Minimum valid call: ./bmp_tool info <file>  -> argc == 3
     */
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    /* --------------------------------------------------------
     *  Dispatch: "info" subcommand
     *  Usage: ./bmp_tool info <image.bmp>
     * -------------------------------------------------------- */
    if (strcmp(argv[1], "info") == 0) {

        /* The "info" command needs exactly one file argument. */
        if (argc != 3) {
            fprintf(stderr,
                    "Error: 'info' requires exactly one file argument.\n"
                    "Usage: %s info <image.bmp>\n", argv[0]);
            return EXIT_FAILURE;
        }

        /*
         * Declare both header structs on the stack.
         * They are small (14 + 40 = 54 bytes total) so stack allocation is
         * fine here. parse_bmp_headers() will populate them via pointer.
         */
        BMPFileHeader file_hdr;
        BMPInfoHeader info_hdr;

        if (parse_bmp_headers(argv[2], &file_hdr, &info_hdr) != 0) {
            /* parse_bmp_headers already printed the specific error to stderr. */
            return EXIT_FAILURE;
        }

        print_bmp_metadata(&file_hdr, &info_hdr);
        return EXIT_SUCCESS;
    }

    /* --------------------------------------------------------
     *  Dispatch: "invert" subcommand
     *  Usage: ./bmp_tool invert <input.bmp> <output.bmp>
     * -------------------------------------------------------- */
    if (strcmp(argv[1], "invert") == 0) {

        /* The "invert" command needs exactly two file arguments. */
        if (argc != 4) {
            fprintf(stderr,
                    "Error: 'invert' requires an input and an output file.\n"
                    "Usage: %s invert <input.bmp> <output.bmp>\n", argv[0]);
            return EXIT_FAILURE;
        }

        /*
         * Guard against the user accidentally passing the same path for both
         * input and output. Writing to the input file mid-read would corrupt
         * it irreversibly.
         */
        if (strcmp(argv[2], argv[3]) == 0) {
            fprintf(stderr,
                    "Error: input and output paths must be different files.\n");
            return EXIT_FAILURE;
        }

        if (apply_negative_filter(argv[2], argv[3]) != 0) {
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    /* --------------------------------------------------------
     *  Unknown subcommand
     * -------------------------------------------------------- */
    fprintf(stderr, "Error: unknown command '%s'.\n\n", argv[1]);
    print_usage(argv[0]);
    return EXIT_FAILURE;
}


/* ============================================================
 *  print_usage() — static helper (internal linkage only)
 *
 *  'static' means this function is not visible outside this translation unit.
 *  It can't be called from bmp_core.c and won't pollute the linker's symbol
 *  table. This is the correct visibility for a file-private helper.
 * ============================================================ */
static void print_usage(const char *program_name)
{
    fprintf(stderr,
            "BMP Image Metadata Parser & Processor\n"
            "\n"
            "Usage:\n"
            "  %s info   <image.bmp>                 Print BMP metadata\n"
            "  %s invert <input.bmp> <output.bmp>    Apply negative filter\n"
            "\n"
            "Examples:\n"
            "  %s info   test_images/photo.bmp\n"
            "  %s invert test_images/photo.bmp test_images/photo_neg.bmp\n",
            program_name, program_name,
            program_name, program_name);
}
