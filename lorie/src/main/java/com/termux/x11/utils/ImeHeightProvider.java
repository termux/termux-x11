package com.termux.x11.utils;

import static android.os.Build.VERSION.SDK_INT;

import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.os.Build.VERSION_CODES;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.PopupWindow;

import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import com.termux.x11.MainActivity;

public class ImeHeightProvider {
    // Reports the soft keyboard height so the activity window never has to resize for it.
    // WindowInsetsCompat.Type.ime() is only reliable since API 30; below that the height comes from
    // an invisible zero-width PopupWindow which declares ADJUST_RESIZE and is resized by the system
    // in the activity's stead. For the technique, see https://github.com/turnsk/keyboardheight

    public static void assistActivity(MainActivity activity) {
        FrameLayout content = activity.findViewById(android.R.id.content);

        if (SDK_INT >= VERSION_CODES.R) {
            ViewCompat.setOnApplyWindowInsetsListener(activity.getWindow().getDecorView(), (v, insets) -> {
                report(activity, content, insets.getInsets(WindowInsetsCompat.Type.ime()).bottom, insets);
                return ViewCompat.onApplyWindowInsets(v, insets);
            });
            return;
        }

        View sensor = new View(activity);
        PopupWindow popup = new PopupWindow(sensor, 0, WindowManager.LayoutParams.MATCH_PARENT);
        popup.setBackgroundDrawable(new ColorDrawable(0));
        popup.setFocusable(false);
        popup.setTouchable(false);
        popup.setInputMethodMode(PopupWindow.INPUT_METHOD_NEEDED);
        popup.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);

        content.addOnAttachStateChangeListener(new View.OnAttachStateChangeListener() {
            @Override public void onViewAttachedToWindow(View v) {
                popup.showAtLocation(content, Gravity.NO_GRAVITY, 0, 0);
            }

            @Override public void onViewDetachedFromWindow(View v) {
                popup.dismiss();
            }
        });

        sensor.getViewTreeObserver().addOnGlobalLayoutListener(() -> {
            Rect visibleFrame = new Rect();
            sensor.getWindowVisibleDisplayFrame(visibleFrame);

            DisplayMetrics metrics = new DisplayMetrics();
            activity.getRealMetrics(metrics);

            report(activity, content, metrics.heightPixels - visibleFrame.bottom, ViewCompat.getRootWindowInsets(content));
        });
    }

    // Both sources measure from the bottom of the screen, so they include the navigation bar
    // whenever it is shown below the keyboard - that space is already handled by fitsSystemWindows.
    private static void report(MainActivity activity, FrameLayout content, int bottom, WindowInsetsCompat insets) {
        View child = content.getChildAt(0);
        int imeHeight = 0;
        if (activity.hasWindowFocus() && !SamsungDexUtils.checkDeXEnabled(activity) && !activity.isInPictureInPictureMode()) {
            int navBottom = (insets != null && child.getFitsSystemWindows())
                    ? insets.getInsets(WindowInsetsCompat.Type.navigationBars()).bottom : 0;
            imeHeight = Math.max(0, bottom - navBottom);
        }
        activity.setImeHeight(imeHeight);
    }
}
