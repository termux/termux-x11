package com.termux.x11;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.KeyEvent;
import android.view.KeyCharacterMap;

import android.widget.EditText;

import androidx.annotation.NonNull;
import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;

import com.termux.shared.terminal.io.extrakeys.ExtraKeysView;
import com.termux.x11.TerminalExtraKeys;

public class TerminalToolbarViewPager {

    public static class PageAdapter extends PagerAdapter {

        final MainActivity act;
	private final View.OnKeyListener mEventListener;
	private final KeyCharacterMap mVirtualKeyboardKeyCharacterMap = KeyCharacterMap.load(KeyCharacterMap.VIRTUAL_KEYBOARD);

	public PageAdapter(MainActivity activity, View.OnKeyListener listen) {
            this.act = activity;
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
            LayoutInflater inflater = LayoutInflater.from(act);
            View layout;
            if (position == 0) {
                layout = inflater.inflate(R.layout.view_terminal_toolbar_extra_keys, collection, false);
                ExtraKeysView extraKeysView = (ExtraKeysView) layout;
                extraKeysView.reload(act.getProperties().getExtraKeysInfo());
		extraKeysView.setExtraKeysViewClient(new TerminalExtraKeys(mEventListener, act));
		act.setExtraKeysView(extraKeysView);
            } else {
                layout = inflater.inflate(R.layout.view_terminal_toolbar_text_input, collection, false);
                final EditText editText = layout.findViewById(R.id.terminal_toolbar_text_input);

                editText.setOnEditorActionListener((v, actionId, event) -> {
                        String textToSend = editText.getText().toString();
			if (textToSend.length() == 0) textToSend = "\r";

			KeyEvent[] events = mVirtualKeyboardKeyCharacterMap.getEvents(textToSend.toCharArray());
		        for (KeyEvent evnt : events) {
                	    Integer keyCode = evnt.getKeyCode();
                	    mEventListener.onKey(act.getlorieView(), keyCode, evnt);
           		}

                        editText.setText("");
			return true;
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
                act.getlorieView().requestFocus();
            } else {
                final EditText editText = mTerminalToolbarViewPager.findViewById(R.id.terminal_toolbar_text_input);
                if (editText != null) editText.requestFocus();
            }
        }

    }

}
