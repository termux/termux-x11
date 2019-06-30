package com.termux.wtermux;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.SurfaceView;
import android.view.WindowManager;


public class MainActivity extends AppCompatActivity {

    LorieService weston;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        SurfaceView surface = findViewById(R.id.WestonView);
        weston = new LorieService();
        weston.connectSurfaceView(surface);
    }
}
