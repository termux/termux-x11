package com.termux.x11;

import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Typeface;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;
import androidx.preference.Preference;
import android.provider.Settings;

import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceScreen;

import androidx.preference.Preference.OnPreferenceChangeListener;
import androidx.preference.SeekBarPreference;

import android.text.method.LinkMovementMethod;
import android.text.util.Linkify;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.TextView;

import com.termux.x11.utils.ExtraKeyConfigPreference;

import java.util.Objects;

public class LoriePreferences extends AppCompatActivity {

    static final String ACTION_PREFERENCES_CHANGED = "com.termux.x11.ACTION_PREFERENCES_CHANGED";
    static final String SHOW_IME_WITH_HARD_KEYBOARD = "show_ime_with_hard_keyboard";
    LoriePreferenceFragment loriePreferenceFragment;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        loriePreferenceFragment = new LoriePreferenceFragment();
        getSupportFragmentManager().beginTransaction().replace(android.R.id.content, loriePreferenceFragment).commit();

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

    public static class LoriePreferenceFragment extends PreferenceFragmentCompat implements OnPreferenceChangeListener {
        @Override
        public void onCreatePreferences(@Nullable Bundle savedInstanceState, @Nullable String rootKey) {
            addPreferencesFromResource(R.xml.preferences);
        }

        @SuppressWarnings("ConstantConditions")
        void updatePreferencesLayout() {
            SharedPreferences preferences = getPreferenceManager().getSharedPreferences();
            SeekBarPreference scalePreference = findPreference("displayScale");
            scalePreference.setMin(30);
            scalePreference.setMax(200);
            scalePreference.setSeekBarIncrement(10);
            scalePreference.setShowSeekBarValue(true);

            switch (preferences.getString("displayResolutionMode", "native")) {
                case "scaled":
                    findPreference("displayScale").setVisible(true);
                    findPreference("displayResolutionExact").setVisible(false);
                    break;
                case "exact":
                    findPreference("displayScale").setVisible(false);
                    findPreference("displayResolutionExact").setVisible(true);
                    break;
                default:
                    findPreference("displayScale").setVisible(false);
                    findPreference("displayResolutionExact").setVisible(false);
            }
        }

        @Override
        public void onCreate(final Bundle savedInstanceState)
        {
            super.onCreate(savedInstanceState);
            SharedPreferences preferences = getPreferenceManager().getSharedPreferences();

            String showImeEnabled = Settings.Secure.getString(requireActivity().getContentResolver(), SHOW_IME_WITH_HARD_KEYBOARD);
            if (showImeEnabled == null) showImeEnabled = "0";
            SharedPreferences.Editor p = Objects.requireNonNull(preferences).edit();
            p.putBoolean("showIMEWhileExternalConnected", showImeEnabled.equals("1"));
            p.apply();

            PreferenceScreen s = getPreferenceScreen();
            for (int i=0; i<s.getPreferenceCount(); i++) {
                s.getPreference(i).setOnPreferenceChangeListener(this);
            }

            updatePreferencesLayout();
        }

        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue) {
            String key = preference.getKey();
            Log.e("Preferences", "changed preference: " + key);
            updatePreferencesLayout();

            if ("showIMEWhileExternalConnected".equals(key)) {
                boolean enabled = newValue.toString().equals("true");
                try {
                    Settings.Secure.putString(requireActivity().getContentResolver(), SHOW_IME_WITH_HARD_KEYBOARD, enabled ? "1" : "0");
                } catch (Exception e) {
                    if (e instanceof SecurityException) {
                        new AlertDialog.Builder(requireActivity())
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

            if ("displayScale".equals(key)) {
                int scale = (Integer) newValue;
                if (scale % 10 != 0) {
                    scale = Math.round( ( (float) scale ) / 10 ) * 10;
                    ((SeekBarPreference) preference).setValue(scale);
                    return false;
                }
            }

            Intent intent = new Intent(ACTION_PREFERENCES_CHANGED);
            intent.setPackage("com.termux.x11");
            requireContext().sendBroadcast(intent);
            return true;
        }

        @Override
        public void onDisplayPreferenceDialog(@NonNull Preference preference) {
            if (preference instanceof ExtraKeyConfigPreference) {
                ExtraKeysConfigFragment f = new ExtraKeysConfigFragment();
                Bundle b = new Bundle();
                b.putString(ExtraKeysConfigFragment.KEY, preference.getKey());
                b.putInt(ExtraKeysConfigFragment.KEY_LAYOUT_RES_ID,  ((ExtraKeyConfigPreference) preference).getDialogLayoutResource());

                f.setArguments(b);
                f.setTargetFragment(this, 0);
                assert getFragmentManager() != null;
                f.show(getFragmentManager(), null);
            } else super.onDisplayPreferenceDialog(preference);
        }

        public static class ExtraKeysConfigFragment extends DialogFragment {

            public static final String KEY = "key";
            public static final String KEY_LAYOUT_RES_ID = "resid";


            @Nullable
            @Override
            public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
                return inflater.inflate(requireArguments().getInt(KEY_LAYOUT_RES_ID), container, false);
            }

            @Override
            public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
                super.onViewCreated(view, savedInstanceState);
                EditText config = view.findViewById(R.id.extra_keys_config);
                config.setTypeface(Typeface.MONOSPACE);

                TextView desc = view.findViewById(R.id.extra_keys_config_description);
                desc.setMovementMethod(LinkMovementMethod.getInstance());
                Linkify.addLinks(desc, Linkify.WEB_URLS);
            }
            @Override
            public Dialog onCreateDialog(Bundle savedInstanceState) {
                return new android.app.AlertDialog.Builder(getActivity())
                        .setView(R.layout.extra_keys_config)
                        .setTitle("Extra keys config")
                        .setPositiveButton("OK",
                                (dialog, whichButton) -> {
                                    // do something...
                                }
                        )
                        .setNegativeButton("Cancel",
                                (dialog, whichButton) -> dialog.dismiss()
                        )
                        .create();
            }
        }

    }
}
