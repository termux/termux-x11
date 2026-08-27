package com.termux.x11.inputcontrols;

import android.view.KeyEvent;

import androidx.annotation.NonNull;

import com.termux.x11.input.InputStub;

import java.util.ArrayList;

public enum Binding {
    NONE, MOUSE_LEFT_BUTTON, MOUSE_MIDDLE_BUTTON, MOUSE_RIGHT_BUTTON, MOUSE_MOVE_LEFT, MOUSE_MOVE_RIGHT, MOUSE_MOVE_UP, MOUSE_MOVE_DOWN, MOUSE_SCROLL_UP, MOUSE_SCROLL_DOWN, KEY_UP, KEY_RIGHT, KEY_DOWN, KEY_LEFT, KEY_ENTER, KEY_ESC, KEY_BKSP, KEY_DEL, KEY_INSERT, KEY_TAB, KEY_SPACE, KEY_CTRL_L, KEY_CTRL_R, KEY_SHIFT_L, KEY_SHIFT_R, KEY_ALT_L, KEY_ALT_R, KEY_HOME, KEY_PRTSCN, KEY_PG_UP, KEY_PG_DOWN, KEY_END, KEY_CAPS_LOCK, KEY_NUM_LOCK, KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z, KEY_BRACKET_LEFT, KEY_BRACKET_RIGHT, KEY_BACKSLASH, KEY_SLASH, KEY_SEMICOLON, KEY_COMMA, KEY_PERIOD, KEY_APOSTROPHE, KEY_KP_ADD, KEY_MINUS, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_KP_0, KEY_KP_1, KEY_KP_2, KEY_KP_3, KEY_KP_4, KEY_KP_5, KEY_KP_6, KEY_KP_7, KEY_KP_8, KEY_KP_9, GAMEPAD_BUTTON_A, GAMEPAD_BUTTON_B, GAMEPAD_BUTTON_X, GAMEPAD_BUTTON_Y, GAMEPAD_BUTTON_L1, GAMEPAD_BUTTON_R1, GAMEPAD_BUTTON_SELECT, GAMEPAD_BUTTON_START, GAMEPAD_BUTTON_L3, GAMEPAD_BUTTON_R3, GAMEPAD_BUTTON_L2, GAMEPAD_BUTTON_R2, GAMEPAD_LEFT_THUMB_UP, GAMEPAD_LEFT_THUMB_RIGHT, GAMEPAD_LEFT_THUMB_DOWN, GAMEPAD_LEFT_THUMB_LEFT, GAMEPAD_RIGHT_THUMB_UP, GAMEPAD_RIGHT_THUMB_RIGHT, GAMEPAD_RIGHT_THUMB_DOWN, GAMEPAD_RIGHT_THUMB_LEFT, GAMEPAD_DPAD_UP, GAMEPAD_DPAD_RIGHT, GAMEPAD_DPAD_DOWN, GAMEPAD_DPAD_LEFT, KEY_VOL_UP, KEY_VOL_DOWN;

    /** Android KeyEvent.KEYCODE_* for keyboard bindings, 0 otherwise (mouse/gamepad/NONE). */
    public final int keyCode;

    Binding() {
        switch (name()) {
            case "KEY_UP": keyCode = KeyEvent.KEYCODE_DPAD_UP; break;
            case "KEY_RIGHT": keyCode = KeyEvent.KEYCODE_DPAD_RIGHT; break;
            case "KEY_DOWN": keyCode = KeyEvent.KEYCODE_DPAD_DOWN; break;
            case "KEY_LEFT": keyCode = KeyEvent.KEYCODE_DPAD_LEFT; break;
            case "KEY_ENTER": keyCode = KeyEvent.KEYCODE_ENTER; break;
            case "KEY_ESC": keyCode = KeyEvent.KEYCODE_ESCAPE; break;
            case "KEY_BKSP": keyCode = KeyEvent.KEYCODE_DEL; break;
            case "KEY_DEL": keyCode = KeyEvent.KEYCODE_FORWARD_DEL; break;
            case "KEY_INSERT": keyCode = KeyEvent.KEYCODE_INSERT; break;
            case "KEY_TAB": keyCode = KeyEvent.KEYCODE_TAB; break;
            case "KEY_SPACE": keyCode = KeyEvent.KEYCODE_SPACE; break;
            case "KEY_CTRL_L": keyCode = KeyEvent.KEYCODE_CTRL_LEFT; break;
            case "KEY_CTRL_R": keyCode = KeyEvent.KEYCODE_CTRL_RIGHT; break;
            case "KEY_SHIFT_L": keyCode = KeyEvent.KEYCODE_SHIFT_LEFT; break;
            case "KEY_SHIFT_R": keyCode = KeyEvent.KEYCODE_SHIFT_RIGHT; break;
            case "KEY_ALT_L": keyCode = KeyEvent.KEYCODE_ALT_LEFT; break;
            case "KEY_ALT_R": keyCode = KeyEvent.KEYCODE_ALT_RIGHT; break;
            case "KEY_HOME": keyCode = KeyEvent.KEYCODE_MOVE_HOME; break;
            case "KEY_PRTSCN": keyCode = KeyEvent.KEYCODE_SYSRQ; break;
            case "KEY_PG_UP": keyCode = KeyEvent.KEYCODE_PAGE_UP; break;
            case "KEY_PG_DOWN": keyCode = KeyEvent.KEYCODE_PAGE_DOWN; break;
            case "KEY_END": keyCode = KeyEvent.KEYCODE_MOVE_END; break;
            case "KEY_CAPS_LOCK": keyCode = KeyEvent.KEYCODE_CAPS_LOCK; break;
            case "KEY_NUM_LOCK": keyCode = KeyEvent.KEYCODE_NUM_LOCK; break;
            case "KEY_0": keyCode = KeyEvent.KEYCODE_0; break;
            case "KEY_1": keyCode = KeyEvent.KEYCODE_1; break;
            case "KEY_2": keyCode = KeyEvent.KEYCODE_2; break;
            case "KEY_3": keyCode = KeyEvent.KEYCODE_3; break;
            case "KEY_4": keyCode = KeyEvent.KEYCODE_4; break;
            case "KEY_5": keyCode = KeyEvent.KEYCODE_5; break;
            case "KEY_6": keyCode = KeyEvent.KEYCODE_6; break;
            case "KEY_7": keyCode = KeyEvent.KEYCODE_7; break;
            case "KEY_8": keyCode = KeyEvent.KEYCODE_8; break;
            case "KEY_9": keyCode = KeyEvent.KEYCODE_9; break;
            case "KEY_A": keyCode = KeyEvent.KEYCODE_A; break;
            case "KEY_B": keyCode = KeyEvent.KEYCODE_B; break;
            case "KEY_C": keyCode = KeyEvent.KEYCODE_C; break;
            case "KEY_D": keyCode = KeyEvent.KEYCODE_D; break;
            case "KEY_E": keyCode = KeyEvent.KEYCODE_E; break;
            case "KEY_F": keyCode = KeyEvent.KEYCODE_F; break;
            case "KEY_G": keyCode = KeyEvent.KEYCODE_G; break;
            case "KEY_H": keyCode = KeyEvent.KEYCODE_H; break;
            case "KEY_I": keyCode = KeyEvent.KEYCODE_I; break;
            case "KEY_J": keyCode = KeyEvent.KEYCODE_J; break;
            case "KEY_K": keyCode = KeyEvent.KEYCODE_K; break;
            case "KEY_L": keyCode = KeyEvent.KEYCODE_L; break;
            case "KEY_M": keyCode = KeyEvent.KEYCODE_M; break;
            case "KEY_N": keyCode = KeyEvent.KEYCODE_N; break;
            case "KEY_O": keyCode = KeyEvent.KEYCODE_O; break;
            case "KEY_P": keyCode = KeyEvent.KEYCODE_P; break;
            case "KEY_Q": keyCode = KeyEvent.KEYCODE_Q; break;
            case "KEY_R": keyCode = KeyEvent.KEYCODE_R; break;
            case "KEY_S": keyCode = KeyEvent.KEYCODE_S; break;
            case "KEY_T": keyCode = KeyEvent.KEYCODE_T; break;
            case "KEY_U": keyCode = KeyEvent.KEYCODE_U; break;
            case "KEY_V": keyCode = KeyEvent.KEYCODE_V; break;
            case "KEY_W": keyCode = KeyEvent.KEYCODE_W; break;
            case "KEY_X": keyCode = KeyEvent.KEYCODE_X; break;
            case "KEY_Y": keyCode = KeyEvent.KEYCODE_Y; break;
            case "KEY_Z": keyCode = KeyEvent.KEYCODE_Z; break;
            case "KEY_BRACKET_LEFT": keyCode = KeyEvent.KEYCODE_LEFT_BRACKET; break;
            case "KEY_BRACKET_RIGHT": keyCode = KeyEvent.KEYCODE_RIGHT_BRACKET; break;
            case "KEY_BACKSLASH": keyCode = KeyEvent.KEYCODE_BACKSLASH; break;
            case "KEY_SLASH": keyCode = KeyEvent.KEYCODE_SLASH; break;
            case "KEY_SEMICOLON": keyCode = KeyEvent.KEYCODE_SEMICOLON; break;
            case "KEY_COMMA": keyCode = KeyEvent.KEYCODE_COMMA; break;
            case "KEY_PERIOD": keyCode = KeyEvent.KEYCODE_PERIOD; break;
            case "KEY_APOSTROPHE": keyCode = KeyEvent.KEYCODE_APOSTROPHE; break;
            case "KEY_KP_ADD": keyCode = KeyEvent.KEYCODE_NUMPAD_ADD; break;
            case "KEY_MINUS": keyCode = KeyEvent.KEYCODE_MINUS; break;
            case "KEY_F1": keyCode = KeyEvent.KEYCODE_F1; break;
            case "KEY_F2": keyCode = KeyEvent.KEYCODE_F2; break;
            case "KEY_F3": keyCode = KeyEvent.KEYCODE_F3; break;
            case "KEY_F4": keyCode = KeyEvent.KEYCODE_F4; break;
            case "KEY_F5": keyCode = KeyEvent.KEYCODE_F5; break;
            case "KEY_F6": keyCode = KeyEvent.KEYCODE_F6; break;
            case "KEY_F7": keyCode = KeyEvent.KEYCODE_F7; break;
            case "KEY_F8": keyCode = KeyEvent.KEYCODE_F8; break;
            case "KEY_F9": keyCode = KeyEvent.KEYCODE_F9; break;
            case "KEY_F10": keyCode = KeyEvent.KEYCODE_F10; break;
            case "KEY_F11": keyCode = KeyEvent.KEYCODE_F11; break;
            case "KEY_F12": keyCode = KeyEvent.KEYCODE_F12; break;
            case "KEY_KP_0": keyCode = KeyEvent.KEYCODE_NUMPAD_0; break;
            case "KEY_KP_1": keyCode = KeyEvent.KEYCODE_NUMPAD_1; break;
            case "KEY_KP_2": keyCode = KeyEvent.KEYCODE_NUMPAD_2; break;
            case "KEY_KP_3": keyCode = KeyEvent.KEYCODE_NUMPAD_3; break;
            case "KEY_KP_4": keyCode = KeyEvent.KEYCODE_NUMPAD_4; break;
            case "KEY_KP_5": keyCode = KeyEvent.KEYCODE_NUMPAD_5; break;
            case "KEY_KP_6": keyCode = KeyEvent.KEYCODE_NUMPAD_6; break;
            case "KEY_KP_7": keyCode = KeyEvent.KEYCODE_NUMPAD_7; break;
            case "KEY_KP_8": keyCode = KeyEvent.KEYCODE_NUMPAD_8; break;
            case "KEY_KP_9": keyCode = KeyEvent.KEYCODE_NUMPAD_9; break;
            case "KEY_VOL_UP": keyCode = KeyEvent.KEYCODE_VOLUME_UP; break;
            case "KEY_VOL_DOWN": keyCode = KeyEvent.KEYCODE_VOLUME_DOWN; break;
            default: keyCode = 0; break;
        }
    }

    @NonNull
    @Override
    public String toString() {
        switch (this) {
            case KEY_SHIFT_L:
                return "L SHIFT";
            case KEY_SHIFT_R:
                return "R SHIFT";
            case KEY_CTRL_L:
                return "L CTRL";
            case KEY_CTRL_R:
                return "R CTRL";
            case KEY_ALT_L:
                return "L ALT";
            case KEY_ALT_R:
                return "R ALT";
            case KEY_BRACKET_LEFT:
                return "[";
            case KEY_BRACKET_RIGHT:
                return "]";
            case KEY_BACKSLASH:
                return "\\";
            case KEY_SLASH:
                return "/";
            case KEY_SEMICOLON:
                return ";";
            case KEY_COMMA:
                return ",";
            case KEY_PERIOD:
                return ".";
            case KEY_APOSTROPHE:
                return "'";
            case KEY_MINUS:
                return "-";
            case KEY_KP_ADD:
                return "+";
            case KEY_VOL_UP:
                return "VOL +";
            case KEY_VOL_DOWN:
                return "VOL -";
            default:
                return super.toString().replaceAll("^(MOUSE_)|(KEY_)|(GAMEPAD_)", "").replace("KP_", "NUMPAD_").replace("_", " ");
        }
    }

    public static Binding fromString(String name) {
        switch (name) {
            case "KEY_CTRL":
                return Binding.KEY_CTRL_L;
            case "KEY_SHIFT":
                return Binding.KEY_SHIFT_L;
            case "KEY_ALT":
                return Binding.KEY_ALT_L;
            default:
                return valueOf(name);
        }
    }

    /** InputStub.BUTTON_* for a plain mouse-button binding, or null if this isn't one. */
    public Integer getPointerButton() {
        switch (this) {
            case MOUSE_LEFT_BUTTON:
                return InputStub.BUTTON_LEFT;
            case MOUSE_MIDDLE_BUTTON:
                return InputStub.BUTTON_MIDDLE;
            case MOUSE_RIGHT_BUTTON:
                return InputStub.BUTTON_RIGHT;
            default:
                return null;
        }
    }

    public boolean isMouse() {
        return name().startsWith("MOUSE_");
    }

    public boolean isKeyboard() {
        return name().startsWith("KEY_") || this == NONE;
    }

    public boolean isGamepad() {
        return name().startsWith("GAMEPAD_");
    }

    public boolean isMouseMove() {
        return this == MOUSE_MOVE_UP || this == MOUSE_MOVE_RIGHT || this == MOUSE_MOVE_DOWN || this == MOUSE_MOVE_LEFT;
    }

    public boolean isMouseScroll() {
        return this == MOUSE_SCROLL_UP || this == MOUSE_SCROLL_DOWN;
    }

    public static String[] mouseBindingLabels() {
        ArrayList<String> names = new ArrayList<>();
        for (Binding binding : values()) if (binding.isMouse()) names.add(binding.toString());
        return names.toArray(new String[0]);
    }

    public static String[] keyboardBindingLabels() {
        ArrayList<String> labels = new ArrayList<>();
        for (Binding binding : values()) if (binding.isKeyboard()) labels.add(binding.toString());
        return labels.toArray(new String[0]);
    }

    public static String[] gamepadBindingLabels() {
        ArrayList<String> names = new ArrayList<>();
        for (Binding binding : values()) if (binding.isGamepad()) names.add(binding.toString());
        return names.toArray(new String[0]);
    }

    public static Binding[] mouseBindingValues() {
        ArrayList<Binding> labels = new ArrayList<>();
        for (Binding binding : values()) if (binding.isMouse()) labels.add(binding);
        return labels.toArray(new Binding[0]);
    }

    public static Binding[] keyboardBindingValues() {
        ArrayList<Binding> values = new ArrayList<>();
        for (Binding binding : values()) if (binding.isKeyboard()) values.add(binding);
        return values.toArray(new Binding[0]);
    }

    public static Binding[] gamepadBindingValues() {
        ArrayList<Binding> labels = new ArrayList<>();
        for (Binding binding : values()) if (binding.isGamepad()) labels.add(binding);
        return labels.toArray(new Binding[0]);
    }
}
