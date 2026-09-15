# Input keycode regression

After building the Android native library, use its compilation database to
compile the test with the same NDK, generated headers and include paths:

```sh
python3 tests/run-input-keysyms.py \
  lorie/.cxx/Debug/<build-id>/arm64-v8a/compile_commands.json --adb adb
```

Use the ABI of the connected test device. Omit `--adb` to only compile.
The test includes the production `InputXKB.c`; unused server functions are
discarded at link time. It checks that reassignment after a keymap replacement
keeps one cache record per keycode and evicts the least recently used live key.
The old implementation fails the `count == 2` assertion.

This is a cache regression test, not an end-to-end guarantee for CJK input.
For device testing, repeat Chinese IME commits in both a GTK editor and a
Chromium/Electron input, including deletion/retyping and long commits that
exhaust unused keycodes. Also switch between XTEST input (e.g. xdotool) and
Android Unicode input to check the first character after a keyboard switch.
