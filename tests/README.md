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

## Hardware Ctrl shortcuts

After building `:lorie-app:assembleSharedUidDebug`, run the Java regression on
an attached Android device (the APK is loaded for testing, not installed):

```sh
python3 tests/run-input-shortcuts.py --sdk "$ANDROID_HOME" \
  --java-home "$JAVA_HOME" --adb adb \
  --apk lorie-app/build/intermediates/apk/sharedUid/debug/termux-x11-universal-sharedUid-debug.apk
```

The test uses Android's real `KeyEvent` and the production shortcut tracker and
`InputEventSender`, with a recording injector instead of X11. It covers Ctrl
being released before C/V, IME-consumed releases leaving stale text markers,
canceled raw releases, focus loss, and distinct keyboard devices. A physical
keyboard/IME retest is still required to verify Android's dispatch path.
