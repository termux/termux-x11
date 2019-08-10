/*
 * Copyright © 2008-2011 Kristian Høgsberg
 * Copyright © 2011 Intel Corporation
 * Copyright © 2015 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "config.h"
#include "wayland-version.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <getopt.h>
#include <limits.h>
#include <unistd.h>

#if HAVE_LIBXML
#include <libxml/parser.h>

/* Embedded wayland.dtd file, see dtddata.S */
extern char DTD_DATA_begin;
extern int DTD_DATA_len;
#endif

/* Expat must be included after libxml as both want to declare XMLCALL; see
 * the Git commit that 'git blame' for this comment points to for more. */
#include <expat.h>

#include "wayland-util.h"

#define PROGRAM_NAME "wayland-scanner"

static int
usage(int ret)
{
	fprintf(stderr, "usage: %s [OPTION] [client-header|server-header|private-code|public-code]"
		" [input_file output_file]\n", PROGRAM_NAME);
	fprintf(stderr, "\n");
	fprintf(stderr, "Converts XML protocol descriptions supplied on "
			"stdin or input file to client\n"
			"headers, server headers, or protocol marshalling code.\n\n"
			"Use \"public-code\" only if the marshalling code will be public - "
			"aka DSO will export it while other components will be using it.\n"
			"Using \"private-code\" is strongly recommended.\n\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, "    -h,  --help                  display this help and exit.\n"
			"    -v,  --version               print the wayland library version that\n"
			"                                 the scanner was built against.\n"
			"    -c,  --include-core-only     include the core version of the headers,\n"
			"                                 that is e.g. wayland-client-core.h instead\n"
			"                                 of wayland-client.h.\n"
			"    -s,  --strict                exit immediately with an error if DTD\n"
			"                                 verification fails.\n");
	exit(ret);
}

static int
scanner_version(int ret)
{
	fprintf(stderr, "%s %s\n", PROGRAM_NAME, WAYLAND_VERSION);
	exit(ret);
}

static bool
is_dtd_valid(FILE *input, const char *filename)
{
	bool rc = true;
#if HAVE_LIBXML
	xmlParserCtxtPtr ctx = NULL;
	xmlDocPtr doc = NULL;
	xmlDtdPtr dtd = NULL;
	xmlValidCtxtPtr	dtdctx;
	xmlParserInputBufferPtr	buffer;
	int fd = fileno(input);

	dtdctx = xmlNewValidCtxt();
	ctx = xmlNewParserCtxt();
	if (!ctx || !dtdctx)
		abort();

	buffer = xmlParserInputBufferCreateMem(&DTD_DATA_begin,
					       DTD_DATA_len,
					       XML_CHAR_ENCODING_UTF8);
	if (!buffer) {
		fprintf(stderr, "Failed to init buffer for DTD.\n");
		abort();
	}

	dtd = xmlIOParseDTD(NULL, buffer, XML_CHAR_ENCODING_UTF8);
	if (!dtd) {
		fprintf(stderr, "Failed to parse DTD.\n");
		abort();
	}

	doc = xmlCtxtReadFd(ctx, fd, filename, NULL, 0);
	if (!doc) {
		fprintf(stderr, "Failed to read XML\n");
		abort();
	}

	rc = xmlValidateDtd(dtdctx, doc, dtd);
	xmlFreeDoc(doc);
	xmlFreeParserCtxt(ctx);
	xmlFreeDtd(dtd);
	xmlFreeValidCtxt(dtdctx);
	/* xmlIOParseDTD consumes buffer */

	if (lseek(fd, 0, SEEK_SET) != 0) {
		fprintf(stderr, "Failed to reset fd, output would be garbage.\n");
		abort();
	}
#endif
	return rc;
}

#define XML_BUFFER_SIZE 4096

struct location {
	const char *filename;
	int line_number;
};

struct protocol {
	char *name;
	char *uppercase_name;
	struct wl_list interface_list;
	int type_index;
	int null_run_length;
	char *copyright;
	bool core_headers;
};

struct interface {
	struct location loc;
	char *name;
	char *uppercase_name;
	int version;
	int since;
	struct wl_list request_list;
	struct wl_list event_list;
	struct wl_list enumeration_list;
	struct wl_list link;
};

struct message {
	struct location loc;
	char *name;
	char *uppercase_name;
	struct wl_list arg_list;
	struct wl_list link;
	int arg_count;
	int new_id_count;
	int type_index;
	int all_null;
	int destructor;
	int since;
};

enum arg_type {
	NEW_ID,
	INT,
	UNSIGNED,
	FIXED,
	STRING,
	OBJECT,
	ARRAY,
	FD
};

struct arg {
	char *name;
	enum arg_type type;
	int nullable;
	char *interface_name;
	struct wl_list link;
	char *enumeration_name;
};

struct enumeration {
	char *name;
	char *uppercase_name;
	struct wl_list entry_list;
	struct wl_list link;
	bool bitfield;
	int since;
};

struct entry {
	char *name;
	char *uppercase_name;
	char *value;
	int since;
	struct wl_list link;
};

struct parse_context {
	struct location loc;
	XML_Parser parser;
	struct protocol *protocol;
	struct interface *interface;
	struct message *message;
	struct enumeration *enumeration;
	char character_data[8192];
	unsigned int character_data_length;
};

static void *
fail_on_null(void *p)
{
	if (p == NULL) {
		fprintf(stderr, "%s: out of memory\n", PROGRAM_NAME);
		exit(EXIT_FAILURE);
	}

	return p;
}

static void *
zalloc(size_t s)
{
	return calloc(s, 1);
}

static void *
xzalloc(size_t s)
{
	return fail_on_null(zalloc(s));
}

static char *
xstrdup(const char *s)
{
	return fail_on_null(strdup(s));
}

static char *
uppercase_dup(const char *src)
{
	char *u;
	int i;

	u = xstrdup(src);
	for (i = 0; u[i]; i++)
		u[i] = toupper(u[i]);
	u[i] = '\0';

	return u;
}

static const char *indent(int n)
{
	const char *whitespace[] = {
		"\t\t\t\t\t\t\t\t\t\t\t\t",
		"\t\t\t\t\t\t\t\t\t\t\t\t ",
		"\t\t\t\t\t\t\t\t\t\t\t\t  ",
		"\t\t\t\t\t\t\t\t\t\t\t\t   ",
		"\t\t\t\t\t\t\t\t\t\t\t\t    ",
		"\t\t\t\t\t\t\t\t\t\t\t\t     ",
		"\t\t\t\t\t\t\t\t\t\t\t\t      ",
		"\t\t\t\t\t\t\t\t\t\t\t\t       "
	};

	return whitespace[n % 8] + 12 - n / 8;
}

static void
desc_dump(char *desc, const char *fmt, ...) WL_PRINTF(2, 3);

static void
desc_dump(char *desc, const char *fmt, ...)
{
	/*va_list ap;
	char buf[128], hang;
	int col, i, j, k, startcol, newlines;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);

	for (i = 0, col = 0; buf[i] != '*'; i++) {
		if (buf[i] == '\t')
			col = (col + 8) & ~7;
		else
			col++;
	}

	printf("%s", buf);

	if (!desc) {
		printf("(none)\n");
		return;
	}

	startcol = col;
	col += strlen(&buf[i]);
	if (col - startcol > 2)
		hang = '\t';
	else
		hang = ' ';

	for (i = 0; desc[i]; ) {
		k = i;
		newlines = 0;
		while (desc[i] && isspace(desc[i])) {
			if (desc[i] == '\n')
				newlines++;
			i++;
		}
		if (!desc[i])
			break;

		j = i;
		while (desc[i] && !isspace(desc[i]))
			i++;

		if (newlines > 1)
			printf("\n%s*", indent(startcol));
		if (newlines > 1 || col + i - j > 72) {
			printf("\n%s*%c", indent(startcol), hang);
			col = startcol;
		}

		if (col > startcol && k > 0)
			col += printf(" ");
		col += printf("%.*s", i - j, &desc[j]);
	}
	putchar('\n');*/
}

static void __attribute__ ((noreturn))
fail(struct location *loc, const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	fprintf(stderr, "%s:%d: error: ",
		loc->filename, loc->line_number);
	vfprintf(stderr, msg, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(EXIT_FAILURE);
}

static void
warn(struct location *loc, const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	fprintf(stderr, "%s:%d: warning: ",
		loc->filename, loc->line_number);
	vfprintf(stderr, msg, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

static bool
is_nullable_type(struct arg *arg)
{
	switch (arg->type) {
	/* Strings, objects, and arrays are possibly nullable */
	case STRING:
	case OBJECT:
	case NEW_ID:
	case ARRAY:
		return true;
	default:
		return false;
	}
}

static struct message *
create_message(struct location loc, const char *name)
{
	struct message *message;

	message = xzalloc(sizeof *message);
	message->loc = loc;
	message->name = xstrdup(name);
	message->uppercase_name = uppercase_dup(name);
	wl_list_init(&message->arg_list);

	return message;
}

static void
free_arg(struct arg *arg)
{
	free(arg->name);
	free(arg->interface_name);
	free(arg->enumeration_name);
	free(arg);
}

static struct arg *
create_arg(const char *name)
{
	struct arg *arg;

	arg = xzalloc(sizeof *arg);
	arg->name = xstrdup(name);

	return arg;
}

static bool
set_arg_type(struct arg *arg, const char *type)
{
	if (strcmp(type, "int") == 0)
		arg->type = INT;
	else if (strcmp(type, "uint") == 0)
		arg->type = UNSIGNED;
	else if (strcmp(type, "fixed") == 0)
		arg->type = FIXED;
	else if (strcmp(type, "string") == 0)
		arg->type = STRING;
	else if (strcmp(type, "array") == 0)
		arg->type = ARRAY;
	else if (strcmp(type, "fd") == 0)
		arg->type = FD;
	else if (strcmp(type, "new_id") == 0)
		arg->type = NEW_ID;
	else if (strcmp(type, "object") == 0)
		arg->type = OBJECT;
	else
		return false;

	return true;
}

static void
free_message(struct message *message)
{
	struct arg *a, *a_next;

	free(message->name);
	free(message->uppercase_name);

	wl_list_for_each_safe(a, a_next, &message->arg_list, link)
		free_arg(a);

	free(message);
}

static struct enumeration *
create_enumeration(const char *name)
{
	struct enumeration *enumeration;

	enumeration = xzalloc(sizeof *enumeration);
	enumeration->name = xstrdup(name);
	enumeration->uppercase_name = uppercase_dup(name);
	enumeration->since = 1;

	wl_list_init(&enumeration->entry_list);

	return enumeration;
}

static struct entry *
create_entry(const char *name, const char *value)
{
	struct entry *entry;

	entry = xzalloc(sizeof *entry);
	entry->name = xstrdup(name);
	entry->uppercase_name = uppercase_dup(name);
	entry->value = xstrdup(value);

	return entry;
}

static void
free_entry(struct entry *entry)
{
	free(entry->name);
	free(entry->uppercase_name);
	free(entry->value);

	free(entry);
}

static void
free_enumeration(struct enumeration *enumeration)
{
	struct entry *e, *e_next;

	free(enumeration->name);
	free(enumeration->uppercase_name);

	wl_list_for_each_safe(e, e_next, &enumeration->entry_list, link)
		free_entry(e);

	free(enumeration);
}

static struct interface *
create_interface(struct location loc, const char *name, int version)
{
	struct interface *interface;

	interface = xzalloc(sizeof *interface);
	interface->loc = loc;
	interface->name = xstrdup(name);
	interface->uppercase_name = uppercase_dup(name);
	interface->version = version;
	interface->since = 1;
	wl_list_init(&interface->request_list);
	wl_list_init(&interface->event_list);
	wl_list_init(&interface->enumeration_list);

	return interface;
}

static void
free_interface(struct interface *interface)
{
	struct message *m, *next_m;
	struct enumeration *e, *next_e;

	free(interface->name);
	free(interface->uppercase_name);

	wl_list_for_each_safe(m, next_m, &interface->request_list, link)
		free_message(m);
	wl_list_for_each_safe(m, next_m, &interface->event_list, link)
		free_message(m);
	wl_list_for_each_safe(e, next_e, &interface->enumeration_list, link)
		free_enumeration(e);

	free(interface);
}

/* Convert string to unsigned integer
 *
 * Parses a non-negative base-10 number from the given string.  If the
 * specified string is blank, contains non-numerical characters, is out
 * of range, or results in a negative number, -1 is returned to indicate
 * an error.
 *
 * Upon error, this routine does not modify or set errno.
 *
 * Returns -1 on error, or a non-negative integer on success
 */
static int
strtouint(const char *str)
{
	long int ret;
	char *end;
	int prev_errno = errno;

	errno = 0;
	ret = strtol(str, &end, 10);
	if (errno != 0 || end == str || *end != '\0')
		return -1;

	/* check range */
	if (ret < 0 || ret > INT_MAX) {
		return -1;
	}

	errno = prev_errno;
	return (int)ret;
}

static int
version_from_since(struct parse_context *ctx, const char *since)
{
	int version;

	if (since != NULL) {
		version = strtouint(since);
		if (version == -1) {
			fail(&ctx->loc, "invalid integer (%s)\n", since);
		} else if (version > ctx->interface->version) {
			fail(&ctx->loc, "since (%u) larger than version (%u)\n",
			     version, ctx->interface->version);
		}
	} else {
		version = 1;
	}


	return version;
}

static int is_interface_blacklisted(const char *iname) {
	if (!iname) return 0;
	if (
		!strcmp(iname, "wl_display") ||
		//!strcmp(iname, "wl_callback") ||
		!strcmp(iname, "wl_registry") ||
		!strcmp(iname, "wl_shm") ||
		!strcmp(iname, "wl_shm_pool") ||
		0) return 1;
	return 0;
}

static void
start_element(void *data, const char *element_name, const char **atts)
{
	struct parse_context *ctx = data;
	struct interface *interface;
	struct message *message;
	struct arg *arg;
	struct enumeration *enumeration;
	struct entry *entry;
	const char *name = NULL;
	const char *type = NULL;
	const char *interface_name = NULL;
	const char *value = NULL;
	const char *summary = NULL;
	const char *since = NULL;
	const char *allow_null = NULL;
	const char *enumeration_name = NULL;
	const char *bitfield = NULL;
	int i, version = 0;

	ctx->loc.line_number = XML_GetCurrentLineNumber(ctx->parser);
	for (i = 0; atts[i]; i += 2) {
		if (strcmp(atts[i], "name") == 0)
			name = atts[i + 1];
		if (strcmp(atts[i], "version") == 0) {
			version = strtouint(atts[i + 1]);
			if (version == -1)
				fail(&ctx->loc, "wrong version (%s)", atts[i + 1]);
		}
		if (strcmp(atts[i], "type") == 0)
			type = atts[i + 1];
		if (strcmp(atts[i], "value") == 0)
			value = atts[i + 1];
		if (strcmp(atts[i], "interface") == 0)
			interface_name = atts[i + 1];
		if (strcmp(atts[i], "summary") == 0)
			summary = atts[i + 1];
		if (strcmp(atts[i], "since") == 0)
			since = atts[i + 1];
		if (strcmp(atts[i], "allow-null") == 0)
			allow_null = atts[i + 1];
		if (strcmp(atts[i], "enum") == 0)
			enumeration_name = atts[i + 1];
		if (strcmp(atts[i], "bitfield") == 0)
			bitfield = atts[i + 1];
	}

	ctx->character_data_length = 0;
	if (strcmp(element_name, "protocol") == 0) {
		if (name == NULL)
			fail(&ctx->loc, "no protocol name given");

		ctx->protocol->name = xstrdup(name);
		ctx->protocol->uppercase_name = uppercase_dup(name);
	} else if (strcmp(element_name, "interface") == 0) {
		if (name == NULL)
			fail(&ctx->loc, "no interface name given");

		if (version == 0)
			fail(&ctx->loc, "no interface version given");

		interface = create_interface(ctx->loc, name, version);
		ctx->interface = interface;
		if (!is_interface_blacklisted(name)) 
			wl_list_insert(ctx->protocol->interface_list.prev,
			       &interface->link);
	} else if (strcmp(element_name, "request") == 0 ||
		   strcmp(element_name, "event") == 0) {
		if (is_interface_blacklisted(interface_name)) return;
		if (name == NULL)
			fail(&ctx->loc, "no request name given");

		message = create_message(ctx->loc, name);

		if (strcmp(element_name, "request") == 0)
			wl_list_insert(ctx->interface->request_list.prev,
				       &message->link);
		else
			wl_list_insert(ctx->interface->event_list.prev,
				       &message->link);

		if (type != NULL && strcmp(type, "destructor") == 0)
			message->destructor = 1;

		version = version_from_since(ctx, since);

		if (version < ctx->interface->since)
			warn(&ctx->loc, "since version not increasing\n");
		ctx->interface->since = version;
		message->since = version;

		if (strcmp(name, "destroy") == 0 && !message->destructor)
			fail(&ctx->loc, "destroy request should be destructor type");

		ctx->message = message;
	} else if (strcmp(element_name, "arg") == 0) {
		if (is_interface_blacklisted(interface_name)) return;
		if (name == NULL)
			fail(&ctx->loc, "no argument name given");

		arg = create_arg(name);
		if (!set_arg_type(arg, type))
			fail(&ctx->loc, "unknown type (%s)", type);

		switch (arg->type) {
		case NEW_ID:
			ctx->message->new_id_count++;
			/* fallthrough */
		case OBJECT:
			if (interface_name)
				arg->interface_name = xstrdup(interface_name);
			break;
		default:
			if (interface_name != NULL)
				fail(&ctx->loc, "interface attribute not allowed for type %s", type);
			break;
		}

		if (allow_null) {
			if (strcmp(allow_null, "true") == 0)
				arg->nullable = 1;
			else if (strcmp(allow_null, "false") != 0)
				fail(&ctx->loc,
				     "invalid value for allow-null attribute (%s)",
				     allow_null);

			if (!is_nullable_type(arg))
				fail(&ctx->loc,
				     "allow-null is only valid for objects, strings, and arrays");
		}

		if (enumeration_name == NULL || strcmp(enumeration_name, "") == 0)
			arg->enumeration_name = NULL;
		else
			arg->enumeration_name = xstrdup(enumeration_name);

		wl_list_insert(ctx->message->arg_list.prev, &arg->link);
		ctx->message->arg_count++;
	} else if (strcmp(element_name, "enum") == 0) {
		if (is_interface_blacklisted(interface_name)) return;
		if (name == NULL)
			fail(&ctx->loc, "no enum name given");

		enumeration = create_enumeration(name);

		if (bitfield == NULL || strcmp(bitfield, "false") == 0)
			enumeration->bitfield = false;
		else if (strcmp(bitfield, "true") == 0)
			enumeration->bitfield = true;
		else
			fail(&ctx->loc,
			     "invalid value (%s) for bitfield attribute (only true/false are accepted)",
			     bitfield);

		wl_list_insert(ctx->interface->enumeration_list.prev,
			       &enumeration->link);

		ctx->enumeration = enumeration;
	} else if (strcmp(element_name, "entry") == 0) {
		if (is_interface_blacklisted(interface_name)) return;
		if (name == NULL)
			fail(&ctx->loc, "no entry name given");

		entry = create_entry(name, value);
		version = version_from_since(ctx, since);

		if (version < ctx->enumeration->since)
			warn(&ctx->loc, "since version not increasing\n");
		ctx->enumeration->since = version;
		entry->since = version;

		wl_list_insert(ctx->enumeration->entry_list.prev,
			       &entry->link);
	}
}

static struct enumeration *
find_enumeration(struct protocol *protocol,
		 struct interface *interface,
		 char *enum_attribute)
{
	struct interface *i;
	struct enumeration *e;
	char *enum_name;
	uint32_t idx = 0, j;

	for (j = 0; j + 1 < strlen(enum_attribute); j++) {
		if (enum_attribute[j] == '.') {
			idx = j;
		}
	}

	if (idx > 0) {
		enum_name = enum_attribute + idx + 1;

		wl_list_for_each(i, &protocol->interface_list, link)
			if (strncmp(i->name, enum_attribute, idx) == 0)
				wl_list_for_each(e, &i->enumeration_list, link)
					if (strcmp(e->name, enum_name) == 0)
						return e;
	} else if (interface) {
		enum_name = enum_attribute;

		wl_list_for_each(e, &interface->enumeration_list, link)
			if (strcmp(e->name, enum_name) == 0)
				return e;
	}

	return NULL;
}

static void
verify_arguments(struct parse_context *ctx,
		 struct interface *interface,
		 struct wl_list *messages,
		 struct wl_list *enumerations)
{
	struct message *m;
	wl_list_for_each(m, messages, link) {
		struct arg *a;
		wl_list_for_each(a, &m->arg_list, link) {
			struct enumeration *e;

			if (!a->enumeration_name)
				continue;


			e = find_enumeration(ctx->protocol, interface,
					     a->enumeration_name);

			switch (a->type) {
			case INT:
				if (e && e->bitfield)
					fail(&ctx->loc,
					     "bitfield-style enum must only be referenced by uint");
				break;
			case UNSIGNED:
				break;
			default:
				fail(&ctx->loc,
				     "enumeration-style argument has wrong type");
			}
		}
	}

}

static void
end_element(void *data, const XML_Char *name)
{
	struct parse_context *ctx = data;

	if (strcmp(name, "copyright") == 0) {
		ctx->protocol->copyright =
			strndup(ctx->character_data,
				ctx->character_data_length);
	} else if (strcmp(name, "description") == 0) {
	} else if (strcmp(name, "request") == 0 ||
		   strcmp(name, "event") == 0) {
		ctx->message = NULL;
	} else if (strcmp(name, "enum") == 0) {
		if (wl_list_empty(&ctx->enumeration->entry_list)) {
			fail(&ctx->loc, "enumeration %s was empty",
			     ctx->enumeration->name);
		}
		ctx->enumeration = NULL;
	} else if (strcmp(name, "protocol") == 0) {
		struct interface *i;

		wl_list_for_each(i, &ctx->protocol->interface_list, link) {
			verify_arguments(ctx, i, &i->request_list, &i->enumeration_list);
			verify_arguments(ctx, i, &i->event_list, &i->enumeration_list);
		}
	}
}

static void
character_data(void *data, const XML_Char *s, int len)
{
	struct parse_context *ctx = data;

	if (ctx->character_data_length + len > sizeof (ctx->character_data)) {
		fprintf(stderr, "too much character data");
		exit(EXIT_FAILURE);
	    }

	memcpy(ctx->character_data + ctx->character_data_length, s, len);
	ctx->character_data_length += len;
}

static void
emit_type(struct arg *a)
{
	switch (a->type) {
	default:
	case INT:
	case FD:
		printf("int32_t ");
		break;
	case NEW_ID:
	case UNSIGNED:
		printf("uint32_t ");
		break;
	case FIXED:
		printf("wl_fixed_t ");
		break;
	case STRING:
		printf("const char *");
		break;
	case OBJECT:
		printf("struct %s *", a->interface_name);
		break;
	case ARRAY:
		printf("struct wl_array *");
		break;
	}
}

static void
emit_header_classes(struct wl_list *message_list_requests, struct wl_list *message_list_events, struct interface *interface)
{
	struct message *m;
	struct arg *a;
	int n;
	
	if (wl_list_empty(message_list_requests) && wl_list_empty(message_list_events))
		return;

	printf("class %s_t : public wl_resource_t {\n", interface->name);
	printf("public:\n");
	printf("\tvoid init() override;\n");
	
	if (!wl_list_empty(message_list_requests)) {
		wl_list_for_each(m, message_list_requests, link) {

			printf("\tvirtual void request_%s(", m->name);

			n = strlen(m->name) + 35;

			wl_list_for_each(a, &m->arg_list, link) {
				if (&a->link != m->arg_list.next) 
					printf(",\n%s", indent(n));
				if (a->type == OBJECT)
					printf("struct wl_resource *");
				else if (a->type == NEW_ID && a->interface_name == NULL)
					printf("const char *interface, uint32_t version, uint32_t ");
				else
					emit_type(a);

				printf("%s", a->name);
			}

			printf(") = 0;\n");
		}
	}
	if (!wl_list_empty(message_list_events)) {

		wl_list_for_each(m, message_list_events, link) {
			printf("\tvoid send_%s(", m->name);

			wl_list_for_each(a, &m->arg_list, link) {
				if (&a->link != m->arg_list.next) 
					printf(", ");
				switch (a->type) {
				case NEW_ID:
				case OBJECT:
					printf("struct wl_resource *");
					break;
				default:
					emit_type(a);
				}
				printf("%s", a->name);
			}

			printf(");\n");
		}
	}

	printf("};\n\n");
}

static void emit_code_init(struct wl_list *message_list_requests, struct wl_list *message_list_events, struct interface *interface)
{
	printf("void %s_t::init() {\n", interface->name);
	printf("\tinterface = &::%s_interface;\n", interface->name);
	printf("\tversion = %d;\n", interface->version);
	printf("\timplementation = ");
	if (!wl_list_empty(message_list_requests)) printf("&%s_interface_implementation;\n", interface->name);
	else printf("nullptr;\n");
	printf("}\n\n");
}

static void
emit_source_classes(struct wl_list *message_list_requests, struct wl_list *message_list_events, struct interface *interface)
{
	struct message *m;
	struct arg *a;
	int n;
	
	if (!wl_list_empty(message_list_requests)) {
		printf("\n");
		wl_list_for_each(m, message_list_requests, link) {
			printf("static void %s_request_%s(struct wl_client *client, struct wl_resource *resource", interface->name, m->name);

			wl_list_for_each(a, &m->arg_list, link) {
				printf(", ");
				switch (a->type) {
				case OBJECT:
					printf("struct wl_resource *");
					break;
				default:
					emit_type(a);
				}
				printf("%s", a->name);
			}

			printf(") {\n");
			printf("\t%s_t* res = static_cast<%s_t*>(wl_resource_get_user_data(resource));\n", interface->name, interface->name);
			printf("\tif (res == nullptr) return;\n\n");
			printf("\tres->request_%s(", m->name);
			wl_list_for_each(a, &m->arg_list, link) {
				if (&a->link != m->arg_list.next) 
					printf(", ");
				printf("%s", a->name);
			}
			printf(");\n}\n");
		}
	}

	if (!wl_list_empty(message_list_requests)) {
		printf("\n\nstruct %s_interface %s_interface_implementation = {\n", interface->name, interface->name);
		wl_list_for_each(m, message_list_requests, link) {
			printf("\t%s_request_%s", interface->name, m->name);
			if (m->link.next != (message_list_requests))
				printf(",");
			printf("\n");
		}
		printf("};\n");
	}

	if (!wl_list_empty(message_list_events)) {
		printf("\n");
		wl_list_for_each(m, message_list_events, link) {
			printf("void %s_t::send_%s(", interface->name, m->name);

			wl_list_for_each(a, &m->arg_list, link) {
				if (&a->link != m->arg_list.next) 
					printf(", ");
				switch (a->type) {
				case NEW_ID:
				case OBJECT:
					printf("struct wl_resource *");
					break;
				default:
					emit_type(a);
				}
				printf("%s", a->name);
			}

			printf(") {\n");
			printf("\t if (resource == nullptr) return;\n");
			printf("\t %s_send_%s(resource", interface->name, m->name);
			wl_list_for_each(a, &m->arg_list, link) {
				printf(", %s", a->name);
			}
			printf(");\n}\n");
		}
	}
}

static void
emit_structs(struct wl_list *message_list, struct interface *interface)
{
	struct message *m;
	struct arg *a;
	int n;

	if (wl_list_empty(message_list))
		return;

	printf("struct %s_%s {\n", interface->name, "interface");

	wl_list_for_each(m, message_list, link) {
		printf("\tvoid (*%s)(", m->name);

		n = strlen(m->name) + 17;
			printf("struct wl_client *client,\n"
			       "%sstruct wl_resource *resource",
			       indent(n));

		wl_list_for_each(a, &m->arg_list, link) {
			printf(",\n%s", indent(n));

			if (a->type == OBJECT)
				printf("struct wl_resource *");
			else if (a->type == NEW_ID && a->interface_name == NULL)
				printf("void *");

			else if (a->type == NEW_ID)
				printf("struct %s *", a->interface_name);
			else
				emit_type(a);

			printf("%s", a->name);
		}

		printf(");\n");
	}

	printf("};\n\n");
}

static void
emit_types_forward_declarations(struct protocol *protocol,
				struct wl_list *message_list,
				struct wl_array *types)
{
	struct message *m;
	struct arg *a;
	int length;
	char **p;

	wl_list_for_each(m, message_list, link) {
		length = 0;
		m->all_null = 1;
		wl_list_for_each(a, &m->arg_list, link) {
			length++;
			switch (a->type) {
			case NEW_ID:
			case OBJECT:
				if (!a->interface_name)
					continue;

				m->all_null = 0;
				p = fail_on_null(wl_array_add(types, sizeof *p));
				*p = a->interface_name;
				break;
			default:
				break;
			}
		}

		if (m->all_null && length > protocol->null_run_length)
			protocol->null_run_length = length;
	}
}

static void
emit_header(struct protocol *protocol)
{
	struct interface *i, *i_next;
	struct wl_array types;
	char **p, *prev;

	printf("#pragma once\n");
	printf("#include <wayland-server.hpp>\n");

	wl_array_init(&types);
	wl_list_for_each(i, &protocol->interface_list, link) {
		emit_types_forward_declarations(protocol, &i->request_list, &types);
		emit_types_forward_declarations(protocol, &i->event_list, &types);
	}

	wl_list_for_each(i, &protocol->interface_list, link) {
		p = fail_on_null(wl_array_add(&types, sizeof *p));
		*p = i->name;
	}

	wl_list_for_each_safe(i, i_next, &protocol->interface_list, link) {
		emit_header_classes(&i->request_list, &i->event_list, i);
	}
	
	printf("\n");
	wl_list_for_each_safe(i, i_next, &protocol->interface_list, link) {
		printf("struct %s_interface;\n", i->name);
	}
	printf("\n");
	wl_list_for_each_safe(i, i_next, &protocol->interface_list, link) {
		if (!wl_list_empty(&i->request_list))
			printf("extern struct %s_interface %s_interface_implementation;\n", i->name, i->name);
	}
	printf("\n");
}

static void
emit_null_run(struct protocol *protocol)
{
	int i;

	for (i = 0; i < protocol->null_run_length; i++)
		printf("\tNULL,\n");
}

static void
emit_types(struct protocol *protocol, struct wl_list *message_list)
{
	struct message *m;
	struct arg *a;

	wl_list_for_each(m, message_list, link) {
		if (m->all_null) {
			m->type_index = 0;
			continue;
		}

		m->type_index =
			protocol->null_run_length + protocol->type_index;
		protocol->type_index += m->arg_count;

		wl_list_for_each(a, &m->arg_list, link) {
			switch (a->type) {
			case NEW_ID:
			case OBJECT:
				if (a->interface_name)
					printf("\t&%s_interface,\n",
					       a->interface_name);
				else
					printf("\tNULL,\n");
				break;
			default:
				printf("\tNULL,\n");
				break;
			}
		}
	}
}

static void
emit_messages(struct wl_list *message_list,
	      struct interface *interface, const char *suffix)
{
	struct message *m;
	struct arg *a;

	if (wl_list_empty(message_list))
		return;

	printf("static const struct wl_message "
	       "%s_%s[] = {\n",
	       interface->name, suffix);

	wl_list_for_each(m, message_list, link) {
		printf("\t{ \"%s\", \"", m->name);

		if (m->since > 1)
			printf("%d", m->since);

		wl_list_for_each(a, &m->arg_list, link) {
			if (is_nullable_type(a) && a->nullable)
				printf("?");

			switch (a->type) {
			default:
			case INT:
				printf("i");
				break;
			case NEW_ID:
				if (a->interface_name == NULL)
					printf("su");
				printf("n");
				break;
			case UNSIGNED:
				printf("u");
				break;
			case FIXED:
				printf("f");
				break;
			case STRING:
				printf("s");
				break;
			case OBJECT:
				printf("o");
				break;
			case ARRAY:
				printf("a");
				break;
			case FD:
				printf("h");
				break;
			}
		}
		printf("\", types + %d },\n", m->type_index);
	}

	printf("};\n\n");
}


static void
emit_code(struct protocol *protocol)
{
	struct interface *i, *i_next;
	struct wl_array types;
	char **p, *prev;

	printf("#include \"%s.hpp\"\n", protocol->name); 

	wl_array_init(&types);
	wl_list_for_each(i, &protocol->interface_list, link) {
		emit_types_forward_declarations(protocol, &i->request_list, &types);
		emit_types_forward_declarations(protocol, &i->event_list, &types);
	}

	wl_list_for_each_safe(i, i_next, &protocol->interface_list, link) {
		emit_source_classes(&i->request_list, &i->event_list, i);
	}

	wl_list_for_each_safe(i, i_next, &protocol->interface_list, link) {
		emit_code_init(&i->request_list, &i->event_list, i);
	}
}

static void
free_protocol(struct protocol *protocol)
{
	free(protocol->name);
	free(protocol->uppercase_name);
	free(protocol->copyright);
}

int main(int argc, char *argv[])
{
	struct parse_context ctx;
	struct protocol protocol;
	FILE *input = stdin;
	char *input_filename = NULL;
	int len;
	void *buf;
	bool help = false;
	bool core_headers = false;
	bool version = false;
	bool strict = false;
	bool fail = false;
	int opt;
	enum {
		CLIENT_HEADER,
		SERVER_HEADER,
		PRIVATE_CODE,
		PUBLIC_CODE,
		CODE,
	} mode;

	static const struct option options[] = {
		{ "help",              no_argument, NULL, 'h' },
		{ "version",           no_argument, NULL, 'v' },
		{ "include-core-only", no_argument, NULL, 'c' },
		{ "strict",            no_argument, NULL, 's' },
		{ 0,                   0,           NULL, 0 }
	};

	while (1) {
		opt = getopt_long(argc, argv, "hvcs", options, NULL);

		if (opt == -1)
			break;

		switch (opt) {
		case 'h':
			help = true;
			break;
		case 'v':
			version = true;
			break;
		case 'c':
			core_headers = true;
			break;
		case 's':
			strict = true;
			break;
		default:
			fail = true;
			break;
		}
	}

	argv += optind;
	argc -= optind;

	if (help)
		usage(EXIT_SUCCESS);
	else if (version)
		scanner_version(EXIT_SUCCESS);
	else if ((argc != 1 && argc != 3) || fail)
		usage(EXIT_FAILURE);
	else if (strcmp(argv[0], "help") == 0)
		usage(EXIT_SUCCESS);
	else if (strcmp(argv[0], "client-header") == 0)
		mode = CLIENT_HEADER;
	else if (strcmp(argv[0], "server-header") == 0)
		mode = SERVER_HEADER;
	else if (strcmp(argv[0], "private-code") == 0)
		mode = PRIVATE_CODE;
	else if (strcmp(argv[0], "public-code") == 0)
		mode = PUBLIC_CODE;
	else if (strcmp(argv[0], "code") == 0)
		mode = CODE;
	else
		usage(EXIT_FAILURE);

	if (argc == 3) {
		input_filename = argv[1];
		input = fopen(input_filename, "r");
		if (input == NULL) {
			fprintf(stderr, "Could not open input file: %s\n",
				strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	/* initialize protocol structure */
	memset(&protocol, 0, sizeof protocol);
	wl_list_init(&protocol.interface_list);
	protocol.core_headers = core_headers;

	/* initialize context */
	memset(&ctx, 0, sizeof ctx);
	ctx.protocol = &protocol;
	if (input == stdin)
		ctx.loc.filename = "<stdin>";
	else
		ctx.loc.filename = input_filename;

	if (!is_dtd_valid(input, ctx.loc.filename)) {
		fprintf(stderr,
		"*******************************************************\n"
		"*                                                     *\n"
		"* WARNING: XML failed validation against built-in DTD *\n"
		"*                                                     *\n"
		"*******************************************************\n");
		if (strict) {
			fclose(input);
			exit(EXIT_FAILURE);
		}
	}

	/* create XML parser */
	ctx.parser = XML_ParserCreate(NULL);
	XML_SetUserData(ctx.parser, &ctx);
	if (ctx.parser == NULL) {
		fprintf(stderr, "failed to create parser\n");
		fclose(input);
		exit(EXIT_FAILURE);
	}

	XML_SetElementHandler(ctx.parser, start_element, end_element);
	XML_SetCharacterDataHandler(ctx.parser, character_data);

	do {
		buf = XML_GetBuffer(ctx.parser, XML_BUFFER_SIZE);
		len = fread(buf, 1, XML_BUFFER_SIZE, input);
		if (len < 0) {
			fprintf(stderr, "fread: %m\n");
			fclose(input);
			exit(EXIT_FAILURE);
		}
		if (XML_ParseBuffer(ctx.parser, len, len == 0) == 0) {
			fprintf(stderr,
				"Error parsing XML at line %ld col %ld: %s\n",
				XML_GetCurrentLineNumber(ctx.parser),
				XML_GetCurrentColumnNumber(ctx.parser),
				XML_ErrorString(XML_GetErrorCode(ctx.parser)));
			fclose(input);
			exit(EXIT_FAILURE);
		}
	} while (len > 0);

	XML_ParserFree(ctx.parser);
	
	char b[2048];
	sprintf(b, "%s.hpp", protocol.name);
	if (freopen(b, "w", stdout) == NULL) {
		fprintf(stderr, "Could not open output file: %s\n",
			strerror(errno));
		fclose(input);
		exit(EXIT_FAILURE);
	}
	emit_header(&protocol);
	
	sprintf(b, "%s.cpp", protocol.name);
	if (freopen(b, "w", stdout) == NULL) {
		fprintf(stderr, "Could not open output file: %s\n",
			strerror(errno));
		fclose(input);
		exit(EXIT_FAILURE);
	}
	emit_code(&protocol);

	free_protocol(&protocol);
	fclose(input);

	return 0;
}
