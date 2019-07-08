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
#include "xkbcomp-priv.h"
#include "rules.h"

int
main(int argc, char *argv[])
{
    int opt;
    struct xkb_rule_names rmlvo = { NULL };
    struct xkb_context *ctx;
    struct xkb_component_names kccgst;

    while ((opt = getopt(argc, argv, "r:m:l:v:o:h")) != -1) {
        switch (opt) {
        case 'r':
            rmlvo.rules = optarg;
            break;
        case 'm':
            rmlvo.model = optarg;
            break;
        case 'l':
            rmlvo.layout = optarg;
            break;
        case 'v':
            rmlvo.variant = optarg;
            break;
        case 'o':
            rmlvo.options = optarg;
            break;
        case 'h':
        case '?':
            fprintf(stderr, "Usage: %s [-r <rules>] [-m <model>] "
                    "[-l <layout>] [-v <variant>] [-o <options>]\n",
                    argv[0]);
            return 1;
        }
    }

    ctx = test_get_context(0);
    if (!ctx) {
        fprintf(stderr, "Failed to get xkb context\n");
        return 1;
    }

    xkb_context_sanitize_rule_names(ctx, &rmlvo);

    if (!xkb_components_from_rules(ctx, &rmlvo, &kccgst))
        return 1;

    printf("keycodes: %s\n", kccgst.keycodes);
    printf("types:    %s\n", kccgst.types);
    printf("compat:   %s\n", kccgst.compat);
    printf("symbols:  %s\n", kccgst.symbols);

    free(kccgst.keycodes);
    free(kccgst.types);
    free(kccgst.compat);
    free(kccgst.symbols);
    xkb_context_unref(ctx);
    return 0;
}
