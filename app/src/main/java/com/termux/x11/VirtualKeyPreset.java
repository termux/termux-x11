package com.termux.x11;

import android.content.Context;
import android.content.SharedPreferences;
import java.util.HashSet;
import java.util.Set;

public class VirtualKeyPreset {
    private static final String PREFS_NAME = "button_prefs";
    private static final String PRESET_PREFIX = "preset_";

    public static void savePreset(Context context, String presetName, Set<String> buttonData) {
        SharedPreferences prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();

        editor.putStringSet(PRESET_PREFIX + presetName, buttonData);
        editor.apply();
    }

    public static Set<String> loadPreset(Context context, String presetName) {
        SharedPreferences prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        return prefs.getStringSet(PRESET_PREFIX + presetName, new HashSet<>());
    }

    public static Set<String> getPresetNames(Context context) {
        SharedPreferences prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        Set<String> savedKeys = prefs.getAll().keySet();
        Set<String> presetNames = new HashSet<>();

        for (String key : savedKeys) {
            if (key.startsWith(PRESET_PREFIX)) {
                presetNames.add(key.replace(PRESET_PREFIX, ""));
            }
        }
        return presetNames;
    }
}
