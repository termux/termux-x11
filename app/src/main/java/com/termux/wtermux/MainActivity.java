package com.termux.wtermux;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.WindowManager;


public class MainActivity extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        LorieService root = new LorieService(this);
        setContentView(root);
        root.finishInit();
    }

    @Override
    public void onBackPressed() {}
}
