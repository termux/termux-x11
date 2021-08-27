package com.termux.x11;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.os.Build;
import android.preference.PreferenceManager;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.PointerIcon;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.FrameLayout;
import com.termux.shared.terminal.io.extrakeys.ExtraKeysView;
import com.termux.x11.TermuxAppSharedProperties;
import com.termux.x11.TerminalExtraKeys;

public class MainActivity extends AppCompatActivity {

    private TermuxAppSharedProperties mProperties;

    ExtraKeysView kbd;
    FrameLayout frm;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

    	mProperties = new TermuxAppSharedProperties(this);

    	LorieService.setMainActivity(this);
        LorieService.start(LorieService.ACTION_START_FROM_ACTIVITY);

        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN|
                WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.main_activity);

        kbd = findViewById(R.id.additionalKbd);
	frm = findViewById(R.id.frame);

	kbd.setExtraKeysViewClient(new TerminalExtraKeys(LorieService.getOnKeyListener(), this));

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
            getWindow().
             getDecorView().
              setPointerIcon(PointerIcon.getSystemIcon(this, PointerIcon.TYPE_NULL));

        Intent i = getIntent();
        if (i != null && i.getStringExtra(LorieService.LAUNCHED_BY_COMPATION) == null) {
            LorieService.sendRunCommand(this);
        }
    }

    int orientation;
    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        if (newConfig.orientation != orientation && kbd != null && kbd.getVisibility() == View.VISIBLE) {
            InputMethodManager imm = (InputMethodManager) getSystemService(Activity.INPUT_METHOD_SERVICE);
            View view = getCurrentFocus();
            if (view == null) {
                view = new View(this);
            }
            imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        }

        orientation = newConfig.orientation;
    }

    public void onLorieServiceStart(LorieService instance) {
        SurfaceView lorieView = findViewById(R.id.lorieView);

        instance.setListeners(lorieView);
	kbd.reload(mProperties.getExtraKeysInfo());
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        Window window = getWindow();

	if (preferences.getBoolean("Reseed", true))
	{
	    window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
	} else {
	    window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN|
                WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);
	}
    }

    @Override
    public void onBackPressed() {}

    @Override
    public void onUserLeaveHint () {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
	if (preferences.getBoolean("PIP", true)) {
            enterPictureInPictureMode();
	}
    }

    @Override
    public void onPictureInPictureModeChanged (boolean isInPictureInPictureMode, Configuration newConfig) {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);

	if (isInPictureInPictureMode) {
	    if (kbd.getVisibility() != View.GONE)
                kbd.setVisibility(View.GONE);
		frm.setPadding(0,0,0,0);
	    return;
	} else {
	    if (kbd.getVisibility() != View.VISIBLE)
		if (preferences.getBoolean("showAdditionalKbd", true)) {
                    kbd.setVisibility(View.VISIBLE);
		    int paddingDp = 35;
		    float density = this.getResources().getDisplayMetrics().density;
		    int paddingPixel = (int)(paddingDp * density);
		    frm.setPadding(0,0,0,paddingPixel);
	    	}
	    return;
	}
    }

}
