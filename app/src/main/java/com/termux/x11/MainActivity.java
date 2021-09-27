package com.termux.x11;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.preference.PreferenceManager;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.viewpager.widget.ViewPager;

import android.os.Bundle;
import android.os.Build;

import android.view.KeyEvent;
import android.view.PointerIcon;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;

import android.view.ViewGroup;
import android.view.LayoutInflater;

import android.widget.Toast;
import android.widget.FrameLayout;

import com.termux.shared.terminal.io.extrakeys.ExtraKeysView;

import com.termux.x11.TermuxAppSharedProperties;
import com.termux.x11.TerminalExtraKeys;
import com.termux.x11.TerminalToolbarViewPager;
import com.termux.x11.Fullscreenworkaround;

public class MainActivity extends AppCompatActivity {

    private TermuxAppSharedProperties mProperties;
    private int mTerminalToolbarDefaultHeight;

    ExtraKeysView mExtraKeysView;
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

	frm = findViewById(R.id.frame);

	setTerminalToolbarView();
	toggleTerminalToolbar();

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

        if (newConfig.orientation != orientation && this.getTerminalToolbarViewPager() != null && this.getTerminalToolbarViewPager().getVisibility() == View.VISIBLE) {
            InputMethodManager imm = (InputMethodManager) getSystemService(Activity.INPUT_METHOD_SERVICE);
            View view = getCurrentFocus();
            if (view == null) {
                view = new View(this);
            }
            imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        }

        orientation = newConfig.orientation;
    }

    public TermuxAppSharedProperties getProperties() {
        return mProperties;
    }

    public ExtraKeysView getExtraKeysView() {
        return mExtraKeysView;
    }

    public void setExtraKeysView(ExtraKeysView extraKeysView) {
        mExtraKeysView = extraKeysView;
    }

    public void onLorieServiceStart(LorieService instance) {
        instance.setListeners(this.getlorieView());
	SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
    }

    public SurfaceView getlorieView() {
        SurfaceView lorieView = findViewById(R.id.lorieView);
	return lorieView;
    }

    public ViewPager getTerminalToolbarViewPager() {
        return (ViewPager) findViewById(R.id.terminal_toolbar_view_pager);
    }

    private void setTerminalToolbarView() {
        final ViewPager terminalToolbarViewPager = getTerminalToolbarViewPager();

        ViewGroup.LayoutParams layoutParams = terminalToolbarViewPager.getLayoutParams();
        mTerminalToolbarDefaultHeight = layoutParams.height;

        setLorieToolbarHeight();

        terminalToolbarViewPager.setAdapter(new TerminalToolbarViewPager.PageAdapter(this, LorieService.getOnKeyListener()));
        terminalToolbarViewPager.addOnPageChangeListener(new TerminalToolbarViewPager.OnPageChangeListener(this, terminalToolbarViewPager));
    }

    private void setLorieToolbarHeight() {
        final ViewPager terminalToolbarViewPager = getTerminalToolbarViewPager();
        if (terminalToolbarViewPager == null) return;

        ViewGroup.LayoutParams layoutParams = terminalToolbarViewPager.getLayoutParams();
        layoutParams.height = (int) Math.round(mTerminalToolbarDefaultHeight *
            (mProperties.getExtraKeysInfo() == null ? 0 : mProperties.getExtraKeysInfo().getMatrix().length) *
            mProperties.getTerminalToolbarHeightScaleFactor());
        terminalToolbarViewPager.setLayoutParams(layoutParams);
    }


    public void toggleTerminalToolbar() {
        final ViewPager terminalToolbarViewPager = getTerminalToolbarViewPager();
        if (terminalToolbarViewPager == null) return;

	SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        boolean showNow = preferences.getBoolean("Toolbar", true);

        terminalToolbarViewPager.setVisibility(showNow ? View.VISIBLE : View.GONE);
        findViewById(R.id.terminal_toolbar_view_pager).requestFocus();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        Window window = getWindow();
	View decorView = window.getDecorView();

        if (hasFocus && preferences.getBoolean("fullscreen", false))
        {
            window.setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                    WindowManager.LayoutParams.FLAG_FULLSCREEN);
            decorView.setSystemUiVisibility(
                   View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                 | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                 | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                 | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                 | View.SYSTEM_UI_FLAG_FULLSCREEN
                 | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        } else {
            window.clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
            decorView.setSystemUiVisibility(0);
        }

	if (preferences.getBoolean("Reseed", true))
	{
	    if (preferences.getBoolean("fullscreen", false))
	    {
	    	window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
	    } else {
		// Taken from Stackoverflow answer https://stackoverflow.com/questions/7417123/android-how-to-adjust-layout-in-full-screen-mode-when-softkeyboard-is-visible/7509285#
		Fullscreenworkaround.assistActivity(this);
	    }
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

	if (isInPictureInPictureMode) {
	    if (this.getTerminalToolbarViewPager().getVisibility() != View.GONE)
                this.getTerminalToolbarViewPager().setVisibility(View.GONE);
		frm.setPadding(0,0,0,0);
	    return;
	} else {
	    if (this.getTerminalToolbarViewPager().getVisibility() != View.VISIBLE)
                this.getTerminalToolbarViewPager().setVisibility(View.VISIBLE);
	    return;
	}
    }

}
