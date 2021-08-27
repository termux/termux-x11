package com.termux.x11;

import android.content.Context;

import com.termux.shared.terminal.io.extrakeys.ExtraKeysConstants;
import com.termux.shared.terminal.io.extrakeys.ExtraKeysConstants.EXTRA_KEY_DISPLAY_MAPS;
import com.termux.shared.terminal.io.extrakeys.ExtraKeysInfo;
import com.termux.shared.logger.Logger;
import com.termux.shared.settings.properties.TermuxPropertyConstants;
import com.termux.shared.settings.properties.TermuxSharedProperties;

import org.json.JSONException;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import androidx.annotation.NonNull;

public class TermuxAppSharedProperties extends TermuxSharedProperties {

    private ExtraKeysInfo mExtraKeysInfo;

    private static final String LOG_TAG = "TermuxAppSharedProperties";

    public TermuxAppSharedProperties(@NonNull Context context) {
        super(context);
    }

    /**
     * Reload the termux properties from disk into an in-memory cache.
     */
    @Override
    public void loadTermuxPropertiesFromDisk() {
        super.loadTermuxPropertiesFromDisk();

        setExtraKeys();
    }

    /**
     * Set the terminal extra keys.
     */
    private void setExtraKeys() {
        mExtraKeysInfo = null;

        try {
            // The mMap stores the extra key and style string values while loading properties
            // Check {@link #getExtraKeysInternalPropertyValueFromValue(String)} and
            // {@link #getExtraKeysStyleInternalPropertyValueFromValue(String)}
            String extrakeys = (String) getInternalPropertyValue(TermuxPropertyConstants.KEY_EXTRA_KEYS, true);
            String extraKeysStyle = (String) getInternalPropertyValue(TermuxPropertyConstants.KEY_EXTRA_KEYS_STYLE, true);

            ExtraKeysConstants.ExtraKeyDisplayMap extraKeyDisplayMap = ExtraKeysInfo.getCharDisplayMapForStyle(extraKeysStyle);
            if (EXTRA_KEY_DISPLAY_MAPS.DEFAULT_CHAR_DISPLAY.equals(extraKeyDisplayMap) && !TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS_STYLE.equals(extraKeysStyle)) {
                Logger.logError(TermuxSharedProperties.LOG_TAG, "The style \"" + extraKeysStyle + "\" for the key \"" + TermuxPropertyConstants.KEY_EXTRA_KEYS_STYLE + "\" is invalid. Using default style instead.");
                extraKeysStyle = TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS_STYLE;
            }

            mExtraKeysInfo = new ExtraKeysInfo(extrakeys, extraKeysStyle, ExtraKeysConstants.CONTROL_CHARS_ALIASES);
        } catch (JSONException e) {
            Logger.showToast(mContext, "Could not load and set the \"" + TermuxPropertyConstants.KEY_EXTRA_KEYS + "\" property from the properties file: " + e.toString(), true);
            Logger.logStackTraceWithMessage(LOG_TAG, "Could not load and set the \"" + TermuxPropertyConstants.KEY_EXTRA_KEYS + "\" property from the properties file: ", e);

            try {
                mExtraKeysInfo = new ExtraKeysInfo(TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS, TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS_STYLE, ExtraKeysConstants.CONTROL_CHARS_ALIASES);
            } catch (JSONException e2) {
                Logger.showToast(mContext, "Can't create default extra keys",true);
                Logger.logStackTraceWithMessage(LOG_TAG, "Could create default extra keys: ", e);
                mExtraKeysInfo = null;
            }
        }
    }



    public ExtraKeysInfo getExtraKeysInfo() {
        return mExtraKeysInfo;
    }



    /**
     * Load the {@link TermuxPropertyConstants#KEY_TERMINAL_TRANSCRIPT_ROWS} value from termux properties file on disk.
     */
    public static int getTerminalTranscriptRows(Context context) {
        return  (int) TermuxSharedProperties.getInternalPropertyValue(context, TermuxPropertyConstants.KEY_TERMINAL_TRANSCRIPT_ROWS);
    }

}
