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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <limits.h>
#include <locale.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <linux/input.h>

#include "test.h"

struct keyboard {
    char *path;
    int fd;
    struct xkb_state *state;
    struct xkb_compose_state *compose_state;
    struct keyboard *next;
};

static bool terminate;
static int evdev_offset = 8;
static bool report_state_changes;
static bool with_compose;

#define NLONGS(n) (((n) + LONG_BIT - 1) / LONG_BIT)

static bool
evdev_bit_is_set(const unsigned long *array, int bit)
{
    return array[bit / LONG_BIT] & (1LL << (bit % LONG_BIT));
}

/* Some heuristics to see if the device is a keyboard. */
static bool
is_keyboard(int fd)
{
    int i;
    unsigned long evbits[NLONGS(EV_CNT)] = { 0 };
    unsigned long keybits[NLONGS(KEY_CNT)] = { 0 };

    errno = 0;
    ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), evbits);
    if (errno)
        return false;

    if (!evdev_bit_is_set(evbits, EV_KEY))
        return false;

    errno = 0;
    ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits);
    if (errno)
        return false;

    for (i = KEY_RESERVED; i <= KEY_MIN_INTERESTING; i++)
        if (evdev_bit_is_set(keybits, i))
            return true;

    return false;
}

static int
keyboard_new(struct dirent *ent, struct xkb_keymap *keymap,
             struct xkb_compose_table *compose_table, struct keyboard **out)
{
    int ret;
    char *path;
    int fd;
    struct xkb_state *state;
    struct xkb_compose_state *compose_state = NULL;
    struct keyboard *kbd;

    ret = asprintf(&path, "/dev/input/%s", ent->d_name);
    if (ret < 0)
        return -ENOMEM;

    fd = open(path, O_NONBLOCK | O_CLOEXEC | O_RDONLY);
    if (fd < 0) {
        ret = -errno;
        goto err_path;
    }

    if (!is_keyboard(fd)) {
        /* Dummy "skip this device" value. */
        ret = -ENOTSUP;
        goto err_fd;
    }

    state = xkb_state_new(keymap);
    if (!state) {
        fprintf(stderr, "Couldn't create xkb state for %s\n", path);
        ret = -EFAULT;
        goto err_fd;
    }

    if (with_compose) {
        compose_state = xkb_compose_state_new(compose_table,
                                              XKB_COMPOSE_STATE_NO_FLAGS);
        if (!compose_state) {
            fprintf(stderr, "Couldn't create compose state for %s\n", path);
            ret = -EFAULT;
            goto err_state;
        }
    }

    kbd = calloc(1, sizeof(*kbd));
    if (!kbd) {
        ret = -ENOMEM;
        goto err_compose_state;
    }

    kbd->path = path;
    kbd->fd = fd;
    kbd->state = state;
    kbd->compose_state = compose_state;
    *out = kbd;
    return 0;

err_compose_state:
    xkb_compose_state_unref(compose_state);
err_state:
    xkb_state_unref(state);
err_fd:
    close(fd);
err_path:
    free(path);
    return ret;
}

static void
keyboard_free(struct keyboard *kbd)
{
    if (!kbd)
        return;
    if (kbd->fd >= 0)
        close(kbd->fd);
    free(kbd->path);
    xkb_state_unref(kbd->state);
    xkb_compose_state_unref(kbd->compose_state);
    free(kbd);
}

static int
filter_device_name(const struct dirent *ent)
{
    return !fnmatch("event*", ent->d_name, 0);
}

static struct keyboard *
get_keyboards(struct xkb_keymap *keymap,
              struct xkb_compose_table *compose_table)
{
    int ret, i, nents;
    struct dirent **ents;
    struct keyboard *kbds = NULL, *kbd = NULL;

    nents = scandir("/dev/input", &ents, filter_device_name, alphasort);
    if (nents < 0) {
        fprintf(stderr, "Couldn't scan /dev/input: %s\n", strerror(errno));
        return NULL;
    }

    for (i = 0; i < nents; i++) {
        ret = keyboard_new(ents[i], keymap, compose_table, &kbd);
        if (ret) {
            if (ret == -EACCES) {
                fprintf(stderr, "Couldn't open /dev/input/%s: %s. "
                                "You probably need root to run this.\n",
                        ents[i]->d_name, strerror(-ret));
                break;
            }
            if (ret != -ENOTSUP) {
                fprintf(stderr, "Couldn't open /dev/input/%s: %s. Skipping.\n",
                        ents[i]->d_name, strerror(-ret));
            }
            continue;
        }

        kbd->next = kbds;
        kbds = kbd;
    }

    if (!kbds) {
        fprintf(stderr, "Couldn't find any keyboards I can use! Quitting.\n");
        goto err;
    }

err:
    for (i = 0; i < nents; i++)
        free(ents[i]);
    free(ents);
    return kbds;
}

static void
free_keyboards(struct keyboard *kbds)
{
    struct keyboard *next;

    while (kbds) {
        next = kbds->next;
        keyboard_free(kbds);
        kbds = next;
    }
}

/* The meaning of the input_event 'value' field. */
enum {
    KEY_STATE_RELEASE = 0,
    KEY_STATE_PRESS = 1,
    KEY_STATE_REPEAT = 2,
};

static void
process_event(struct keyboard *kbd, uint16_t type, uint16_t code, int32_t value)
{
    xkb_keycode_t keycode;
    struct xkb_keymap *keymap;
    enum xkb_state_component changed;
    enum xkb_compose_status status;

    if (type != EV_KEY)
        return;

    keycode = evdev_offset + code;
    keymap = xkb_state_get_keymap(kbd->state);

    if (value == KEY_STATE_REPEAT && !xkb_keymap_key_repeats(keymap, keycode))
        return;

    if (with_compose && value != KEY_STATE_RELEASE) {
        xkb_keysym_t keysym = xkb_state_key_get_one_sym(kbd->state, keycode);
        xkb_compose_state_feed(kbd->compose_state, keysym);
    }

    if (value != KEY_STATE_RELEASE)
        test_print_keycode_state(kbd->state, kbd->compose_state, keycode);

    if (with_compose) {
        status = xkb_compose_state_get_status(kbd->compose_state);
        if (status == XKB_COMPOSE_CANCELLED || status == XKB_COMPOSE_COMPOSED)
            xkb_compose_state_reset(kbd->compose_state);
    }

    if (value == KEY_STATE_RELEASE)
        changed = xkb_state_update_key(kbd->state, keycode, XKB_KEY_UP);
    else
        changed = xkb_state_update_key(kbd->state, keycode, XKB_KEY_DOWN);

    if (report_state_changes)
        test_print_state_changes(changed);
}

static int
read_keyboard(struct keyboard *kbd)
{
    ssize_t len;
    struct input_event evs[16];

    /* No fancy error checking here. */
    while ((len = read(kbd->fd, &evs, sizeof(evs))) > 0) {
        const size_t nevs = len / sizeof(struct input_event);
        for (size_t i = 0; i < nevs; i++)
            process_event(kbd, evs[i].type, evs[i].code, evs[i].value);
    }

    if (len < 0 && errno != EWOULDBLOCK) {
        fprintf(stderr, "Couldn't read %s: %s\n", kbd->path, strerror(errno));
        return -errno;
    }

    return 0;
}

static int
loop(struct keyboard *kbds)
{
    int i, ret;
    int epfd;
    struct keyboard *kbd;
    struct epoll_event ev;
    struct epoll_event evs[16];

    epfd = epoll_create1(0);
    if (epfd < 0) {
        fprintf(stderr, "Couldn't create epoll instance: %s\n",
                strerror(errno));
        return -errno;
    }

    for (kbd = kbds; kbd; kbd = kbd->next) {
        memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLIN;
        ev.data.ptr = kbd;
        ret = epoll_ctl(epfd, EPOLL_CTL_ADD, kbd->fd, &ev);
        if (ret) {
            ret = -errno;
            fprintf(stderr, "Couldn't add %s to epoll: %s\n",
                    kbd->path, strerror(errno));
            goto err_epoll;
        }
    }

    while (!terminate) {
        ret = epoll_wait(epfd, evs, 16, -1);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            ret = -errno;
            fprintf(stderr, "Couldn't poll for events: %s\n",
                    strerror(errno));
            goto err_epoll;
        }

        for (i = 0; i < ret; i++) {
            kbd = evs[i].data.ptr;
            ret = read_keyboard(kbd);
            if (ret) {
                goto err_epoll;
            }
        }
    }

    close(epfd);
    return 0;

err_epoll:
    close(epfd);
    return ret;
}

static void
sigintr_handler(int signum)
{
    terminate = true;
}

int
main(int argc, char *argv[])
{
    int ret;
    int opt;
    struct keyboard *kbds;
    struct xkb_context *ctx;
    struct xkb_keymap *keymap;
    struct xkb_compose_table *compose_table = NULL;
    const char *rules = NULL;
    const char *model = NULL;
    const char *layout = NULL;
    const char *variant = NULL;
    const char *options = NULL;
    const char *keymap_path = NULL;
    const char *locale;
    struct sigaction act;

    setlocale(LC_ALL, "");

    while ((opt = getopt(argc, argv, "r:m:l:v:o:k:n:cd")) != -1) {
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
        case 'n':
            errno = 0;
            evdev_offset = strtol(optarg, NULL, 10);
            if (errno) {
                fprintf(stderr, "error: -n option expects a number\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'c':
            report_state_changes = true;
            break;
        case 'd':
            with_compose = true;
            break;
        case '?':
            fprintf(stderr, "   Usage: %s [-r <rules>] [-m <model>] "
                    "[-l <layout>] [-v <variant>] [-o <options>]\n",
                    argv[0]);
            fprintf(stderr, "      or: %s -k <path to keymap file>\n",
                    argv[0]);
            fprintf(stderr, "For both: -n <evdev keycode offset>\n"
                            "          -c (to report changes to the state)\n"
                            "          -d (to enable compose)\n");
            exit(2);
        }
    }

    ctx = test_get_context(0);
    if (!ctx) {
        ret = -1;
        fprintf(stderr, "Couldn't create xkb context\n");
        goto err_out;
    }

    if (keymap_path) {
        FILE *file = fopen(keymap_path, "r");
        if (!file) {
            ret = EXIT_FAILURE;
            fprintf(stderr, "Couldn't open '%s': %s\n",
                    keymap_path, strerror(errno));
            goto err_ctx;
        }
        keymap = xkb_keymap_new_from_file(ctx, file,
                                          XKB_KEYMAP_FORMAT_TEXT_V1, 0);
        fclose(file);
    }
    else {
        keymap = test_compile_rules(ctx, rules, model, layout, variant,
                                    options);
    }

    if (!keymap) {
        ret = -1;
        fprintf(stderr, "Couldn't create xkb keymap\n");
        goto err_ctx;
    }

    if (with_compose) {
        locale = setlocale(LC_CTYPE, NULL);
        compose_table =
            xkb_compose_table_new_from_locale(ctx, locale,
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
        if (!compose_table) {
            ret = -1;
            fprintf(stderr, "Couldn't create compose from locale\n");
            goto err_xkb;
        }
    }

    kbds = get_keyboards(keymap, compose_table);
    if (!kbds) {
        ret = -1;
        goto err_compose;
    }

    act.sa_handler = sigintr_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    /* Instead of fiddling with termios.. */
    system("stty -echo");

    ret = loop(kbds);
    if (ret)
        goto err_stty;

err_stty:
    system("stty echo");
    free_keyboards(kbds);
err_compose:
    xkb_compose_table_unref(compose_table);
err_xkb:
    xkb_keymap_unref(keymap);
err_ctx:
    xkb_context_unref(ctx);
err_out:
    exit(ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
