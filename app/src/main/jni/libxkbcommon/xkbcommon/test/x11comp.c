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

#include "config.h"

#include <stdio.h>
#include <spawn.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "test.h"
#include "xkbcommon/xkbcommon-x11.h"

int
main(void)
{
    struct xkb_context *ctx = test_get_context(0);
    struct xkb_keymap *keymap;
    xcb_connection_t *conn;
    int32_t device_id;
    int ret, status;
    char display[512];
    char *xkb_path;
    char *original, *dump;
    char *envp[] = { NULL };
    char *xvfb_argv[] = {
        (char *) "Xvfb", display, NULL
    };
    pid_t xvfb_pid = 0;
    char *xkbcomp_argv[] = {
        (char *) "xkbcomp", (char *) "-I", NULL /* xkb_path */, display, NULL
    };
    pid_t xkbcomp_pid;

    char *xhost = NULL;
    int xdpy_current;
    int xdpy_candidate;

    /*
     * What all of this mess does is:
     * 1. Launch Xvfb on available DISPLAY.
     * 2. Make an xcb connection to this display.
     * 3. Launch xkbcomp to change the keymap of the new display (doing
     *    this programmatically is major work [which we may yet do some
     *    day for xkbcommon-x11] so we use xkbcomp for now).
     * 4. Download the keymap back from the display using xkbcommon-x11.
     * 5. Compare received keymap to the uploaded keymap.
     * 6. Kill the server & clean up.
     */

    ret = xcb_parse_display(NULL, &xhost, &xdpy_current, NULL);
    assert(ret != 0);
    /*
     * IANA assigns TCP port numbers from 6000 through 6063 to X11
     * clients.  In addition, the current XCB implementaion shows
     * that, when an X11 client tries to establish a TCP connetion,
     * the port number needed is specified by adding 6000 to a given
     * display number.  So, one of reasonable ranges of xdpy_candidate
     * is [0, 63].
     */
    for (xdpy_candidate = 63; xdpy_candidate >= 0; xdpy_candidate--) {
        if (xdpy_candidate == xdpy_current) {
            continue;
        }
        snprintf(display, sizeof(display), "%s:%d", xhost, xdpy_candidate);
        ret = posix_spawnp(&xvfb_pid, "Xvfb", NULL, NULL, xvfb_argv, envp);
        if (ret == 0) {
            break;
        }
    }
    free(xhost);
    if (ret != 0) {
        ret = SKIP_TEST;
        goto err_ctx;
    }

    /* Wait for Xvfb fully waking up to accept a connection from a client. */
    sleep(1);

    conn = xcb_connect(display, NULL);
    if (xcb_connection_has_error(conn)) {
        ret = SKIP_TEST;
        goto err_xvfd;
    }
    ret = xkb_x11_setup_xkb_extension(conn,
                                      XKB_X11_MIN_MAJOR_XKB_VERSION,
                                      XKB_X11_MIN_MINOR_XKB_VERSION,
                                      XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
                                      NULL, NULL, NULL, NULL);
    if (!ret) {
        ret = SKIP_TEST;
        goto err_xcb;
    }
    device_id = xkb_x11_get_core_keyboard_device_id(conn);
    assert(device_id != -1);

    xkb_path = test_get_path("keymaps/host.xkb");
    assert(ret >= 0);
    xkbcomp_argv[2] = xkb_path;
    ret = posix_spawnp(&xkbcomp_pid, "xkbcomp", NULL, NULL, xkbcomp_argv, envp);
    free(xkb_path);
    if (ret != 0) {
        ret = SKIP_TEST;
        goto err_xcb;
    }
    ret = waitpid(xkbcomp_pid, &status, 0);
    if (ret < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        ret = SKIP_TEST;
        goto err_xcb;
    }

    keymap = xkb_x11_keymap_new_from_device(ctx, conn, device_id,
                                            XKB_KEYMAP_COMPILE_NO_FLAGS);
    assert(keymap);

    original = test_read_file("keymaps/host.xkb");
    assert(original);

    dump = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
    assert(dump);

    if (!streq(original, dump)) {
        fprintf(stderr,
                "round-trip test failed: dumped map differs from original\n");
        fprintf(stderr, "length: dumped %lu, original %lu\n",
                (unsigned long) strlen(dump),
                (unsigned long) strlen(original));
        fprintf(stderr, "dumped map:\n");
        fprintf(stderr, "%s\n", dump);
        ret = 1;
        goto err_dump;
    }

    ret = 0;
err_dump:
    free(original);
    free(dump);
    xkb_keymap_unref(keymap);
err_xcb:
    xcb_disconnect(conn);
err_xvfd:
    if (xvfb_pid > 0)
        kill(xvfb_pid, SIGTERM);
err_ctx:
    xkb_context_unref(ctx);
    return ret;
}
