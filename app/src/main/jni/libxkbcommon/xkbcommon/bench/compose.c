/*
 * Copyright © 2014 Ran Benita <ran234@gmail.com>
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

#include "xkbcommon/xkbcommon-compose.h"

#include "../test/test.h"

#define BENCHMARK_ITERATIONS 1000

int
main(void)
{
    struct xkb_context *ctx;
    struct timespec start, stop, elapsed;
    char *path;
    FILE *file;
    struct xkb_compose_table *table;

    ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(ctx);

    path = test_get_path("compose/en_US.UTF-8/Compose");
    file = fopen(path, "r");

    xkb_context_set_log_level(ctx, XKB_LOG_LEVEL_CRITICAL);
    xkb_context_set_log_verbosity(ctx, 0);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        rewind(file);
        table = xkb_compose_table_new_from_file(ctx, file, "",
                                                XKB_COMPOSE_FORMAT_TEXT_V1,
                                                XKB_COMPOSE_COMPILE_NO_FLAGS);
        assert(table);
        xkb_compose_table_unref(table);
    }
    clock_gettime(CLOCK_MONOTONIC, &stop);

    fclose(file);
    free(path);

    elapsed.tv_sec = stop.tv_sec - start.tv_sec;
    elapsed.tv_nsec = stop.tv_nsec - start.tv_nsec;
    if (elapsed.tv_nsec < 0) {
        elapsed.tv_nsec += 1000000000;
        elapsed.tv_sec--;
    }

    fprintf(stderr, "compiled %d compose tables in %ld.%09lds\n",
            BENCHMARK_ITERATIONS, elapsed.tv_sec, elapsed.tv_nsec);

    xkb_context_unref(ctx);
    return 0;
}
