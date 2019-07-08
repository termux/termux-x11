/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
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

#include <unistd.h>

#include "test.h"

int
main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    int opt;
    struct xkb_context *ctx;
    struct xkb_keymap *keymap;
    const char *rules = NULL;
    const char *model = NULL;
    const char *layout = NULL;
    const char *variant = NULL;
    const char *options = NULL;
    const char *keymap_path = NULL;
    char *dump;

    while ((opt = getopt(argc, argv, "r:m:l:v:o:k:h")) != -1) {
        switch (opt) {
        case 'r':
            rules = optarg;
            break;
        case 'm':
            model = optarg;
            break;
        case 'l':
            layout = optarg;
            break;
        case 'v':
            variant = optarg;
            break;
        case 'o':
            options = optarg;
            break;
        case 'k':
            keymap_path = optarg;
            break;
        case 'h':
        case '?':
            fprintf(stderr, "Usage: %s [-r <rules>] [-m <model>] "
                    "[-l <layout>] [-v <variant>] [-o <options>]\n",
                    argv[0]);
            fprintf(stderr, "   or: %s -k <path to keymap file>\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    ctx = test_get_context(0);
    if (!ctx) {
        fprintf(stderr, "Couldn't create xkb context\n");
        goto err_out;
    }

    if (keymap_path)
        keymap = test_compile_file(ctx, keymap_path);
    else
        keymap = test_compile_rules(ctx, rules, model, layout, variant,
                                    options);
    if (!keymap) {
        fprintf(stderr, "Couldn't create xkb keymap\n");
        goto err_ctx;
    }

    dump = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
    if (!dump) {
        fprintf(stderr, "Couldn't get the keymap string\n");
        goto err_map;
    }

    fputs(dump, stdout);

    ret = EXIT_SUCCESS;
    free(dump);
err_map:
    xkb_keymap_unref(keymap);
err_ctx:
    xkb_context_unref(ctx);
err_out:
    return ret;
}
