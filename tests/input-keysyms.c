#include <assert.h>
#include "../lorie/src/main/cpp/lorie/InputXKB.c"

int main(void) {
    XkbDescRec xkb = {0};
    XkbClientMapRec map = {0};
    XkbSymMapRec symmap[256] = {0};
    KeySym syms[256] = {0};
    xkb.map = &map;
    map.key_sym_map = symmap;
    map.syms = syms;
    symmap[8].group_info = symmap[9].group_info = 1;
    symmap[8].offset = 8;
    symmap[9].offset = 9;
    syms[8] = 0x1006b63;
    syms[9] = 0x1005728;
    saveAddedKeysym(8, syms[8]);
    saveAddedKeysym(9, syms[9]);
    /* Simulate a keymap reset followed by assigning the same symbol again. */
    saveAddedKeysym(8, syms[8]);
    int count = 0;
    AddedKeySym *item;
    xorg_list_for_each_entry(item, &addedKeysyms, entry) count++;
    assert(count == 2);
    assert(getReusableKeycode(&xkb) == 9);
    assert(getReusableKeycode(&xkb) == 8);
    assert(getReusableKeycode(&xkb) == 0);
    /* Reassigning a different symbol also updates the existing record. */
    saveAddedKeysym(8, 0x1004e2d);
    syms[8] = 0x1006587;
    saveAddedKeysym(8, syms[8]);
    assert(getReusableKeycode(&xkb) == 8);
    assert(getReusableKeycode(&xkb) == 0);
    puts("PASS: one record per keycode; oldest live key evicted after reassignment");
    return 0;
}
