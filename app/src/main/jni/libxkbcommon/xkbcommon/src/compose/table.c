/*
 * Copyright © 2013 Ran Benita <ran234@gmail.com>
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
#include "table.h"
#include "parser.h"
#include "paths.h"

static struct xkb_compose_table *
xkb_compose_table_new(struct xkb_context *ctx,
                      const char *locale,
                      enum xkb_compose_format format,
                      enum xkb_compose_compile_flags flags)
{
    char *resolved_locale;
    struct xkb_compose_table *table;
    struct compose_node root;

    resolved_locale = resolve_locale(locale);
    if (!resolved_locale)
        return NULL;

    table = calloc(1, sizeof(*table));
    if (!table) {
        free(resolved_locale);
        return NULL;
    }

    table->refcnt = 1;
    table->ctx = xkb_context_ref(ctx);

    table->locale = resolved_locale;
    table->format = format;
    table->flags = flags;

    darray_init(table->nodes);
    darray_init(table->utf8);

    root.keysym = XKB_KEY_NoSymbol;
    root.next = 0;
    root.is_leaf = true;
    root.u.leaf.utf8 = 0;
    root.u.leaf.keysym = XKB_KEY_NoSymbol;
    darray_append(table->nodes, root);

    darray_append(table->utf8, '\0');

    return table;
}

XKB_EXPORT struct xkb_compose_table *
xkb_compose_table_ref(struct xkb_compose_table *table)
{
    table->refcnt++;
    return table;
}

XKB_EXPORT void
xkb_compose_table_unref(struct xkb_compose_table *table)
{
    if (!table || --table->refcnt > 0)
        return;
    free(table->locale);
    darray_free(table->nodes);
    darray_free(table->utf8);
    xkb_context_unref(table->ctx);
    free(table);
}

XKB_EXPORT struct xkb_compose_table *
xkb_compose_table_new_from_file(struct xkb_context *ctx,
                                FILE *file,
                                const char *locale,
                                enum xkb_compose_format format,
                                enum xkb_compose_compile_flags flags)
{
    struct xkb_compose_table *table;
    bool ok;

    if (flags & ~(XKB_COMPOSE_COMPILE_NO_FLAGS)) {
        log_err_func(ctx, "unrecognized flags: %#x\n", flags);
        return NULL;
    }

    if (format != XKB_COMPOSE_FORMAT_TEXT_V1) {
        log_err_func(ctx, "unsupported compose format: %d\n", format);
        return NULL;
    }

    table = xkb_compose_table_new(ctx, locale, format, flags);
    if (!table)
        return NULL;

    ok = parse_file(table, file, "(unknown file)");
    if (!ok) {
        xkb_compose_table_unref(table);
        return NULL;
    }

    return table;
}

XKB_EXPORT struct xkb_compose_table *
xkb_compose_table_new_from_buffer(struct xkb_context *ctx,
                                  const char *buffer, size_t length,
                                  const char *locale,
                                  enum xkb_compose_format format,
                                  enum xkb_compose_compile_flags flags)
{
    struct xkb_compose_table *table;
    bool ok;

    if (flags & ~(XKB_COMPOSE_COMPILE_NO_FLAGS)) {
        log_err_func(ctx, "unrecognized flags: %#x\n", flags);
        return NULL;
    }

    if (format != XKB_COMPOSE_FORMAT_TEXT_V1) {
        log_err_func(ctx, "unsupported compose format: %d\n", format);
        return NULL;
    }

    table = xkb_compose_table_new(ctx, locale, format, flags);
    if (!table)
        return NULL;

    ok = parse_string(table, buffer, length, "(input string)");
    if (!ok) {
        xkb_compose_table_unref(table);
        return NULL;
    }

    return table;
}

XKB_EXPORT struct xkb_compose_table *
xkb_compose_table_new_from_locale(struct xkb_context *ctx,
                                  const char *locale,
                                  enum xkb_compose_compile_flags flags)
{
    struct xkb_compose_table *table;
    char *path = NULL;
    const char *cpath;
    FILE *file;
    bool ok;

    if (flags & ~(XKB_COMPOSE_COMPILE_NO_FLAGS)) {
        log_err_func(ctx, "unrecognized flags: %#x\n", flags);
        return NULL;
    }

    table = xkb_compose_table_new(ctx, locale, XKB_COMPOSE_FORMAT_TEXT_V1,
                                  flags);
    if (!table)
        return NULL;

    cpath = get_xcomposefile_path();
    if (cpath) {
        file = fopen(cpath, "r");
        if (file)
            goto found_path;
    }

    cpath = path = get_home_xcompose_file_path();
    if (path) {
        file = fopen(path, "r");
        if (file)
            goto found_path;
    }
    free(path);
    path = NULL;

    cpath = path = get_locale_compose_file_path(table->locale);
    if (path) {
        file = fopen(path, "r");
        if (file)
            goto found_path;
    }
    free(path);
    path = NULL;

    log_err(ctx, "couldn't find a Compose file for locale \"%s\"\n", locale);
    xkb_compose_table_unref(table);
    return NULL;

found_path:
    ok = parse_file(table, file, cpath);
    fclose(file);
    if (!ok) {
        xkb_compose_table_unref(table);
        return NULL;
    }

    log_dbg(ctx, "created compose table from locale %s with path %s\n",
            table->locale, path);

    free(path);
    return table;
}
