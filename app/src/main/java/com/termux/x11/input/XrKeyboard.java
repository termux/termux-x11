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

    private InputConnection connection;
    private TextWatcher listener;
    public XrKeyboard(Context context) {
        super(context);
    }

    public void reset() {
        TextWatcher l = listener;
        setListener(null);
        getEditableText().clear();
        getEditableText().append(" ");
        setListener(l);
        requestFocus();
    }

    public void setListener(TextWatcher tw) {
        listener = tw;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        connection = super.onCreateInputConnection(outAttrs);
        return new InputConnection() {
            @Nullable
            @Override
            public CharSequence getTextBeforeCursor(int i, int i1) {
                return connection.getTextBeforeCursor(i, i1);
            }

            @Nullable
            @Override
            public CharSequence getTextAfterCursor(int i, int i1) {
                return connection.getTextAfterCursor(i, i1);
            }

            @Override
            public CharSequence getSelectedText(int i) {
                return connection.getSelectedText(i);
            }

            @Override
            public int getCursorCapsMode(int i) {
                return connection.getCursorCapsMode(i);
            }

            @Override
            public ExtractedText getExtractedText(ExtractedTextRequest extractedTextRequest, int i) {
                return connection.getExtractedText(extractedTextRequest, i);
            }

            @Override
            public boolean deleteSurroundingText(int i, int i1) {
                return connection.deleteSurroundingText(i, i1);
            }

            @Override
            public boolean deleteSurroundingTextInCodePoints(int i, int i1) {
                return connection.deleteSurroundingTextInCodePoints(i, i1);
            }

            @Override
            public boolean setComposingText(CharSequence charSequence, int i) {
                if (listener != null) {
                    listener.onTextChanged(charSequence.toString());
                }
                return connection.setComposingText(charSequence, i);
            }

            @Override
            public boolean setComposingRegion(int i, int i1) {
                return connection.setComposingRegion(i, i1);
            }

            @Override
            public boolean finishComposingText() {
                return connection.finishComposingText();
            }

            @Override
            public boolean commitText(CharSequence charSequence, int i) {
                if (listener != null) {
                    listener.onTextChanged(charSequence.toString());
                }
                return connection.commitText(charSequence, i);
            }

            @Override
            public boolean commitCompletion(CompletionInfo completionInfo) {
                return connection.commitCompletion(completionInfo);
            }

            @Override
            public boolean commitCorrection(CorrectionInfo correctionInfo) {
                return connection.commitCorrection(correctionInfo);
            }

            @Override
            public boolean setSelection(int i, int i1) {
                return connection.setSelection(i, i1);
            }

            @Override
            public boolean performEditorAction(int i) {
                return connection.performEditorAction(i);
            }

            @Override
            public boolean performContextMenuAction(int i) {
                return connection.performContextMenuAction(i);
            }

            @Override
            public boolean beginBatchEdit() {
                return connection.beginBatchEdit();
            }

            @Override
            public boolean endBatchEdit() {
                return connection.endBatchEdit();
            }

            @Override
            public boolean sendKeyEvent(KeyEvent keyEvent) {
                return connection.sendKeyEvent(keyEvent);
            }

            @Override
            public boolean clearMetaKeyStates(int i) {
                return connection.clearMetaKeyStates(i);
            }

            @Override
            public boolean reportFullscreenMode(boolean b) {
                return connection.reportFullscreenMode(b);
            }

            @Override
            public boolean performPrivateCommand(String s, Bundle bundle) {
                return connection.performPrivateCommand(s, bundle);
            }

            @Override
            public boolean requestCursorUpdates(int i) {
                return connection.requestCursorUpdates(i);
            }

            @Nullable
            @Override
            public Handler getHandler() {
                return connection.getHandler();
            }

            @Override
            public void closeConnection() {
                connection.closeConnection();
            }

            @Override
            public boolean commitContent(@NonNull InputContentInfo inputContentInfo, int i, @Nullable Bundle bundle) {
                return connection.commitContent(inputContentInfo, i, bundle);
            }
        };
    }
}
