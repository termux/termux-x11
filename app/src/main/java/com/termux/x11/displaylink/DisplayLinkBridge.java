package com.termux.x11.displaylink;

import static android.app.PendingIntent.FLAG_IMMUTABLE;
import static android.app.PendingIntent.FLAG_MUTABLE;
import static android.app.PendingIntent.FLAG_UPDATE_CURRENT;
import static android.os.Build.VERSION.SDK_INT;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.util.Log;

import androidx.annotation.Keep;
import androidx.annotation.Nullable;

import com.displaylink.manager.NativeDriver;
import com.displaylink.manager.NativeDriverListener;
import com.displaylink.manager.display.DisplayMode;
import com.displaylink.manager.display.MonitorInfo;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.IntBuffer;
import java.nio.charset.StandardCharsets;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

@Keep
public final class DisplayLinkBridge {
    private static final String TAG = "DisplayLinkBridge";
    private static final String DISPLAYLINK_PACKAGE = "com.displaylink.presenter";
    private static final String ACTION_USB_PERMISSION = "com.displaylink.ACTION_USB_PERMISSION";
    private static final String ACTION_USB_DEVICE_ATTACHED = "com.displaylink.ACTION_USB_DEVICE_ATTACHED";
    private static final String ACTION_DETECT_DEVICES = "com.displaylink.ACTION_DETECT_DEVICES";
    private static final int DISPLAYLINK_VENDOR_ID = 0x17e9;
    private static final String MARKER_FILE_NAME = "displaylink-bridge.log";
    private static final int EVENT_SCREEN_OFF = 0;
    private static final int EVENT_SCREEN_ON = 1;
    private static final int MAX_ATTACH_RETRIES = 2;
    private static final int MAX_DRIVER_RESTARTS = 2;
    private static final int INITIAL_FRAME_REPLAY_COUNT = 0;
    private static final int INITIAL_FRAME_REPLAY_DELAY_MS = 80;

    private static final DisplayLinkBridge INSTANCE = new DisplayLinkBridge();

    private final Object lock = new Object();
    private final Map<String, UsbDeviceConnection> connections = new ConcurrentHashMap<>();
    private final Map<String, Integer> attachRetryCounts = new ConcurrentHashMap<>();
    private final Set<String> pendingPermissionDevices = new HashSet<>();
    private final Set<String> deniedPermissionDevices = new HashSet<>();
    private final Set<String> attachingDevices = new HashSet<>();
    private final BroadcastReceiver usbReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
            Context markerContext = appContext != null ? appContext : context;
            String deviceName = device != null ? device.getDeviceName() : "null";
            writeMarker(markerContext, "bridge.usbReceiver.onReceive action=" + action + " device=" + deviceName);

            if (ACTION_USB_PERMISSION.equals(action)) {
                boolean granted = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false);
                writeMarker(markerContext, "bridge.usbReceiver.permission device=" + deviceName + " granted=" + granted);
                Log.i(TAG, "USB permission result name=" + deviceName + " granted=" + granted);
                handlePermissionResult(device, granted);
                return;
            }

            if (ACTION_DETECT_DEVICES.equals(action)) {
                writeMarker(markerContext, "bridge.usbReceiver.detect_devices");
                scheduleProbe("detect_devices", 0);
                return;
            }

            if (device == null || !isDisplayLinkDevice(device)) {
                return;
            }

            if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)
                    || ACTION_USB_DEVICE_ATTACHED.equals(action)) {
                writeMarker(markerContext, "bridge.usbReceiver.attached name=" + device.getDeviceName());
                Log.i(TAG, "USB attached " + device.getDeviceName());
                handleDeviceAttached(device);
                return;
            }

            if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
                writeMarker(markerContext, "bridge.usbReceiver.detached name=" + device.getDeviceName());
                Log.i(TAG, "USB detached " + device.getDeviceName());
                handleDeviceDetached(device);
            }
        }
    };

    private Context appContext;
    private UsbManager usbManager;
    private NativeDriver nativeDriver;
    private boolean started;
    private boolean starting;
    private long activeEncoder;
    private int lastWidth;
    private int lastHeight;
    private int lastRowStride;
    private int framePostCount;
    private final ByteBuffer[] frameBuffers = new ByteBuffer[2];
    private int retainedFrameBufferIndex = -1;
    @Nullable private ByteBuffer clearFrameTemplate;
    @Nullable private ByteBuffer pendingFrameBuffer;
    private int pendingFrameWidth;
    private int pendingFrameHeight;
    private int pendingFrameRowStride;
    private boolean replayScheduled;
    private boolean driverRestartScheduled;
    private int driverRestartCount;
    @Nullable private MonitorInfo lastMonitorInfo;
    @Nullable private String outstandingPermissionDeviceName;
    private boolean permissionRequestOutstanding;

    private DisplayLinkBridge() {
    }

    public static DisplayLinkBridge getInstance() {
        return INSTANCE;
    }

    @Nullable
    public Context getAppContextForDebug() {
        return appContext;
    }

    public static void writeMarker(Context context, String message) {
        if (context == null || message == null) {
            return;
        }

        File marker = new File(context.getCacheDir(), MARKER_FILE_NAME);
        String line = System.currentTimeMillis() + " " + message + "\n";
        try (FileOutputStream out = new FileOutputStream(marker, true)) {
            out.write(line.getBytes(StandardCharsets.UTF_8));
            out.flush();
        } catch (IOException e) {
            Log.w(TAG, "Failed to write marker", e);
        }
    }

    public void start(Context context) {
        writeMarker(context, "bridge.start.enter context=" + context);
        if (context == null) {
            Log.w(TAG, "start() skipped because context is null");
            return;
        }

        synchronized (lock) {
            if (started || starting) {
                Log.i(TAG, "start() ignored started=" + started + " starting=" + starting);
                return;
            }

            starting = true;
        }

        Context resolvedContext = context.getApplicationContext();
        if (resolvedContext == null) {
            resolvedContext = context;
        }
        if (resolvedContext == null) {
            synchronized (lock) {
                starting = false;
            }
            writeMarker(context, "bridge.start.skip application_context_unavailable");
            Log.w(TAG, "start() skipped because application context is unavailable");
            return;
        }
        final Context appContext = resolvedContext;
        writeMarker(appContext, "bridge.start.schedule_init_thread");
        Log.i(TAG, "start() scheduling init thread");
        Thread initThread = new Thread(() -> startInternal(appContext), "DisplayLinkBridgeInit");
        initThread.setDaemon(true);
        initThread.start();
    }

    private void startInternal(Context context) {
        boolean ready = false;
        NativeDriver createdDriver = null;

        writeMarker(context, "bridge.startInternal.begin context=" + context);
        Log.i(TAG, "startInternal() begin");

        if (context == null) {
            synchronized (lock) {
                starting = false;
            }
            writeMarker(appContext, "bridge.startInternal.abort null_context");
            Log.w(TAG, "startInternal() aborted because context is null");
            return;
        }

        synchronized (lock) {
            if (started) {
                writeMarker(context, "bridge.startInternal.already_started");
                Log.i(TAG, "startInternal() exiting because already started");
                starting = false;
                return;
            }

            appContext = context;
            writeMarker(appContext, "bridge.startInternal.set_app_context");
            usbManager = (UsbManager) appContext.getSystemService(Context.USB_SERVICE);
            if (usbManager == null) {
                writeMarker(appContext, "bridge.startInternal.usb_manager_unavailable");
                Log.e(TAG, "UsbManager unavailable");
                starting = false;
                return;
            }

            try {
                writeMarker(appContext, "bridge.startInternal.ensureLoaded.begin");
                Log.i(TAG, "Calling NativeDriver.ensureLoaded()");
                NativeDriver.ensureLoaded(appContext);
                writeMarker(appContext, "bridge.startInternal.ensureLoaded.done");
                Log.i(TAG, "NativeDriver.ensureLoaded() finished");
                File firmwareDir = prepareFirmwareDirectory(appContext);
                writeMarker(appContext, "bridge.startInternal.prepareFirmware.done dir=" + firmwareDir);
                nativeDriver = new NativeDriver();
                int rc = nativeDriver.create(new NativeDriverListener(), firmwareDir.getAbsolutePath(), false);
                writeMarker(appContext, "bridge.startInternal.nativeCreate rc=" + rc);
                Log.i(TAG, "NativeDriver.create rc=" + rc + " firmwareDir=" + firmwareDir);
                if (rc != 0 || !nativeDriver.isValid()) {
                    writeMarker(appContext, "bridge.startInternal.native_invalid");
                    Log.e(TAG, "DisplayLink native driver did not become valid");
                    nativeDriver = null;
                    starting = false;
                    return;
                }

                createdDriver = nativeDriver;
            } catch (Throwable e) {
                writeMarker(appContext, "bridge.startInternal.exception " + e.getClass().getName() + ":" + e.getMessage());
                Log.e(TAG, "Failed to initialise DisplayLink driver", e);
                nativeDriver = null;
                starting = false;
                return;
            }

            registerUsbReceiver(appContext);
            writeMarker(appContext, "bridge.startInternal.receiver_registered");
            started = true;
            starting = false;
            ready = true;
        }

        if (ready) {
            notifyDriverEvent(createdDriver, appContext, EVENT_SCREEN_ON, "startInternal");
            writeMarker(appContext, "bridge.startInternal.probe_devices");
            Log.i(TAG, "startInternal() probing attached USB devices");
            probeAttachedDevices();
        }
    }

    public void stop() {
        Context context;
        NativeDriver driver;
        List<Map.Entry<String, UsbDeviceConnection>> openConnections;
        synchronized (lock) {
            if (!started) {
                return;
            }

            context = appContext;
            driver = nativeDriver;
            openConnections = new ArrayList<>(connections.entrySet());
            connections.clear();
            attachRetryCounts.clear();
            nativeDriver = null;
            activeEncoder = 0;
            lastMonitorInfo = null;
            lastWidth = 0;
            lastHeight = 0;
            lastRowStride = 0;
            framePostCount = 0;
            clearFrameBuffersLocked();
            replayScheduled = false;
            clearPendingFrameLocked();
            pendingPermissionDevices.clear();
            deniedPermissionDevices.clear();
            attachingDevices.clear();
            outstandingPermissionDeviceName = null;
            permissionRequestOutstanding = false;
            driverRestartScheduled = false;
            driverRestartCount = 0;
            started = false;
            starting = false;

            try {
                if (context != null) {
                    context.unregisterReceiver(usbReceiver);
                }
            } catch (IllegalArgumentException e) {
                Log.w(TAG, "Receiver already unregistered", e);
            }
        }

        for (Map.Entry<String, UsbDeviceConnection> entry : openConnections) {
            try {
                if (driver != null) {
                    driver.usbDeviceDetached(entry.getKey());
                }
            } catch (Throwable e) {
                Log.w(TAG, "usbDeviceDetached failed for " + entry.getKey(), e);
            }
            entry.getValue().close();
        }

        if (driver != null) {
            try {
                notifyDriverEvent(driver, context, EVENT_SCREEN_OFF, "stop");
                driver.destroy();
            } catch (Throwable e) {
                Log.w(TAG, "NativeDriver.destroy failed", e);
            }
        }
    }

    @Keep
    public static boolean pushFrame(ByteBuffer buffer, int width, int height, int rowStride) {
        Context context = INSTANCE.appContext;
        if (context != null && INSTANCE.framePostCount == 0) {
            writeMarker(context, "bridge.pushFrame.entry width=" + width + " height=" + height + " rowStride=" + rowStride);
        }
        return INSTANCE.pushFrameInternal(buffer, width, height, rowStride);
    }

    @Keep
    public static boolean isCaptureReady() {
        return INSTANCE.isCaptureReadyInternal();
    }

    @Keep
    public static int getCaptureWidth() {
        return INSTANCE.getCaptureWidthInternal();
    }

    @Keep
    public static int getCaptureHeight() {
        return INSTANCE.getCaptureHeightInternal();
    }

    private boolean isCaptureReadyInternal() {
        synchronized (lock) {
            return started && nativeDriver != null && activeEncoder != 0;
        }
    }

    private int getCaptureWidthInternal() {
        synchronized (lock) {
            DisplayMode preferred = preferredModeOf(lastMonitorInfo);
            return preferred != null ? preferred.width : 0;
        }
    }

    private int getCaptureHeightInternal() {
        synchronized (lock) {
            DisplayMode preferred = preferredModeOf(lastMonitorInfo);
            return preferred != null ? preferred.height : 0;
        }
    }

    public void onDisplayConnected(long encoder) {
        synchronized (lock) {
            activeEncoder = encoder;
            lastWidth = 0;
            lastHeight = 0;
            lastRowStride = 0;
            clearFrameBuffersLocked();
            attachRetryCounts.clear();
            driverRestartScheduled = false;
            driverRestartCount = 0;
            if (nativeDriver != null) {
                try {
                    nativeDriver.setProtection(encoder, false);
                    writeMarker(appContext, "bridge.onDisplayConnected.setProtection encoder=0x" + Long.toHexString(encoder) + " protected=false");
                } catch (Throwable e) {
                    writeMarker(appContext, "bridge.onDisplayConnected.setProtection_failed encoder=0x" + Long.toHexString(encoder)
                            + " error=" + e.getClass().getSimpleName());
                    Log.w(TAG, "setProtection(false) failed for encoder=0x" + Long.toHexString(encoder), e);
                }
            }
            writeMarker(appContext, "bridge.onDisplayConnected encoder=0x" + Long.toHexString(encoder));
            Log.i(TAG, "Active encoder set to 0x" + Long.toHexString(encoder));
        }

        flushPendingFrame("display_connected");
    }

    public void onDisplayDisconnected(long encoder) {
        synchronized (lock) {
            if (activeEncoder == encoder) {
                activeEncoder = 0;
                lastWidth = 0;
                lastHeight = 0;
                lastRowStride = 0;
                lastMonitorInfo = null;
            }
            writeMarker(appContext, "bridge.onDisplayDisconnected encoder=0x" + Long.toHexString(encoder));
        }
    }

    public void onMonitorInfo(long encoder, @Nullable MonitorInfo monitorInfo) {
        synchronized (lock) {
            boolean encoderChanged = activeEncoder != 0 && activeEncoder != encoder;
            boolean modeChanged = !samePreferredMode(lastMonitorInfo, monitorInfo);

            if (activeEncoder == 0 || activeEncoder == encoder) {
                activeEncoder = encoder;
                lastMonitorInfo = monitorInfo;
                if (encoderChanged || modeChanged) {
                    lastWidth = 0;
                    lastHeight = 0;
                    lastRowStride = 0;
                    clearFrameBuffersLocked();
                }
            }
            int modeCount = monitorInfo == null || monitorInfo.a == null ? 0 : monitorInfo.a.length;
            writeMarker(appContext, "bridge.onMonitorInfo encoder=0x" + Long.toHexString(encoder) + " modeCount=" + modeCount);
            if (modeCount > 0) {
                DisplayMode preferred = monitorInfo.a[0];
                writeMarker(appContext, "bridge.onMonitorInfo.preferred encoder=0x" + Long.toHexString(encoder)
                        + " width=" + preferred.width + " height=" + preferred.height
                        + " refresh=" + preferred.refreshRate);
            }
        }

        flushPendingFrame("monitor_info");
    }

    private boolean pushFrameInternal(ByteBuffer buffer, int width, int height, int rowStride) {
        boolean deferPendingFrame = false;

        synchronized (lock) {
            if (!started || nativeDriver == null || buffer == null) {
                writeMarker(appContext, "bridge.pushFrame.skip started=" + started + " nativeDriver=" + (nativeDriver != null)
                        + " activeEncoder=0x" + Long.toHexString(activeEncoder) + " buffer=" + (buffer != null));
                return false;
            }

            if (activeEncoder == 0) {
                deferPendingFrame = true;
            } else {
                DisplayMode mode = chooseMode(width, height);
                if (mode == null) {
                    return false;
                }

                ByteBuffer postBuffer = preparePostBufferLocked(buffer, width, height, rowStride, mode);
                if (postBuffer == null) {
                    writeMarker(appContext, "bridge.pushFrame.prepare_failed encoder=0x" + Long.toHexString(activeEncoder));
                    return false;
                }

                int targetRowStride = mode.width * 4;

                if (lastWidth != mode.width || lastHeight != mode.height || lastRowStride != targetRowStride) {
                    writeMarker(appContext, "bridge.pushFrame.setMode encoder=0x" + Long.toHexString(activeEncoder)
                            + " width=" + mode.width + " height=" + mode.height + " rowStride=" + targetRowStride);
                    Log.i(TAG, "Setting mode encoder=0x" + Long.toHexString(activeEncoder) + " mode=" + mode
                            + " rowStride=" + targetRowStride + " source=" + width + "x" + height);
                    nativeDriver.setMode(activeEncoder, mode, targetRowStride, 1);
                    lastWidth = mode.width;
                    lastHeight = mode.height;
                    lastRowStride = targetRowStride;
                }

                int rc = nativeDriver.postFrame(activeEncoder, postBuffer);
                if (rc < 0) {
                    writeMarker(appContext, "bridge.pushFrame.post_failed rc=" + rc + " encoder=0x"
                            + Long.toHexString(activeEncoder));
                    Log.e(TAG, "postFrame failed rc=" + rc + " encoder=0x" + Long.toHexString(activeEncoder));
                    return false;
                }

                if (rc != 1 && rc != -2) {
                    retainedFrameBufferIndex = indexOfBuffer(postBuffer);
                }

                framePostCount++;
                if (framePostCount <= 3 || framePostCount % 60 == 0) {
                    writeMarker(appContext, "bridge.pushFrame.post_ok count=" + framePostCount
                            + " encoder=0x" + Long.toHexString(activeEncoder)
                            + " width=" + mode.width + " height=" + mode.height + " rowStride=" + targetRowStride
                            + " rc=" + rc + " source=" + width + "x" + height);
                }

                if (framePostCount == 1 && INITIAL_FRAME_REPLAY_COUNT > 0 && !replayScheduled) {
                    replayScheduled = true;
                    scheduleFrameReplay(activeEncoder, INITIAL_FRAME_REPLAY_COUNT, INITIAL_FRAME_REPLAY_DELAY_MS);
                }

                return true;
            }
        }

        if (deferPendingFrame) {
            ByteBuffer copiedBuffer = copyFrameBuffer(buffer, rowStride * height);
            synchronized (lock) {
                if (!started || nativeDriver == null) {
                    return false;
                }

                if (activeEncoder == 0) {
                    pendingFrameBuffer = copiedBuffer;
                    pendingFrameWidth = width;
                    pendingFrameHeight = height;
                    pendingFrameRowStride = rowStride;
                    writeMarker(appContext, "bridge.pushFrame.defer width=" + width + " height=" + height
                            + " rowStride=" + rowStride);
                    return false;
                }
            }

            return pushFrameInternal(copiedBuffer, width, height, rowStride);
        }

        return false;
    }

    private boolean samePreferredMode(@Nullable MonitorInfo left, @Nullable MonitorInfo right) {
        DisplayMode leftPreferred = preferredModeOf(left);
        DisplayMode rightPreferred = preferredModeOf(right);

        if (leftPreferred == null || rightPreferred == null) {
            return leftPreferred == rightPreferred;
        }

        return leftPreferred.width == rightPreferred.width
                && leftPreferred.height == rightPreferred.height
                && leftPreferred.refreshRate == rightPreferred.refreshRate;
    }

    @Nullable
    private DisplayMode preferredModeOf(@Nullable MonitorInfo info) {
        if (info == null || info.a == null || info.a.length == 0) {
            return null;
        }
        return info.a[0];
    }

    @Nullable
    private DisplayMode chooseMode(int width, int height) {
        MonitorInfo info = lastMonitorInfo;
        if (info != null && info.a != null && info.a.length > 0) {
            DisplayMode preferred = info.a[0];
            if (preferred.width != width || preferred.height != height) {
                Log.w(TAG, "Source size " + width + "x" + height + " differs from preferred mode " + preferred);
            }
            return preferred;
        }

        return new DisplayMode(width, height, 60);
    }

    @Nullable
    private ByteBuffer preparePostBufferLocked(ByteBuffer source, int sourceWidth, int sourceHeight, int sourceRowStride,
                                               DisplayMode mode) {
        int targetWidth = mode.width;
        int targetHeight = mode.height;
        int targetRowStride = targetWidth * 4;
        int targetSize = targetRowStride * targetHeight;

        int bufferIndex = retainedFrameBufferIndex == 0 ? 1 : 0;
        ByteBuffer target = frameBuffers[bufferIndex];
        if (target == null || target.capacity() != targetSize) {
            target = ByteBuffer.allocateDirect(targetSize);
            frameBuffers[bufferIndex] = target;
        }

        if (sourceWidth == targetWidth && sourceHeight == targetHeight) {
            copyFrame(source, sourceRowStride, target, targetRowStride, sourceWidth, sourceHeight);
        } else {
            clearBuffer(target, targetSize);
            blitScaledCentered(source, sourceWidth, sourceHeight, sourceRowStride, target, targetWidth, targetHeight, targetRowStride);
        }

        target.position(0);
        target.limit(targetSize);
        return target;
    }

    private void copyFrame(ByteBuffer source, int sourceRowStride, ByteBuffer target, int targetRowStride, int width, int height) {
        ByteBuffer sourceRow = source.duplicate();
        ByteBuffer targetRow = target.duplicate();
        int rowBytes = width * 4;

        for (int y = 0; y < height; y++) {
            int sourceOffset = (height - 1 - y) * sourceRowStride;
            int targetOffset = y * targetRowStride;

            sourceRow.position(sourceOffset);
            sourceRow.limit(sourceOffset + rowBytes);
            targetRow.position(targetOffset);
            targetRow.put(sourceRow);
        }
    }

    private void blitScaledCentered(ByteBuffer source, int sourceWidth, int sourceHeight, int sourceRowStride,
                                    ByteBuffer target, int targetWidth, int targetHeight, int targetRowStride) {
        float scale = Math.min((float) targetWidth / (float) sourceWidth, (float) targetHeight / (float) sourceHeight);
        int scaledWidth = Math.max(1, Math.round(sourceWidth * scale));
        int scaledHeight = Math.max(1, Math.round(sourceHeight * scale));
        int offsetX = (targetWidth - scaledWidth) / 2;
        int offsetY = (targetHeight - scaledHeight) / 2;
        int sourceStridePixels = sourceRowStride / 4;
        int targetStridePixels = targetRowStride / 4;
        IntBuffer sourcePixels = source.duplicate().asIntBuffer();
        IntBuffer targetPixels = target.duplicate().asIntBuffer();

        for (int y = 0; y < scaledHeight; y++) {
            int sourceY = y * sourceHeight / scaledHeight;
            int flippedSourceY = sourceHeight - 1 - sourceY;
            int sourceRowBase = flippedSourceY * sourceStridePixels;
            int targetRowBase = (offsetY + y) * targetStridePixels + offsetX;
            for (int x = 0; x < scaledWidth; x++) {
                int sourceX = x * sourceWidth / scaledWidth;
                targetPixels.put(targetRowBase + x, sourcePixels.get(sourceRowBase + sourceX));
            }
        }
    }

    private void clearBuffer(ByteBuffer buffer, int size) {
        if (clearFrameTemplate == null || clearFrameTemplate.capacity() != size) {
            clearFrameTemplate = ByteBuffer.allocateDirect(size);
        }

        ByteBuffer blank = clearFrameTemplate.duplicate();
        blank.position(0);
        blank.limit(size);
        buffer.position(0);
        buffer.limit(size);
        buffer.put(blank);
        buffer.position(0);
        buffer.limit(size);
    }

    private int indexOfBuffer(ByteBuffer buffer) {
        for (int i = 0; i < frameBuffers.length; i++) {
            if (frameBuffers[i] == buffer) {
                return i;
            }
        }
        return -1;
    }

    private void clearFrameBuffersLocked() {
        Arrays.fill(frameBuffers, null);
        retainedFrameBufferIndex = -1;
        replayScheduled = false;
        clearFrameTemplate = null;
    }

    private void clearPendingFrameLocked() {
        pendingFrameBuffer = null;
        pendingFrameWidth = 0;
        pendingFrameHeight = 0;
        pendingFrameRowStride = 0;
    }

    private ByteBuffer copyFrameBuffer(ByteBuffer buffer, int size) {
        ByteBuffer source = buffer.duplicate();
        ByteBuffer copy = ByteBuffer.allocateDirect(size);
        source.position(0);
        source.limit(size);
        copy.put(source);
        copy.position(0);
        copy.limit(size);
        return copy;
    }

    private void flushPendingFrame(String reason) {
        ByteBuffer pending;
        int width;
        int height;
        int rowStride;

        synchronized (lock) {
            if (!started || nativeDriver == null || activeEncoder == 0 || pendingFrameBuffer == null) {
                return;
            }

            pending = pendingFrameBuffer.duplicate();
            pending.position(0);
            pending.limit(pendingFrameBuffer.limit());
            width = pendingFrameWidth;
            height = pendingFrameHeight;
            rowStride = pendingFrameRowStride;
            clearPendingFrameLocked();
        }

        writeMarker(appContext, "bridge.pendingFrame.flush reason=" + reason + " width=" + width
                + " height=" + height + " rowStride=" + rowStride);
        pushFrameInternal(pending, width, height, rowStride);
    }

    private void scheduleFrameReplay(long encoder, int repeats, long delayMs) {
        Thread replayThread = new Thread(() -> {
            for (int i = 0; i < repeats; i++) {
                try {
                    Thread.sleep(delayMs);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    return;
                }

                synchronized (lock) {
                    if (!started || nativeDriver == null || activeEncoder != encoder || retainedFrameBufferIndex < 0) {
                        replayScheduled = false;
                        return;
                    }

                    ByteBuffer retained = frameBuffers[retainedFrameBufferIndex];
                    if (retained == null) {
                        replayScheduled = false;
                        return;
                    }

                    int rc = nativeDriver.postFrame(activeEncoder, retained);
                    writeMarker(appContext, "bridge.pushFrame.replay index=" + (i + 1) + " rc=" + rc
                            + " encoder=0x" + Long.toHexString(activeEncoder));
                }
            }

            synchronized (lock) {
                replayScheduled = false;
            }
        }, "DisplayLinkFrameReplay");
        replayThread.setDaemon(true);
        replayThread.start();
    }

    private void registerUsbReceiver(Context context) {
        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        filter.addAction(ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(ACTION_DETECT_DEVICES);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        if (SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            context.registerReceiver(usbReceiver, filter, Context.RECEIVER_EXPORTED);
        } else {
            context.registerReceiver(usbReceiver, filter);
        }
    }

    private void probeAttachedDevices() {
        synchronized (lock) {
            if (!started || usbManager == null) {
                return;
            }
        }

        writeMarker(appContext, "bridge.probeAttachedDevices.enter");
        List<UsbDevice> displayLinkDevices = new ArrayList<>();
        List<UsbDevice> devicesToAttach = new ArrayList<>();
        UsbDevice permissionRequest = null;
        try {
            HashMap<String, UsbDevice> devices = usbManager.getDeviceList();
            writeMarker(appContext, "bridge.probeAttachedDevices.count=" + devices.size());
            cleanupStaleConnections(devices);
            for (UsbDevice device : devices.values()) {
                writeMarker(appContext, "bridge.probeAttachedDevices.device name=" + device.getDeviceName()
                        + " vid=0x" + Integer.toHexString(device.getVendorId())
                        + " pid=0x" + Integer.toHexString(device.getProductId()));
                if (isDisplayLinkDevice(device)) {
                    displayLinkDevices.add(device);
                    writeMarker(appContext, "bridge.probeAttachedDevices.displaylink_found name=" + device.getDeviceName());
                }
            }

            synchronized (lock) {
                pruneMissingDeviceStateLocked(displayLinkDevices);
                for (UsbDevice device : displayLinkDevices) {
                    String deviceName = device.getDeviceName();
                    if (connections.containsKey(deviceName) || attachingDevices.contains(deviceName)) {
                        continue;
                    }

                    if (usbManager.hasPermission(device)) {
                        pendingPermissionDevices.remove(deviceName);
                        deniedPermissionDevices.remove(deviceName);
                        devicesToAttach.add(device);
                        continue;
                    }

                    if (deniedPermissionDevices.contains(deviceName)) {
                        continue;
                    }

                    pendingPermissionDevices.add(deviceName);
                    if (!permissionRequestOutstanding && permissionRequest == null) {
                        permissionRequestOutstanding = true;
                        outstandingPermissionDeviceName = deviceName;
                        permissionRequest = device;
                    }
                }
            }
        } catch (Throwable e) {
            writeMarker(appContext, "bridge.probeAttachedDevices.exception " + e.getClass().getName() + ":" + e.getMessage());
            Log.e(TAG, "probeAttachedDevices failed", e);
            synchronized (lock) {
                permissionRequestOutstanding = false;
                outstandingPermissionDeviceName = null;
            }
            return;
        }

        if (permissionRequest != null) {
            requestPermission(permissionRequest);
        }

        for (UsbDevice device : devicesToAttach) {
            attachDevice(device);
        }
    }

    private void requestPermission(UsbDevice device) {
        writeMarker(appContext, "bridge.requestPermission name=" + device.getDeviceName()
                + " hasPermission=" + usbManager.hasPermission(device));
        Intent intent = new Intent(ACTION_USB_PERMISSION).setPackage(appContext.getPackageName());
        int flags = FLAG_UPDATE_CURRENT;
        if (SDK_INT >= Build.VERSION_CODES.S) {
            flags |= FLAG_MUTABLE;
        } else {
            flags |= FLAG_IMMUTABLE;
        }
        writeMarker(appContext, "bridge.requestPermission.begin name=" + device.getDeviceName()
                + " flags=0x" + Integer.toHexString(flags));
        PendingIntent pendingIntent = PendingIntent.getBroadcast(appContext, 0, intent, flags);
        usbManager.requestPermission(device, pendingIntent);
    }

    private void handleDeviceAttached(UsbDevice device) {
        synchronized (lock) {
            deniedPermissionDevices.remove(device.getDeviceName());
        }
        scheduleProbe("usb_attached", 0);
    }

    private void handleDeviceDetached(UsbDevice device) {
        synchronized (lock) {
            String deviceName = device.getDeviceName();
            pendingPermissionDevices.remove(deviceName);
            deniedPermissionDevices.remove(deviceName);
            attachingDevices.remove(deviceName);
            if (deviceName.equals(outstandingPermissionDeviceName)) {
                outstandingPermissionDeviceName = null;
                permissionRequestOutstanding = false;
            }
        }
        detachDevice(device.getDeviceName());
        scheduleProbe("usb_detached", 0);
    }

    private void handlePermissionResult(@Nullable UsbDevice device, boolean granted) {
        synchronized (lock) {
            permissionRequestOutstanding = false;
            outstandingPermissionDeviceName = null;
            if (device != null) {
                String deviceName = device.getDeviceName();
                pendingPermissionDevices.remove(deviceName);
                if (granted) {
                    deniedPermissionDevices.remove(deviceName);
                } else {
                    deniedPermissionDevices.add(deviceName);
                }
            }
        }
        scheduleProbe(granted ? "permission_granted" : "permission_denied", 0);
    }

    private void pruneMissingDeviceStateLocked(List<UsbDevice> devices) {
        Set<String> deviceNames = new HashSet<>();
        for (UsbDevice device : devices) {
            deviceNames.add(device.getDeviceName());
        }

        pendingPermissionDevices.retainAll(deviceNames);
        deniedPermissionDevices.retainAll(deviceNames);
        attachingDevices.retainAll(deviceNames);
        if (outstandingPermissionDeviceName != null && !deviceNames.contains(outstandingPermissionDeviceName)) {
            outstandingPermissionDeviceName = null;
            permissionRequestOutstanding = false;
        }
    }

    private void attachDevice(UsbDevice device) {
        String deviceName = device.getDeviceName();
        synchronized (lock) {
            if (!started || nativeDriver == null) {
                writeMarker(appContext, "bridge.attachDevice.skip started=" + started + " nativeDriver=" + nativeDriver);
                return;
            }

            if (connections.containsKey(deviceName) || attachingDevices.contains(deviceName)) {
                writeMarker(appContext, "bridge.attachDevice.skip existing name=" + deviceName);
                return;
            }

            attachingDevices.add(deviceName);
        }

        UsbDeviceConnection connection = usbManager.openDevice(device);
        if (connection == null) {
            synchronized (lock) {
                attachingDevices.remove(deviceName);
            }
            writeMarker(appContext, "bridge.attachDevice.open_failed name=" + deviceName);
            Log.e(TAG, "Failed to open USB device " + deviceName);
            return;
        }

        byte[] descriptors = connection.getRawDescriptors();
        if (descriptors == null) {
            descriptors = new byte[0];
        }

        int rc;
        synchronized (lock) {
            if (!started || nativeDriver == null) {
                attachingDevices.remove(deviceName);
                writeMarker(appContext, "bridge.attachDevice.abort_after_open started=" + started + " nativeDriver=" + nativeDriver);
                connection.close();
                return;
            }
            rc = nativeDriver.usbDeviceAttached(deviceName, connection.getFileDescriptor(), descriptors, descriptors.length);
        }

        writeMarker(appContext, "bridge.attachDevice.rc name=" + deviceName + " rc=" + rc + " descriptors=" + descriptors.length);
        Log.i(TAG, "usbDeviceAttached rc=" + rc + " name=" + deviceName + " fd=" + connection.getFileDescriptor()
                + " descriptors=" + descriptors.length);
        if (rc != 0) {
            synchronized (lock) {
                attachingDevices.remove(deviceName);
            }
            connection.close();
            return;
        }

        connections.put(deviceName, connection);
        synchronized (lock) {
            attachingDevices.remove(deviceName);
            pendingPermissionDevices.remove(deviceName);
            deniedPermissionDevices.remove(deviceName);
        }
        scheduleNotifyScreenOn("attach_success", 250);
        scheduleNotifyScreenOn("attach_success_retry", 1500);
        scheduleProbe("attach_success", 1000);
        scheduleConnectedWatchdog(deviceName, 2500);
    }

    private void detachDevice(String deviceName) {
        UsbDeviceConnection connection = connections.remove(deviceName);
        NativeDriver driver;
        synchronized (lock) {
            attachingDevices.remove(deviceName);
            pendingPermissionDevices.remove(deviceName);
            driver = nativeDriver;
        }
        if (driver != null) {
            try {
                driver.usbDeviceDetached(deviceName);
            } catch (Throwable e) {
                Log.w(TAG, "usbDeviceDetached failed for " + deviceName, e);
            }
        }
        if (connection != null) {
            connection.close();
        }
    }

    private void scheduleProbe(String reason, long delayMs) {
        Context context = appContext;
        if (context == null) {
            return;
        }

        writeMarker(context, "bridge.scheduleProbe reason=" + reason + " delayMs=" + delayMs);
        Thread probeThread = new Thread(() -> {
            try {
                Thread.sleep(delayMs);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                return;
            }
            probeAttachedDevices();
        }, "DisplayLinkProbe");
        probeThread.setDaemon(true);
        probeThread.start();
    }

    private void scheduleNotifyScreenOn(String reason, long delayMs) {
        Context context = appContext;
        if (context == null) {
            return;
        }

        writeMarker(context, "bridge.scheduleNotify reason=" + reason + " delayMs=" + delayMs);
        Thread notifyThread = new Thread(() -> {
            try {
                Thread.sleep(delayMs);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                return;
            }

            synchronized (lock) {
                NativeDriver driver = nativeDriver;
                Context currentContext = appContext;
                if (!started || driver == null) {
                    return;
                }

                notifyDriverEvent(driver, currentContext, EVENT_SCREEN_ON, reason);
            }
        }, "DisplayLinkNotify");
        notifyThread.setDaemon(true);
        notifyThread.start();
    }

    private void scheduleConnectedWatchdog(String deviceName, long delayMs) {
        Context context = appContext;
        if (context == null) {
            return;
        }

        writeMarker(context, "bridge.scheduleWatchdog name=" + deviceName + " delayMs=" + delayMs);
        Thread watchdogThread = new Thread(() -> {
            try {
                Thread.sleep(delayMs);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                return;
            }

            synchronized (lock) {
                if (!started || nativeDriver == null || activeEncoder != 0 || !connections.containsKey(deviceName)) {
                    return;
                }

                int retryCount = attachRetryCounts.getOrDefault(deviceName, 0);
                if (retryCount >= MAX_ATTACH_RETRIES) {
                    if (driverRestartScheduled || driverRestartCount >= MAX_DRIVER_RESTARTS) {
                        writeMarker(appContext, "bridge.watchdog.give_up name=" + deviceName + " retries=" + retryCount
                                + " restartScheduled=" + driverRestartScheduled + " restartCount=" + driverRestartCount);
                        return;
                    }

                    driverRestartScheduled = true;
                    driverRestartCount++;
                    writeMarker(appContext, "bridge.watchdog.global_reattach name=" + deviceName + " retries=" + retryCount
                            + " restartCount=" + driverRestartCount);
                    scheduleGlobalReattach("watchdog_" + deviceName, 100);
                    return;
                }

                attachRetryCounts.put(deviceName, retryCount + 1);
                writeMarker(appContext, "bridge.watchdog.reattach name=" + deviceName + " retry=" + (retryCount + 1));
            }

            forceReattach(deviceName);
        }, "DisplayLinkWatchdog");
        watchdogThread.setDaemon(true);
        watchdogThread.start();
    }

    private void forceReattach(String deviceName) {
        UsbDevice device = usbManager.getDeviceList().get(deviceName);
        if (device == null) {
            writeMarker(appContext, "bridge.forceReattach.missing name=" + deviceName);
            return;
        }

        detachDevice(deviceName);
        scheduleNotifyScreenOn("force_reattach", 150);
        attachDevice(device);
    }

    public void restart(String reason) {
        scheduleGlobalReattach(reason, 0);
    }

    private void scheduleGlobalReattach(String reason, long delayMs) {
        Context context = appContext;
        if (context == null) {
            return;
        }

        writeMarker(context, "bridge.scheduleGlobalReattach reason=" + reason + " delayMs=" + delayMs);
        Thread reattachThread = new Thread(() -> {
            if (delayMs > 0) {
                try {
                    Thread.sleep(delayMs);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    synchronized (lock) {
                        driverRestartScheduled = false;
                    }
                    return;
                }
            }

            globalReattach(reason);
        }, "DisplayLinkGlobalReattach");
        reattachThread.setDaemon(true);
        reattachThread.start();
    }

    private void globalReattach(String reason) {
        Context context;
        List<String> deviceNames;
        synchronized (lock) {
            context = appContext;
            if (!started || nativeDriver == null || context == null) {
                driverRestartScheduled = false;
                return;
            }

            deviceNames = new ArrayList<>(connections.keySet());
            attachRetryCounts.clear();
        }

        writeMarker(context, "bridge.globalReattach.begin reason=" + reason + " activeConnections=" + deviceNames.size());
        for (String deviceName : deviceNames) {
            writeMarker(context, "bridge.globalReattach.detach name=" + deviceName + " reason=" + reason);
            detachDevice(deviceName);
        }

        synchronized (lock) {
            driverRestartScheduled = false;
        }

        scheduleNotifyScreenOn("global_reattach_" + reason, 150);
        scheduleProbe("global_reattach_" + reason, 250);
        scheduleProbe("global_reattach_retry_" + reason, 1200);
    }

    private void scheduleDriverRestart(String reason, long delayMs) {
        Context context = appContext;
        if (context == null) {
            return;
        }

        writeMarker(context, "bridge.scheduleDriverRestart reason=" + reason + " delayMs=" + delayMs);
        Thread restartThread = new Thread(() -> {
            if (delayMs > 0) {
                try {
                    Thread.sleep(delayMs);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    synchronized (lock) {
                        driverRestartScheduled = false;
                    }
                    return;
                }
            }

            restartDriver(reason);
        }, "DisplayLinkDriverRestart");
        restartThread.setDaemon(true);
        restartThread.start();
    }

    private void restartDriver(String reason) {
        Context context;
        NativeDriver oldDriver;
        List<Map.Entry<String, UsbDeviceConnection>> openConnections;
        synchronized (lock) {
            context = appContext;
            if (!started || context == null) {
                driverRestartScheduled = false;
                return;
            }
            oldDriver = nativeDriver;
            openConnections = new ArrayList<>(connections.entrySet());
            connections.clear();
            attachRetryCounts.clear();
            activeEncoder = 0;
            lastMonitorInfo = null;
            lastWidth = 0;
            lastHeight = 0;
            lastRowStride = 0;
            framePostCount = 0;
            clearFrameBuffersLocked();
            nativeDriver = null;
        }

        writeMarker(context, "bridge.restartDriver.begin reason=" + reason);

        for (Map.Entry<String, UsbDeviceConnection> entry : openConnections) {
            writeMarker(context, "bridge.restartDriver.detach name=" + entry.getKey());
            try {
                if (oldDriver != null) {
                    oldDriver.usbDeviceDetached(entry.getKey());
                    writeMarker(context, "bridge.restartDriver.detached name=" + entry.getKey());
                }
            } catch (Throwable e) {
                writeMarker(context, "bridge.restartDriver.detach_failed name=" + entry.getKey() + " error="
                        + e.getClass().getSimpleName());
                Log.w(TAG, "usbDeviceDetached failed during restart for " + entry.getKey(), e);
            }
            entry.getValue().close();
            writeMarker(context, "bridge.restartDriver.closed name=" + entry.getKey());
        }

        if (oldDriver != null) {
            try {
                writeMarker(context, "bridge.restartDriver.notify_off.begin reason=" + reason);
                notifyDriverEvent(oldDriver, context, EVENT_SCREEN_OFF, "restart_" + reason);
                writeMarker(context, "bridge.restartDriver.notify_off.done reason=" + reason);
                writeMarker(context, "bridge.restartDriver.destroy.begin reason=" + reason);
                oldDriver.destroy();
                writeMarker(context, "bridge.restartDriver.destroy.done reason=" + reason);
            } catch (Throwable e) {
                writeMarker(context, "bridge.restartDriver.destroy_failed reason=" + reason + " error="
                        + e.getClass().getSimpleName());
                Log.w(TAG, "NativeDriver.destroy failed during restart", e);
            }
        }

        try {
            File firmwareDir = prepareFirmwareDirectory(context);
            writeMarker(context, "bridge.restartDriver.prepareFirmware.done dir=" + firmwareDir);
            NativeDriver replacement = new NativeDriver();
            writeMarker(context, "bridge.restartDriver.nativeCreate.begin reason=" + reason);
            int rc = replacement.create(new NativeDriverListener(), firmwareDir.getAbsolutePath(), false);
            boolean restartReady = false;
            Context restartContext = null;
            synchronized (lock) {
                if (!started) {
                    driverRestartScheduled = false;
                    try {
                        replacement.destroy();
                    } catch (Throwable e) {
                        Log.w(TAG, "NativeDriver.destroy failed after late restart cancellation", e);
                    }
                    return;
                }

                nativeDriver = replacement;
                writeMarker(appContext, "bridge.restartDriver.nativeCreate rc=" + rc + " reason=" + reason);
                if (rc != 0 || !nativeDriver.isValid()) {
                    writeMarker(appContext, "bridge.restartDriver.native_invalid reason=" + reason);
                    try {
                        nativeDriver.destroy();
                    } catch (Throwable e) {
                        Log.w(TAG, "NativeDriver.destroy failed after invalid restart", e);
                    }
                    nativeDriver = null;
                    driverRestartScheduled = false;
                    return;
                }

                restartReady = true;
                restartContext = appContext;
                driverRestartScheduled = false;
            }

            if (restartReady) {
                notifyDriverEvent(replacement, restartContext, EVENT_SCREEN_ON, "restart_" + reason);
            }
        } catch (Throwable e) {
            synchronized (lock) {
                driverRestartScheduled = false;
            }
            writeMarker(context, "bridge.restartDriver.exception reason=" + reason + " "
                    + e.getClass().getName() + ":" + e.getMessage());
            Log.e(TAG, "Failed to restart DisplayLink driver, reason=" + reason, e);
            return;
        }

        writeMarker(context, "bridge.restartDriver.probe reason=" + reason);
        scheduleProbe("restart_" + reason, 150);
        scheduleNotifyScreenOn("restart_" + reason, 300);
    }

    private void notifyDriverEvent(NativeDriver driver, @Nullable Context context, int eventCode, String reason) {
        if (driver == null) {
            return;
        }

        try {
            driver.notifyEvent(eventCode);
            writeMarker(context, "bridge.notifyEvent code=" + eventCode + " reason=" + reason);
        } catch (Throwable e) {
            writeMarker(context, "bridge.notifyEvent_failed code=" + eventCode + " reason=" + reason
                    + " error=" + e.getClass().getSimpleName());
            Log.w(TAG, "notifyEvent failed code=" + eventCode + " reason=" + reason, e);
        }
    }

    private static boolean isDisplayLinkDevice(UsbDevice device) {
        return device.getVendorId() == DISPLAYLINK_VENDOR_ID;
    }

    private void cleanupStaleConnections(HashMap<String, UsbDevice> devices) {
        List<String> staleConnections = new ArrayList<>();
        for (String deviceName : connections.keySet()) {
            if (!devices.containsKey(deviceName)) {
                staleConnections.add(deviceName);
            }
        }

        for (String deviceName : staleConnections) {
            writeMarker(appContext, "bridge.probeAttachedDevices.stale_detach name=" + deviceName);
            detachDevice(deviceName);
        }

        synchronized (lock) {
            pendingPermissionDevices.retainAll(devices.keySet());
            deniedPermissionDevices.retainAll(devices.keySet());
            attachingDevices.retainAll(devices.keySet());
            if (outstandingPermissionDeviceName != null && !devices.containsKey(outstandingPermissionDeviceName)) {
                outstandingPermissionDeviceName = null;
                permissionRequestOutstanding = false;
            }
        }
    }

    private static File prepareFirmwareDirectory(Context context) throws IOException, PackageManager.NameNotFoundException {
        Context displayLinkContext = context.createPackageContext(DISPLAYLINK_PACKAGE, 0);
        AssetManager assets = displayLinkContext.getAssets();
        File dir = context.getDir("displaylink-fw", Context.MODE_PRIVATE);
        Log.i(TAG, "Preparing firmware directory at " + dir);
        String[] assetNames = assets.list("");
        if (assetNames == null) {
            throw new IOException("DisplayLink asset list unavailable");
        }

        for (String assetName : assetNames) {
            if (!assetName.endsWith(".spkg")) {
                continue;
            }

            File outFile = new File(dir, assetName);
            try (InputStream in = assets.open(assetName);
                 FileOutputStream out = new FileOutputStream(outFile, false)) {
                byte[] buffer = new byte[16384];
                int read;
                while ((read = in.read(buffer)) != -1) {
                    out.write(buffer, 0, read);
                }
                out.getFD().sync();
            }
        }

        return dir;
    }
}
