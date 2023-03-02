package com.termux.x11;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import java.io.DataInputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Arrays;
import java.util.List;

public class CmdEntryPoint extends ICmdEntryInterface.Stub {
    public static final String ACTION_START = "com.termux.x11.CmdEntryPoint.ACTION_START";
    public static final int PORT = 7892;
    public static final byte[] MAGIC = "0xDEADBEEF".getBytes();

    // this constant is used in Xwayland itself
    // https://github.com/freedesktop/xorg-xserver/blob/ccdd431cd8f1cabae9d744f0514b6533c438908c/hw/xwayland/xwayland-screen.c#L66
    public static final int DEFAULT_DPI = 96;

    @SuppressLint("StaticFieldLeak")
    private static Handler handler;
    private final Context ctx;

    /**
     * Command-line entry point.
     * [1]
     * Application creates socket with name $WAYLAND_DISPLAY in folder $XDG_RUNTIME_DIR
     * If these environment variables are unset they are specified as
     * WAYLAND_DISPLAY="termux-x11"
     * XDG_RUNTIME_DIR="/data/data/com.termux/files/usr/tmp"
     * Any commandline arguments passed to this program are passed to Xwayland.
     * The only exception is `--no-xwayland-start`, read [4].
     *
     * [2]
     * Please, do not set WAYLAND_DISPLAY to "wayland-0", lots of programs (primarily GTK-based)
     * are trying to connect the compositor and crashing. Compositor in termux-x11 is a fake and a stub.
     * It is only designed to handle Xwayland.
     *
     * [3]
     * Also you should set TMPDIR environment variable in the case if you are running Xwayland in
     * non-termux environment. That is the only way to localize X connection socket.
     * TMPDIR should be accessible by termux-x11.
     *
     * [4]
     * You can run `termux-x11 :0 --no-xwayland-start` to spawn compositor, but not to spawn Termux's
     * Xwayland in the case you want to use Xwayland contained in chroot environment.
     * Do not forget about [2] to avoid application crashes.
     * Also you must specify the same display number you will set to Xwayland.
     * You must start `termux-x11` as root if you want to make it work with chroot'd Xwayland.
     * `root@termux: ~# WAYLAND_DISPLAY="termux-x11" XDG_RUNTIME_DIR=/<chroot path>/var/run/1000/ TMDIR=/<chroot path>/tmp/ termux-x11 :0 --no-xwayland-start`
     * `root@chroot: ~# WAYLAND_DISPLAY="termux-x11" XDG_RUNTIME_DIR=/<chroot path>/var/run/1000/ TMDIR=/<chroot path>/tmp/ Xwayland :0 -ac`
     *
     * [5]
     * You can specify DPI by using `-dpi` option.
     * It is Xwayland option, but termux-x11 also handles it.
     *
     * [6]
     * Start termux-x11 with TERMUX_X11_DEBUG=1 flag to redirect Termux:X11 logcat to termux-x11 stderr.
     * `env TERMUX_X11_DEBUG=1 termux-x11 :0 -ac`
     *
     * @param args The command-line arguments
     */
    public static void main(String[] args) {
        handler = new Handler();
        handler.post(() -> new CmdEntryPoint(args));
        Looper.loop();
    }

    @SuppressWarnings("FieldCanBeLocal")
    private int dpi = DEFAULT_DPI;
    @SuppressWarnings("FieldCanBeLocal")
    private int display = 0;

    CmdEntryPoint(String[] args) {
        ctx = android.app.ActivityThread.systemMain().getSystemContext();
        if (socketExists()) {
            System.err.println("termux-x11 is already running. You can kill it with command `pkill -9 Xwayland`");
            handler.post(() -> System.exit(0));
        } else {
            List<String> a = Arrays.asList(args);
            try {
                int dpiIndex = a.lastIndexOf("-dpi");
                if (dpiIndex != -1 && a.size() >= dpiIndex)
                    dpi = Integer.parseInt(a.get(dpiIndex + 1));
            } catch (NumberFormatException ignored) {} // No need to handle it, anyway user will see Xwayland error

            try {
                for (String arg: a)
                    if (arg.startsWith(":"))
                        display = Integer.parseInt(arg.substring(1));
            } catch (NumberFormatException ignored) {} // No need to handle it, anyway user will see Xwayland error

            spawnCompositor(display, dpi);
            spawnListeningThread();
            sendBroadcast();

            if (!a.contains("--no-xwayland-start")) {
                System.err.println("Starting Xwayland");
                exec(args);
            }
        }
    }

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
            try (ServerSocket listeningSocket =
                         new ServerSocket(PORT, 0, InetAddress.getByName("127.0.0.1"))) {
                listeningSocket.setReuseAddress(true);
                while(true) {
                    try (Socket client = listeningSocket.accept()) {
                        System.err.println("Somebody connected!");
                        // We should ensure that it is some
                        byte[] b = new byte[MAGIC.length];
                        DataInputStream reader = new DataInputStream(client.getInputStream());
                        reader.readFully(b);
                        if (Arrays.equals(MAGIC, b)) {
                            System.err.println("New client connection!");
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

    @Override
    public ParcelFileDescriptor getXConnection() {
        int fd = connect();
        return fd == -1 ? null : ParcelFileDescriptor.adoptFd(fd);
    }

    @Override
    public ParcelFileDescriptor getLogcatOutput() {
        int fd = stderr();
        return fd == -1 ? null : ParcelFileDescriptor.adoptFd(fd);
    }

    private native void spawnCompositor(int display, int dpi);
    public native void outputResize(int width, int height);
    private static native boolean socketExists();
    private static native void exec(String[] argv);
    private static native int connect();
    private static native int stderr();

    static {
        System.loadLibrary("lorie");
    }
}
