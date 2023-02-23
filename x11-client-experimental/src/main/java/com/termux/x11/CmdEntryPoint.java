package com.termux.x11;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.util.Log;

import java.util.Arrays;

import dalvik.system.PathClassLoader;

public class CmdEntryPoint {
    public static void main(String[] args) throws Exception {
//        if (Arrays.stream(args).noneMatch(s->s.equals("asd"))) {
//            Context ctx = android.app.ActivityThread.systemMain().getSystemContext();
//            PackageManager pm = ctx.getPackageManager();
//            ApplicationInfo target = pm.getApplicationInfo("com.termux.x11", 0);
//            @SuppressLint("SdCardPath")
//            String librarySearchPath = "/data/data/com.termux/files/usr/lib/:" + target.sourceDir + "!/lib/" + Build.SUPPORTED_ABIS[0] + "/";
//            PathClassLoader classLoader = new PathClassLoader(target.sourceDir, librarySearchPath,
//                    ClassLoader.getSystemClassLoader());
//            Class<?> targetClass = Class.forName("com.termux.x11.CmdEntryPoint", true, classLoader);
//            targetClass.getMethod("main", String[].class).invoke(null, (Object) new String[]{"asd"});
//            return;
//        }
//
//        System.loadLibrary("lorie");

        System.err.println("CmdEntryPoint started");
        Context ctx = android.app.ActivityThread.systemMain().getSystemContext();

        Intent intent = new Intent();
        Bundle bundle = new Bundle();
        intent.setPackage("com.termux.x11");
        intent.setAction("a");
        intent.putExtra("", bundle);
        bundle.putBinder("", new ICmdEntryPointInterface.Stub() {
            @Override
            public ParcelFileDescriptor getConnectionFd() throws RemoteException {
                int fd = connect();
                System.err.println("Sending fd " + fd);
                return (fd == -1) ? null : ParcelFileDescriptor.adoptFd(fd);
            }
        });

        ctx.sendBroadcast(intent);

        Looper.loop();
//        connect();
    }

    static native int connect();

    static {
        System.loadLibrary("lorie");
    }
}
