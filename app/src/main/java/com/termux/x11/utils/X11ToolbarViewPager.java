package com.termux.x11.utils;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.KeyEvent;
import android.view.KeyCharacterMap;

import android.widget.EditText;

import androidx.annotation.NonNull;
import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;

import com.termux.shared.termux.extrakeys.ExtraKeysView;
import com.termux.x11.MainActivity;
import com.termux.x11.R;

public class X11ToolbarViewPager {
    public static class PageAdapter extends PagerAdapter {

        final MainActivity mActivity;
        private final View.OnKeyListener mEventListener;

        public PageAdapter(MainActivity activity, View.OnKeyListener listen) {
            this.mActivity = activity;
            this.mEventListener = listen;
        }

        @Override
        public int getCount() {
            return 2;
        }

        @Override
        public boolean isViewFromObject(@NonNull View view, @NonNull Object object) {
            return view == object;
        }

        @NonNull
        @Override
        public Object instantiateItem(@NonNull ViewGroup collection, int position) {
            LayoutInflater inflater = LayoutInflater.from(mActivity);
            View layout;
            if (position == 0) {
                layout = inflater.inflate(R.layout.view_terminal_toolbar_extra_keys, collection, false);
                ExtraKeysView extraKeysView = (ExtraKeysView) layout;
                mActivity.mExtraKeys = new TermuxX11ExtraKeys(mEventListener, mActivity, extraKeysView);
                int mTerminalToolbarDefaultHeight = mActivity.getTerminalToolbarViewPager().getLayoutParams().height;
                int height = mTerminalToolbarDefaultHeight *
                        ((mActivity.mExtraKeys.getExtraKeysInfo() == null) ? 0 : mActivity.mExtraKeys.getExtraKeysInfo().getMatrix().length);
                extraKeysView.reload(mActivity.mExtraKeys.getExtraKeysInfo(), height);
                extraKeysView.setExtraKeysViewClient(mActivity.mExtraKeys);
                mActivity.setDefaultToolbarHeight(mTerminalToolbarDefaultHeight);
            } else {
                layout = inflater.inflate(R.layout.view_terminal_toolbar_text_input, collection, false);
                final EditText editText = layout.findViewById(R.id.terminal_toolbar_text_input);

                editText.setOnEditorActionListener((v, actionId, event) -> {
                    String textToSend = editText.getText().toString();
                    if (textToSend.length() == 0) textToSend = "\r";
                    KeyEvent e = new KeyEvent(0, textToSend, KeyCharacterMap.VIRTUAL_KEYBOARD, 0);
                    mEventListener.onKey(mActivity.getLorieView(), 0, e);

                    editText.setText("");
                    return true;
                });

                editText.setOnCapturedPointerListener((v2, e2) -> {
                    v2.releasePointerCapture();
                    return false;
                });
            }
            collection.addView(layout);
            return layout;
        }

        @Override
        public void destroyItem(@NonNull ViewGroup collection, int position, @NonNull Object view) {
            collection.removeView((View) view);
        }

    }

    public static class OnPageChangeListener extends ViewPager.SimpleOnPageChangeListener {

        final MainActivity act;
        final ViewPager mTerminalToolbarViewPager;

        public OnPageChangeListener(MainActivity activity, ViewPager viewPager) {
            this.act = activity;
            this.mTerminalToolbarViewPager = viewPager;
        }

        @Override
        public void onPageSelected(int position) {
            if (position == 0) {
                act.getLorieView().requestFocus();
            } else {
                final EditText editText = mTerminalToolbarViewPager.findViewById(R.id.terminal_toolbar_text_input);
                if (editText != null) editText.requestFocus();
            }
        }
    }
}
