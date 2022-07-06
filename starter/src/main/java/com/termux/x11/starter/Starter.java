/*
**
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/


package com.termux.x11.starter;

import android.annotation.SuppressLint;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Parcel;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.util.AndroidException;

import java.io.File;
import java.io.IOException;
import java.io.PrintStream;
import java.util.Objects;

import com.termux.x11.common.ITermuxX11Internal;

@SuppressLint("UnsafeDynamicallyLoadedCode")
@SuppressWarnings({"unused", "RedundantThrows", "SameParameterValue", "FieldCanBeLocal"})
public class Starter {
    @SuppressLint("SdCardPath")
    private final String XwaylandPath = "/data/data/com.termux/files/usr/bin/Xwayland";
    private final String TermuxX11ComponentName = "com.termux.x11/.TermuxX11StarterReceiver";
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
                    (new Starter()).onRun(args);
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
            System.err.println("Looks like Termux lacks \"Draw Over Apps\" permission.");
            System.err.println("You can grant Termux the \"Draw Over Apps\" permission from its App Info activity:");
            System.err.println("\tAndroid Settings -> Apps -> Termux -> Advanced -> Draw over other apps.");
    };

    public void onRun(String[] args) throws Throwable {
        checkXdgRuntimeDir();
        prepareLogFD();

        if (checkWaylandSocket()) {
          System.err.println("termux-x11 is already running");
          startXwayland();
        } else {
            Starter.this.args = args;
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
        ComponentName cn = ComponentName.unflattenFromString(TermuxX11ComponentName);
        if (cn == null)
            throw new IllegalArgumentException("Bad component name: " + TermuxX11ComponentName);

        intent.setComponent(cn);

        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP|
                Intent.FLAG_ACTIVITY_SINGLE_TOP);
        IActivityManager mAm;
        try {
            mAm = new IActivityManager();
        } catch (Throwable e) {
            e.printStackTrace();
            throw new AndroidException("Can't connect to activity manager; is the system running?");
        }

        int res;
        res = mAm.startActivityAsUser(intent,null, 0, null, 0);

        PrintStream out = System.err;
        boolean launched = false;
        out.println("res = " + res);
        switch (res) {
            case ActivityManager.START_SUCCESS:
                launched = true;
                break;
            case ActivityManager.START_SWITCHES_CANCELED:
                launched = true;
                out.println(
                        "Warning: Activity not started because the "
                                + " current activity is being kept for the user.");
                break;
            case ActivityManager.START_DELIVERED_TO_TOP:
                launched = true;
                out.println(
                        "Warning: Activity not started, intent has "
                                + "been delivered to currently running "
                                + "top-most instance.");
                break;
            case ActivityManager.START_RETURN_INTENT_TO_CALLER:
                launched = true;
                out.println(
                        "Warning: Activity not started because intent "
                                + "should be handled by the caller");
                break;
            case ActivityManager.START_TASK_TO_FRONT:
                launched = true;
                out.println(
                        "Warning: Activity not started, its current "
                                + "task has been brought to the front");
                break;
            case ActivityManager.START_INTENT_NOT_RESOLVED:
                out.println(
                        "Error: Activity not started, unable to "
                                + "resolve " + intent.toString());
                break;
            case ActivityManager.START_CLASS_NOT_FOUND:
                out.println("Error: Activity class " +
                        Objects.requireNonNull(intent.getComponent()).toShortString()
                        + " does not exist.");
                break;
            case ActivityManager.START_FORWARD_AND_REQUEST_CONFLICT:
                out.println(
                        "Error: Activity not started, you requested to "
                                + "both forward and receive its result");
                break;
            case ActivityManager.START_PERMISSION_DENIED:
                out.println(
                        "Error: Activity not started, you do not "
                                + "have permission to access it.");
                break;
            case ActivityManager.START_NOT_VOICE_COMPATIBLE:
                out.println(
                        "Error: Activity not started, voice control not allowed for: "
                                + intent);
                break;
            case ActivityManager.START_NOT_CURRENT_USER_ACTIVITY:
                out.println(
                        "Error: Not allowed to start background user activity"
                                + " that shouldn't be displayed for all users.");
                break;
            default:
                out.println(
                        "Error: Activity not started, unknown error code " + res);
                break;
        }

        System.err.println("Activity is" + (launched?"":" not") + " started");
        PendingIntent p;

        return launched;
    }

    private void startXwayland(String[] args) throws IOException {
        System.err.println("Starting Xwayland");
        ExecHelper.exec(XwaylandPath, args);
    }

    class Service extends ITermuxX11Internal.Stub {
        @Override
        public boolean onTransact(int code, android.os.Parcel data, android.os.Parcel reply, int flags) throws RemoteException {
            Starter.this.onTransact(code, data, reply, flags);
            return super.onTransact(code, data, reply, flags);
        }

        @Override
        public ParcelFileDescriptor getWaylandFD() throws RemoteException {
            System.err.println("Got getWaylandFD");
            try {
                return Starter.this.getWaylandFD();
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
            handler.postDelayed(Starter.this::onFinish, 10);
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
    @SuppressWarnings("FieldMayBeFinal")
    private static Handler handler;
    static {
        Looper.prepare();
        handler = new Handler();

        boolean loaded = false;
        @SuppressLint("SdCardPath")
        final String DistDir = "/data/data/com.termux/files/usr/libexec/termux-x11";
        for (int i = 0; i < Build.SUPPORTED_ABIS.length; i++) {
            @SuppressLint("SdCardPath")
            final String libPath = DistDir + "/" + Build.SUPPORTED_ABIS[i] + "/libx11-starter.so";
            File libFile = new File(libPath);
            if (libFile.exists()) {
                System.load(libPath);
                loaded = true;
                break;
            } else System.err.println(libPath + "not found");
        }

        if (!loaded) {
            System.err.println("Can not find some core libraries.");
            System.err.println("Please, check termux-x11 package installation.");
            System.exit(1);
        }
    }

}
