#include <string.h>
#include "keymaps.h"
void get_character_data(char** layout, int *shift, int *ec, char *ch) {
	int i;
	for (i=0; lorie_keymaps[i]; i++) {
		for ((*ec)=0; (*ec)<(KEYCODE_MAX-KEYCODE_MIN); (*ec)++) {
			if (!strcmp(ch, lorie_keymaps[i]->keysyms[*ec].normal)) {
				*layout = lorie_keymaps[i]->name;
				*shift = 0;
				return;
			}
			if (!strcmp(ch, lorie_keymaps[i]->keysyms[*ec].shift)) {
				*layout = lorie_keymaps[i]->name;
				*shift = 1;
				return;
			}
		}
	}
	*ec = 0;
	*shift = 0;
}

void android_keycode_get_eventcode(int kc, int *ec, int *shift) {
	if (lorie_keymap_android[kc].eventCode != 0) {
		*ec = lorie_keymap_android[kc].eventCode;
		*shift = lorie_keymap_android[kc].shift;
	} else {
		*ec = *shift = 0;
	}
};
