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

#include "utils.h"
#include "paths.h"

enum resolve_name_direction {
    LEFT_TO_RIGHT,
    RIGHT_TO_LEFT,
};

const char *
get_xlocaledir_path(void)
{
    const char *dir = secure_getenv("XLOCALEDIR");
    if (!dir)
        dir = XLOCALEDIR;
    return dir;
}

/*
 * Files like compose.dir have the format LEFT: RIGHT.  Lookup @name in
 * such a file and return its matching value, according to @direction.
 * @filename is relative to the xlocaledir.
 */
static char *
resolve_name(const char *filename, enum resolve_name_direction direction,
             const char *name)
{
    int ret;
    bool ok;
    const char *xlocaledir;
    char path[512];
    FILE *file;
    const char *string, *end;
    size_t string_size;
    const char *s, *left, *right;
    char *match;
    size_t left_len, right_len, name_len;

    xlocaledir = get_xlocaledir_path();

    ret = snprintf(path, sizeof(path), "%s/%s", xlocaledir, filename);
    if (ret < 0 || (size_t) ret >= sizeof(path))
        return false;

    file = fopen(path, "r");
    if (!file)
        return false;

    ok = map_file(file, &string, &string_size);
    fclose(file);
    if (!ok)
        return false;

    s = string;
    end = string + string_size;
    name_len = strlen(name);
    match = NULL;

    while (s < end) {
        /* Skip spaces. */
        while (s < end && is_space(*s))
            s++;

        /* Skip comments. */
        if (s < end && *s == '#') {
            while (s < end && *s != '\n')
                s++;
            continue;
        }

        /* Get the left value. */
        left = s;
        while (s < end && !is_space(*s) && *s != ':')
            s++;
        left_len = s - left;

        /* There's an optional colon between left and right. */
        if (s < end && *s == ':')
            s++;

        /* Skip spaces. */
        while (s < end && is_space(*s))
            s++;

        /* Get the right value. */
        right = s;
        while (s < end && !is_space(*s))
            s++;
        right_len = s - right;

        /* Discard rest of line. */
        while (s < end && *s != '\n')
            s++;

        if (direction == LEFT_TO_RIGHT) {
            if (left_len == name_len && memcmp(left, name, left_len) == 0) {
                match = strndup(right, right_len);
                break;
            }
        }
        else if (direction == RIGHT_TO_LEFT) {
            if (right_len == name_len && memcmp(right, name, right_len) == 0) {
                match = strndup(left, left_len);
                break;
            }
        }
    }

    unmap_file(string, string_size);
    return match;
}

char *
resolve_locale(const char *locale)
{
    char *alias = resolve_name("locale.alias", LEFT_TO_RIGHT, locale);
    return alias ? alias : strdup(locale);
}

const char *
get_xcomposefile_path(void)
{
    return secure_getenv("XCOMPOSEFILE");
}

char *
get_home_xcompose_file_path(void)
{
    int ret;
    const char *home;
    char *path;

    home = secure_getenv("HOME");
    if (!home)
        return NULL;

    ret = asprintf(&path, "%s/.XCompose", home);
    if (ret <0)
        return NULL;

    return path;
}

char *
get_locale_compose_file_path(const char *locale)
{
    int ret;
    const char *xlocaledir;
    char *resolved;
    char *path;

    /*
     * WARNING: Random workaround ahead.
     *
     * We currently do not support non-UTF-8 Compose files.  The C/POSIX
     * locale is specified to be the default fallback locale with an
     * ASCII charset.  But for some reason the compose.dir points the C
     * locale to the iso8859-1/Compose file, which is not ASCII but
     * ISO8859-1.  Since this is bound to happen a lot, and since our API
     * is UTF-8 based, and since 99% of the time a C locale is really just
     * a misconfiguration for UTF-8, let's do the most helpful thing.
     */
    if (streq(locale, "C"))
        locale = "en_US.UTF-8";

    resolved = resolve_name("compose.dir", RIGHT_TO_LEFT, locale);
    if (!resolved)
        return NULL;

    if (resolved[0] == '/') {
        path = resolved;
    }
    else {
        xlocaledir = get_xlocaledir_path();
        ret = asprintf(&path, "%s/%s", xlocaledir, resolved);
        free(resolved);
        if (ret < 0)
            return NULL;
    }

    return path;
}
