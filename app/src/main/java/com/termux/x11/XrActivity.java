package com.termux.x11;

import android.app.Activity;
import android.app.ActivityOptions;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.hardware.display.DisplayManager;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.view.Display;
import android.view.KeyEvent;
import android.view.PixelCopy;
import android.view.ViewGroup;

import com.termux.x11.input.InputStub;
import com.termux.x11.utils.GLUtility;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class XrActivity extends MainActivity implements GLSurfaceView.Renderer {
    // Order of the enum has to be the same as in xrio/android.c
    public enum ControllerAxis {
        L_PITCH, L_YAW, L_ROLL, L_THUMBSTICK_X, L_THUMBSTICK_Y, L_X, L_Y, L_Z,
        R_PITCH, R_YAW, R_ROLL, R_THUMBSTICK_X, R_THUMBSTICK_Y, R_X, R_Y, R_Z,
        HMD_PITCH, HMD_YAW, HMD_ROLL, HMD_X, HMD_Y, HMD_Z, HMD_IPD
    }

    // Order of the enum has to be the same as in xrio/android.c
    public enum ControllerButton {
        L_GRIP,  L_MENU, L_THUMBSTICK_PRESS, L_THUMBSTICK_LEFT, L_THUMBSTICK_RIGHT, L_THUMBSTICK_UP, L_THUMBSTICK_DOWN, L_TRIGGER, L_X, L_Y,
        R_A, R_B, R_GRIP, R_THUMBSTICK_PRESS, R_THUMBSTICK_LEFT, R_THUMBSTICK_RIGHT, R_THUMBSTICK_UP, R_THUMBSTICK_DOWN, R_TRIGGER,
    }

    // Order of the enum has to be the same as in xrio/android.c
    public enum RenderParam {
        CANVAS_DISTANCE, IMMERSIVE, PASSTHROUGH, SBS, VIEWPORT_WIDTH, VIEWPORT_HEIGHT,
    }

    private static boolean isDeviceDetectionFinished = false;
    private static boolean isDeviceSupported = false;

    private static boolean isImmersive = false;
    private static boolean isSBS = false;
    private static final float[] lastAxes = new float[ControllerAxis.values().length];
    private static final boolean[] lastButtons = new boolean[ControllerButton.values().length];

    private Bitmap bitmap;
    private HandlerThread handlerThread;
    private int program;
    private int texture;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        GLSurfaceView view = new GLSurfaceView(this);
        view.setEGLContextClientVersion(2);
        view.setRenderer(this);
        frm.addView(view);

        // For testing without headset
        if (isEnabled() && !isSupported()) {
            ViewGroup.LayoutParams lp = view.getLayoutParams();
            lp.width = 256;
            lp.height = 256;
            view.setLayoutParams(lp);
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        // Going back to the Android 2D rendering isn't supported.
        // Kill the app to ensure there is no unexpected behaviour.
        System.exit(0);
    }

    public static boolean isEnabled() {
        return isSupported();
    }

    public static boolean isSupported() {
        if (!isDeviceDetectionFinished) {
            if (Build.MANUFACTURER.compareToIgnoreCase("META") == 0) {
                isDeviceSupported = true;
            }
            if (Build.MANUFACTURER.compareToIgnoreCase("OCULUS") == 0) {
                isDeviceSupported = true;
            }
            isDeviceDetectionFinished = true;
        }
        return isDeviceSupported;
    }

    public static void openIntent(Activity context) {
        // 0. Create the launch intent
        Intent intent = new Intent(context, XrActivity.class);

        // 1. Locate the main display ID and add that to the intent
        final int mainDisplayId = getMainDisplay(context);
        ActivityOptions options = ActivityOptions.makeBasic().setLaunchDisplayId(mainDisplayId);

        // 2. Set the flags: start in a new task
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);

        // 3. Launch the activity.
        // Don't use the container's ContextWrapper, which is adding arguments
        context.getBaseContext().startActivity(intent, options.toBundle());

        // 4. Finish the previous activity: this avoids an audio bug
        context.finish();
    }

    private static int getMainDisplay(Context context) {
        final DisplayManager displayManager =
                (DisplayManager) context.getSystemService(Context.DISPLAY_SERVICE);
        for (Display display : displayManager.getDisplays()) {
            if (display.getDisplayId() == Display.DEFAULT_DISPLAY) {
                return display.getDisplayId();
            }
        }
        return -1;
    }

    @Override
    public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {
        System.loadLibrary("XRio");
        if (isSupported()) {
            init();
        }
    }

    @Override
    public void onSurfaceChanged(GL10 gl10, int w, int h) {
        if (handlerThread != null) {
            handlerThread.quitSafely();
        }

        bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        program = GLUtility.createProgram();
        texture = GLUtility.createTexture();

        handlerThread = new HandlerThread("PixelCopier");
        handlerThread.start();
        requestBitmap();
    }

    @Override
    public void onDrawFrame(GL10 gl10) {
        if (isSupported()) {
            setRenderParam(RenderParam.CANVAS_DISTANCE.ordinal(), 5);
            setRenderParam(RenderParam.IMMERSIVE.ordinal(), isImmersive ? 1 : 0);
            setRenderParam(RenderParam.PASSTHROUGH.ordinal(), isImmersive ? 0 : 1);
            setRenderParam(RenderParam.SBS.ordinal(), isSBS ? 1 : 0);

            if (beginFrame()) {
                renderFrame(gl10);
                finishFrame();
                processInput();
            }
        } else {
            renderFrame(gl10);
        }
    }

    private void processInput() {
        float[] axes = getAxes();
        boolean[] buttons = getButtons();
        LorieView view = getLorieView();
        Rect frame = view.getHolder().getSurfaceFrame();

        // Mouse control with hand
        float meter2px = frame.width() * 10.0f;
        float dx = (axes[ControllerAxis.R_X.ordinal()] - lastAxes[ControllerAxis.R_X.ordinal()]) * meter2px;
        float dy = (axes[ControllerAxis.R_Y.ordinal()] - lastAxes[ControllerAxis.R_Y.ordinal()]) * meter2px;

        // Mouse control with head
        if (isImmersive) {
            float angle2px = frame.width() * 0.05f;
            dx = getAngleDiff(lastAxes[ControllerAxis.HMD_YAW.ordinal()], axes[ControllerAxis.HMD_YAW.ordinal()]) * angle2px;
            dy = getAngleDiff(lastAxes[ControllerAxis.HMD_PITCH.ordinal()], axes[ControllerAxis.HMD_PITCH.ordinal()]) * angle2px;
            if (Float.isNaN(dy)) {
                dy = 0;
            }
        }

        // Mouse speed adjust
        float mouseSpeed = 0.25f;
        dx *= mouseSpeed;
        dy *= mouseSpeed;

        // Mouse "snap turn"
        int snapturn = isImmersive ? 125 : 25;
        if (getButtonClicked(buttons, ControllerButton.R_THUMBSTICK_LEFT)) {
            dx = -snapturn;
        }
        if (getButtonClicked(buttons, ControllerButton.R_THUMBSTICK_RIGHT)) {
            dx = snapturn;
        }

        // Set mouse status
        int scrollSpeed = 5;
        view.sendMouseEvent(dx, -dy, 0, false, true);
        view.sendMouseEvent(0, 0, InputStub.BUTTON_LEFT, buttons[ControllerButton.R_TRIGGER.ordinal()], true);
        view.sendMouseEvent(0, 0, InputStub.BUTTON_RIGHT, buttons[ControllerButton.R_GRIP.ordinal()], true);
        view.sendMouseEvent(0, 0, InputStub.BUTTON_MIDDLE, buttons[ControllerButton.L_THUMBSTICK_PRESS.ordinal()], true);
        if (buttons[ControllerButton.R_THUMBSTICK_UP.ordinal()]) {
            view.sendMouseWheelEvent(0, -scrollSpeed);
        } else if (buttons[ControllerButton.R_THUMBSTICK_DOWN.ordinal()]) {
            view.sendMouseWheelEvent(0, scrollSpeed);
        }

        // Switch immersive/SBS mode
        if (getButtonClicked(buttons, ControllerButton.L_THUMBSTICK_PRESS)) {
            if (buttons[ControllerButton.R_GRIP.ordinal()]) {
                isSBS = !isSBS;
            }
            else {
                isImmersive = !isImmersive;
            }
        }

        // Update keyboard
        view.sendKeyEvent(0, KeyEvent.KEYCODE_A, buttons[ControllerButton.R_A.ordinal()]);
        view.sendKeyEvent(0, KeyEvent.KEYCODE_B, buttons[ControllerButton.R_B.ordinal()]);
        view.sendKeyEvent(0, KeyEvent.KEYCODE_X, buttons[ControllerButton.L_X.ordinal()]);
        view.sendKeyEvent(0, KeyEvent.KEYCODE_Y, buttons[ControllerButton.L_Y.ordinal()]);
        view.sendKeyEvent(0, KeyEvent.KEYCODE_SPACE, buttons[ControllerButton.L_GRIP.ordinal()]);
        view.sendKeyEvent(0, KeyEvent.KEYCODE_BACK, buttons[ControllerButton.L_MENU.ordinal()]);
        view.sendKeyEvent(0, KeyEvent.KEYCODE_ENTER, buttons[ControllerButton.L_TRIGGER.ordinal()]);
        view.sendKeyEvent(0, KeyEvent.KEYCODE_DPAD_LEFT, buttons[ControllerButton.L_THUMBSTICK_LEFT.ordinal()]);
        view.sendKeyEvent(0, KeyEvent.KEYCODE_DPAD_RIGHT, buttons[ControllerButton.L_THUMBSTICK_RIGHT.ordinal()]);
        view.sendKeyEvent(0, KeyEvent.KEYCODE_DPAD_UP, buttons[ControllerButton.L_THUMBSTICK_UP.ordinal()]);
        view.sendKeyEvent(0, KeyEvent.KEYCODE_DPAD_DOWN, buttons[ControllerButton.L_THUMBSTICK_DOWN.ordinal()]);

        // Store the OpenXR data
        System.arraycopy(axes, 0, lastAxes, 0, axes.length);
        System.arraycopy(buttons, 0, lastButtons, 0, buttons.length);
    }

    private static float getAngleDiff(float oldAngle, float newAngle) {
        float diff = oldAngle - newAngle;
        while (diff > 180) {
            diff -= 360;
        }
        while (diff < -180) {
            diff += 360;
        }
        return diff;
    }

    private static boolean getButtonClicked(boolean[] buttons, ControllerButton button) {
        return buttons[button.ordinal()] && !lastButtons[button.ordinal()];
    }

    private void renderFrame(GL10 gl10) {
        GLUtility.bindTexture(texture);
        GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, bitmap, 0);

        if (isSupported()) {
            int w = getRenderParam(RenderParam.VIEWPORT_WIDTH.ordinal());
            int h = getRenderParam(RenderParam.VIEWPORT_HEIGHT.ordinal());
            gl10.glViewport(0, 0, w, h);
        }

        LorieView view = getLorieView();
        int w = view.getWidth();
        int h = view.getHeight();
        float aspect = Math.min(w, h) / (float)Math.max(w, h);
        GLUtility.drawTexture(program, texture, -aspect);
    }

    private void requestBitmap() {
        PixelCopy.request(getLorieView(), bitmap, (copyResult) -> requestBitmap(),
                new Handler(handlerThread.getLooper()));
    }

    private native void init();
    private native boolean beginFrame();
    private native void finishFrame();
    public native float[] getAxes();
    public native boolean[] getButtons();
    public native int getRenderParam(int param);
    public native void setRenderParam(int param, int value);
}
