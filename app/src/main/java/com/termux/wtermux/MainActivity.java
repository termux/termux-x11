package com.termux.wtermux;

import android.content.Context;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.SurfaceView;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;


public class MainActivity extends AppCompatActivity implements KeyboardUtils.SoftKeyboardToggleListener {

    LorieService weston;
    boolean kbdVisible = false;
    InputMethodManager imm;
    SurfaceView root;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //jniLoader.loadLibrary("wayland-server");
        setContentView(R.layout.activity_main);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        root = findViewById(R.id.WestonView);
        weston = new LorieService(this);
        weston.connectSurfaceView(root);
        KeyboardUtils.addKeyboardToggleListener(this, this);

    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        Log.d("LorieActivity", "keydown: " + keyCode + "(keycode_back = " + KeyEvent.KEYCODE_BACK + " )");
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            onBackPressed();
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }


    @Override
    public void onBackPressed() {
        //KeyboardUtils.toggleKeyboardVisibility(this);
        Log.d("Asdasd", "asdasdasd");
        if (root == null) return;
        if (kbdVisible) {
            ((InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE))
                    .hideSoftInputFromWindow(root.getWindowToken(), 0);
        } else
        {
            ((InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE))
                    .showSoftInput(root, InputMethodManager.SHOW_FORCED);
        }
    }


    @Override
    public void onToggleSoftKeyboard(boolean isVisible) {
        kbdVisible = isVisible;
        Log.d("LorieActivity", "keyboard is " + (isVisible?"visible":"not visible"));
    }
}
