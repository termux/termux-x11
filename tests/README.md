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

## IME text matrix

`ime-cases.json` contains 26 distinct fixtures. Repeat the complete matrix at
least twice in a fresh, empty editor and compare the entire resulting text with
`expected`, including punctuation and supplementary Unicode characters.

The fixtures describe calls to the Android input connection:

- `compose`: `setComposingText(text, 1)`
- `commit`: `commitText(text, 1)`
- `finish`: `finishComposingText()`
- `delete`: `deleteSurroundingText(before, after)` (`after` defaults to zero)
- `begin` / `end`: `beginBatchEdit()` / `endBatchEdit()`
- `raw`: `LorieView.sendTextEvent(text.getBytes(UTF_8))`, bypassing composition
- `delay`: milliseconds after a call; zero exercises burst delivery

The first 12 fixtures cover the reported sentence, 48 distinct Han characters,
pinyin-prefix composition, candidate replacement, delete/retype, mixed text,
repeated words, batches, short commits, shrinking composition, and emoji append.
Another 12 add 256-character commits, repeated keycode exhaustion,
zero-delay one/two-character commits, ASCII/code punctuation, emoji replacement
and shrink, supplementary Han characters, and other scripts. The final two cover a 160-key composition replacement
burst and empty text commits.

The opt-in [replay harness](ime-replay/README.md) implements this procedure.
For device replay, wait for `LorieView.sendSync()` after each fixture before
reading the editor's selection. Allow time for the application to process its
X events too. A stale clipboard marker or an editor that fails an initial ASCII
read/write check is a harness failure, not a text-input result. Do not send the
test text as a chat message. This method changes the clipboard and the test draft.

Also repeat representative cases using the actual physical keyboard and Android
IME. InputConnection replay does not cover an OEM IME's real callback sequence.
Record the APK revision, keyboard/IME, client, and exact expected/actual strings;
a successful raw-text injection alone does not establish that IME input works.
