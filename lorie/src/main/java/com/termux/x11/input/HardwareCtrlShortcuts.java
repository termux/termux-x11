package com.termux.x11.input;

import android.view.InputDevice;
import android.view.KeyEvent;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.function.Consumer;

/** Keep both halves of hardware Ctrl shortcuts out of IME processing. */
public final class HardwareCtrlShortcuts {
    private final LinkedHashMap<Long, KeyEvent> pressed = new LinkedHashMap<>();

    public boolean intercept(KeyEvent event) {
        if (event.getDeviceId() < 0 || !event.isFromSource(InputDevice.SOURCE_KEYBOARD)
                || (event.getAction() != KeyEvent.ACTION_DOWN && event.getAction() != KeyEvent.ACTION_UP))
            return false;

        long id = ((long) event.getDeviceId() << 32) | event.getKeyCode();
        boolean tracked = pressed.containsKey(id);
        if (!tracked && !event.isCtrlPressed()
                && event.getKeyCode() != KeyEvent.KEYCODE_CTRL_LEFT
                && event.getKeyCode() != KeyEvent.KEYCODE_CTRL_RIGHT)
            return false;

        // Ctrl may already be up when the letter is released. Route that
        // release exactly like its press, even if the IME would consume it.
        if (event.getAction() == KeyEvent.ACTION_DOWN)
            pressed.putIfAbsent(id, new KeyEvent(event));
        else
            pressed.remove(id);
        return true;
    }

    public void releaseAll(Consumer<KeyEvent> send) {
        ArrayList<KeyEvent> keys = new ArrayList<>(pressed.values());
        pressed.clear();
        for (int i = keys.size() - 1; i >= 0; i--)
            send.accept(KeyEvent.changeAction(keys.get(i), KeyEvent.ACTION_UP));
    }
}
