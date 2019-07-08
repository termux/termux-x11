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

#include <stdlib.h>
#include <time.h>

#include "../test/test.h"

#define BENCHMARK_ITERATIONS 20000000

static void
bench(struct xkb_state *state)
{
    int8_t keys[256] = { 0 };
    xkb_keycode_t keycode;
    xkb_keysym_t keysym;
    int i;

    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
        keycode = (rand() % (255 - 9)) + 9;
        if (keys[keycode]) {
            xkb_state_update_key(state, keycode, XKB_KEY_UP);
            keys[keycode] = 0;
            keysym = xkb_state_key_get_one_sym(state, keycode);
            (void) keysym;
        } else {
            xkb_state_update_key(state, keycode, XKB_KEY_DOWN);
            keys[keycode] = 1;
        }
    }
}

int
main(void)
{
    struct xkb_context *ctx;
    struct xkb_keymap *keymap;
    struct xkb_state *state;
    struct timespec start, stop, elapsed;

    ctx = test_get_context(0);
    assert(ctx);

    keymap = test_compile_rules(ctx, "evdev", "pc104", "us,ru,il,de",
                                ",,,neo", "grp:menu_toggle");
    assert(keymap);

    state = xkb_state_new(keymap);
    assert(state);

    xkb_context_set_log_level(ctx, XKB_LOG_LEVEL_CRITICAL);
    xkb_context_set_log_verbosity(ctx, 0);

    srand(time(NULL));

    clock_gettime(CLOCK_MONOTONIC, &start);
    bench(state);
    clock_gettime(CLOCK_MONOTONIC, &stop);

    elapsed.tv_sec = stop.tv_sec - start.tv_sec;
    elapsed.tv_nsec = stop.tv_nsec - start.tv_nsec;
    if (elapsed.tv_nsec < 0) {
        elapsed.tv_nsec += 1000000000;
        elapsed.tv_sec--;
    }

    fprintf(stderr, "ran %d iterations in %ld.%09lds\n",
            BENCHMARK_ITERATIONS, elapsed.tv_sec, elapsed.tv_nsec);

    xkb_state_unref(state);
    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);

    return 0;
}
