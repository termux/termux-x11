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

#include <time.h>

#include "../test/test.h"
#include "xkbcomp-priv.h"
#include "rules.h"

#define BENCHMARK_ITERATIONS 20000

int
main(int argc, char *argv[])
{
    struct xkb_context *ctx;
    struct timespec start, stop, elapsed;
    int i;
    struct xkb_rule_names rmlvo = {
        "evdev", "pc105", "us,il", ",", "ctrl:nocaps,grp:menu_toggle",
    };
    struct xkb_component_names kccgst;

    ctx = test_get_context(0);
    assert(ctx);

    xkb_context_set_log_level(ctx, XKB_LOG_LEVEL_CRITICAL);
    xkb_context_set_log_verbosity(ctx, 0);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
        assert(xkb_components_from_rules(ctx, &rmlvo, &kccgst));
        free(kccgst.keycodes);
        free(kccgst.types);
        free(kccgst.compat);
        free(kccgst.symbols);
    }
    clock_gettime(CLOCK_MONOTONIC, &stop);

    elapsed.tv_sec = stop.tv_sec - start.tv_sec;
    elapsed.tv_nsec = stop.tv_nsec - start.tv_nsec;
    if (elapsed.tv_nsec < 0) {
        elapsed.tv_nsec += 1000000000;
        elapsed.tv_sec--;
    }

    fprintf(stderr, "processed %d rule files in %ld.%09lds\n",
            BENCHMARK_ITERATIONS, elapsed.tv_sec, elapsed.tv_nsec);

    xkb_context_unref(ctx);
    return 0;
}
