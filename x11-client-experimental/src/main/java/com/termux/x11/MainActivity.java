package com.termux.x11;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main_activity);
        BroadcastReceiver receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                try {
                    Log.i("App", "Got intent");
                    ICmdEntryPointInterface iface = ICmdEntryPointInterface.Stub.asInterface(intent.getBundleExtra("").getBinder(""));
                    int fd = iface.getConnectionFd().detachFd();
                    Log.i("App", "Starting with fd "+ fd);
                    start(fd);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        };
        registerReceiver(receiver, new IntentFilter("a"));

        SurfaceView v = findViewById(R.id.lorieView);
        v.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(@NonNull SurfaceHolder holder) {
                Log.e("asd", "surfaceCreated");
                surface(holder.getSurface());
            }

            @Override
            public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
                Log.e("asd", "surfaceChanged");
                surface(holder.getSurface());
            }

            @Override
            public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
                Log.e("asd", "surfaceDestroyed");
                surface(null);
            }
        });
    }

    @Override
    public void setTheme(int resId) {
        super.setTheme(R.style.NoActionBar);
    }

    @Override
    public void onBackPressed() {
    }

    private native void start(int fd);
    private  native void surface(Surface sfc);

    static {
        System.loadLibrary("lorie");
    }
}
