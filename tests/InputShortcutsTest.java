import android.os.Looper;
import android.view.InputDevice;
import android.view.KeyEvent;
import com.termux.x11.input.HardwareCtrlShortcuts;
import com.termux.x11.input.InputEventSender;
import com.termux.x11.input.InputStub;
import java.util.ArrayList;
import java.util.List;

public class InputShortcutsTest {
    static KeyEvent key(int action, int code, int meta, int device, int flags) {
        return new KeyEvent(1, 2, action, code, 0, meta, device, 0, flags,
                InputDevice.SOURCE_KEYBOARD);
    }

    static void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }

    static final class Recorder implements InputStub {
        final List<String> events = new ArrayList<>();
        public boolean sendKeyEvent(int scan, int code, boolean down) {
            events.add(code + (down ? ":down" : ":up"));
            return true;
        }
        public void sendTextEvent(byte[] text) { events.add("text"); }
        public void sendMouseEvent(float x, float y, int b, boolean down, boolean relative) {}
        public void sendMouseWheelEvent(float x, float y) {}
        public void sendTouchEvent(int action, int id, int x, int y) {}
        public void sendStylusEvent(float x, float y, int pressure, int tx, int ty,
                int orientation, int buttons, boolean eraser, boolean mouse) {}
        public void sendLockKeysState(int state) {}
    }

    public static void main(String[] args) {
        try {
            runTests();
        } catch (Throwable error) {
            error.printStackTrace();
            System.exit(1);
        }
    }

    static void runTests() {
        Looper.prepareMainLooper();
        HardwareCtrlShortcuts shortcuts = new HardwareCtrlShortcuts();
        int ctrl = KeyEvent.META_CTRL_ON;
        int down = KeyEvent.ACTION_DOWN, up = KeyEvent.ACTION_UP;
        for (int code : new int[]{KeyEvent.KEYCODE_C, KeyEvent.KEYCODE_V}) {
            check(shortcuts.intercept(key(down, KeyEvent.KEYCODE_CTRL_LEFT, ctrl, 7, 0)), "Ctrl down");
            check(shortcuts.intercept(key(down, code, ctrl, 7, 0)), "shortcut down");
            check(shortcuts.intercept(key(up, KeyEvent.KEYCODE_CTRL_LEFT, 0, 7, 0)), "Ctrl up first");
            check(shortcuts.intercept(key(up, code, 0, 7, 0)), "letter release after Ctrl");
            check(!shortcuts.intercept(key(down, code, 0, 7, 0)), "plain text still reaches IME");
            check(!shortcuts.intercept(key(up, code, 0, 7, 0)), "plain release reaches IME");
        }
        check(!shortcuts.intercept(key(down, KeyEvent.KEYCODE_A, ctrl, -1, 0)), "software input");
        shortcuts.intercept(key(down, KeyEvent.KEYCODE_C, ctrl, 7, 0));
        check(!shortcuts.intercept(key(up, KeyEvent.KEYCODE_C, 0, 8, 0)), "different keyboard");
        List<KeyEvent> released = new ArrayList<>();
        shortcuts.releaseAll(released::add);
        check(released.size() == 1 && released.get(0).getAction() == up, "focus loss release");
        shortcuts.releaseAll(released::add);
        check(released.size() == 1, "focus loss releases once");

        Recorder recorder = new Recorder();
        InputEventSender sender = new InputEventSender(null, recorder);
        sender.sendKeyEvent(key(down, KeyEvent.KEYCODE_C, ctrl, -1, 0));
        sender.sendKeyEvent(key(up, KeyEvent.KEYCODE_C, 0, -1, KeyEvent.FLAG_CANCELED));
        check(recorder.events.equals(List.of("31:down", "31:up")), "canceled raw press must release");
        sender.sendKeyEvent(key(up, KeyEvent.KEYCODE_C, 0, -1, KeyEvent.FLAG_CANCELED));
        check(recorder.events.size() == 2, "untracked cancellation must not inject");
        recorder.events.clear();
        sender.sendKeyEvent(key(down, KeyEvent.KEYCODE_C, 0, -1, 0));
        // The IME consumes the text key-up, then a physical Ctrl+C follows.
        sender.sendKeyEvent(key(down, KeyEvent.KEYCODE_C, ctrl, -1, 0));
        sender.sendKeyEvent(key(up, KeyEvent.KEYCODE_C, 0, -1, 0));
        check(recorder.events.equals(List.of("text", "31:down", "31:up")), "stale text marker must not swallow shortcut release");
        System.out.println("PASS: Ctrl routing, release order, focus loss, device isolation and canceled release");
    }
}
