# The XKB keymap text format, V1

This document describes the `XKB_KEYMAP_FORMAT_TEXT_V1` keymap format,
as implemented by libxkbcommon.

NOTE: This document is ever incomplete. Some additional resources are:

- [Ivan Pascal's XKB documentation](https://web.archive.org/web/20190724015820/http://pascal.tsu.ru/en/xkb/)
- [An Unreliable Guide to XKB Configuration](https://www.charvolant.org/doug/xkb/html/index.html)
- [ArchWiki XKB page](https://wiki.archlinux.org/index.php/X_keyboard_extension)

A keymap consists of a single top-level `xkb_keymap` block, underwhich
are nested the following sections.


## The `xkb_keycodes` section

This is the simplest section type, and is the first one to be
compiled. The purpose of this is mostly to map between the
hardware/evdev scancodes and xkb keycodes. Each key is given a name
by which it can be referred to later, e.g. in the symbols section.

### Keycode statements

Statements of the form:

    <TLDE> = 49;
    <AE01> = 10;

The above would let 49 and 10 be valid keycodes in the keymap, and
assign them the names `TLDE` and `AE01` respectively. The format
`<WXYZ>` is always used to refer to a key by name.

[The naming convention `<AE01>` just denotes the position of the key
in the main alphanumeric section of a standard QWERTY keyboard, with
the two letters specifying the row and the two digits specifying the
column, from the bottom left.]

In the common case this just maps to the evdev scancodes from
`/usr/include/linux/input.h`, e.g. the following definitions:

     #define KEY_GRAVE            41
     #define KEY_1                2

correspond to the ones above. Similar definitions appear in the
xf86-input-keyboard driver. Note that in all current keymaps there's a
constant offset of 8 (for historical reasons).

If there's a conflict, like the same name given to different keycodes,
or same keycode given different names, it is resolved according to the
merge mode which applies to the definitions.

### Alias statements

Statements of the form:

    alias <MENU> = <COMP>;

Allows to refer to a previously defined key (here `<COMP>`) by another
name (here `<MENU>`). Conflicts are handled similarly to keycode
statements.

### LED name statements

Statements of the form:

    indicator 1 = "Caps Lock";
    indicator 2 = "Num Lock";
    indicator 3 = "Scroll Lock";

Assigns a name to the keyboard LED (AKA indicator) with the given
index. The LED may be referred by this name later in the compat
section and by the user.


## The `xkb_types` section

This section is the second to be processed, after `xkb_keycodes`.
However, it is completely independent and could have been the first to
be processed (it does not refer to specific keys as specified in the
`xkb_keycodes` section).

This section defines key types, which, given a key and a keyboard
state (i.e. modifier state and group), determine the shift level to be
used in translating the key to keysyms. These types are assigned to
each group in each key, in the `xkb_symbols` section.

Key types are called this way because, in a way, they really describe
the "type" of the key (or more correctly, a specific group of the
key). For example, an ordinary keymap will provide a type called
`KEYPAD`, which consists of two levels, with the second level being
chosen according to the state of the Num Lock (or Shift) modifiers.
Another example is a type called `ONE_LEVEL`, which is usually
assigned to keys such as Escape; these have just one level and are not
affected by the modifier state. Yet more common examples are
`TWO_LEVEL` (with Shift choosing the second level), `ALPHABETIC`
(where Caps Lock may also choose the second level), etc.

### Type definitions

Statements of the form:

    type "FOUR_LEVEL" { ... }

The above would create a new type named `FOUR_LEVEL`.
The body of the definition may include statements of the following
forms:

#### `level_name` statements

    level_name[Level1] = "Base";

Mandatory for each level in the type.

Gives each level in this type a descriptive name. It isn't used
for anything.

Note: A level may be specified as Level[1-8] or just a number (can
be more than 8).

#### `modifiers` statement

    modifiers = Shift+Lock+LevelThree;

Mandatory, should be specified only once.

A mask of real and virtual modifiers. These are the only modifiers
being considered when matching the modifier state against the type.
The other modifiers, whether active or not, are masked out in the
calculation.

#### `map` entry statements

    map[Shift+LevelThree] = Level4;

Should have at least as many mappings as there are levels in the type.

If the active modifiers, masked with the type's modifiers (as stated
above), match (i.e. equal) the modifiers inside the `map[]` statement,
then the level in the right hand side is chosen. For example, in the
above, if in the current keyboard state the `Shift` and `LevelThree`
modifiers are active, while the `Lock` modifier is not, then the
keysym(s) in the 4th level of the group will be returned to the user.

#### `preserve` statements

    map[Shift+Lock+LevelThree] = Level5;
    preserve[Shift+Lock+LevelThree] = Lock;

When a key type is used for keysym translation, its modifiers are said
to be "consumed". For example, in a simple US keymap, the "g" "g" key
is assigned an ordinary `ALPHABETIC` key type, whose modifiers are
Shift and Lock; then for the "g" key, these two modifiers are consumed
by the translation. This information is relevant for applications
which further process the modifiers, since by then the consumed
modifiers have already "done their part" and should be masked out.

However, sometimes even if a modifier had already affected the key
translation through the type, it should *not* be reported as consumed,
for various reasons. In this case, a `preserve[]` statement can be
used to augment the map entry. The modifiers inside the square
brackets should match one of the map[] statements in the type (if
there is no matching map entry, one mapping to Level1 is implicitly
added). The right hand side should consists of modifiers from the
type's modifiers; these modifiers are then "preserved" and not
reported as consumed.


## The `xkb_compat` section

This section is the third to be processed, after `xkb_keycodes` and
`xkb_types`.

### Interpret statements

Statements of the form:

    interpret Num_Lock+Any { ... }
    interpret Shift_Lock+AnyOf(Shift+Lock) { ... }

The `xkb_symbols` section (see below) allows the keymap author to
perform, among other things, the following things for each key:

- Bind an action, like SetMods or LockGroup, to the key. Actions, like
  symbols, are specified for each level of each group in the key
  separately.

- Add a virtual modifier to the key's virtual modifier mapping
  (vmodmap).

- Specify whether the key should repeat or not.

However, doing this for each key (or level) is tedious and inflexible.
Interpret's are a mechanism to apply these settings to a bunch of
keys/levels at once.

Each interpret specifies a condition by which it attaches to certain
levels. The condition consists of two parts:

- A keysym. If the level has a different (or more than one) keysym,
  the match fails. Leaving out the keysym is equivalent to using the
  `NoSymbol` keysym, which always matches successfully.

- A modifier predicate. The predicate consists of a matching operation
  and a mask of (real) modifiers. The modifiers are matched against
  the key's modifier map (modmap). The matching operation can be one
  of the following:

  * `AnyOfOrNone` - The modmap must either be empty or include at
    least one of the specified modifiers.
  * `AnyOf` - The modmap must include at least one of the specified
    modifiers.
  * `NoneOf` - The modmap must not include any of the specified
    modifiers.
  * `AllOf` - The modmap must include all of the specified modifiers
    (but may include others as well).
  * `Exactly` - The modmap must be exactly the same as the specified
    modifiers.

  Leaving out the predicate is equivalent to using `AnyOfOrNone` while
  specifying all modifiers. Leaving out just the matching condition is
  equivalent to using `Exactly`.

An interpret may also include `useModMapMods = level1;` - see below.

If a level fulfils the conditions of several interprets, only the
most specific one is used:

- A specific keysym will always match before a generic `NoSymbol`
  condition.

- If the keysyms are the same, the interpret with the more specific
  matching operation is used. The above list is sorted from least to
  most specific.

- If both the keysyms and the matching operations are the same (but the
  modifiers are different), the first interpret is used.

As described above, once an interpret "attaches" to a level, it can bind
an action to that level, add one virtual modifier to the key's vmodmap,
or set the key's repeat setting. You should note the following:

- The key repeat is a property of the entire key; it is not
  level-specific. In order to avoid confusion, it is only inspected
  for the first level of the first group; the interpret's repeat
  setting is ignored when applied to other levels.

- If one of the above fields was set directly for a key in
  `xkb_symbols`, the explicit setting takes precedence over the
  interpret.

The body of the statement may include statements of the following
forms (all of which are optional):

#### `useModMapMods` statement

    useModMapMods = level1;

When set to `level1`, the interpret will only match levels which are
the first level of the first group of the keys. This can be useful in
conjunction with e.g. a `virtualModifier` statement.

#### `action` statement

    action = LockMods(modifiers=NumLock);

Bind this action to the matching levels.

#### `virtualModifier` statement

    virtualModifier = NumLock;

Add this virtual modifier to the key's vmodmap. The given virtual
modifier must be declared at the top level of the file with a
`virtual_modifiers` statement, e.g.:

    virtual_modifiers NumLock;

#### `repeat` statement

    repeat = True;

Set whether the key should repeat or not. Must be a boolean value.

### LED map statements

Statements of the form:

    indicator "Shift Lock" { ... }

This statement specifies the behavior and binding of the LED (AKA
indicator) with the given name ("Shift Lock" above). The name should
have been declared previously in the `xkb_keycodes` section (see LED
name statement), and given an index there. If it wasn't, it is created
with the next free index.

The body of the statement describes the conditions of the keyboard
state which will cause the LED to be lit. It may include the following
statements:

#### `modifiers` statement

    modifiers = ScrollLock;

If the given modifiers are in the required state (see below), the
LED is lit.

#### `whichModState` statement

    whichModState = Latched+Locked;

Can be any combination of:

* `base`, `latched`, `locked`, `effective`
* `any` (i.e. all of the above)
* `none` (i.e. none of the above)
* `compat` (legacy value, treated as effective)

This will cause the respective portion of the modifier state (see
`struct xkb_state`) to be matched against the modifiers given in the
`modifiers` statement.

Here's a simple example:

indicator "Num Lock" {
    modifiers = NumLock;
    whichModState = Locked;
};

Whenever the NumLock modifier is locked, the Num Lock LED will light
up.

#### `groups` statement

    groups = All - group1;

If the given groups are in the required state (see below), the LED is
lit.

#### `whichGroupState` statement

    whichGroupState = Effective;

Can be any combination of:

* `base`, `latched`, `locked`, `effective`
* `any` (i.e. all of the above)
* `none` (i.e. none of the above)

This will cause the respective portion of the group state (see
`struct xkb_state`) to be matched against the groups given in the
`groups` statement.

Note: the above conditions are disjunctive, i.e. if any of them are
satisfied the LED is lit.


## The `xkb_symbols` section

NOTE: The documentation of this section is incomplete.

This section is the fourth to be processed, after `xkb_keycodes`, `xkb_types`
and `xkb_compat`.

Statements of the form:

    xkb_symbols "basic" {
        ...
    }

Declare a symbols map named `basic`. Statements inside the curly braces only
affect the symbols map.

A map can have various flags applied to it above the statement, separated by
whitespace:

    partial alphanumeric_keys
    xkb_symbols "basic" {
        ...
    }

The possible flags are:

  * `partial` - Indicates that the map doesn't cover a complete keyboard.
  * `default` - Marks the symbol map as the default map in the file when no
    explicit map is specified. If no map is marked as a default, the first map
    in the file is the default.
  * `hidden` - Variant that can only be used internally
  * `alphanumeric_keys` - Indicates that the map contains alphanumeric keys
  * `modifier_keys` - Indicates that the map contains modifier keys
  * `keypad_keys` - Indicates that the map contains keypad keys
  * `function_keys` - Indicates that the map contains function keys
  * `alternate_group` - Indicates that the map contains keys for an alternate
    group

If no `*_keys` flags are supplied, then the map is assumed to cover a complete
keyboard.

At present, except for `default`, none of the flags affect key processing in
libxkbcommon, and only serve as metadata.

### Name statements

Statements of the form:

    name[Group1] = "US/ASCII";
    groupName[1] = "US/ASCII";

Gives the name "US/ASCII" to the first group of symbols. Other groups can be
named using a different group index (ex: `Group2`), and with a different name.
A group must be named.

`group` and `groupName` mean the same thing, and the `Group` in `Group1` is
optional.

### Include statements

Statements of the form:

    include "nokia_vndr/rx-51(nordic_base)

Will include data from another `xkb_symbols` section, possibly located in
another file. Here it would include the `xkb_symbols` section called
`nordic_base`, from the file `rx-51` located in the `nokia_vndr` folder, itself
located in an XKB include path.

### Key statement

Statements of the form:

    key <AD01> { [ q, Q ] };

Describes the mapping of a keycode `<AD01>` to a given group of symbols. The
possible keycodes are the keycodes defined in the `xkb_keycodes` section.

Symbols are named using the symbolic names from the
`xkbcommon/xkbcommon-keysyms.h` file. A group of symbols is enclosed in brackets
and separated by commas. Each element of the symbol arrays corresponds to a
different modifier level. In this example, the symbol (keysym) `XKB_KEY_q` for
level 1 and `XKB_KEY_Q` for level 2.

#### Groups

Each group represents a list of symbols mapped to a keycode:

    name[Group1]= "US/ASCII";
    name[Group2]= "Russian";
    ...
    key <AD01> { [ q, Q ],
                 [ Cyrillic_shorti, Cyrillic_SHORTI ] };

A long-form syntax can also be used:

    key <AD01> {
        symbols[Group1]= [ q, Q ],
        symbols[Group2]= [ Cyrillic_shorti, Cyrillic_SHORTI ]
    };

Groups can also be omitted, but the brackets must be present. The following
statement only defines the Group3 of a mapping:

    key <AD01> { [], [], [ q, Q ] };

## Virtual modifier statements

Statements of the form:

    virtual_modifiers LControl;

Can appear in the `xkb_types`, `xkb_compat`, `xkb_symbols` sections.

TODO
