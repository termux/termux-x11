package com.termux.x11.input;

import android.content.Context;
import android.text.InputType;
import android.view.KeyEvent;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.InputConnection;
import android.widget.EditText;

public class XrKeyboard extends EditText {
    public interface XrKeyboardListener {
        void onHideKeyboardRequest();
        void onSendKeyRequest(int keycode);
        void onSendTextRequest(String text);
    };

    private XrKeyboardListener listener;
    private String lastText = "";
    private String text;

    public XrKeyboard(Context context) {
        super(context);
    }

    public void reset() {
        requestFocus();
        text = " ";
        lastText = " ";
    }

    public void setListener(XrKeyboardListener tw) {
        listener = tw;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_FULLSCREEN;
        outAttrs.inputType = InputType.TYPE_TEXT_FLAG_MULTI_LINE;

        return new BaseInputConnection(this, true) {
            @Override
            public ExtractedText getExtractedText(ExtractedTextRequest extractedTextRequest, int i) {
                ExtractedText extractedText = new ExtractedText();
                extractedText.text = text;
                return extractedText;
            }

            @Override
            public boolean setComposingText(CharSequence charSequence, int i) {
                processText(charSequence.toString());
                text = charSequence.toString();
                return true;
            }

            @Override
            public boolean commitText(CharSequence charSequence, int i) {
                processText(charSequence.toString());
                return true;
            }
        };
    }

    @Override
    public boolean onCheckIsTextEditor() {
        return true;
    }

    private void processText(String s) {
        // Hitting enter passes the data into the system
        if (!s.isEmpty() && (s.charAt(s.length() - 1) == '\n')) {
            if (listener != null) {
                listener.onSendTextRequest(lastText);
                listener.onSendKeyRequest(KeyEvent.KEYCODE_ENTER);
                listener.onHideKeyboardRequest();
            }
        }
        // Backspace works only when there is no text from user
        else if (s.isEmpty()) {
            if ((lastText.compareTo(" ") == 0)) {
                listener.onSendKeyRequest(KeyEvent.KEYCODE_DEL);
            }
            reset();
        }
        // Keep info about the last text to be able to call backspace
        else {
            lastText = s;
        }
    }
}
