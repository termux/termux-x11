package com.termux.x11;

import android.content.Intent;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.PreferenceManager;

import com.termux.x11.inputcontrols.ControlsProfile;

public class InputControlsActivity extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Prefs prefs = new Prefs(this);
        InputControlsFragment fragment = new InputControlsFragment(prefs.activeControlsProfile.get());
        fragment.setOnProfileSelectedListener((ControlsProfile profile) -> {
            // IntPreference has no setter (SeekBarPreference is normally only written from its own UI widget).
            PreferenceManager.getDefaultSharedPreferences(this).edit()
                    .putInt("activeControlsProfile", profile != null ? profile.id : 0).apply();
            prefs.showInputControls.put(profile != null);
            notifyPreferencesChanged();
        });

        getSupportFragmentManager().beginTransaction()
                .replace(android.R.id.content, fragment)
                .commit();
    }

    /** Lets an already-running MainActivity pick up the profile/visibility change immediately. */
    private void notifyPreferencesChanged() {
        Intent intent = new Intent(LoriePreferences.ACTION_PREFERENCES_CHANGED);
        intent.putExtra("key", "showInputControls");
        intent.putExtra("fromBroadcast", true);
        intent.setPackage(getPackageName());
        sendBroadcast(intent);
    }
}
