package com.displaylink.manager;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.util.Log;
import android.view.Surface;

import androidx.annotation.Keep;

import com.displaylink.manager.display.DisplayMode;

import java.io.File;
import java.nio.ByteBuffer;

@Keep
public class NativeDriver {
    private static final String TAG = "DisplayLinkNativeDriver";
    private static final String DISPLAYLINK_PACKAGE = "com.displaylink.presenter";
    private static final String[] REQUIRED_LIBS = {
        "libusb_android.so",
        "libAndroidDLM.so",
        "libDisplayLinkManager.so",
    };

    private static volatile boolean sLoaded;

    public static synchronized void ensureLoaded(Context context) throws PackageManager.NameNotFoundException {
        if (sLoaded) {
            return;
        }

        ApplicationInfo info = context.getPackageManager().getApplicationInfo(DISPLAYLINK_PACKAGE, 0);
        String nativeLibraryDir = info.nativeLibraryDir;
        if (nativeLibraryDir == null) {
            throw new IllegalStateException("DisplayLink nativeLibraryDir missing");
        }

        for (String lib : REQUIRED_LIBS) {
            File path = new File(nativeLibraryDir, lib);
            Log.i(TAG, "Loading " + path);
            System.load(path.getAbsolutePath());
        }

        sLoaded = true;
    }

    public native int create(NativeDriverListener nativeDriverListener, String firmwareDirectory, boolean forceCpuEncoding);

    public native long createImageReader(long encoder, DisplayMode displayMode, boolean z, boolean z2);

    public native void destroy();

    public native void destroyImageReader(long imageReaderHandle);

    public native Surface getImageReaderSurface(long imageReaderHandle);

    public native boolean isValid();

    public native void log(String tag, int priority, String message);

    public native void notifyEvent(int eventCode);

    public native int postFrame(long encoder, ByteBuffer byteBuffer);

    public native boolean receiveDdcCiData(long encoder, byte[] data, int length);

    public native boolean sendDdcCiData(long encoder, byte[] data, int length);

    public native void setMode(long encoder, DisplayMode displayMode, int rowStride, int format);

    public native void setProtection(long encoder, boolean protectedContent);

    public native int usbDeviceAttached(String deviceName, int fd, byte[] descriptors, int descriptorLength);

    public native void usbDeviceDetached(String deviceName);
}
