package com.termux.shared.logger;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.Toast;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.List;

public class Logger {
    /**
     * The maximum size of the log entry payload that can be written to the logger. An attempt to
     * write more than this amount will result in a truncated log entry.
     * <p>
     * The limit is 4068 but this includes log tag and log level prefix "D/" before log tag and ": "
     * suffix after it.
     * <p>
     * #define LOGGER_ENTRY_MAX_PAYLOAD 4068
     * <a href="https://cs.android.com/android/_/android/platform/system/core/+/android10-release:liblog/include/log/log_read.h;l=127">...</a>
     */
    public static final int LOGGER_ENTRY_MAX_PAYLOAD = 4068; // 4068 bytes

    public static void logMessage(int logPriority, String tag, String message) {
        if (logPriority == Log.ERROR)
            Log.e(getFullTag(tag), message);
        else if (logPriority == Log.WARN)
            Log.w(getFullTag(tag), message);
        else if (logPriority == Log.INFO)
            Log.i(getFullTag(tag), message);
    }

    public static void logExtendedMessage(int logLevel, String tag, String message) {
        if (message == null) return;

        int cutOffIndex;
        int nextNewlineIndex;
        String prefix = "";

        // -8 for prefix "(xx/xx)" (max 99 sections), - log tag length, -4 for log tag prefix "D/" and suffix ": "
        int maxEntrySize = LOGGER_ENTRY_MAX_PAYLOAD - 8 - getFullTag(tag).length() - 4;

        List<String> messagesList = new ArrayList<>();

        while(!message.isEmpty()) {
            if (message.length() > maxEntrySize) {
                cutOffIndex = maxEntrySize;
                nextNewlineIndex = message.lastIndexOf('\n', cutOffIndex);
                if (nextNewlineIndex != -1) {
                    cutOffIndex = nextNewlineIndex + 1;
                }
                messagesList.add(message.substring(0, cutOffIndex));
                message = message.substring(cutOffIndex);
            } else {
                messagesList.add(message);
                break;
            }
        }

        for(int i=0; i<messagesList.size(); i++) {
            if (messagesList.size() > 1)
                prefix = "(" + (i + 1) + "/" + messagesList.size() + ")\n";
            logMessage(logLevel, tag, prefix + messagesList.get(i));
        }
    }

    public static void logErrorExtended(String tag, String message) {
        logExtendedMessage(Log.ERROR, tag, message);
    }

    public static void logStackTraceWithMessage(String tag, String message, Throwable throwable) {
        Logger.logErrorExtended(tag, getMessageAndStackTraceString(message, throwable));
    }

    public static String getMessageAndStackTraceString(String message, Throwable throwable) {
        if (message == null && throwable == null)
            return null;
        else if (message != null && throwable != null)
            return message + ":\n" + getStackTraceString(throwable);
        else if (throwable == null)
            return message;
        else
            return getStackTraceString(throwable);
    }

    public static String getStackTraceString(Throwable throwable) {
        if (throwable == null) return null;

        String stackTraceString = null;

        try {
            StringWriter errors = new StringWriter();
            PrintWriter pw = new PrintWriter(errors);
            throwable.printStackTrace(pw);
            pw.close();
            stackTraceString = errors.toString();
            errors.close();
        } catch (IOException e) {
            e.printStackTrace();
        }

        return stackTraceString;
    }

    public static void showToast(final Context context, final String toastText, boolean longDuration) {
        if (context == null || toastText == null || "".equals(toastText)) return;

        new Handler(Looper.getMainLooper()).post(() -> Toast.makeText(context, toastText, longDuration ? Toast.LENGTH_LONG : Toast.LENGTH_SHORT).show());
    }

    /** The colon character ":" must not exist inside the tag, otherwise the `logcat` command
     * filterspecs arguments `<tag>[:priority]` will not work and will throw `Invalid filter expression`
     * error.
     * <a href="https://cs.android.com/android/platform/superproject/+/android-12.0.0_r4:system/logging/liblog/logprint.cpp;l=363">...</a>
     * <a href="https://cs.android.com/android/platform/superproject/+/android-12.0.0_r4:system/logging/logcat/logcat.cpp;l=884">...</a>
     * */
    public static String getFullTag(String tag) {
        String DEFAULT_LOG_TAG = "Logger";
        if (DEFAULT_LOG_TAG.equals(tag))
            return tag;
        else
            return DEFAULT_LOG_TAG + "." + tag;
    }
}
