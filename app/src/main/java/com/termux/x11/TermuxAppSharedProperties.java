package com.termux.x11;

import android.content.Context;
import android.net.Uri;

import com.termux.shared.terminal.io.extrakeys.ExtraKeysConstants;
import com.termux.shared.terminal.io.extrakeys.ExtraKeysConstants.EXTRA_KEY_DISPLAY_MAPS;
import com.termux.shared.terminal.io.extrakeys.ExtraKeysInfo;
import com.termux.shared.logger.Logger;
import com.termux.shared.settings.properties.TermuxPropertyConstants;
import com.termux.shared.settings.properties.TermuxSharedProperties;
import com.termux.shared.termux.TermuxConstants;

import org.json.JSONException;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.io.File;
import java.io.InputStream;
import java.io.FileOutputStream;
import java.io.IOException;

import androidx.annotation.NonNull;

public class TermuxAppSharedProperties extends TermuxSharedProperties {

    private ExtraKeysInfo mExtraKeysInfo;

    private static final String LOG_TAG = "TermuxAppSharedProperties";
    public static final String TERMUX_X11_APP_NAME = "Termux:X11";

    private TermuxAppSharedProperties(@NonNull Context context, File propertiesFile) {
    	super(context, TERMUX_X11_APP_NAME, propertiesFile,
    	TermuxPropertyConstants.TERMUX_PROPERTIES_LIST, new SharedPropertiesParserClient());
    }

    public static TermuxAppSharedProperties build(Context context) {
    	Uri propertiesFileUri = Uri.parse("content://" + TermuxConstants.TERMUX_FILE_SHARE_URI_AUTHORITY + TermuxConstants.TERMUX_PROPERTIES_PRIMARY_FILE_PATH);
    	File propertiesFile = new File(context.getFilesDir(), "termux.properties");
    	runTermuxContentProviderReadCommand(context, propertiesFileUri, propertiesFile);
    	return new TermuxAppSharedProperties(context, propertiesFile);
    }

    private static void runTermuxContentProviderReadCommand(Context context, Uri inputUri, File outFile) {
    	InputStream inputStream = null;
    	FileOutputStream fileOutputStream = null;

    	try {
             inputStream = context.getContentResolver().openInputStream(inputUri);

            if (!outFile.exists())
            	outFile.createNewFile();
            fileOutputStream = new FileOutputStream(outFile);
            byte[] buffer = new byte[4096];
            int readBytes;
            while ((readBytes = inputStream.read(buffer)) > 0) {
            // Log.d(LOG_TAG, "data: " + new String(buffer, 0, readBytes, Charset.defaultCharset()));
                fileOutputStream.write(buffer, 0, readBytes);
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            try {
                if (inputStream != null)
                    inputStream.close();
                if (fileOutputStream != null)
                    fileOutputStream.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
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
        return  (int) TermuxSharedProperties.getInternalPropertyValue(context, TermuxPropertyConstants.getTermuxPropertiesFile(),
		TermuxPropertyConstants.KEY_TERMINAL_TRANSCRIPT_ROWS, new SharedPropertiesParserClient());
    }

}
