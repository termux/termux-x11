package com.termux.x11;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.view.Surface;

import java.io.DataInputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Arrays;

@SuppressLint("StaticFieldLeak")
public class CmdEntryPoint extends ICmdEntryInterface.Stub {
    public static final String ACTION_START = "com.termux.x11.CmdEntryPoint.ACTION_START";
    public static final int PORT = 7892;
    public static final byte[] MAGIC = "0xDEADBEEF".getBytes();
    private static final Handler handler = new Handler();
    public static Context ctx;

    /**
     * Command-line entry point.
     *
     * @param args The command-line arguments
     */
    public static void main(String[] args) {
        handler.post(() -> new CmdEntryPoint(args));
        Looper.loop();
    }

    CmdEntryPoint(String[] args) {
        try {
            if (ctx == null)
                ctx = android.app.ActivityThread.systemMain().getSystemContext();
        } catch (Exception e) {
            Log.e("CmdEntryPoint", "Problem during obtaining Context: ", e);
        }

        if (!start(args))
            System.exit(1);

        spawnListeningThread();
        sendBroadcast();
    }

    @SuppressLint("WrongConstant")
    void sendBroadcast() {
        // We should not care about multiple instances, it should be called only by `Termux:X11` app
        // which is single instance...
        Bundle bundle = new Bundle();
        bundle.putBinder("", this);

        Intent intent = new Intent(ACTION_START);
        intent.putExtra("", bundle);
        intent.setPackage("com.termux.x11");
        ctx.sendBroadcast(intent);
    }

    void spawnListeningThread() {
        new Thread(() -> { // New thread is needed to avoid android.os.NetworkOnMainThreadException
            /*
                The purpose of this function is simple. If the application has not been launched
                before running termux-x11, the initial sendBroadcast had no effect because no one
                received the intent. To allow the application to reconnect freely, we will listen on
                port `PORT` and when receiving a magic phrase, we will send another intent.
             */
            Log.e("CmdEntryPoint", "Listening port " + PORT);
            try (ServerSocket listeningSocket =
                         new ServerSocket(PORT, 0, InetAddress.getByName("127.0.0.1"))) {
                listeningSocket.setReuseAddress(true);
                while(true) {
                    try (Socket client = listeningSocket.accept()) {
                        Log.e("CmdEntryPoint", "Somebody connected!");
                        // We should ensure that it is some
                        byte[] b = new byte[MAGIC.length];
                        DataInputStream reader = new DataInputStream(client.getInputStream());
                        reader.readFully(b);
                        if (Arrays.equals(MAGIC, b)) {
                            Log.e("CmdEntryPoint", "New client connection!");
                            sendBroadcast();
                        }
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }).start();
    }

    public static void requestConnection() {
        System.err.println("Requesting connection...");
        new Thread(() -> { // New thread is needed to avoid android.os.NetworkOnMainThreadException
            try (Socket socket = new Socket("127.0.0.1", CmdEntryPoint.PORT)){
                socket.getOutputStream().write(CmdEntryPoint.MAGIC);
            } catch (Exception e) {
                Log.e("CmdEntryPoint", "Something went wrong when we requested connection", e);
            }
        }).start();
    }

    public static native boolean start(String[] args);
    public native void windowChanged(Surface surface);
    public native ParcelFileDescriptor getXConnection();
    public native ParcelFileDescriptor getLogcatOutput();

    static {
        System.loadLibrary("Xlorie");
    }
}
