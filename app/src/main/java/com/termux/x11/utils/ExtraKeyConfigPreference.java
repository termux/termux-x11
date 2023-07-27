package com.termux.x11.utils;

import android.content.Context;
import android.util.AttributeSet;
import androidx.preference.DialogPreference;

import com.termux.x11.R;

public class ExtraKeyConfigPreference extends DialogPreference {
    public ExtraKeyConfigPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        setDialogLayoutResource(R.layout.extra_keys_config);
        setDialogIcon(null);
    }
}
