package com.termux.x11;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.util.Log;
import android.widget.Toast;

import com.termux.x11.common.ITermuxX11Internal;

import java.io.FileOutputStream;
import java.io.IOException;

public class TermuxX11StarterReceiver extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Intent intent = getIntent();
        if (intent != null)
            handleIntent(intent);

        Intent main = new Intent(this, MainActivity.class);
        main.putExtra(LorieService.LAUNCHED_BY_COMPATION, true);
        main.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        startActivity(main);

        finish();
    }

    private void log(String s) {
        Log.e("NewIntent", s);
    }

    private void handleIntent(Intent intent) {
        final String extraName = "com.termux.x11.starter";
        Bundle bundle;
        IBinder token;
        ITermuxX11Internal svc;
        ParcelFileDescriptor pfd = null;
        String toastText;

        // We do not use Object.equals(Object obj) for the case same intent was passed twice
        if (intent == null)
            return;

        toastText = intent.getStringExtra("toast");
        if (toastText != null)
            Toast.makeText(this, toastText, Toast.LENGTH_LONG).show();

        bundle = intent.getBundleExtra(extraName);
        if (bundle == null) {
            log("Got intent without " + extraName + " bundle");
            return;
        }

        token = bundle.getBinder("");
        if (token == null) {
            log("got " + extraName + " extra but it has no Binder token");
            return;
        }

        svc = ITermuxX11Internal.Stub.asInterface(token);
        if (svc == null) {
            log("Could not create " + extraName + " service proxy");
            return;
        }

        try {
            pfd = svc.getWaylandFD();
            if (pfd != null)
                LorieService.adoptWaylandFd(pfd.getFd());
        } catch (Exception e) {
            log("Failed to receive ParcelFileDescriptor");
            e.printStackTrace();
        }

        try {
            pfd = svc.getLogFD();
            if (pfd != null) {
                LorieService.startLogcatForFd(pfd.getFd());
            }
        } catch (Exception e) {
            log("Failed to receive ParcelFileDescriptor");
            e.printStackTrace();
        }

        try {
            svc.finish();
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }
}
