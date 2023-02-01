package com.termux.x11.utils;

import android.content.Context;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;

import com.termux.shared.theme.ThemeUtils;
import com.termux.x11.R;

public class ViewUtils {

    public static void setWarningTextViewAndButtonState(@NonNull Context context,
                                                        @NonNull TextView textView, @NonNull Button button,
                                                        boolean warningState, String text) {
        if (warningState) {
            textView.setTextColor(ContextCompat.getColor(context, R.color.red_error));
            textView.setLinkTextColor(ContextCompat.getColor(context, R.color.red_error_link));
            button.setEnabled(true);
            button.setAlpha(1);
        } else {
            textView.setTextColor(ThemeUtils.getTextColorPrimary(context));
            textView.setLinkTextColor(ThemeUtils.getTextColorLink(context));
            button.setEnabled(false);
            button.setAlpha(0.5f);
        }

        button.setText(text);
    }

}
