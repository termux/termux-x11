package com.termux.x11.input;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.view.KeyEvent;
import android.view.inputmethod.CompletionInfo;
import android.view.inputmethod.CorrectionInfo;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputContentInfo;
import android.widget.EditText;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class XrKeyboard extends EditText {
    public interface TextWatcher {
        void onTextChanged(String text);
    };

    private final Handler handler;
    private TextWatcher listener;
    private String text;

    public XrKeyboard(Context context) {
        super(context);
        handler = new Handler();
        text = "";
    }

    public void reset() {
        TextWatcher l = listener;
        setListener(null);
        getEditableText().clear();
        getEditableText().append(" ");
        setListener(l);
        requestFocus();
        text = " ";
    }

    public void setListener(TextWatcher tw) {
        listener = tw;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        super.onCreateInputConnection(outAttrs);
        return new InputConnection() {
            @Override
            public CharSequence getTextBeforeCursor(int i, int i1) {
                return "";
            }

            @Override
            public CharSequence getTextAfterCursor(int i, int i1) {
                return "";
            }

            @Override
            public CharSequence getSelectedText(int i) {
                return "";
            }

            @Override
            public int getCursorCapsMode(int i) {
                return 0;
            }

            @Override
            public ExtractedText getExtractedText(ExtractedTextRequest extractedTextRequest, int i) {
                ExtractedText extractedText = new ExtractedText();
                extractedText.text = text;
                return extractedText;
            }

            @Override
            public boolean deleteSurroundingText(int i, int i1) {
                return true;
            }

            @Override
            public boolean deleteSurroundingTextInCodePoints(int i, int i1) {
                return true;
            }

            @Override
            public boolean setComposingText(CharSequence charSequence, int i) {
                if (listener != null) {
                    listener.onTextChanged(charSequence.toString());
                }
                text = charSequence.toString();
                return true;
            }

            @Override
            public boolean setComposingRegion(int i, int i1) {
                return true;
            }

            @Override
            public boolean finishComposingText() {
                return true;
            }

            @Override
            public boolean commitText(CharSequence charSequence, int i) {
                if (listener != null) {
                    listener.onTextChanged(charSequence.toString());
                }
                return true;
            }

            @Override
            public boolean commitCompletion(CompletionInfo completionInfo) {
                return true;
            }

            @Override
            public boolean commitCorrection(CorrectionInfo correctionInfo) {
                return false;
            }

            @Override
            public boolean setSelection(int i, int i1) {
                return true;
            }

            @Override
            public boolean performEditorAction(int i) {
                return true;
            }

            @Override
            public boolean performContextMenuAction(int i) {
                return true;
            }

            @Override
            public boolean beginBatchEdit() {
                return true;
            }

            @Override
            public boolean endBatchEdit() {
                return true;
            }

            @Override
            public boolean sendKeyEvent(KeyEvent keyEvent) {
                return true;
            }

            @Override
            public boolean clearMetaKeyStates(int i) {
                return true;
            }

            @Override
            public boolean reportFullscreenMode(boolean b) {
                return false;
            }

            @Override
            public boolean performPrivateCommand(String s, Bundle bundle) {
                return false;
            }

            @Override
            public boolean requestCursorUpdates(int i) {
                return true;
            }

            @Nullable
            @Override
            public Handler getHandler() {
                return handler;
            }

            @Override
            public void closeConnection() {
            }

            @Override
            public boolean commitContent(@NonNull InputContentInfo inputContentInfo, int i, @Nullable Bundle bundle) {
                return true;
            }
        };
    }
}
