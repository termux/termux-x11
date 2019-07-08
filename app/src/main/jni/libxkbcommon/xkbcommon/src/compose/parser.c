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

/******************************************************************

              Copyright 1992 by Oki Technosystems Laboratory, Inc.
              Copyright 1992 by Fuji Xerox Co., Ltd.

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of Oki Technosystems
Laboratory and Fuji Xerox not be used in advertising or publicity
pertaining to distribution of the software without specific, written
prior permission.
Oki Technosystems Laboratory and Fuji Xerox make no representations
about the suitability of this software for any purpose.  It is provided
"as is" without express or implied warranty.

OKI TECHNOSYSTEMS LABORATORY AND FUJI XEROX DISCLAIM ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL OKI TECHNOSYSTEMS
LABORATORY AND FUJI XEROX BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
OR PERFORMANCE OF THIS SOFTWARE.

  Author: Yasuhiro Kawai        Oki Technosystems Laboratory
  Author: Kazunori Nishihara    Fuji Xerox

******************************************************************/

#include <errno.h>

#include "utils.h"
#include "scanner-utils.h"
#include "table.h"
#include "paths.h"
#include "utf8.h"
#include "parser.h"

#define MAX_LHS_LEN 10
#define MAX_INCLUDE_DEPTH 5

#define KEYSYM_FROM_NAME_CACHE_SIZE 8

/*
 * xkb_keysym_from_name() is fairly slow, because for internal reasons
 * it must use strcasecmp().
 * A small cache reduces about 20% from the compilation time of
 * en_US.UTF-8/Compose.
 */
struct keysym_from_name_cache {
    struct {
        char name[64];
        unsigned len;
        xkb_keysym_t keysym;
    } cache[KEYSYM_FROM_NAME_CACHE_SIZE];
    unsigned next;
};

static xkb_keysym_t
cached_keysym_from_name(struct keysym_from_name_cache *cache,
                        const char *name, size_t len)
{
    xkb_keysym_t keysym;

    if (len >= sizeof(cache->cache[0].name))
        return XKB_KEY_NoSymbol;

    for (unsigned i = 0; i < KEYSYM_FROM_NAME_CACHE_SIZE; i++)
        if (cache->cache[i].len == len &&
            memcmp(cache->cache[i].name, name, len) == 0)
            return cache->cache[i].keysym;

    keysym = xkb_keysym_from_name(name, XKB_KEYSYM_NO_FLAGS);
    strcpy(cache->cache[cache->next].name, name);
    cache->cache[cache->next].len = len;
    cache->cache[cache->next].keysym = keysym;
    cache->next = (cache->next + 1) % KEYSYM_FROM_NAME_CACHE_SIZE;
    return keysym;
}

/*
 * Grammar adapted from libX11/modules/im/ximcp/imLcPrs.c.
 * See also the XCompose(5) manpage.
 *
 * FILE          ::= { [PRODUCTION] [COMMENT] "\n" | INCLUDE }
 * INCLUDE       ::= "include" '"' INCLUDE_STRING '"'
 * PRODUCTION    ::= LHS ":" RHS [ COMMENT ]
 * COMMENT       ::= "#" {<any character except null or newline>}
 * LHS           ::= EVENT { EVENT }
 * EVENT         ::= [MODIFIER_LIST] "<" keysym ">"
 * MODIFIER_LIST ::= ("!" {MODIFIER} ) | "None"
 * MODIFIER      ::= ["~"] modifier_name
 * RHS           ::= ( STRING | keysym | STRING keysym )
 * STRING        ::= '"' { CHAR } '"'
 * CHAR          ::= GRAPHIC_CHAR | ESCAPED_CHAR
 * GRAPHIC_CHAR  ::= locale (codeset) dependent code
 * ESCAPED_CHAR  ::= ('\\' | '\"' | OCTAL | HEX )
 * OCTAL         ::= '\' OCTAL_CHAR [OCTAL_CHAR [OCTAL_CHAR]]
 * OCTAL_CHAR    ::= (0|1|2|3|4|5|6|7)
 * HEX           ::= '\' (x|X) HEX_CHAR [HEX_CHAR]]
 * HEX_CHAR      ::= (0|1|2|3|4|5|6|7|8|9|A|B|C|D|E|F|a|b|c|d|e|f)
 *
 * INCLUDE_STRING is a filesystem path, with the following %-expansions:
 *     %% - '%'.
 *     %H - The user's home directory (the $HOME environment variable).
 *     %L - The name of the locale specific Compose file (e.g.,
 *          "/usr/share/X11/locale/<localename>/Compose").
 *     %S - The name of the system directory for Compose files (e.g.,
 *          "/usr/share/X11/locale").
 */

enum rules_token {
    TOK_END_OF_FILE = 0,
    TOK_END_OF_LINE,
    TOK_INCLUDE,
    TOK_INCLUDE_STRING,
    TOK_LHS_KEYSYM,
    TOK_COLON,
    TOK_BANG,
    TOK_TILDE,
    TOK_STRING,
    TOK_IDENT,
    TOK_ERROR
};

/* Values returned with some tokens, like yylval. */
union lvalue {
    struct {
        /* Still \0-terminated. */
        const char *str;
        size_t len;
    } string;
};

static enum rules_token
lex(struct scanner *s, union lvalue *val)
{
skip_more_whitespace_and_comments:
    /* Skip spaces. */
    while (is_space(peek(s)))
        if (next(s) == '\n')
            return TOK_END_OF_LINE;

    /* Skip comments. */
    if (chr(s, '#')) {
        skip_to_eol(s);
        goto skip_more_whitespace_and_comments;
    }

    /* See if we're done. */
    if (eof(s)) return TOK_END_OF_FILE;

    /* New token. */
    s->token_line = s->line;
    s->token_column = s->column;
    s->buf_pos = 0;

    /* LHS Keysym. */
    if (chr(s, '<')) {
        while (peek(s) != '>' && !eol(s))
            buf_append(s, next(s));
        if (!chr(s, '>')) {
            scanner_err(s, "unterminated keysym literal");
            return TOK_ERROR;
        }
        if (!buf_append(s, '\0')) {
            scanner_err(s, "keysym literal is too long");
            return TOK_ERROR;
        }
        val->string.str = s->buf;
        val->string.len = s->buf_pos;
        return TOK_LHS_KEYSYM;
    }

    /* Colon. */
    if (chr(s, ':'))
        return TOK_COLON;
    if (chr(s, '!'))
        return TOK_BANG;
    if (chr(s, '~'))
        return TOK_TILDE;

    /* String literal. */
    if (chr(s, '\"')) {
        while (!eof(s) && !eol(s) && peek(s) != '\"') {
            if (chr(s, '\\')) {
                uint8_t o;
                if (chr(s, '\\')) {
                    buf_append(s, '\\');
                }
                else if (chr(s, '"')) {
                    buf_append(s, '"');
                }
                else if (chr(s, 'x') || chr(s, 'X')) {
                    if (hex(s, &o))
                        buf_append(s, (char) o);
                    else
                        scanner_warn(s, "illegal hexadecimal escape sequence in string literal");
                }
                else if (oct(s, &o)) {
                    buf_append(s, (char) o);
                }
                else {
                    scanner_warn(s, "unknown escape sequence (%c) in string literal", peek(s));
                    /* Ignore. */
                }
            } else {
                buf_append(s, next(s));
            }
        }
        if (!chr(s, '\"')) {
            scanner_err(s, "unterminated string literal");
            return TOK_ERROR;
        }
        if (!buf_append(s, '\0')) {
            scanner_err(s, "string literal is too long");
            return TOK_ERROR;
        }
        if (!is_valid_utf8(s->buf, s->buf_pos - 1)) {
            scanner_err(s, "string literal is not a valid UTF-8 string");
            return TOK_ERROR;
        }
        val->string.str = s->buf;
        val->string.len = s->buf_pos;
        return TOK_STRING;
    }

    /* Identifier or include. */
    if (is_alpha(peek(s)) || peek(s) == '_') {
        s->buf_pos = 0;
        while (is_alnum(peek(s)) || peek(s) == '_')
            buf_append(s, next(s));
        if (!buf_append(s, '\0')) {
            scanner_err(s, "identifier is too long");
            return TOK_ERROR;
        }

        if (streq(s->buf, "include"))
            return TOK_INCLUDE;

        val->string.str = s->buf;
        val->string.len = s->buf_pos;
        return TOK_IDENT;
    }

    /* Discard rest of line. */
    skip_to_eol(s);

    scanner_err(s, "unrecognized token");
    return TOK_ERROR;
}

static enum rules_token
lex_include_string(struct scanner *s, struct xkb_compose_table *table,
                   union lvalue *val_out)
{
    while (is_space(peek(s)))
        if (next(s) == '\n')
            return TOK_END_OF_LINE;

    s->token_line = s->line;
    s->token_column = s->column;
    s->buf_pos = 0;

    if (!chr(s, '\"')) {
        scanner_err(s, "include statement must be followed by a path");
        return TOK_ERROR;
    }

    while (!eof(s) && !eol(s) && peek(s) != '\"') {
        if (chr(s, '%')) {
            if (chr(s, '%')) {
                buf_append(s, '%');
            }
            else if (chr(s, 'H')) {
                const char *home = secure_getenv("HOME");
                if (!home) {
                    scanner_err(s, "%%H was used in an include statement, but the HOME environment variable is not set");
                    return TOK_ERROR;
                }
                if (!buf_appends(s, home)) {
                    scanner_err(s, "include path after expanding %%H is too long");
                    return TOK_ERROR;
                }
            }
            else if (chr(s, 'L')) {
                char *path = get_locale_compose_file_path(table->locale);
                if (!path) {
                    scanner_err(s, "failed to expand %%L to the locale Compose file");
                    return TOK_ERROR;
                }
                if (!buf_appends(s, path)) {
                    free(path);
                    scanner_err(s, "include path after expanding %%L is too long");
                    return TOK_ERROR;
                }
                free(path);
            }
            else if (chr(s, 'S')) {
                const char *xlocaledir = get_xlocaledir_path();
                if (!buf_appends(s, xlocaledir)) {
                    scanner_err(s, "include path after expanding %%S is too long");
                    return TOK_ERROR;
                }
            }
            else {
                scanner_err(s, "unknown %% format (%c) in include statement", peek(s));
                return TOK_ERROR;
            }
        } else {
            buf_append(s, next(s));
        }
    }
    if (!chr(s, '\"')) {
        scanner_err(s, "unterminated include statement");
        return TOK_ERROR;
    }
    if (!buf_append(s, '\0')) {
        scanner_err(s, "include path is too long");
        return TOK_ERROR;
    }
    val_out->string.str = s->buf;
    val_out->string.len = s->buf_pos;
    return TOK_INCLUDE_STRING;
}

struct production {
    xkb_keysym_t lhs[MAX_LHS_LEN];
    unsigned int len;
    xkb_keysym_t keysym;
    char string[256];
    bool has_keysym;
    bool has_string;

    xkb_mod_mask_t mods;
    xkb_mod_mask_t modmask;
};

static uint32_t
add_node(struct xkb_compose_table *table, xkb_keysym_t keysym)
{
    struct compose_node new = {
        .keysym = keysym,
        .next = 0,
        .is_leaf = true,
    };
    darray_append(table->nodes, new);
    return darray_size(table->nodes) - 1;
}

static void
add_production(struct xkb_compose_table *table, struct scanner *s,
               const struct production *production)
{
    unsigned lhs_pos;
    uint32_t curr;
    struct compose_node *node;

    curr = 0;
    node = &darray_item(table->nodes, curr);

    /*
     * Insert the sequence to the trie, creating new nodes as needed.
     *
     * TODO: This can be sped up a bit by first trying the path that the
     * previous production took, and only then doing the linear search
     * through the trie levels.  This will work because sequences in the
     * Compose files are often clustered by a common prefix; especially
     * in the 1st and 2nd keysyms, which is where the largest variation
     * (thus, longest search) is.
     */
    for (lhs_pos = 0; lhs_pos < production->len; lhs_pos++) {
        while (production->lhs[lhs_pos] != node->keysym) {
            if (node->next == 0) {
                uint32_t next = add_node(table, production->lhs[lhs_pos]);
                /* Refetch since add_node could have realloc()ed. */
                node = &darray_item(table->nodes, curr);
                node->next = next;
            }

            curr = node->next;
            node = &darray_item(table->nodes, curr);
        }

        if (lhs_pos + 1 == production->len)
            break;

        if (node->is_leaf) {
            if (node->u.leaf.utf8 != 0 ||
                node->u.leaf.keysym != XKB_KEY_NoSymbol) {
                scanner_warn(s, "a sequence already exists which is a prefix of this sequence; overriding");
                node->u.leaf.utf8 = 0;
                node->u.leaf.keysym = XKB_KEY_NoSymbol;
            }

            {
                uint32_t successor = add_node(table, production->lhs[lhs_pos + 1]);
                /* Refetch since add_node could have realloc()ed. */
                node = &darray_item(table->nodes, curr);
                node->is_leaf = false;
                node->u.successor = successor;
            }
        }

        curr = node->u.successor;
        node = &darray_item(table->nodes, curr);
    }

    if (!node->is_leaf) {
        scanner_warn(s, "this compose sequence is a prefix of another; skipping line");
        return;
    }

    if (node->u.leaf.utf8 != 0 || node->u.leaf.keysym != XKB_KEY_NoSymbol) {
        if (streq(&darray_item(table->utf8, node->u.leaf.utf8),
                  production->string) &&
            node->u.leaf.keysym == production->keysym) {
            scanner_warn(s, "this compose sequence is a duplicate of another; skipping line");
            return;
        }
        scanner_warn(s, "this compose sequence already exists; overriding");
    }

    if (production->has_string) {
        node->u.leaf.utf8 = darray_size(table->utf8);
        darray_append_items(table->utf8, production->string,
                            strlen(production->string) + 1);
    }
    if (production->has_keysym) {
        node->u.leaf.keysym = production->keysym;
    }
}

static xkb_mod_index_t
resolve_modifier(const char *name)
{
    static const struct {
        const char *name;
        xkb_mod_index_t mod;
    } mods[] = {
        { "Shift", 0 },
        { "Ctrl", 2 },
        { "Alt", 3 },
        { "Meta", 3 },
        { "Lock", 1 },
        { "Caps", 1 },
    };

    for (unsigned i = 0; i < ARRAY_SIZE(mods); i++)
        if (streq(name, mods[i].name))
            return mods[i].mod;

    return XKB_MOD_INVALID;
}

static bool
parse(struct xkb_compose_table *table, struct scanner *s,
      unsigned include_depth);

static bool
do_include(struct xkb_compose_table *table, struct scanner *s,
           const char *path, unsigned include_depth)
{
    FILE *file;
    bool ok;
    const char *string;
    size_t size;
    struct scanner new_s;

    if (include_depth >= MAX_INCLUDE_DEPTH) {
        scanner_err(s, "maximum include depth (%d) exceeded; maybe there is an include loop?",
                    MAX_INCLUDE_DEPTH);
        return false;
    }

    file = fopen(path, "r");
    if (!file) {
        scanner_err(s, "failed to open included Compose file \"%s\": %s",
                    path, strerror(errno));
        return false;
    }

    ok = map_file(file, &string, &size);
    if (!ok) {
        scanner_err(s, "failed to read included Compose file \"%s\": %s",
                    path, strerror(errno));
        goto err_file;
    }

    scanner_init(&new_s, table->ctx, string, size, path, s->priv);

    ok = parse(table, &new_s, include_depth + 1);
    if (!ok)
        goto err_unmap;

err_unmap:
    unmap_file(string, size);
err_file:
    fclose(file);
    return ok;
}

static bool
parse(struct xkb_compose_table *table, struct scanner *s,
      unsigned include_depth)
{
    enum rules_token tok;
    union lvalue val;
    struct keysym_from_name_cache *cache = s->priv;
    xkb_keysym_t keysym;
    struct production production;
    enum { MAX_ERRORS = 10 };
    int num_errors = 0;

initial:
    production.len = 0;
    production.has_keysym = false;
    production.has_string = false;
    production.mods = 0;
    production.modmask = 0;

    /* fallthrough */

initial_eol:
    switch (tok = lex(s, &val)) {
    case TOK_END_OF_LINE:
        goto initial_eol;
    case TOK_END_OF_FILE:
        goto finished;
    case TOK_INCLUDE:
        goto include;
    default:
        goto lhs_tok;
    }

include:
    switch (tok = lex_include_string(s, table, &val)) {
    case TOK_INCLUDE_STRING:
        goto include_eol;
    default:
        goto unexpected;
    }

include_eol:
    switch (tok = lex(s, &val)) {
    case TOK_END_OF_LINE:
        if (!do_include(table, s, val.string.str, include_depth))
            goto fail;
        goto initial;
    default:
        goto unexpected;
    }

lhs:
    tok = lex(s, &val);
lhs_tok:
    switch (tok) {
    case TOK_COLON:
        if (production.len <= 0) {
            scanner_warn(s, "expected at least one keysym on left-hand side; skipping line");
            goto skip;
        }
        goto rhs;
    case TOK_IDENT:
        if (!streq(val.string.str, "None")) {
            scanner_err(s, "unrecognized identifier \"%s\"", val.string.str);
            goto error;
        }
        production.mods = 0;
        /* XXX Should only include the mods in resolve_mods(). */
        production.modmask = 0xff;
        goto lhs_keysym;
    case TOK_BANG:
        goto lhs_mod_list;
    default:
        goto lhs_keysym_tok;
    }

lhs_keysym:
    tok = lex(s, &val);
lhs_keysym_tok:
    switch (tok) {
    case TOK_LHS_KEYSYM:
        keysym = cached_keysym_from_name(cache, val.string.str, val.string.len);
        if (keysym == XKB_KEY_NoSymbol) {
            scanner_err(s, "unrecognized keysym \"%s\" on left-hand side",
                        val.string.str);
            goto error;
        }
        if (production.len + 1 > MAX_LHS_LEN) {
            scanner_warn(s, "too many keysyms (%d) on left-hand side; skipping line",
                         MAX_LHS_LEN + 1);
            goto skip;
        }
        production.lhs[production.len++] = keysym;
        production.mods = 0;
        production.modmask = 0;
        goto lhs;
    default:
        goto unexpected;
    }

lhs_mod_list: {
        bool tilde = false;
        xkb_mod_index_t mod;

        tok = lex(s, &val);
        if (tok == TOK_TILDE) {
            tilde = true;
            tok = lex(s, &val);
        }

        if (tok != TOK_IDENT) {
            if (tilde || production.modmask == 0)
                goto unexpected;
            goto lhs_keysym_tok;
        }

        mod = resolve_modifier(val.string.str);
        if (mod == XKB_MOD_INVALID) {
            scanner_err(s, "unrecognized modifier \"%s\"",
                        val.string.str);
            goto error;
        }

        production.modmask |= 1 << mod;
        if (tilde)
            production.mods &= ~(1 << mod);
        else
            production.mods |= 1 << mod;

        goto lhs_mod_list;
    }

rhs:
    switch (tok = lex(s, &val)) {
    case TOK_STRING:
        if (production.has_string) {
            scanner_warn(s, "right-hand side can have at most one string; skipping line");
            goto skip;
        }
        if (val.string.len <= 0) {
            scanner_warn(s, "right-hand side string must not be empty; skipping line");
            goto skip;
        }
        if (val.string.len >= sizeof(production.string)) {
            scanner_warn(s, "right-hand side string is too long; skipping line");
            goto skip;
        }
        strcpy(production.string, val.string.str);
        production.has_string = true;
        goto rhs;
    case TOK_IDENT:
        keysym = cached_keysym_from_name(cache, val.string.str, val.string.len);
        if (keysym == XKB_KEY_NoSymbol) {
            scanner_err(s, "unrecognized keysym \"%s\" on right-hand side",
                        val.string.str);
            goto error;
        }
        if (production.has_keysym) {
            scanner_warn(s, "right-hand side can have at most one keysym; skipping line");
            goto skip;
        }
        production.keysym = keysym;
        production.has_keysym = true;
    case TOK_END_OF_LINE:
        if (!production.has_string && !production.has_keysym) {
            scanner_warn(s, "right-hand side must have at least one of string or keysym; skipping line");
            goto skip;
        }
        add_production(table, s, &production);
        goto initial;
    default:
        goto unexpected;
    }

unexpected:
    if (tok != TOK_ERROR)
        scanner_err(s, "unexpected token");
error:
    num_errors++;
    if (num_errors <= MAX_ERRORS)
        goto skip;

    scanner_err(s, "too many errors");
    goto fail;

fail:
    scanner_err(s, "failed to parse file");
    return false;

skip:
    while (tok != TOK_END_OF_LINE && tok != TOK_END_OF_FILE)
        tok = lex(s, &val);
    goto initial;

finished:
    return true;
}

bool
parse_string(struct xkb_compose_table *table, const char *string, size_t len,
             const char *file_name)
{
    struct scanner s;
    struct keysym_from_name_cache cache;
    memset(&cache, 0, sizeof(cache));
    scanner_init(&s, table->ctx, string, len, file_name, &cache);
    if (!parse(table, &s, 0))
        return false;
    /* Maybe the allocator can use the excess space. */
    darray_shrink(table->nodes);
    darray_shrink(table->utf8);
    return true;
}

bool
parse_file(struct xkb_compose_table *table, FILE *file, const char *file_name)
{
    bool ok;
    const char *string;
    size_t size;

    ok = map_file(file, &string, &size);
    if (!ok) {
        log_err(table->ctx, "Couldn't read Compose file %s: %s\n",
                file_name, strerror(errno));
        return false;
    }

    ok = parse_string(table, string, size, file_name);
    unmap_file(string, size);
    return ok;
}
