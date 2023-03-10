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
import java.io.File;
import java.io.RandomAccessFile;
import java.lang.reflect.InvocationTargetException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.channels.FileLock;
import java.util.Arrays;

public class CmdEntryPoint extends ICmdEntryInterface.Stub {
    public static final String ACTION_START = "com.termux.x11.CmdEntryPoint.ACTION_START";
    public static final int PORT = 7892;
    public static final byte[] MAGIC = "0xDEADBEEF".getBytes();

    @SuppressWarnings("FieldCanBeLocal")
    @SuppressLint("StaticFieldLeak")
    private static Handler handler;
    private final Context ctx;

    String helpText =
      "`termux-x11` does not accept arguments.\n" +
      "\n" +
      "Like any other X11 program, this program requires that the `DISPLAY` environment variable be set.\n" +
      "`DISPLAY=:0 termux-x11`\n" +
      "  or\n" +
      "`export DISPLAY=:0`\n" +
      "`termux-x11`\n" +
      "\n" +
      "The program does not support Xauth, so the X server must be launched with the `-ac` option or\n" +
      "the `xhost +` command must be executed when starting the desktop environment.\n" +
      "`Xvfb :0 -ac -screen 0 4096x4096`\n" +
      "\n" +
      "Program is designed to work only with `Xvfb`, but work with other X servers is also\n" +
      "possible but not stable.\n" +
      "You must start `Xvfb` with parameters `-screen 0 4096x4096x32` in the case if you want changing\n" +
      "display resolution to be available and fully supported. In the case if you not pass it **maximal**\n" +
      "resolution will be set to `1280x1024`.\n" +
      "\n" +
      "Proot:\n" +
      "If you plan to use the program with `proot`, keep in mind that you need to launch `proot`/`proot-distro`\n" +
      "with the `--shared-tmp` option. If passing this option is not possible, set the `TMPDIR` environment\n" +
      "variable to point to the directory that corresponds to /tmp in the target container.\n" +
      "\n" +
      "Chroot:\n" +
      "If you plan to use the program with `chroot` or `unshare`, you need to run it as root and set the\n" +
      "`TMPDIR` environment variable to point to the directory that corresponds to /tmp in the target container.\n" +
      "This directory must be accessible from the shell from which you launch termux-x11,\n" +
      "i.e. it must be in the same SELinux context, same mount namespace, and so on.\n" +
      "\n" +
      "Logs:\n" +
      "If you need to obtain logs from the `com.termux.x11` application,\n" +
      "set the `TERMUX_X11_DEBUG` environment variable to 1, like this:\n" +
      "    `DISPLAY=:0 TERMUX_X11_DEBUG=1 termux-x11`\n" +
      "\n" +
      "The log obtained in this way can be quite long.\n" +
      "It's better to redirect the output of the command to a file right away.\n" +
      "\n" +
      "Caveat:\n" +
      "`termux-x11` does not launch the `Termux:X11` application and does not bring it to the foreground\n" +
      "as previous versions did. The program is designed to be run only once, at the device or\n" +
      "Termux environment startup.\n";

    /**
     * Command-line entry point.
     *
     * @param args The command-line arguments
     */
    public static void main(String[] args) {
        setArgV0("termux-x11");
        if (!lockInstance("termux-x11.lock"))
            return;

        handler = new Handler();
        handler.post(() -> new CmdEntryPoint(args));
        Looper.loop();
    }

    CmdEntryPoint(String[] args) {
        if (System.getenv("DISPLAY") == null) {
            System.err.println("DISPLAY variable is not set. Please, read help article carefully.");
            System.err.println("  termux-x11 --help");
            System.exit(0);
        }

        if (args.length != 0) {
            printHelp();
            System.exit(0);
        }

        ctx = android.app.ActivityThread.systemMain().getSystemContext();
        spawnListeningThread();
        sendBroadcast();
    }

    void printHelp() {
        System.err.println(helpText);
    }

    @SuppressLint("WrongConstant")
    void sendBroadcast() {
        Bundle bundle = new Bundle();
        bundle.putBinder("", this);

        Intent intent = new Intent(ACTION_START);
        intent.putExtra("", bundle);
        intent.setPackage("com.termux.x11");

        if (getuid() == 0 || getuid() == 2000) {
            intent.setFlags(/* FLAG_RECEIVER_FROM_SHELL */ 0x00400000);
        }

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

    private static boolean lockInstance(@SuppressWarnings("SameParameterValue") final String lockName) {
        try {
            final File file = new File(System.getenv("TMPDIR") + "/" + lockName);
            //noinspection resource
            final RandomAccessFile randomAccessFile = new RandomAccessFile(file, "rw");
            //noinspection resource
            final FileLock fileLock = randomAccessFile.getChannel().tryLock();
            if (fileLock != null) {
                randomAccessFile.writeBytes(String.valueOf(android.os.Process.myPid()));
                Runtime.getRuntime().addShutdownHook(new Thread() {
                    public void run() {
                        try {
                            fileLock.release();
                            randomAccessFile.close();
                            //noinspection ResultOfMethodCallIgnored
                            file.delete();
                        } catch (Exception e) {
                            System.err.println("Unable to remove lock file: " + lockName);
                            e.printStackTrace();
                        }
                    }
                });
                return true;
            } else {
                int pid = Integer.parseInt(randomAccessFile.readLine());
                System.err.println("termux-x11 is already running. You can kill it with command `kill " + pid + "`");
            }
        } catch (Exception e) {
            System.err.println("Unable to create and/or lock file: " + lockName);
            e.printStackTrace();
        }
        return false;
    }

    private static void setArgV0(@SuppressWarnings("SameParameterValue") String name) {
        try {
            //noinspection JavaReflectionMemberAccess
            Process.class.
                    getDeclaredMethod("setArgv0", String.class).
                    invoke(null, name);
        } catch (NoSuchMethodException | InvocationTargetException | IllegalAccessException ignored) {}

        try {
            Class.forName("dalvik.system.VMRuntime").
                    getMethod("setProcessPackageName", String.class).
                    invoke(null, name);
        } catch (NoSuchMethodException | InvocationTargetException | IllegalAccessException |
                 ClassNotFoundException ignored) {}
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

    private static native int connect();
    private static native int stderr();
    private static native int getuid();

    static {
        System.loadLibrary("lorie");
    }
}
