package com.termux.x11.utils;

import static com.termux.shared.termux.extrakeys.ExtraKeysConstants.PRIMARY_KEY_CODES_FOR_STRINGS;

import android.annotation.SuppressLint;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.text.TextUtils;
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

    private int metaAltState = 0;

    public TermuxX11ExtraKeys(@NonNull View.OnKeyListener eventlistener, MainActivity activity, ExtraKeysView extrakeysview) {
        mEventListener = eventlistener;
        mActivity = activity;
        mExtraKeysView = extrakeysview;
    }

    private final KeyCharacterMap mVirtualKeyboardKeyCharacterMap = KeyCharacterMap.load(KeyCharacterMap.VIRTUAL_KEYBOARD);
    static final String ACTION_START_PREFERENCES_ACTIVITY = "com.termux.x11.start_preferences_activity";

    @Override
    public void onExtraKeyButtonClick(View view, ExtraKeyButton buttonInfo, MaterialButton button) {
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

    protected void onTerminalExtraKeyButtonClick(View view, String key, boolean ctrlDown, boolean altDown, boolean shiftDown, boolean fnDown) {
        if (PRIMARY_KEY_CODES_FOR_STRINGS.containsKey(key)) {
            Integer keyCode = PRIMARY_KEY_CODES_FOR_STRINGS.get(key);
            if (keyCode == null) return;
            int metaState = 0;

            if (ctrlDown) {
                metaState |= KeyEvent.META_CTRL_ON | KeyEvent.META_CTRL_LEFT_ON;
                metaAltState |= KeyEvent.KEYCODE_CTRL_LEFT | KeyEvent.KEYCODE_CTRL_RIGHT;
            }

            if (altDown) {
                metaState |= KeyEvent.META_ALT_ON | KeyEvent.META_ALT_LEFT_ON;
                metaAltState |= KeyEvent.KEYCODE_ALT_LEFT | KeyEvent.KEYCODE_ALT_RIGHT;
            }

            if (shiftDown) {
                metaState |= KeyEvent.META_SHIFT_ON | KeyEvent.META_SHIFT_LEFT_ON;
            }

            if (fnDown) {
                metaState |= KeyEvent.META_FUNCTION_ON;
            }

            mEventListener.onKey(mActivity.getLorieView(), keyCode, new KeyEvent(0, 0, KeyEvent.ACTION_DOWN, keyCode, 0, metaState));
            mEventListener.onKey(mActivity.getLorieView(), keyCode, new KeyEvent(0, 0, KeyEvent.ACTION_UP, keyCode, 0, metaState));
        } else {
            // not a control char
            key.codePoints().forEach(codePoint -> {
                char[] ch;
                ch = Character.toChars(codePoint);
                KeyEvent[] events = mVirtualKeyboardKeyCharacterMap.getEvents(ch);
                if (events != null) {
                    for (KeyEvent event : events) {

                        int keyCode = event.getKeyCode();

                        if (metaAltState != 0) {
                            mEventListener.onKey(mActivity.getLorieView(), keyCode, new KeyEvent(metaAltState, keyCode));
                        }

                        mEventListener.onKey(mActivity.getLorieView(), keyCode, new KeyEvent(KeyEvent.ACTION_UP, keyCode));

                        if (metaAltState != 0) {
                            mEventListener.onKey(mActivity.getLorieView(), keyCode, new KeyEvent(metaAltState, keyCode));
                            metaAltState = 0;
                        }
                    }
                }
            });
        }
    }

    @Override
    public boolean performExtraKeyButtonHapticFeedback(View view, ExtraKeyButton buttonInfo, MaterialButton button) {
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
                KeyboardUtils.toggleKeyboardVisibility(mActivity);
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
