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
    int pipefds[2];
    int ret, status;
    char displayfd[128], display[128];
    char *search_path, *search_path_arg, *xkb_path;
    char *original, *dump;
    char *envp[] = { NULL };
    char *xvfb_argv[] = { "Xvfb", "-displayfd", displayfd, NULL };
    pid_t xvfb_pid;
    char *xkbcomp_argv[] = { "xkbcomp", "-I", NULL /* search_path_arg */,
                             NULL /* xkb_path */, display, NULL };
    pid_t xkbcomp_pid;

    /*
     * What all of this mess does is:
     * 1. Launch Xvfb on the next available DISPLAY. Xvfb reports the
     *    display number to an fd passed with -displayfd once it's
     *    initialized. We pass a pipe there to read it.
     * 2. Make an xcb connection to this display.
     * 3. Launch xkbcomp to change the keymap of the new display (doing
     *    this programmatically is major work [which we may yet do some
     *    day for xkbcommon-x11] so we use xkbcomp for now).
     * 4. Download the keymap back from the display using xkbcommon-x11.
     * 5. Compare received keymap to the uploaded keymap.
     * 6. Kill the server & clean up.
     */

    ret = pipe(pipefds);
    assert(ret == 0);

    ret = snprintf(displayfd, sizeof(displayfd), "%d", pipefds[1]);
    assert(ret >= 0 && ret < sizeof(displayfd));

    ret = posix_spawnp(&xvfb_pid, "Xvfb", NULL, NULL, xvfb_argv, envp);
    if (ret != 0) {
        ret = SKIP_TEST;
        goto err_ctx;
    }

    close(pipefds[1]);

    display[0] = ':';
    ret = read(pipefds[0], display + 1, sizeof(display) - 1);
    if (ret <= 0 || 1 + ret >= sizeof(display) - 1) {
        ret = SKIP_TEST;
        goto err_xvfd;
    }
    if (display[ret] == '\n')
        display[ret] = '\0';
    display[1 + ret] = '\0';
    close(pipefds[0]);

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
    search_path = test_get_path("");
    assert(search_path);
    ret = asprintf(&search_path_arg, "-I%s", search_path);
    assert(ret >= 0);
    xkbcomp_argv[2] = search_path_arg;
    xkbcomp_argv[3] = xkb_path;
    ret = posix_spawnp(&xkbcomp_pid, "xkbcomp", NULL, NULL, xkbcomp_argv, envp);
    free(search_path_arg);
    free(search_path);
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

    ret = streq(original, dump);
    if (!ret) {
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
    kill(xvfb_pid, SIGTERM);
err_ctx:
    xkb_context_unref(ctx);
    return ret;
}
