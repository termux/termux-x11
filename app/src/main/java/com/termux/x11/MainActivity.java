package com.termux.x11;

import android.content.res.Configuration;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.WindowManager;

public class MainActivity extends AppCompatActivity {
    LorieService svc;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        svc = new LorieService(this);
        setContentView(svc);
        svc.finishInit();
        svc.onConfigurationChanged(getResources().getConfiguration());
    }
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig );
        svc.onConfigurationChanged(newConfig);
    }

    @Override
    public void onBackPressed() {}


    @Override
    public void onDestroy() {
        svc.terminate();
        super.onDestroy();
    }
}
