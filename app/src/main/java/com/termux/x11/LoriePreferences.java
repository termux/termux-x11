package com.termux.x11;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.provider.Settings;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;
import android.view.MenuItem;

public class LoriePreferences extends AppCompatActivity {

    static final String ACTION_PREFERENCES_CHANGED = "com.termux.x11.ACTION_PREFERENCES_CHANGED";
    static final String SHOW_IME_WITH_HARD_KEYBOARD = "show_ime_with_hard_keyboard";
    LoriePreferenceFragment loriePreferenceFragment;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        loriePreferenceFragment = new LoriePreferenceFragment();

        getFragmentManager().beginTransaction().replace(android.R.id.content, loriePreferenceFragment).commit();

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setHomeButtonEnabled(true);
            actionBar.setTitle("Preferences");
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();

        if (id == android.R.id.home) {
            finish();
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @SuppressWarnings("deprecation")
    public static class LoriePreferenceFragment extends PreferenceFragment implements PreferenceScreen.OnPreferenceClickListener, Preference.OnPreferenceChangeListener {

        @Override
        public void onCreate(final Bundle savedInstanceState)
        {
            super.onCreate(savedInstanceState);
            addPreferencesFromResource(R.xml.preferences);

            String showImeEnabled = Settings.Secure.getString(getActivity().getContentResolver(), SHOW_IME_WITH_HARD_KEYBOARD);
            if (showImeEnabled == null) showImeEnabled = "0";
            SharedPreferences.Editor p = getPreferenceManager().getSharedPreferences().edit();
            p.putBoolean("showIMEWhileExternalConnected", showImeEnabled.equals("1"));
            p.apply();

            PreferenceScreen s = getPreferenceScreen();
            for (int i=0; i<s.getPreferenceCount(); i++) {
                s.getPreference(i).setOnPreferenceClickListener(this);
                s.getPreference(i).setOnPreferenceChangeListener(this);
            }
        }

        @Override
        public boolean onPreferenceClick(Preference preference) {
            return false;
        }

        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue) {
            String key = preference.getKey();
            Log.e("Preferences", "changed preference: " + key);
            if (key.equals("showIMEWhileExternalConnected")) {
                boolean enabled = newValue.toString().equals("true");
                try {
                    Settings.Secure.putString(getActivity().getContentResolver(), SHOW_IME_WITH_HARD_KEYBOARD, enabled ? "1" : "0");
                } catch (Exception e) {
                    if (e instanceof SecurityException) {
                        new AlertDialog.Builder(getActivity())
                                .setTitle("Permission denied")
                                .setMessage("Android requires WRITE_SECURE_SETTINGS permission to change this setting.\n" +
                                            "Please, launch this command using ADB:\n" +
                                            "adb shell pm grant com.termux.x11 android.permission.WRITE_SECURE_SETTINGS")
                                .setNegativeButton("OK", null)
                                .create()
                                .show();
                    } else e.printStackTrace();
                    return false;
                }
            }

            Intent intent = new Intent(ACTION_PREFERENCES_CHANGED);
            intent.setPackage("com.termux.x11");
            getContext().sendBroadcast(intent);
            return true;
        }
    }
}
