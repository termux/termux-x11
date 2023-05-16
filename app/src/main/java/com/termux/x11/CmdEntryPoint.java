package com.termux.x11;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.view.Surface;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.MulticastSocket;

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
        if (ctx == null)
            ctx = android.app.ActivityThread.systemMain().getSystemContext();

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
            /*  The purpose of this function is simple. If the application has not been launched
                before running termux-x11, the initial sendBroadcast had no effect because no one
                received the intent. To allow the application to reconnect freely, we will listen on
                port `PORT` and when receiving a magic phrase, we will send another intent. */
        new Thread(() -> { // New thread is needed to avoid android.os.NetworkOnMainThreadException
            try (MulticastSocket socket = new MulticastSocket(PORT)) {
                InetAddress group = InetAddress.getByName("230.0.0.0");
                socket.joinGroup(group);
                DatagramPacket packet = new DatagramPacket(MAGIC, MAGIC.length);
                while(true) {
                    socket.receive(packet);
                    sendBroadcast();
                }
            } catch (IOException e) { throw new RuntimeException(e); }
        }).start();
    }

    public static void requestConnection() {
        System.err.println("Requesting connection...");
        new Thread(() -> { // New thread is needed to avoid android.os.NetworkOnMainThreadException
//            try (MulticastSocket socket = new MulticastSocket()) {
//                InetAddress group = InetAddress.getByName("230.0.0.0");
//                socket.joinGroup(group);
//                byte[] buffer = "Hello".getBytes();
//                DatagramPacket packet = new DatagramPacket(MAGIC, MAGIC.length, group, PORT);
//                socket.send(packet);
//                socket.leaveGroup(group);
//            } catch (IOException e) { throw new RuntimeException(e); }
            try (DatagramSocket socket = new DatagramSocket()) {
                InetAddress group = InetAddress.getByName("230.0.0.0");
                DatagramPacket packet = new DatagramPacket(MAGIC, MAGIC.length, group, PORT);
                socket.send(packet);
            } catch (IOException e) { throw new RuntimeException(e); }
        }).start();
    }

    public static native boolean start(String[] args);
    public native void windowChanged(Surface surface);
    public native ParcelFileDescriptor getXConnection();
    public native ParcelFileDescriptor getLogcatOutput();

    static {
        System.loadLibrary("Xlorie");
    }

    /** That class is only needed to test X server with Android Studio debugger */
    public static class FakeLoader extends Activity {
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            startForegroundService(new Intent(this, ForegroundService.class));
            finish();
        }
    }

    /** That class is only needed to test X server with Android Studio debugger */
    public static class ForegroundService extends Service {
        public int onStartCommand(Intent intent, int flags, int startId) {
            NotificationManager manager = getSystemService(NotificationManager.class);
            manager.createNotificationChannel(new NotificationChannel("Channel", "Channel", NotificationManager.IMPORTANCE_NONE));
            startForeground(1, new Notification.Builder(this, "Channel")
                    .setContentIntent(PendingIntent.getActivity(this, 0, new Intent(), PendingIntent.FLAG_IMMUTABLE))
                    .setContentTitle("Foreground Service").
                    build());

            CmdEntryPoint.ctx = this;
            new Thread(() -> {
                Looper.prepare();
                CmdEntryPoint.main(new String[] { ":1", "-listen", "tcp", "-ac" });
            }).start();
            return START_NOT_STICKY;
        }

        public IBinder onBind(Intent intent) {
            return null;
        }
    }
}
