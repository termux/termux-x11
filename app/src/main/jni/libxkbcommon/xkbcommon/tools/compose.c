/*
 * Copyright © 2021 Ran Benita <ran@unusedvar.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <getopt.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-compose.h"

static void
usage(FILE *fp, char *progname)
{
    fprintf(fp,
            "Usage: %s [--locale LOCALE | --locale-from-env | --locale-from-setlocale]\n",
            progname);
    fprintf(fp,
            "   --locale - specify the locale directly\n"
            "   --locale-from-env - get the locale from the LC_ALL/LC_CTYPE/LANG environment variables (falling back to C)\n"
            "   --locale-from-setlocale - get the locale using setlocale(3)\n"
    );
}

int
main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    struct xkb_context *ctx = NULL;
    struct xkb_compose_table *compose_table = NULL;
    const char *locale = NULL;
    enum options {
        OPT_LOCALE,
        OPT_LOCALE_FROM_ENV,
        OPT_LOCALE_FROM_SETLOCALE,
    };
    static struct option opts[] = {
        {"locale",                required_argument,      0, OPT_LOCALE},
        {"locale-from-env",       no_argument,            0, OPT_LOCALE_FROM_ENV},
        {"locale-from-setlocale", no_argument,            0, OPT_LOCALE_FROM_SETLOCALE},
        {0, 0, 0, 0},
    };

    setlocale(LC_ALL, "");

    while (1) {
        int opt;
        int option_index = 0;

        opt = getopt_long(argc, argv, "h", opts, &option_index);
        if (opt == -1)
            break;

        switch (opt) {
        case OPT_LOCALE:
            locale = optarg;
            break;
        case OPT_LOCALE_FROM_ENV:
            locale = getenv("LC_ALL");
            if (!locale)
                locale = getenv("LC_CTYPE");
            if (!locale)
                locale = getenv("LANG");
            if (!locale)
                locale = "C";
            break;
        case OPT_LOCALE_FROM_SETLOCALE:
            locale = setlocale(LC_CTYPE, NULL);
            break;
        case 'h':
            usage(stdout, argv[0]);
            return EXIT_SUCCESS;
        case '?':
            usage(stderr, argv[0]);
            return EXIT_INVALID_USAGE;
        }
    }
    if (locale == NULL) {
        usage(stderr, argv[0]);
        return EXIT_INVALID_USAGE;
    }

    ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!ctx) {
        fprintf(stderr, "Couldn't create xkb context\n");
        goto out;
    }

    compose_table =
        xkb_compose_table_new_from_locale(ctx, locale,
                                          XKB_COMPOSE_COMPILE_NO_FLAGS);
    if (!compose_table) {
        fprintf(stderr, "Couldn't create compose from locale\n");
        goto out;
    }

    printf("Compiled successfully from locale %s\n", locale);

out:
    xkb_compose_table_unref(compose_table);
    xkb_context_unref(ctx);

    return ret;
}
