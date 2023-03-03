package com.termux.x11.utils;

import static com.termux.shared.termux.extrakeys.ExtraKeysConstants.PRIMARY_KEY_CODES_FOR_STRINGS;
import static com.termux.x11.MainActivity.KeyPress;
import static com.termux.x11.MainActivity.KeyRelease;
import static com.termux.x11.MainActivity.toggleKeyboardVisibility;

import android.annotation.SuppressLint;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.viewpager.widget.ViewPager;

import com.google.android.material.button.MaterialButton;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.extrakeys.ExtraKeyButton;
import com.termux.shared.termux.extrakeys.ExtraKeysConstants;
import com.termux.shared.termux.extrakeys.ExtraKeysInfo;
import com.termux.shared.termux.extrakeys.ExtraKeysView;
import com.termux.shared.termux.extrakeys.SpecialButton;
import com.termux.shared.termux.settings.properties.TermuxPropertyConstants;
import com.termux.x11.LoriePreferences;
import com.termux.x11.MainActivity;
import com.termux.x11.R;

import org.json.JSONException;

public class TermuxX11ExtraKeys implements ExtraKeysView.IExtraKeysView {

    private final View.OnKeyListener mEventListener;
    private final MainActivity mActivity;
    private final ExtraKeysView mExtraKeysView;
    private ExtraKeysInfo mExtraKeysInfo;

    private boolean ctrlDown;
    private boolean altDown;
    private boolean shiftDown;

    public TermuxX11ExtraKeys(@NonNull View.OnKeyListener eventlistener, MainActivity activity, ExtraKeysView extrakeysview) {
        mEventListener = eventlistener;
        mActivity = activity;
        mExtraKeysView = extrakeysview;
    }

    private final KeyCharacterMap mVirtualKeyboardKeyCharacterMap = KeyCharacterMap.load(KeyCharacterMap.VIRTUAL_KEYBOARD);
    static final String ACTION_START_PREFERENCES_ACTIVITY = "com.termux.x11.start_preferences_activity";

    @Override
    public void onExtraKeyButtonClick(View view, ExtraKeyButton buttonInfo, MaterialButton button) {
        Log.e("keys", "key " + buttonInfo.getDisplay());
        if (buttonInfo.isMacro()) {
            String[] keys = buttonInfo.getKey().split(" ");
            boolean ctrlDown = false;
            boolean altDown = false;
            boolean shiftDown = false;
            boolean fnDown = false;
            for (String key : keys) {
                if (SpecialButton.CTRL.getKey().equals(key)) {
                    ctrlDown = true;
                } else if (SpecialButton.ALT.getKey().equals(key)) {
                    altDown = true;
                } else if (SpecialButton.SHIFT.getKey().equals(key)) {
                    shiftDown = true;
                } else if (SpecialButton.FN.getKey().equals(key)) {
                    fnDown = true;
                } else {
                    ctrlDown = false;
                    altDown = false;
                    shiftDown = false;
                    fnDown = false;
                }
                onLorieExtraKeyButtonClick(view, key, ctrlDown, altDown, shiftDown, fnDown);
            }
        } else {
            onLorieExtraKeyButtonClick(view, buttonInfo.getKey(), false, false, false, false);
        }
    }

    protected void onTerminalExtraKeyButtonClick(@SuppressWarnings("unused") View view, String key, boolean ctrlDown, boolean altDown, boolean shiftDown, @SuppressWarnings("unused") boolean fnDown) {
        if (this.ctrlDown != ctrlDown) {
            this.ctrlDown = ctrlDown;
            mActivity.onKeySym(KeyEvent.KEYCODE_CTRL_LEFT, 0, null, 0, ctrlDown ? KeyPress : KeyRelease);
        }

        if (this.altDown != altDown) {
            this.altDown = altDown;
            mActivity.onKeySym(KeyEvent.KEYCODE_ALT_LEFT, 0, null, 0, altDown ? KeyPress : KeyRelease);
        }

        if (this.shiftDown != shiftDown) {
            this.shiftDown = shiftDown;
            mActivity.onKeySym(KeyEvent.KEYCODE_SHIFT_LEFT, 0, null, 0, shiftDown ? KeyPress : KeyRelease);
        }

        if (PRIMARY_KEY_CODES_FOR_STRINGS.containsKey(key)) {
            Integer keyCode = PRIMARY_KEY_CODES_FOR_STRINGS.get(key);
            if (keyCode == null) return;

            mActivity.onKeySym(keyCode, 0, null, 0, 0);
        } else {
            // not a control char
            key.codePoints().forEach(codePoint -> mActivity.onKeySym(0, codePoint, null, 0, 0));
        }
    }

    @Override
    public boolean performExtraKeyButtonHapticFeedback(View view, ExtraKeyButton buttonInfo, MaterialButton button) {
        MainActivity.handler.postDelayed(() -> {
            int pressed;
            switch (buttonInfo.getKey()) {
                case "CTRL":
                    pressed = Boolean.TRUE.equals(mExtraKeysView.readSpecialButton(SpecialButton.CTRL, false))
                            ? KeyPress : KeyRelease;
                    mActivity.onKeySym(KeyEvent.KEYCODE_CTRL_LEFT, 0, null, 0, pressed);
                    break;
                case "ALT":
                    pressed = Boolean.TRUE.equals(mExtraKeysView.readSpecialButton(SpecialButton.ALT, false))
                            ? KeyPress : KeyRelease;
                    mActivity.onKeySym(KeyEvent.KEYCODE_ALT_LEFT, 0, null, 0, pressed);
                    break;
                case "SHIFT":
                    pressed = Boolean.TRUE.equals(mExtraKeysView.readSpecialButton(SpecialButton.SHIFT, false))
                            ? KeyPress : KeyRelease;
                    mActivity.onKeySym(KeyEvent.KEYCODE_SHIFT_LEFT, 0, null, 0, pressed);
                    break;
            }
        }, 100);

        return false;
    }

    public void paste(CharSequence input) {
        KeyEvent[] events = mVirtualKeyboardKeyCharacterMap.getEvents(input.toString().toCharArray());
        if (events != null) {
            for (KeyEvent event : events) {
                int keyCode = event.getKeyCode();
                mEventListener.onKey(mActivity.getLorieView(), keyCode, event);
            }
        }
    }

    private ViewPager getToolbarViewPager() {
        return mActivity.findViewById(R.id.terminal_toolbar_view_pager);
    }

    @SuppressLint("RtlHardcoded")
    public void onLorieExtraKeyButtonClick(View view, String key, boolean ctrlDown, boolean altDown, boolean shiftDown, boolean fnDown) {
        if ("KEYBOARD".equals(key)) {
            if (getToolbarViewPager()!=null) {
                getToolbarViewPager().requestFocus();
                toggleKeyboardVisibility(mActivity);
            }
        } else if ("DRAWER".equals(key)) {
            Intent preferencesIntent = new Intent(mActivity, LoriePreferences.class);
            preferencesIntent.setAction(ACTION_START_PREFERENCES_ACTIVITY);
            mActivity.startActivity(preferencesIntent);
        } else if ("PASTE".equals(key)) {
            ClipboardManager clipboard = (ClipboardManager) mActivity.getSystemService(Context.CLIPBOARD_SERVICE);
            ClipData clipData = clipboard.getPrimaryClip();
            if (clipData != null) {
                CharSequence pasted = clipData.getItemAt(0).coerceToText(mActivity);
                if (!TextUtils.isEmpty(pasted)) paste(pasted);
            }
        } else {
            onTerminalExtraKeyButtonClick(view, key, ctrlDown, altDown, shiftDown, fnDown);
        }
    }

    /**
     * Set the terminal extra keys and style.
     */
    private void setExtraKeys() {
        mExtraKeysInfo = null;
        try {
            mExtraKeysInfo = new ExtraKeysInfo(TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS, TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS_STYLE, ExtraKeysConstants.CONTROL_CHARS_ALIASES);
        } catch (JSONException e2) {
            Logger.showToast(mActivity, "Can't create default extra keys",true);
            Logger.logStackTraceWithMessage("TermuxX11ExtraKeys", "Could create default extra keys: ", e2);
            mExtraKeysInfo = null;
        }
    }

    public ExtraKeysInfo getExtraKeysInfo() {
        if (mExtraKeysInfo == null)
            setExtraKeys();
        return mExtraKeysInfo;
    }
}
