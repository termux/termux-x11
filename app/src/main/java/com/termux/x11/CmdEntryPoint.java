package com.termux.x11;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;

import java.io.DataInputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Arrays;
import java.util.List;

public class CmdEntryPoint {
    public static final String ACTION_START = "com.termux.x11.CmdEntryPoint.ACTION_START";
    public static final int PORT = 7892;
    public static final byte[] MAGIC = "0xDEADBEEF".getBytes();

    // this constant is used in Xwayland itself
    // https://github.com/freedesktop/xorg-xserver/blob/ccdd431cd8f1cabae9d744f0514b6533c438908c/hw/xwayland/xwayland-screen.c#L66
    public static final int DEFAULT_DPI = 96;

    @SuppressLint("StaticFieldLeak")
    private static final Context ctx = android.app.ActivityThread.systemMain().getSystemContext();
    private static final Handler handler = new Handler();

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
     * TMPDIR should be accessible by termux-x11
     *
     * [4]
     * You can run `termux-x11 --no-xwayland-start` to spawn compositor, but not to spawn Termux's
     * Xwayland in the case you want to use Xwayland contained in chroot environment.
     * Do not forget about [2] to avoid application crashes.
     *
     * [5]
     * You can specify DPI by using `-dpi` option.
     * It is Xwayland option, but termux-x11 also handles it.
     *
     * @param args The command-line arguments
     */
    public static void main(String[] args) {
        handler.post(() -> new CmdEntryPoint(args));
        Looper.loop();
    }

    @SuppressWarnings("FieldCanBeLocal")
    private int dpi = DEFAULT_DPI;
    @SuppressWarnings("FieldCanBeLocal")
    private int display = 0;

    CmdEntryPoint(String[] args) {
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
            System.err.println("Starting Xwayland");

            if (!a.contains("--no-xwayland-start")) {
                exec(args);
            }
        }
    }

    void sendBroadcast() {
        Bundle bundle = new Bundle();
        // We should not care about multiple instances, it should be called only by `Termux:X11` app
        // which is single instance...
        bundle.putBinder("", new ICmdEntryInterface.Stub() {
            @Override
            public void outputResize(int width, int height) {
                CmdEntryPoint.this.outputResize(width, height);
            }

            @Override
            public ParcelFileDescriptor getXConnection() {
                return ParcelFileDescriptor.adoptFd(connect());
            }
        });

        Intent intent = new Intent(ACTION_START);
        intent.putExtra("com.termux.x11.starter", bundle);
        intent.setPackage("com.termux.x11");
        ctx.sendBroadcast(intent);
    }

    void spawnListeningThread() {
        new Thread(() -> {
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

    private native void spawnCompositor(int display, int dpi);
    private native void outputResize(int width, int height);
    private static native boolean socketExists();
    private static native void exec(String[] argv);
    private static native int connect();

    static {
        System.loadLibrary("lorie");
    }
}
