package com.termux.x11;

import static android.Manifest.permission.POST_NOTIFICATIONS;
import static android.Manifest.permission.WRITE_SECURE_SETTINGS;
import static android.content.pm.PackageManager.PERMISSION_DENIED;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;
import static android.os.Build.VERSION.SDK_INT;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Typeface;
import android.os.Build;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.FragmentManager;
import androidx.preference.ListPreference;
import androidx.preference.Preference;

import android.os.Handler;
import android.os.IBinder;
import android.preference.PreferenceManager;

import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceGroup;

import androidx.preference.Preference.OnPreferenceChangeListener;
import androidx.preference.SeekBarPreference;

import android.text.method.LinkMovementMethod;
import android.util.Log;
import android.view.InputDevice;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.fragment.app.DialogFragment;

import com.termux.x11.utils.KeyInterceptor;
import com.termux.x11.utils.SamsungDexUtils;
import com.termux.x11.utils.TermuxX11ExtraKeys;

import java.io.StringWriter;
import java.io.PrintWriter;
import java.util.Arrays;
import java.util.Objects;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.regex.PatternSyntaxException;

@SuppressWarnings("deprecation")
public class LoriePreferences extends AppCompatActivity {
    static final String ACTION_PREFERENCES_CHANGED = "com.termux.x11.ACTION_PREFERENCES_CHANGED";
    LoriePreferenceFragment loriePreferenceFragment;

    private final BroadcastReceiver receiver = new BroadcastReceiver() {
        @SuppressLint("UnspecifiedRegisterReceiverFlag")
        @Override
        public void onReceive(Context context, Intent intent) {
            if (ACTION_PREFERENCES_CHANGED.equals(intent.getAction())) {
                if (intent.getBooleanExtra("fromBroadcast", false)) {
                    loriePreferenceFragment.getPreferenceScreen().removeAll();
                    loriePreferenceFragment.addPreferencesFromResource(R.xml.preferences);
                }
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        loriePreferenceFragment = new LoriePreferenceFragment(null);
        getSupportFragmentManager().beginTransaction().replace(android.R.id.content, loriePreferenceFragment).commit();

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setHomeButtonEnabled(true);
            actionBar.setTitle("Preferences");
        }
    }

    @SuppressLint("WrongConstant")
    @Override
    protected void onResume() {
        super.onResume();
        IntentFilter filter = new IntentFilter(ACTION_PREFERENCES_CHANGED);
        registerReceiver(receiver, filter, SDK_INT >= Build.VERSION_CODES.TIRAMISU ? RECEIVER_NOT_EXPORTED : 0);
    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(receiver);
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

    public static class LoriePreferenceFragment extends PreferenceFragmentCompat implements OnPreferenceChangeListener, Preference.OnPreferenceClickListener {
        final String preference;
        /** @noinspection unused*/ // Used by `androidx.fragment.app.Fragment.instantiate`...
        public LoriePreferenceFragment() {
            this(null);
        }

        public LoriePreferenceFragment(String preference) {
            this.preference = preference;
        }

        @Override
        public void onCreatePreferences(@Nullable Bundle savedInstanceState, @Nullable String rootKey) {
            Prefs p = Prefs.obtain(getContext());
            if ((Integer.parseInt(p.touchMode.get()) - 1) > 2)
                p.touchMode.put("1");

            addPreferencesFromResource(R.xml.preferences);
            //noinspection DataFlowIssue
            findPreference("showAdditionalKbd").setLayoutResource(R.layout.preference);

            // Hide what user should not see in this instance
            PreferenceGroup g = getPreferenceScreen();
            if (preference != null) {
                for (int i=0; i < g.getPreferenceCount(); i++) {
                    if (g.getPreference(i) instanceof PreferenceGroup) {
                        g.getPreference(i).setVisible(g.getPreference(i).getKey().contentEquals(preference));
                    }
                }
            }
        }

        @SuppressWarnings("ConstantConditions")
        void updatePreferencesLayout() {
            Prefs p = Prefs.obtain(getContext());
            if (!SamsungDexUtils.available())
                findPreference("dexMetaKeyCapture").setVisible(false);
            SeekBarPreference scalePreference = findPreference("displayScale");
            SeekBarPreference capturedPointerSpeedFactor = findPreference("capturedPointerSpeedFactor");
            SeekBarPreference opacityEKBar = findPreference("opacityEKBar");
            scalePreference.setMin(30);
            scalePreference.setMax(200);
            scalePreference.setUpdatesContinuously(true);
            scalePreference.setSeekBarIncrement(10);
            scalePreference.setShowSeekBarValue(true);
            capturedPointerSpeedFactor.setMin(30);
            capturedPointerSpeedFactor.setMax(200);
            capturedPointerSpeedFactor.setSeekBarIncrement(1);
            capturedPointerSpeedFactor.setShowSeekBarValue(true);
            opacityEKBar.setMin(10);
            opacityEKBar.setMax(100);
            opacityEKBar.setSeekBarIncrement(1);
            opacityEKBar.setShowSeekBarValue(true);

            String displayResMode = p.displayResolutionMode.get();
            findPreference("displayScale").setVisible(displayResMode.contentEquals("scaled"));
            findPreference("displayResolutionExact").setVisible(displayResMode.contentEquals("exact"));
            findPreference("displayResolutionCustom").setVisible(displayResMode.contentEquals("custom"));

            findPreference("dexMetaKeyCapture").setEnabled(!p.enableAccessibilityServiceAutomatically.get());
            findPreference("enableAccessibilityServiceAutomatically").setEnabled(!p.dexMetaKeyCapture.get());
            boolean pauseKeyInterceptingWithEscEnabled = p.dexMetaKeyCapture.get() || p.enableAccessibilityServiceAutomatically.get();
            findPreference("pauseKeyInterceptingWithEsc").setEnabled(pauseKeyInterceptingWithEscEnabled);
            findPreference("pauseKeyInterceptingWithEsc").setSummary(pauseKeyInterceptingWithEscEnabled ? "" : "Requires intercepting system shortcuts with Dex mode or with Accessibility service");
            findPreference("filterOutWinkey").setEnabled(p.enableAccessibilityServiceAutomatically.get());

            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P)
                findPreference("hideCutout").setVisible(false);

            findPreference("displayResolutionMode").setSummary(p.displayResolutionMode.get());
            findPreference("displayResolutionExact").setSummary(p.displayResolutionExact.get());
            findPreference("displayResolutionCustom").setSummary(p.displayResolutionCustom.get());
            boolean displayStretchEnabled = "exact".contentEquals(p.displayResolutionMode.get()) || "custom".contentEquals(p.displayResolutionMode.get());
            findPreference("displayStretch").setEnabled(displayStretchEnabled);
            findPreference("displayStretch").setSummary(displayStretchEnabled ? "" : "Requires \"display resolution mode\" to be \"exact\" or \"custom\"");

            int modeValue = Integer.parseInt(p.touchMode.get()) - 1;
            String mode = getResources().getStringArray(R.array.touchscreenInputModesEntries)[modeValue];
            findPreference("touchMode").setSummary(mode);
            boolean scaleTouchpadEnabled = "1".equals(p.touchMode.get()) && !"native".equals(p.displayResolutionMode.get());
            findPreference("scaleTouchpad").setEnabled(scaleTouchpadEnabled);
            findPreference("scaleTouchpad").setSummary(scaleTouchpadEnabled ? "" : "Requires \"Touchscreen input mode\" to be \"Trackpad\" and \"Display resolution mode\" to be not \"native\"");
            findPreference("scaleTouchpad").setSummary(scaleTouchpadEnabled ? "" : "Requires \"Touchscreen input mode\" to be \"Trackpad\" and \"Display resolution mode\" to be not \"native\"");
            findPreference("showMouseHelper").setEnabled("1".equals(p.touchMode.get()));

            AtomicBoolean stylusAvailable = new AtomicBoolean(false);
            Arrays.stream(InputDevice.getDeviceIds())
                    .mapToObj(InputDevice::getDevice)
                    .filter(Objects::nonNull)
                    .forEach((device) -> {
                        //noinspection DataFlowIssue
                        if (device.supportsSource(InputDevice.SOURCE_STYLUS))
                            stylusAvailable.set(true);
                    });

            findPreference("showStylusClickOverride").setVisible(stylusAvailable.get());
            findPreference("stylusIsMouse").setVisible(stylusAvailable.get());
            findPreference("stylusButtonContactModifierMode").setVisible(stylusAvailable.get());

            boolean requestNotificationPermissionVisible =
                    Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
                    && ContextCompat.checkSelfPermission(requireContext(), POST_NOTIFICATIONS) == PERMISSION_DENIED;
            findPreference("requestNotificationPermission").setVisible(requestNotificationPermissionVisible);

            setNoActionOptionText(findPreference("volumeDownAction"), "android volume control");
            setNoActionOptionText(findPreference("volumeUpAction"), "android volume control");
            setDefaultOptionText(findPreference("swipeDownAction"), "toggle additional key bar");
            setDefaultOptionText(findPreference("swipeUpAction"), "none");
            setDefaultOptionText(findPreference("volumeDownAction"), "send volume down");
            setDefaultOptionText(findPreference("volumeUpAction"), "send volume up");
            setDefaultOptionText(findPreference("backButtonAction"), "toggle soft keyboard");
        }

        /** @noinspection SameParameterValue*/
        private void setNoActionOptionText(Preference preference, CharSequence text) {
            ListPreference p = (ListPreference) preference;
            CharSequence[] options = p.getEntries();
            for (int i=0; i<options.length; i++) {
                if ("no action".contentEquals(options[i]))
                    options[i] = "no action (" + text + ")";
            }
        }

        private void setDefaultOptionText(Preference preference, CharSequence text) {
            ListPreference p = (ListPreference) preference;
            CharSequence[] options = p.getEntries();
            for (int i=0; i<options.length; i++) {
                if ("default".contentEquals(options[i]))
                    options[i] = "default (" + text + ")";
            }
        }

        @Override
        public void onCreate(final Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);

            setListeners(getPreferenceScreen());
            updatePreferencesLayout();
        }

        void setListeners(PreferenceGroup g) {
            for (int i=0; i < g.getPreferenceCount(); i++) {
                g.getPreference(i).setOnPreferenceChangeListener(this);
                g.getPreference(i).setOnPreferenceClickListener(this);

                if (g.getPreference(i) instanceof PreferenceGroup)
                    setListeners((PreferenceGroup) g.getPreference(i));
            }
        }

        void setMultilineTitle(PreferenceGroup g) {
            for (int i=0; i < g.getPreferenceCount(); i++) {
                if (g.getPreference(i) instanceof PreferenceGroup)
                    setListeners((PreferenceGroup) g.getPreference(i));
                else {
                    g.getPreference(i).onDependencyChanged(g.getPreference(i), true);
                    g.getPreference(i).onDependencyChanged(g.getPreference(i), false);
                }
            }
        }

        @Override
        public boolean onPreferenceClick(@NonNull Preference preference) {
            if ("extra_keys_config".contentEquals(preference.getKey())) {
                @SuppressLint("InflateParams")
                View view = getLayoutInflater().inflate(R.layout.extra_keys_config, null, false);
                Prefs p = Prefs.obtain(getContext());
                EditText config = view.findViewById(R.id.extra_keys_config);
                config.setTypeface(Typeface.MONOSPACE);
                config.setText(p.extra_keys_config.get());
                TextView desc = view.findViewById(R.id.extra_keys_config_description);
                desc.setLinksClickable(true);
                desc.setText(R.string.extra_keys_config_desc);
                desc.setMovementMethod(LinkMovementMethod.getInstance());
                new android.app.AlertDialog.Builder(getActivity())
                        .setView(view)
                        .setTitle("Extra keys config")
                        .setPositiveButton("OK",
                                (dialog, whichButton) -> {
                                    String text = config.getText().toString();
                                    p.extra_keys_config.put(!text.isEmpty() ? text : TermuxX11ExtraKeys.DEFAULT_IVALUE_EXTRA_KEYS);
                                }
                        )
                        .setNegativeButton("Cancel", (dialog, whichButton) -> dialog.dismiss())
                        .create()
                        .show();
            }
            if ("configureResponseToUserActions".contentEquals(preference.getKey())) {
                //noinspection DataFlowIssue
                new NestedPreferenceFragment("userActions").show(getActivity().getSupportFragmentManager());
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU && "requestNotificationPermission".contentEquals(preference.getKey()))
                ActivityCompat.requestPermissions(requireActivity(), new String[]{ POST_NOTIFICATIONS }, 101);

            updatePreferencesLayout();
            return false;
        }

        @SuppressLint("ApplySharedPref")
        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue) {
            String key = preference.getKey();
            Log.e("Preferences", "changed preference: " + key);
            handler.postDelayed(this::updatePreferencesLayout, 100);

            if ("displayScale".contentEquals(key)) {
                int scale = (Integer) newValue;
                if (scale % 10 != 0) {
                    scale = Math.round( ( (float) scale ) / 10 ) * 10;
                    ((SeekBarPreference) preference).setValue(scale);
                    return false;
                }
            }

            if ("displayResolutionCustom".contentEquals(key)) {
                String value = (String) newValue;
                try {
                    String[] resolution = value.split("x");
                    Integer.parseInt(resolution[0]);
                    Integer.parseInt(resolution[1]);
                } catch (NumberFormatException | PatternSyntaxException ignored) {
                    Toast.makeText(getActivity(), "Wrong resolution format", Toast.LENGTH_SHORT).show();
                    return false;
                }
            }

            if ("showAdditionalKbd".contentEquals(key) && (Boolean) newValue)
                Prefs.obtain(getContext()).additionalKbdVisible.put(true);

            if ("enableAccessibilityServiceAutomatically".contentEquals(key)) {
                if (!((Boolean) newValue))
                    KeyInterceptor.shutdown();
                if (requireContext().checkSelfPermission(WRITE_SECURE_SETTINGS) != PERMISSION_GRANTED) {
                    new AlertDialog.Builder(requireContext())
                            .setTitle("Permission denied")
                            .setMessage("Android requires WRITE_SECURE_SETTINGS permission to start accessibility service automatically.\n" +
                                    "Please, launch this command using ADB:\n" +
                                    "adb shell pm grant com.termux.x11 android.permission.WRITE_SECURE_SETTINGS")
                            .setNegativeButton("OK", null)
                            .create()
                            .show();
                    return false;
                }
            }

            Intent intent = new Intent(ACTION_PREFERENCES_CHANGED);
            intent.putExtra("key", key);
            intent.setPackage("com.termux.x11");
            requireContext().sendBroadcast(intent);

            setMultilineTitle(getPreferenceScreen());
            return true;
        }
    }

    public static class Receiver extends BroadcastReceiver {
        public Receiver() {
            super();
        }

        @Override
        public IBinder peekService(Context myContext, Intent service) {
            return super.peekService(myContext, service);
        }

        /** @noinspection StringConcatenationInLoop*/
        @SuppressLint("ApplySharedPref")
        @Override
        public void onReceive(Context context, Intent intent) {
            try {
                if (intent.getExtras() != null) {
                    Prefs p = Prefs.obtain(MainActivity.getInstance() != null ? MainActivity.getInstance() : context);
                    if (intent.getStringExtra("list") != null) {
                        String result = "";
                        for (PrefsProto.Preference pref : p.keys.values()) {
                            if (pref.type == String.class)
                                result += "\"" + pref.key + "\"=\"" + pref.asString().get() + "\"\n";
                            else if (pref.type == int.class)
                                result += "\"" + pref.key + "\"=\"" + pref.asInt().get() + "\"\n";
                            else if (pref.type == boolean.class)
                                result += "\"" + pref.key + "\"=\"" + pref.asBoolean().get() + "\"\n";
                            else if (pref.type == String[].class) {
                                String[] entries = context.getResources().getStringArray(pref.asList().entries);
                                String[] values = context.getResources().getStringArray(pref.asList().values);
                                String value = pref.asList().get();
                                int index = Arrays.asList(values).indexOf(value);
                                if (index != -1)
                                    value = entries[index];
                                result += "\"" + pref.key + "\"=\"" + value + "\"\n";
                            }
                        }

                        setResultCode(2);
                        setResultData(result);

                        return;
                    }

                    SharedPreferences.Editor edit = p.get().edit();
                    for (String key : intent.getExtras().keySet()) {
                        String newValue = intent.getStringExtra(key);
                        if (newValue == null)
                            continue;

                        switch (key) {
                            case "displayResolutionCustom": {
                                try {
                                    String[] resolution = newValue.split("x");
                                    Integer.parseInt(resolution[0]);
                                    Integer.parseInt(resolution[1]);
                                } catch (NumberFormatException | PatternSyntaxException ignored) {
                                    setResultCode(1);
                                    setResultData("displayResolutionCustom: Wrong resolution format.");
                                    return;
                                }

                                edit.putString("displayResolutionCustom", newValue);
                                break;
                            }
                            case "enableAccessibilityServiceAutomatically": {
                                if (!"true".equals(newValue))
                                    KeyInterceptor.shutdown();
                                else if (context.checkSelfPermission(WRITE_SECURE_SETTINGS) != PERMISSION_GRANTED) {
                                    setResultCode(1);
                                    setResultData("Permission denied.\n" +
                                            "Android requires WRITE_SECURE_SETTINGS permission to change `enableAccessibilityServiceAutomatically` setting.\n" +
                                            "Please, launch this command using ADB:\n" +
                                            "adb shell pm grant com.termux.x11 android.permission.WRITE_SECURE_SETTINGS");
                                    return;
                                }

                                edit.putBoolean("enableAccessibilityServiceAutomatically", "true".contentEquals(newValue));
                                break;
                            }
                            case "extra_keys_config": {
                                edit.putString(key, newValue);
                                break;
                            }
                            default: {
                                PrefsProto.Preference pref = p.keys.get(key);
                                if (pref != null && pref.type == boolean.class) {
                                    edit.putBoolean(key, "true".contentEquals(newValue));
                                    if ("showAdditionalKbd".contentEquals(key) && "true".contentEquals(newValue))
                                        edit.putBoolean("additionalKbdVisible", true);
                                } else if (pref != null && pref.type == int.class) {
                                    try {
                                        edit.putInt(key, Integer.parseInt(newValue));
                                    } catch (NumberFormatException | PatternSyntaxException exception) {
                                        setResultCode(4);
                                        setResultData(key + ": failed to parse integer: " + exception);
                                        return;
                                    }
                                } else if (pref != null && pref.type == String[].class) {
                                    PrefsProto.ListPreference _p = (PrefsProto.ListPreference) pref;
                                    String[] entries = context.getResources().getStringArray(_p.entries);
                                    String[] values = context.getResources().getStringArray(_p.values);
                                    int index = Arrays.asList(entries).indexOf(newValue);

                                    if (index == -1 && _p.entries != _p.values)
                                        index = Arrays.asList(values).indexOf(newValue);

                                    if (index != -1) {
                                        edit.putString(key, values[index]);
                                        break;
                                    }

                                    setResultCode(1);
                                    setResultData(key + ": can not be set to \"" + newValue + "\", possible options are " + Arrays.toString(entries) + (_p.entries != _p.values ? " or " + Arrays.toString(values) : ""));
                                    return;
                                } else {
                                    setResultCode(4);
                                    setResultData(key + ": unrecognised option");
                                    return;
                                }
                            }
                        }

                        Intent intent0 = new Intent(ACTION_PREFERENCES_CHANGED);
                        intent0.putExtra("key", key);
                        intent0.putExtra("fromBroadcast", true);
                        intent0.setPackage("com.termux.x11");
                        context.sendBroadcast(intent0);
                    }
                    edit.commit();
                }

                setResultCode(2);
                setResultData("Done");
            } catch (Exception e) {
                setResultCode(4);
                StringWriter sw = new StringWriter();
                PrintWriter pw = new PrintWriter(sw);
                e.printStackTrace(pw);
                setResultData(sw.toString());
            }
        }
    }

    static Handler handler = new Handler();

    public void onClick(View view) {
        new NestedPreferenceFragment("ekbar").show(getSupportFragmentManager());
    }

    public static class NestedPreferenceFragment extends DialogFragment {
        private final String fragment;
        NestedPreferenceFragment(String fragment) {
            this.fragment = fragment;
        }

        @Nullable @Override
        public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
            getChildFragmentManager().beginTransaction().replace(android.R.id.content, new LoriePreferenceFragment(fragment)).commit();
            return null;
        }

        public void show(@NonNull FragmentManager manager) {
            show(manager, fragment);
        }
    }

    /** @noinspection unused*/
    @SuppressLint("ApplySharedPref")
    public static class PrefsProto {
        public static class Preference {
            protected final String key;
            protected final Class<?> type;
            protected final Object defValue;
            protected Preference(String key, Class<?> class_, Object default_) {
                this.key = key;
                this.type = class_;
                this.defValue = default_;
            }

            ListPreference asList() {
                return (ListPreference) this;
            }

            StringPreference asString() {
                return (StringPreference) this;
            }

            IntPreference asInt() {
                return (IntPreference) this;
            }

            BooleanPreference asBoolean() {
                return (BooleanPreference) this;
            }
        }

        public class BooleanPreference extends Preference {
            public BooleanPreference(String key, boolean defValue) {
                super(key, boolean.class, defValue);
            }

            public boolean get() {
                return preferences.getBoolean(key, (boolean) defValue);
            }

            public void put(boolean v) {
                preferences.edit().putBoolean(key, v).commit();
            }
        }

        public class IntPreference extends Preference {
            public IntPreference(String key, int defValue) {
                super(key, int.class, defValue);
            }

            public int get() {
                return preferences.getInt(key, (int) defValue);
            }

            public int defValue() {
                return preferences.getInt(key, (int) defValue);
            }
        }

        public class StringPreference extends Preference {
            public StringPreference(String key, String defValue) {
                super(key, String.class, defValue);
            }

            public String get() {
                return preferences.getString(key, (String) defValue);
            }

            public void put(String v) {
                preferences.edit().putString(key, v).commit();
            }
        }

        public class ListPreference extends Preference {
            public final int entries, values;
            public ListPreference(String key, String defValue, int entries, int values) {
                super(key, String[].class, defValue);
                this.entries = entries;
                this.values = values;
            }

            public String get() {
                return preferences.getString(key, (String) defValue);
            }

            public void put(String v) {
                preferences.edit().putString(key, v).commit();
            }
        }

        Context ctx;
        SharedPreferences preferences;
        private PrefsProto() {} // No instantiation allowed
        protected PrefsProto(Context ctx) {
            this.ctx = ctx;
            preferences = PreferenceManager.getDefaultSharedPreferences(ctx);
        }

        public SharedPreferences get() {
            return preferences;
        }
    }
}
