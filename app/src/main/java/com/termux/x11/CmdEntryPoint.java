package com.termux.x11;

import android.annotation.SuppressLint;
import android.app.AppOpsManager;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Parcel;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;

import java.io.IOException;
import java.io.PrintStream;
import java.util.Objects;

import com.termux.x11.starter.ActivityManager;
import com.termux.x11.starter.Compat;

@SuppressLint("UnsafeDynamicallyLoadedCode")
@SuppressWarnings({"unused", "RedundantThrows", "SameParameterValue", "FieldCanBeLocal"})
public class CmdEntryPoint {
    @SuppressLint("SdCardPath")
    private final String XwaylandPath = "/data/data/com.termux/files/usr/bin/Xwayland";
    private final ComponentName TermuxX11Component =
            ComponentName.unflattenFromString("com.termux.x11/.TermuxX11StarterReceiver");
    private String[] args;
    private Service svc;
    private ParcelFileDescriptor logFD;

    /**
     * Command-line entry point.
     *
     * @param args The command-line arguments
     */
    public static void main(String[] args) {
        try {
            (new Thread(() -> {
                try {
                    (new CmdEntryPoint()).onRun(args);
                } catch (Throwable e) {
                    e.printStackTrace();
                }
            })).start();

            synchronized (Thread.currentThread()) {
                Looper.loop();
            }
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }

    Runnable failedToStartActivity = () -> {
            System.err.println("Android reported activity started but we did not get any respond");
            System.err.println("Looks like we failed to start activity.");
            exit(1);
    };

    public void onRun(String[] args) throws Throwable {
        CmdEntryPoint.this.args = args;
        checkXdgRuntimeDir();
        prepareLogFD();
        if (!Compat.havePermission(AppOpsManager.OPSTR_SYSTEM_ALERT_WINDOW)) {
            System.err.println("Looks like " + Compat.callingPackage +
                        " lacks \"Draw Over Apps\" permission.");
            System.err.println("You can grant " + Compat.callingPackage +
                        " the \"Draw Over Apps\" permission from its App Info activity:");
            System.err.println("\tAndroid Settings -> Apps -> Termux -> Advanced -> Draw over other apps.");
        }

        if (checkWaylandSocket()) {
          System.err.println("termux-x11 is already running");
          startXwayland();
        } else {
            svc = new Service();
            handler.postDelayed(failedToStartActivity, 2000);
            boolean launched = startActivity(svc);
        }
    }

    public void onTransact(int code, Parcel data, Parcel reply, int flags) {
        handler.removeCallbacks(failedToStartActivity);
    }

    private void prepareLogFD() {
        logFD = ParcelFileDescriptor.adoptFd(openLogFD());
    }

    private ParcelFileDescriptor getWaylandFD() throws Throwable {
        try {
            ParcelFileDescriptor pfd;
            int fd = createWaylandSocket();
            if (fd == -1)
                throw new Exception("Failed to bind a socket");

            pfd = ParcelFileDescriptor.adoptFd(fd);

            System.err.println(pfd);
            return pfd;
        } finally {
            System.err.println("Lorie requested fd");
        }
    }

    private void startXwayland() {
        try {
            boolean started = false;
            for (int i = 0; i < 200; i++) {
                if (checkWaylandSocket()) {
                    started = true;
                    Thread.sleep(200);
                    startXwayland(args);
                    break;
                }
                Thread.sleep(100);
            }
            if (!started)
                System.err.println("Failed to connect to Termux:X11. Something went wrong.");
        } catch (Exception e) {
            e.printStackTrace();
        }
        exit(0);
    }

    private void onFinish() {
        System.err.println("App sent finishing command");
        startXwayland();
    }

    private boolean startActivity(IBinder token) throws Throwable {
        Bundle bundle = new Bundle();
        bundle.putBinder("", token);

        Intent intent = new Intent();
        intent.putExtra("com.termux.x11.starter", bundle);
        intent.setComponent(TermuxX11Component);

        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP|
                Intent.FLAG_ACTIVITY_SINGLE_TOP);

        int res = Compat.startActivity(intent);

        PrintStream out = System.err;
        boolean launched = false;
        out.println("res = " + res);
        switch (res) {
            case ActivityManager.START_SUCCESS:
            case ActivityManager.START_SWITCHES_CANCELED:
            case ActivityManager.START_DELIVERED_TO_TOP:
            case ActivityManager.START_TASK_TO_FRONT:
                launched = true;
                break;
            case ActivityManager.START_INTENT_NOT_RESOLVED:
                out.println(
                        "Error: Activity not started, unable to "
                                + "resolve " + intent);
                break;
            case ActivityManager.START_CLASS_NOT_FOUND:
                out.println("Error: Activity class " +
                        Objects.requireNonNull(intent.getComponent()).toShortString()
                        + " does not exist.");
                break;
            case ActivityManager.START_PERMISSION_DENIED:
                out.println(
                        "Error: Activity not started, you do not "
                                + "have permission to access it.");
                break;
            default:
                out.println(
                        "Error: Activity not started, unknown error code " + res);
                break;
        }

        System.err.println("Activity is" + (launched?"":" not") + " started");

        return launched;
    }

    private void startXwayland(String[] args) throws IOException {
        System.err.println("Starting Xwayland");
        exec(XwaylandPath, args);
    }

    class Service extends ITermuxX11Internal.Stub {
        @Override
        public boolean onTransact(int code, android.os.Parcel data, android.os.Parcel reply, int flags) throws RemoteException {
            CmdEntryPoint.this.onTransact(code, data, reply, flags);
            return super.onTransact(code, data, reply, flags);
        }

        @Override
        public ParcelFileDescriptor getWaylandFD() throws RemoteException {
            System.err.println("Got getWaylandFD");
            try {
                return CmdEntryPoint.this.getWaylandFD();
            } catch (Throwable e) {
                throw new RemoteException(e.getMessage());
            }
        }

        @Override
        public ParcelFileDescriptor getLogFD() throws RemoteException {
            System.err.println("Got getLogFD");
            System.err.println(logFD);
            return logFD;
        }

        @Override
        public void finish() throws RemoteException {
            System.err.println("Got finish request");
            handler.postDelayed(CmdEntryPoint.this::onFinish, 10);
        }
    }

    private void exit(int status) {
        exit(50, status);
    }
    private void exit(int delay, int status) {
        handler.postDelayed(() -> System.exit(status), delay);
    }

    private native void checkXdgRuntimeDir();
    private native int createWaylandSocket();
    private native boolean checkWaylandSocket();
    private native int openLogFD();
    private static native void exec(String path, String[] argv);
    @SuppressWarnings("FieldMayBeFinal")
    private static Handler handler = new Handler();

    static {
        System.loadLibrary("x11-starter");
    }
}
