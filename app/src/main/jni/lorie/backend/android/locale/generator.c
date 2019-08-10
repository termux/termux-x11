// gcc -o generator generator.c -lxkbcommon

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <execinfo.h>
#include <signal.h>
#include <xkbcommon/xkbcommon.h>
#include <linux/input-event-codes.h>
#include "evdev-keycodes.h"
#include "log.h"

#include "android-keycodes.h"

#define KEYCODE_MIN 8
#define KEYCODE_MAX 255
#define EVDEV_OFFSET 8
#define SYM_LENGTH 7

int android_keycode_to_linux_event_code(int keyCode, int *eventCode, int *shift);

struct keysym {
		int eventCode;
		char normal[SYM_LENGTH];
		char shift[SYM_LENGTH];
};

#define KEYMAPS_LENGTH 1
struct keymap {
	char *name;
	struct keysym keysyms[KEYCODE_MAX - KEYCODE_MIN];
} keymaps[KEYMAPS_LENGTH] = {0};

void handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

#define K(c1, c2) case EVDEV_##c1: return KEY_##c2
int keyCode2eventCode(int kc) {
    int keyCode = kc - KEYCODE_MIN;
	switch(keyCode) {
		K(TLDE, GRAVE);

		K(AE01, 1);
		K(AE02, 2);
		K(AE03, 3);
		K(AE04, 4);
		K(AE05, 5);
		K(AE06, 6);
		K(AE07, 7);
		K(AE08, 8);
		K(AE09, 9);
		K(AE10, 0);
		K(AE11, MINUS);
		K(AE12, EQUAL);
		K(BKSP, BACKSPACE);

		K(TAB, TAB);
		K(AD01, Q);
		K(AD02, W);
		K(AD03, E);
		K(AD04, R);
		K(AD05, T);
		K(AD06, Y);
		K(AD07, U);
		K(AD08, I);
		K(AD09, O);
		K(AD10, P);
		K(AD11, LEFTBRACE);
		K(AD12, RIGHTBRACE);
		//K(BKSL, BACKSLASH); // DUPLICATE
		K(RTRN, ENTER);

		K(CAPS, CAPSLOCK);
		K(AC01, A);
		K(AC02, S);
		K(AC03, D);
		K(AC04, F);
		K(AC05, G);
		K(AC06, H);
		K(AC07, J);
		K(AC08, K);
		K(AC09, L);
		K(AC10, SEMICOLON);
		K(AC11, APOSTROPHE);
		K(AC12, BACKSLASH);

		K(LFSH, LEFTSHIFT);
		K(AB01, Z);
		K(AB02, X);
		K(AB03, C);
		K(AB04, V);
		K(AB05, B);
		K(AB06, N);
		K(AB07, M);
		K(AB08, COMMA);
		K(AB09, DOT);
		K(AB10, SLASH);
		K(RTSH, RIGHTSHIFT);
		default: return KEY_UNKNOWN;
	}
	return KEY_UNKNOWN;
}

struct iterator_args {
	struct keymap *keymap;
	uint8_t symcase;
};
void iterate_normal(struct xkb_keymap *xkb_keymap, xkb_keycode_t key, void *data) {
	struct iterator_args *ita = data;
	if (!ita || !ita->keymap) return;
	
	struct keymap *keymap = ita->keymap;
	struct xkb_state *xkb_state = NULL;
	char *buf = NULL;
	xkb_keysym_t ks = 0;

	xkb_state = xkb_state_new(xkb_keymap);
	if (xkb_state == NULL) {
		LOGE("failed to create xkb_state");
	};
	
	if (ita->symcase) {
		xkb_state_update_key(xkb_state, KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
		buf = keymap->keysyms[key].shift;
	} else {
		buf = keymap->keysyms[key].normal;
	}
	
	ks = xkb_state_key_get_one_sym(xkb_state, key);
	xkb_keysym_to_utf8(ks, buf, SYM_LENGTH);

	xkb_state_unref(xkb_state);
}

struct keymap* parse_keymap(struct xkb_context *ctx, char *name) {
	if (!ctx || !name) return NULL;

	struct xkb_rule_names xkb_names = {0};
	struct xkb_keymap *xkb_keymap = NULL;
	struct iterator_args ita = {0};
	struct keymap *result = NULL;
	
	xkb_names.rules = strdup("evdev");
	xkb_names.model = strdup("pc105");
	xkb_names.layout = strdup(name);
	
	xkb_keymap = xkb_keymap_new_from_names(ctx,  &xkb_names, 0);
	if (xkb_keymap == NULL) {
		LOGE("failed to compile global XKB keymap\n");
		LOGE("  tried rules %s, model %s, layout %s, variant %s, "
			"options %s\n",
			xkb_names.rules, xkb_names.model,
			xkb_names.layout, xkb_names.variant,
			xkb_names.options);
		goto end;
	}
	
	result = calloc(1, sizeof(struct keymap));
	if (!result) {
		LOGE("failed to allocate struct keymap");
		goto end;
	}
	
	result->name = strdup(name);
	
	ita.keymap = result;
	ita.symcase = 0;
	xkb_keymap_key_for_each(xkb_keymap, iterate_normal, &ita);
	
	ita.symcase = 1;
	xkb_keymap_key_for_each(xkb_keymap, iterate_normal, &ita);
	
	for (int i=0; i<(KEYCODE_MAX-KEYCODE_MIN);i++)
		result->keysyms[i].eventCode = keyCode2eventCode(i+KEYCODE_MIN);

end:
	if (xkb_keymap) xkb_keymap_unref(xkb_keymap);
	if (xkb_names.rules) free((void*)xkb_names.rules);
	if (xkb_names.model) free((void*)xkb_names.model);
	if (xkb_names.layout) free((void*)xkb_names.layout);
	return result;
}

void print_header(void) {
	printf(	"#include <stdint.h>\n"
			"#define SYM_LENGTH %d\n"
			"#define KEYCODE_MIN 8\n"
			"#define KEYCODE_MAX 255\n"
			"#define NOSYM {{0}, {0}}\n"
			"struct lorie_keymap {\n"
			"\tchar *name;\n"
			"\tstruct keysym {\n"
			"\t\tchar normal[SYM_LENGTH];\n"
			"\t\tchar shift[SYM_LENGTH];\n"
			"\t} keysyms[KEYCODE_MAX - KEYCODE_MIN];\n"
			"};\n\n",
			SYM_LENGTH
	);
}

void print_sym(char *sym) {
	int i;
	if (strlen(sym)) {
		printf("{");
		for(i=0; i<SYM_LENGTH-1; i++) {
			if (i > 0 && i < SYM_LENGTH) printf(", ");
			printf("%d", (i<strlen(sym)) ? sym[i] : 0);
		}
		printf(", 0"); // terminating symbol
		printf("}");
	} else printf("{0}");
}

void print_keysym(struct keysym *keysym) {
	if (!keysym) return;
	if (strlen(keysym->normal) && strlen(keysym->shift)) {
		printf("\t\t{");
		print_sym(keysym->normal);
		printf(", ");
		print_sym(keysym->shift);
		printf("}");
	} else printf("\t\tNOSYM");
}

int ks_displayable(char *sym) {
	if (strlen(sym) == 0) return 0;
	if (strlen(sym) == 1) {
		switch(sym[0]) {
			case 8:
			case 10:
			case 13:
			case 27:
			case 30:
			case 127:
				return 0;
		}
	}
	return 1;
}

int find_keysym_by_eventcode(struct keymap* keymap, int eventCode) {
	for (int i=0; i<(KEYCODE_MAX-KEYCODE_MIN); i++) {
		if (keymap->keysyms[i].eventCode == eventCode)
			return i;
	}
	return -1;
}

void keymap_print(struct keymap* keymap) {
	if (!keymap) return;
	int i, j, latestEventCode;
	
	printf(	"struct lorie_keymap lorie_keymap_%s = {\n"
			"\t.name = (char*) \"%s\",\n"
			"\t.keysyms = {\n",
			keymap->name, keymap->name
	);
	#if 0
	for (i=0; i<(KEYCODE_MAX-KEYCODE_MIN); i++) {
		print_keysym(&keymap->keysyms[i]);
		if (i!=(KEYCODE_MAX-KEYCODE_MIN-1)) printf(",");
		
		printf(" // keyCode: %d; eventCode: %d;", i + KEYCODE_MIN, keymap->keysyms[i].eventCode);
		if (ks_displayable(i, keymap->keysyms[i].normal)) printf(" normal: \"%s\";", keymap->keysyms[i].normal);
		if (ks_displayable(i, keymap->keysyms[i].shift)) printf(" shift: \"%s\";", keymap->keysyms[i].shift);
		printf("\n");
	}
	#else
	latestEventCode = 0;
	for (i=0; i<(KEYCODE_MAX-KEYCODE_MIN); i++) {
		if (keymap->keysyms[i].eventCode > latestEventCode && 
		keymap->keysyms[i].eventCode != KEY_UNKNOWN &&
		strlen(keymap->keysyms[i].normal) &&
		strlen(keymap->keysyms[i].shift)) // check if it is displayable and known
			latestEventCode = keymap->keysyms[i].eventCode;
	}
	//printf("\n\n\nLatest eventCode = %d\n\n\n\n", latestEventCode);
	for (i=0; i<=latestEventCode; i++) {
		j = find_keysym_by_eventcode(keymap, i);
		struct keysym* k = &keymap->keysyms[j];
		if (j >= 0) {
			print_keysym(k);
			if (j!=(KEYCODE_MAX-KEYCODE_MIN-1)) printf(",");
						
			printf(" // eventCode: %d;", i);
			if (ks_displayable(k->normal)) printf(" normal: \"%s\";", k->normal);
			if (ks_displayable(k->shift)) printf(" shift: \"%s\";", k->shift);
			printf("\n");
		} else printf("\t\tNOSYM, // eventCode: %d\n", i);
	}
	#endif
	printf("\t}\n};\n");
}

int main(void) {
	signal(SIGSEGV, handler);
	int i;
	struct xkb_context *xkb_context = NULL;
	struct xkb_rule_names xkb_names = {0};
	struct xkb_keymap *xkb_keymap = NULL;
	struct keymap* map = NULL;
	//char *keymaps[] = { "us", NULL };
	//char *keymaps[] = { "ru", "il", NULL };
	char *keymaps[] = { "ru", NULL };

	if (xkb_context == NULL) {
		xkb_context = xkb_context_new(0);
		if (xkb_context == NULL) {
			LOGE("failed to create XKB context\n");
			return 1;
		}
	}
	print_header();
	for (i=0; keymaps[i]; i++) {
		map = parse_keymap(xkb_context, keymaps[i]);
		if (!map) {
			LOGE("failed to parse keymap \"us\"");
			return 1;
		}
		keymap_print(map);
	}
	
	printf("\nstruct lorie_keymap *lorie_keymaps[] = {");
	for (i=0; keymaps[i]; i++) {
		printf("&lorie_keymap_%s, ", keymaps[i]);
	}
	printf("NULL};\n\n");
	printf("struct lorie_keymap_android {\n");
	printf("\tint eventCode;\n");
	printf("\tint shift;\n");
	printf("} lorie_keymap_android[] = {\n");
	
	int lastWasNull = 0;
	for (i=0; i<=ANDROID_KEYCODE_LAST; i++) {
		int evCode = 0;
		int shift = 0;
		if (android_keycode_to_linux_event_code(i, &evCode, &shift)) {
			if (lastWasNull) printf("\n");
			printf("\t{%d, %d}%s // keycode %d\n", evCode, shift, (i!=ANDROID_KEYCODE_LAST)?",":"", i);
			lastWasNull = 0;
		} else {
			if (!lastWasNull) printf("\t");
			printf("{0, 0}%s", (i!=ANDROID_KEYCODE_LAST)?", ":"\n");
			lastWasNull = 1;
		}
	}; 
	printf("};\n");
}
