# Opt-in Android IME replay

This diagnostic harness is **outside the application source sets**. It exercises
real `LorieView` input-connection methods and reads the resulting text from a
Codex/Electron editor through its clipboard selection. It does not simulate the
OEM keyboard itself. The receiver accepts broadcasts only from callers holding
Android's `DUMP` permission (normally the ADB shell).

## Enable temporarily

Use an Android 13+ device with the shared-UID debug build, Termux, and a
proot-distro Debian desktop containing `xdotool`, `wmctrl`, and `xclip`.

1. Copy `tests/ime-replay/ImeProbeReceiver.java` to
   `lorie/src/main/java/com/termux/x11/ImeProbeReceiver.java`.
2. In `MainActivity.onCreate`, immediately after
   `LorieView lorieView = findViewById(R.id.lorieView);`, temporarily add:
   `ImeProbeReceiver.register(this, lorieView);`.
3. Build and install `:lorie-app:assembleSharedUidDebug`. Restart the native X11
   server too; updating the APK does not replace an already loaded native library.
4. Open a **new, empty, disposable** Codex project draft. Save any existing draft
   and clipboard before proceeding. This harness clears the test field, changes
   focus/fullscreen state and overwrites the clipboard. It never presses Enter
   or clicks Send.

Run from the repository root:

```sh
python3 tests/ime-replay/run.py --adb /path/to/adb \
  --container codex-desktop --user paddev --display :1 \
  --window ChatGPT --x 1600 --y 2090 > ime-results.jsonl
```

Coordinates are physical X11 screen pixels inside the empty prompt, not scaled
screenshot coordinates. Adjust them for your device. The runner checks an ASCII
round-trip before beginning. It executes all 26 fixtures twice by default,
waits for the server's input barrier plus a client processing margin, and compares
complete strings. Use `--cases path.json` and `--repeat 1` for a smaller subset.
Run only one replay at a time. A clipboard marker, setup exception or incomplete
suite is not a successful input test. Exit status is nonzero for any mismatch.

## Remove after testing

Delete only the copied application-source receiver and the temporary registration
line, then rebuild/reinstall the APK and restart the server. Keep the receiver in
this test directory; it is not included in normal builds. Remove the four
`cache/ime-probe-*` files (`cases.json`, `status`, `status.tmp`, `gate`) using
`adb shell run-as com.termux.x11 rm -f ...` if desired. Clear the disposable
editor draft and restore the user's prior draft/clipboard.

Physical-keyboard tests remain necessary: repeat normal pinyin candidate commits,
delete/retype, and Ctrl+C/V with all keys released afterward. Record these results
separately from automated input-connection replay.
