# Input regressions

## Dynamic keycode cache

After building the native library, compile with its NDK compilation database:

```sh
python3 tests/run-input-keysyms.py \
  lorie/.cxx/Debug/<build-id>/arm64-v8a/compile_commands.json --adb adb
```

Use the connected device's ABI; omit `--adb` to compile only. The test checks
keycode uniqueness and LRU eviction after map replacement using production
`InputXKB.c`. The previous implementation fails the duplicate-entry assertion.

## Hardware Ctrl shortcuts

After building `:lorie-app:assembleSharedUidDebug`:

```sh
python3 tests/run-input-shortcuts.py --sdk "$ANDROID_HOME" \
  --java-home "$JAVA_HOME" --adb adb \
  --apk lorie-app/build/intermediates/apk/sharedUid/debug/termux-x11-universal-sharedUid-debug.apk
```

This loads the APK without installing it. Real Android `KeyEvent` objects exercise
production shortcut handling with a recording injector: release order, focus
loss, device isolation, canceled releases and stale text markers.

## Device validation

Also test real IME candidate commits, deletion/retyping, long Chinese text and
Ctrl+C/V in a desktop editor. The regressions above do not cover OEM IME dispatch
or client keymap timing. Record the exact expected and actual text separately
from automated results; see issue #1124 and PR #1125 for the reporting device.
