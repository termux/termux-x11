// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.termux.x11.input;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.PointF;
import android.hardware.input.InputManager;
import android.os.Handler;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.GestureDetector;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;

import androidx.annotation.IntDef;
import androidx.core.math.MathUtils;

import com.termux.x11.LorieView;
import com.termux.x11.MainActivity;
import com.termux.x11.utils.SamsungDexUtils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.InvocationTargetException;
import java.util.Arrays;
import java.util.Objects;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * This class is responsible for handling Touch input from the user.  Touch events which manipulate
 * the local canvas are handled in this class and any input which should be sent to the remote host
 * are passed to the InputStrategyInterface implementation set by the DesktopView.
 */
public class TouchInputHandler {
    private static final float EPSILON = 0.001f;

    public static int STYLUS_INPUT_HELPER_MODE = 1; //1 = Left Click, 2 Middle Click, 3 Right Click

    /** Used to set/store the selected input mode. */
    @SuppressWarnings("unused")
    @IntDef({InputMode.TRACKPAD, InputMode.SIMULATED_TOUCH, InputMode.TOUCH})
    @Retention(RetentionPolicy.SOURCE)
    public @interface InputMode {
        // Values are starting from 0 and don't have gaps.
        int TRACKPAD = 1;
        int SIMULATED_TOUCH = 2;
        int TOUCH = 3;
    }

    @IntDef({CapturedPointerTransformation.NONE, CapturedPointerTransformation.CLOCKWISE, CapturedPointerTransformation.COUNTER_CLOCKWISE, CapturedPointerTransformation.UPSIDE_DOWN})
    @Retention(RetentionPolicy.SOURCE)
    public @interface CapturedPointerTransformation {
        int NONE = 0;
        int CLOCKWISE = 1;
        int COUNTER_CLOCKWISE = 2;
        int UPSIDE_DOWN = 3;
    }

    private final RenderData mRenderData;
    private final RenderStub mRenderStub;
    private final GestureDetector mScroller;
    private final TapGestureDetector mTapDetector;
    private final StylusListener mStylusListener = new StylusListener();
    private final HardwareMouseListener mHMListener = new HardwareMouseListener();
    private final DexListener mDexListener;
    private final TouchInputHandler mTouchpadHandler;

    /** Used to disambiguate a 2-finger gesture as a swipe or a pinch. */
    private final SwipeDetector mSwipePinchDetector;

    private InputStrategyInterface mInputStrategy;
    private final InputEventSender mInjector;
    private final MainActivity mActivity;
    private final DisplayMetrics mMetrics = new DisplayMetrics();

    private boolean keyIntercepting = false;

    /**
     * Used for tracking swipe gestures. Only the Y-direction is needed for responding to swipe-up
     * or swipe-down.
     */
    private float mTotalMotionY;

    /**
     * Distance in pixels beyond which a motion gesture is considered to be a swipe. This is
     * initialized using the Context passed into the constructor.
     */
    private final float mSwipeThreshold;

    /**
     * Set to true to prevent any further movement of the cursor, for example, when showing the
     * keyboard to prevent the cursor wandering from the area where keystrokes should be sent.
     */
    private boolean mSuppressCursorMovement;

    /**
     * Set to true when 3-finger swipe gesture is complete, so that further movement doesn't
     * trigger more swipe actions.
     */
    private boolean mSwipeCompleted;

    /**
     * Set to true when a 1 finger pan gesture originates with a long-press.  This means the user
     * is performing a drag operation.
     */
    private boolean mIsDragging;

    @CapturedPointerTransformation int capturedPointerTransformation = CapturedPointerTransformation.NONE;

    private TouchInputHandler(MainActivity activity, RenderData renderData, RenderStub renderStub,
                              final InputEventSender injector, boolean isTouchpad) {
        if (renderStub == null || injector == null)
            throw new NullPointerException();

        mRenderStub = renderStub;
        mRenderData = renderData != null ? renderData :new RenderData();
        mInjector = injector;
        mActivity = activity;

        GestureListener listener = new GestureListener();
        mScroller = new GestureDetector(/*desktop*/ activity, listener, null, false);

        // If long-press is enabled, the gesture-detector will not emit any further onScroll
        // notifications after the onLongPress notification. Since onScroll is being used for
        // moving the cursor, it means that the cursor would become stuck if the finger were held
        // down too long.
        mScroller.setIsLongpressEnabled(false);

        mTapDetector = new TapGestureDetector(/*desktop*/ activity, listener);
        mSwipePinchDetector = new SwipeDetector(/*desktop*/ activity);

        // The threshold needs to be bigger than the ScaledTouchSlop used by the gesture-detectors,
        // so that a gesture cannot be both a tap and a swipe. It also needs to be small enough so
        // that intentional swipes are usually detected.
        float density = /*desktop*/ activity.getResources().getDisplayMetrics().density;
        mSwipeThreshold = 40 * density;

//        mEdgeSlopInPx = ViewConfiguration.get(/*desktop*/ ctx).getScaledEdgeSlop();

        setInputMode(InputMode.TRACKPAD);
        mDexListener = new DexListener(activity);
        mTouchpadHandler = isTouchpad ? null : new TouchInputHandler(activity, mRenderData, renderStub, injector, true);

        refreshInputDevices();
        ((InputManager) mActivity.getSystemService(Context.INPUT_SERVICE)).registerInputDeviceListener(new InputManager.InputDeviceListener() {
            @Override
            public void onInputDeviceAdded(int deviceId) {
                refreshInputDevices();
            }

            @Override
            public void onInputDeviceRemoved(int deviceId) {
                refreshInputDevices();
            }

            @Override
            public void onInputDeviceChanged(int deviceId) {
                refreshInputDevices();
            }
        }, null);

    }

    public TouchInputHandler(MainActivity activity, RenderStub renderStub, final InputEventSender injector) {
        this(activity, null, renderStub, injector, false);
    }

    static public void refreshInputDevices() {
        AtomicBoolean stylusAvailable = new AtomicBoolean(false);
        Arrays.stream(InputDevice.getDeviceIds())
                .mapToObj(InputDevice::getDevice)
                .filter(Objects::nonNull)
                .forEach((device) -> {
                    //noinspection DataFlowIssue
                    android.util.Log.d("STYLUS", "found device " + device.getName() + " sources " + device.getSources());
                    if (device.supportsSource(InputDevice.SOURCE_STYLUS))
                        stylusAvailable.set(true);
                });
        android.util.Log.d("STYLUS", "requesting stylus " + stylusAvailable.get());
        LorieView.requestStylusEnabled(stylusAvailable.get());
    }

    boolean isDexEvent(MotionEvent event) {
        int SOURCE_DEX = InputDevice.SOURCE_MOUSE | InputDevice.SOURCE_TOUCHSCREEN;
        return ((event.getSource() & SOURCE_DEX) == SOURCE_DEX)
                && ((event.getSource() & InputDevice.SOURCE_TOUCHPAD) != InputDevice.SOURCE_TOUCHPAD)
                && (event.getToolType(event.getActionIndex()) == MotionEvent.TOOL_TYPE_FINGER);
    }

    public boolean handleTouchEvent(View view0, View view, MotionEvent event) {
        if (view0 != view) {
            int[] view0Location = new int[2];
            int[] viewLocation = new int[2];

            view0.getLocationOnScreen(view0Location);
            view.getLocationOnScreen(viewLocation);

            int offsetX = viewLocation[0] - view0Location[0];
            int offsetY = viewLocation[1] - view0Location[1];

            event.offsetLocation(-offsetX, -offsetY);
        }

        if (!view.isFocused() && event.getAction() == MotionEvent.ACTION_DOWN)
            view.requestFocus();

        if (event.getAction() == MotionEvent.ACTION_UP)
            setCapturingEnabled(true);

        if (event.getToolType(event.getActionIndex()) == MotionEvent.TOOL_TYPE_STYLUS
                || event.getToolType(event.getActionIndex()) == MotionEvent.TOOL_TYPE_ERASER)
            return mStylusListener.onTouch(event);

        if (!isDexEvent(event) && (event.getToolType(event.getActionIndex()) == MotionEvent.TOOL_TYPE_MOUSE
                || (event.getSource() & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE)
                || (event.getSource() & InputDevice.SOURCE_MOUSE_RELATIVE) == InputDevice.SOURCE_MOUSE_RELATIVE
                || (event.getPointerCount() == 1 && mTouchpadHandler == null
                   && (event.getSource() & InputDevice.SOURCE_TOUCHPAD) == InputDevice.SOURCE_TOUCHPAD))
            return mHMListener.onTouch(view, event);

        if (event.getToolType(event.getActionIndex()) == MotionEvent.TOOL_TYPE_FINGER) {
            // Dex touchpad sends events as finger, but it should be considered as a mouse.
            if (isDexEvent(event) && mDexListener.onTouch(view, event))
                return true;

            // Regular touchpads and Dex touchpad send events as finger too,
            // but they should be handled as touchscreens with trackpad mode.
            if (mTouchpadHandler != null && (event.getSource() & InputDevice.SOURCE_TOUCHPAD) == InputDevice.SOURCE_TOUCHPAD)
                return mTouchpadHandler.handleTouchEvent(view, view, event);

            // Give the underlying input strategy a chance to observe the current motion event before
            // passing it to the gesture detectors.  This allows the input strategy to react to the
            // event or save the payload for use in recreating the gesture remotely.
            if (mInputStrategy instanceof InputStrategyInterface.NullInputStrategy)
                mInjector.sendTouchEvent(event, mRenderData);
            else
                mInputStrategy.onMotionEvent(event);

            // Avoid short-circuit logic evaluation - ensure all gesture detectors see all events so
            // that they generate correct notifications.
            mScroller.onTouchEvent(event);
            mTapDetector.onTouchEvent(event);
            mSwipePinchDetector.onTouchEvent(event);

            switch (event.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    mSuppressCursorMovement = false;
                    mSwipeCompleted = false;
                    mIsDragging = false;
                    break;

                case MotionEvent.ACTION_SCROLL:
                    float scrollY = -100 * event.getAxisValue(MotionEvent.AXIS_VSCROLL);
                    float scrollX = -100 * event.getAxisValue(MotionEvent.AXIS_HSCROLL);

                    mInjector.sendMouseWheelEvent(scrollX, scrollY);
                    return true;

                case MotionEvent.ACTION_POINTER_DOWN:
                    mTotalMotionY = 0;
                    break;

                default:
                    break;
            }
            return true;
        }

        return false;
    }

    private void resetTransformation() {
        float sx = (float) mRenderData.screenWidth / (float) mRenderData.imageWidth;
        float sy = (float) mRenderData.screenHeight / (float) mRenderData.imageHeight;
        mRenderData.scale.set(sx, sy);
    }

    public void handleClientSizeChanged(int w, int h) {
        mRenderData.screenWidth = w;
        mRenderData.screenHeight = h;

        if (mTouchpadHandler != null)
            mTouchpadHandler.handleClientSizeChanged(w, h);

        moveCursorToScreenPoint((float) w / 2, (float) h / 2);

        resetTransformation();
    }

    public void handleHostSizeChanged(int w, int h) {
        mRenderData.imageWidth = w;
        mRenderData.imageHeight = h;
        moveCursorToScreenPoint((float) w/2, (float) h/2);

        if (mTouchpadHandler != null)
            mTouchpadHandler.handleHostSizeChanged(w, h);

        resetTransformation();
        MainActivity.getRealMetrics(mMetrics);
    }

    public void setInputMode(@InputMode int inputMode) {
        if (inputMode == InputMode.TOUCH)
            mInputStrategy = new InputStrategyInterface.NullInputStrategy();
        else if (inputMode == InputMode.SIMULATED_TOUCH)
            mInputStrategy = new InputStrategyInterface.SimulatedTouchInputStrategy(mRenderData, mInjector, mActivity);
        else
            mInputStrategy = new InputStrategyInterface.TrackpadInputStrategy(mInjector);
    }

    public void setCapturingEnabled(boolean enabled) {
        if (mInjector.pointerCapture && enabled)
            mActivity.getLorieView().requestPointerCapture();
        else
            mActivity.getLorieView().releasePointerCapture();

        if (mInjector.pauseKeyInterceptingWithEsc) {
            if (mInjector.dexMetaKeyCapture)
                SamsungDexUtils.dexMetaKeyCapture(mActivity, enabled);
            keyIntercepting = enabled;
        }
    }

    public void reloadPreferences(SharedPreferences p) {
        setInputMode(Integer.parseInt(p.getString("touchMode", "1")));
        mInjector.tapToMove = p.getBoolean("tapToMove", false);
        mInjector.preferScancodes = p.getBoolean("preferScancodes", false);
        mInjector.pointerCapture = p.getBoolean("pointerCapture", false);
        mInjector.scaleTouchpad = p.getBoolean("scaleTouchpad", true);
        mInjector.capturedPointerSpeedFactor = ((float) p.getInt("capturedPointerSpeedFactor", 100))/100;
        mInjector.dexMetaKeyCapture = p.getBoolean("dexMetaKeyCapture", false);
        mInjector.stylusIsMouse = p.getBoolean("stylusIsMouse", false);
        mInjector.stylusButtonContactModifierMode = p.getBoolean("stylusButtonContactModifierMode", false);
        mInjector.pauseKeyInterceptingWithEsc = p.getBoolean("pauseKeyInterceptingWithEsc", false);
        switch (p.getString("transformCapturedPointer", "no")) {
            case "c":
                capturedPointerTransformation = CapturedPointerTransformation.CLOCKWISE;
                break;
            case "cc":
                capturedPointerTransformation = CapturedPointerTransformation.COUNTER_CLOCKWISE;
                break;
            case "ud":
                capturedPointerTransformation = CapturedPointerTransformation.UPSIDE_DOWN;
                break;
            default:
                capturedPointerTransformation = CapturedPointerTransformation.NONE;
        }

        MainActivity.getRealMetrics(mMetrics);

        if (!p.getBoolean("pointerCapture", false) && mActivity.getLorieView().hasPointerCapture())
            mActivity.getLorieView().releasePointerCapture();

        keyIntercepting = !mInjector.pauseKeyInterceptingWithEsc || mActivity.getLorieView().hasPointerCapture();
        SamsungDexUtils.dexMetaKeyCapture(mActivity, mInjector.dexMetaKeyCapture && keyIntercepting);
    }

    public boolean shouldInterceptKeys() {
        return !mInjector.pauseKeyInterceptingWithEsc || keyIntercepting;
    }

    private void moveCursorByOffset(float deltaX, float deltaY) {
        if (mInputStrategy instanceof InputStrategyInterface.TrackpadInputStrategy)
            mInjector.sendCursorMove(-deltaX, -deltaY, true);
        else if (mInputStrategy instanceof InputStrategyInterface.SimulatedTouchInputStrategy) {
            PointF cursorPos = mRenderData.getCursorPosition();
            cursorPos.offset(-deltaX, -deltaY);
            cursorPos.set(MathUtils.clamp(cursorPos.x, 0, mRenderData.screenWidth), MathUtils.clamp(cursorPos.y, 0, mRenderData.screenHeight));
            if (mRenderData.setCursorPosition(cursorPos.x, cursorPos.y))
                mInjector.sendCursorMove((int) cursorPos.x, (int) cursorPos.y, false);
        }
    }

    /** Moves the cursor to the specified position on the screen. */
    private void moveCursorToScreenPoint(float screenX, float screenY) {
        if (mInputStrategy instanceof InputStrategyInterface.TrackpadInputStrategy || mInputStrategy instanceof InputStrategyInterface.SimulatedTouchInputStrategy) {
            float[] imagePoint = {screenX * mRenderData.scale.x, screenY * mRenderData.scale.y};
            if (mRenderData.setCursorPosition(imagePoint[0], imagePoint[1]))
                mInjector.sendCursorMove((int) imagePoint[0], imagePoint[1], false);
        }
    }

    /** Processes a (multi-finger) swipe gesture. */
    private boolean onSwipe() {
        if (mTotalMotionY > mSwipeThreshold)
            mRenderStub.swipeDown();
        else if (mTotalMotionY < -mSwipeThreshold)
            mRenderStub.swipeUp();
        else
            return false;

        mSuppressCursorMovement = true;
        mSwipeCompleted = true;
        return true;
    }

    /** Responds to touch events filtered by the gesture detectors.
     * @noinspection NullableProblems */
    private class GestureListener extends GestureDetector.SimpleOnGestureListener
            implements TapGestureDetector.OnTapListener {
        private final Handler mGestureListenerHandler = new Handler(msg -> {
            if (msg.what == InputStub.BUTTON_LEFT)
                mInputStrategy.onTap(InputStub.BUTTON_LEFT);
            return true;
        });

        /**
         * Called when the user drags one or more fingers across the touchscreen.
         */
        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
            int pointerCount = e2.getPointerCount();

            if (pointerCount >= 3 && !mSwipeCompleted) {
                // Note that distance values are reversed. For example, dragging a finger in the
                // direction of increasing Y coordinate (downwards) results in distanceY being
                // negative.
                mTotalMotionY -= distanceY;
                return onSwipe();
            }

            if (pointerCount == 2 && mSwipePinchDetector.isSwiping()) {
                if (!(mInputStrategy instanceof InputStrategyInterface.TrackpadInputStrategy)) {
                    // Ensure the cursor is located at the coordinates of the original event,
                    // otherwise the target window may not receive the scroll event correctly.
                    moveCursorToScreenPoint(e1.getX(), e1.getY());
                }
                mInputStrategy.onScroll(distanceX, distanceY);

                // Prevent the cursor being moved or flung by the gesture.
                mSuppressCursorMovement = true;
                return true;
            }

            if (pointerCount != 1 || mSuppressCursorMovement)
                return false;

            if (mInputStrategy instanceof InputStrategyInterface.TrackpadInputStrategy) {
                if (mInjector.scaleTouchpad) {
                    distanceX *= mRenderData.scale.x;
                    distanceY *= mRenderData.scale.y;
                }
                moveCursorByOffset(distanceX, distanceY);
            }
            if (!(mInputStrategy instanceof InputStrategyInterface.TrackpadInputStrategy) && mIsDragging) {
                // Ensure the cursor follows the user's finger when the user is dragging under
                // direct input mode.
                moveCursorToScreenPoint(e2.getX(), e2.getY());
            }
            return true;
        }

        /** Called whenever a gesture starts. Always accepts the gesture so it isn't ignored. */
        @Override
        public boolean onDown(MotionEvent e) {
            return true;
        }

        /**
         * Called when the user taps the screen with one or more fingers.
         */
        @Override
        public void onTap(int pointerCount, float x, float y) {
            int button = mouseButtonFromPointerCount(pointerCount);
            if (button == InputStub.BUTTON_UNDEFINED)
                return;

            if (!(mInputStrategy instanceof InputStrategyInterface.TrackpadInputStrategy)) {
                if (screenPointLiesOutsideImageBoundary(x, y))
                    return;

                moveCursorToScreenPoint(x, y);
            }

            if (button != InputStub.BUTTON_LEFT || !(mInjector.tapToMove && mInputStrategy instanceof InputStrategyInterface.TrackpadInputStrategy))
                mInputStrategy.onTap(button);
            else
                mGestureListenerHandler.sendEmptyMessageDelayed(InputStub.BUTTON_LEFT, ViewConfiguration.getDoubleTapTimeout());
        }


        private float mLastFocusX;
        private float mLastFocusY;
        @Override
        public boolean onDoubleTapEvent(MotionEvent e) {
            if (e.getPointerCount() == 1) {
                switch(e.getActionMasked()) {
                    case MotionEvent.ACTION_DOWN:
                        if (mInjector.tapToMove && mInputStrategy instanceof InputStrategyInterface.TrackpadInputStrategy) {
                            mGestureListenerHandler.removeMessages(InputStub.BUTTON_LEFT);
                            if (mInputStrategy.onPressAndHold(InputStub.BUTTON_LEFT, true))
                                mIsDragging = true;
                        }
                        break;
                    case MotionEvent.ACTION_MOVE:
                        onScroll(null, e, mLastFocusX - e.getX(), mLastFocusY - e.getY());
                        break;
                }

                mLastFocusX = e.getX();
                mLastFocusY = e.getY();
            }

            return true;
        }

        /** Called when a long-press is triggered for one or more fingers. */
        @Override
        public void onLongPress(int pointerCount, float x, float y) {
            int button = mouseButtonFromPointerCount(pointerCount);
            if (button == InputStub.BUTTON_UNDEFINED) {
                return;
            }

            if (!(mInputStrategy instanceof InputStrategyInterface.TrackpadInputStrategy)) {
                if (screenPointLiesOutsideImageBoundary(x, y))
                    return;
                moveCursorToScreenPoint(x, y);
            }

            if (mInputStrategy.onPressAndHold(button, false))
                mIsDragging = true;
        }

        /** Maps the number of fingers in a tap or long-press gesture to a mouse-button. */
        private int mouseButtonFromPointerCount(int pointerCount) {
            switch (pointerCount) {
                case 1:
                    return InputStub.BUTTON_LEFT;
                case 2:
                    return InputStub.BUTTON_RIGHT;
                case 3:
                    return InputStub.BUTTON_MIDDLE;
                default:
                    return InputStub.BUTTON_UNDEFINED;
            }
        }

        /** Determines whether the given screen point lies outside the desktop image. */
        private boolean screenPointLiesOutsideImageBoundary(float screenX, float screenY) {
            float scaledX = screenX * mRenderData.scale.x, scaledY = screenY * mRenderData.scale.y;

            float imageWidth = (float) mRenderData.imageWidth + EPSILON;
            float imageHeight = (float) mRenderData.imageHeight + EPSILON;

            return scaledX < -EPSILON || scaledX > imageWidth || scaledY < -EPSILON || scaledY > imageHeight;
        }
    }

    public boolean sendKeyEvent(KeyEvent event) {
        return mInjector.sendKeyEvent(event);
    }

    private class HardwareMouseListener {
        private int savedBS = 0;
        private int currentBS = 0;

        boolean isMouseButtonChanged(int mask) {
            return (savedBS & mask) != (currentBS & mask);
        }

        boolean mouseButtonDown(int mask) {
            return ((currentBS & mask) != 0);
        }

        private final int[][] buttons = {
                {MotionEvent.BUTTON_PRIMARY, InputStub.BUTTON_LEFT},
                {MotionEvent.BUTTON_TERTIARY, InputStub.BUTTON_MIDDLE},
                {MotionEvent.BUTTON_SECONDARY, InputStub.BUTTON_RIGHT}
        };

        /** @noinspection ReassignedVariable, SuspiciousNameCombination*/
        @SuppressLint("ClickableViewAccessibility")
        boolean onTouch(View v, MotionEvent e) {
            if (e.getAction() == MotionEvent.ACTION_SCROLL) {
                float scrollY = -100 * e.getAxisValue(MotionEvent.AXIS_VSCROLL);
                float scrollX = -100 * e.getAxisValue(MotionEvent.AXIS_HSCROLL);

                mInjector.sendMouseWheelEvent(scrollX, scrollY);
                return true;
            }

            if (!v.hasPointerCapture()) {
                float scaledX = e.getX() * mRenderData.scale.x, scaledY = e.getY() * mRenderData.scale.y;
                if (mRenderData.setCursorPosition(scaledX, scaledY))
                    mInjector.sendCursorMove(scaledX, scaledY, false);
            } else if (e.getAction() == MotionEvent.ACTION_MOVE && e.getPointerCount() == 1) {
                boolean axis_relative_x = e.getDevice().getMotionRange(MotionEvent.AXIS_RELATIVE_X) != null;
                boolean mouse_relative = (e.getSource() & InputDevice.SOURCE_MOUSE_RELATIVE) == InputDevice.SOURCE_MOUSE_RELATIVE;
                if (axis_relative_x || mouse_relative) {
                    float x = axis_relative_x ? e.getAxisValue(MotionEvent.AXIS_RELATIVE_X) : e.getX();
                    float y = axis_relative_x ? e.getAxisValue(MotionEvent.AXIS_RELATIVE_Y) : e.getY();
                    float temp;

                    switch (capturedPointerTransformation) {
                        case CapturedPointerTransformation.NONE:
                            break;
                        case CapturedPointerTransformation.CLOCKWISE:
                            temp = x; x = -y; y = temp; break;
                        case CapturedPointerTransformation.COUNTER_CLOCKWISE:
                            temp = x; x = y; y = -temp; break;
                        case CapturedPointerTransformation.UPSIDE_DOWN:
                            x = -x; y = -y; break;
                    }

                    x *= mInjector.capturedPointerSpeedFactor * mMetrics.density;
                    y *= mInjector.capturedPointerSpeedFactor * mMetrics.density;

                    mInjector.sendCursorMove(x, y, true);
                    if (axis_relative_x && mTouchpadHandler != null)
                        mTouchpadHandler.mTapDetector.onTouchEvent(e);
                }
            }

            currentBS = e.getButtonState();
            for (int[] button: buttons)
                if (isMouseButtonChanged(button[0]))
                    mInjector.sendMouseEvent(null, button[1], mouseButtonDown(button[0]), true);
            savedBS = currentBS;
            return true;
        }
    }

    private class StylusListener {
        private int button = 0;

        // I've got this on SM-N770F
        private static final int ACTION_PRIMARY_DOWN = 0xd3;
        private static final int ACTION_PRIMARY_UP = 0xd4;

        private float x = 0, y = 0, pressure = 0, tilt = 0, orientation = 0;
        private int buttons = 0;

        private int convertOrientation(float value) {
            int newValue = (int) (((value * 180 / Math.PI) + 360) % 360);
            if (newValue > 180)
                newValue = (newValue - 360) % 360;
            return newValue;
        }

        private boolean hasButton(MotionEvent e, int button) {
            return (e.getButtonState() & button) == button;
        }

        int extractButtons(MotionEvent e) {
            if (mInjector.stylusButtonContactModifierMode) {
                if (e.getAction() == MotionEvent.ACTION_DOWN || e.getAction() == MotionEvent.ACTION_MOVE) {
                    if (hasButton(e, MotionEvent.BUTTON_STYLUS_SECONDARY))
                        return (1 << 1);
                    if (hasButton(e, MotionEvent.BUTTON_STYLUS_PRIMARY))
                        return (1 << 2);
                    else
                        return STYLUS_INPUT_HELPER_MODE;
                } else return 0;
            } else {
                int buttons = 0;
                if (e.getAction() == MotionEvent.ACTION_DOWN || e.getAction() == MotionEvent.ACTION_MOVE)
                    buttons = STYLUS_INPUT_HELPER_MODE;
                if (hasButton(e, MotionEvent.BUTTON_STYLUS_SECONDARY))
                    buttons |= (1 << 1);
                if (hasButton(e, MotionEvent.BUTTON_STYLUS_PRIMARY))
                    buttons |= (1 << 2);

                return buttons;
            }
        }

        public boolean isExternal(InputDevice d) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
                return d.isExternal();

            try {
                // isExternal is a hidden method that is not accessible through the SDK_INT before Android Q
                //noinspection DataFlowIssue
                return (Boolean) InputDevice.class.getMethod("isExternal").invoke(d);
            } catch (NullPointerException | NoSuchMethodException | IllegalAccessException | InvocationTargetException e) {
                return false;
            }
        }

        @SuppressLint("ClickableViewAccessibility")
        boolean onTouch(MotionEvent e) {
            if (mInjector.stylusIsMouse)
                return onTouchMouse(e);

            int action = e.getAction();
            int tiltX = 0, tiltY = 0;
            int newButtons = extractButtons(e);
            float newX = e.getX(e.getActionIndex()), newY = e.getY(e.getActionIndex());
            InputDevice.MotionRange rangeX = e.getDevice().getMotionRange(MotionEvent.AXIS_X);
            InputDevice.MotionRange rangeY = e.getDevice().getMotionRange(MotionEvent.AXIS_Y);

            if (MainActivity.getInstance().getLorieView().hasPointerCapture() &&
                    isExternal(e.getDevice()) && rangeX != null && rangeY != null) {
                newX *= mRenderData.imageWidth / rangeX.getMax();
                newY *= mRenderData.imageHeight / rangeY.getMax();
            } else {
                newX *= mRenderData.scale.x;
                newY *= mRenderData.scale.y;
            }

            if (x == newX && y == newY && pressure == e.getPressure() && tilt == e.getAxisValue(MotionEvent.AXIS_TILT) &&
                    orientation == e.getAxisValue(MotionEvent.AXIS_ORIENTATION) && buttons == newButtons)
                return true;

            if (e.getDevice().getMotionRange(MotionEvent.AXIS_TILT) != null &&
                    e.getDevice().getMotionRange(MotionEvent.AXIS_ORIENTATION) != null) {
                orientation = e.getAxisValue(MotionEvent.AXIS_ORIENTATION);
                tilt = e.getAxisValue(MotionEvent.AXIS_TILT);
                tiltX = (int) Math.round((float) Math.asin(-Math.sin(orientation) * Math.sin(tilt)) * 63.5 - 0.5);
                tiltY = (int) Math.round((float) Math.asin( Math.cos(orientation) * Math.sin(tilt)) * 63.5 - 0.5);
            }

            android.util.Log.d("STYLUS_EVENT", "action " + action + " x " + newX + " y " + newY + " pressure " + e.getPressure() + " tilt " + e.getAxisValue(MotionEvent.AXIS_TILT) + " orientation " + e.getAxisValue(MotionEvent.AXIS_ORIENTATION));
            mInjector.sendStylusEvent(
                    x = newX,
                    y = newY,
                    (int) ((pressure = e.getPressure()) * 65535),
                    tiltX,
                    tiltY,
                    convertOrientation(orientation),
                    buttons = newButtons,
                    e.getToolType(e.getActionIndex()) == MotionEvent.TOOL_TYPE_ERASER);

            return true;
        }

        @SuppressLint("ClickableViewAccessibility")
        boolean onTouchMouse(MotionEvent e) {
            int action = e.getAction();
            float scaledX = e.getX(e.getActionIndex()) * mRenderData.scale.x, scaledY = e.getY(e.getActionIndex()) * mRenderData.scale.y;
            if (mRenderData.setCursorPosition(scaledX, scaledY))
                mInjector.sendCursorMove(scaledX, scaledY, false);

            if (action == MotionEvent.ACTION_DOWN || action == ACTION_PRIMARY_DOWN) {
                button = STYLUS_INPUT_HELPER_MODE;
                if (button == 1) {
                    if (e.isButtonPressed(MotionEvent.BUTTON_STYLUS_PRIMARY))
                        button = 3;
                    if (e.isButtonPressed(MotionEvent.BUTTON_STYLUS_SECONDARY))
                        button = 2;
                }
            }

            if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_UP || action == ACTION_PRIMARY_DOWN || action == ACTION_PRIMARY_UP)
                mInjector.sendMouseEvent(mRenderData.getCursorPosition(), button, (action == MotionEvent.ACTION_DOWN || action == ACTION_PRIMARY_DOWN), false);

            if (action == MotionEvent.ACTION_UP)
                button = 0;

            return true;
        }
    }

    /** @noinspection NullableProblems*/
    private class DexListener extends GestureDetector.SimpleOnGestureListener {
        private final GestureDetector mScroller;
        private int savedBS = 0;
        private int currentBS = 0;
        private boolean onTap = false;
        private boolean mIsDragging = false;
        private boolean mIsScrolling = false;
        DexListener(Context ctx) {
            mScroller = new GestureDetector(ctx, this, null, false);
        }
        private final Handler handler = new Handler();
        private final Runnable mouseDownRunnable = () -> mInjector.sendMouseEvent(mRenderData.getCursorPosition(), InputStub.BUTTON_LEFT, true, false);

        private final int[][] buttons = {
                {MotionEvent.BUTTON_PRIMARY, InputStub.BUTTON_LEFT},
                {MotionEvent.BUTTON_TERTIARY, InputStub.BUTTON_MIDDLE},
                {MotionEvent.BUTTON_SECONDARY, InputStub.BUTTON_RIGHT}
        };

        boolean isMouseButtonChanged(int mask) {
            return (savedBS & mask) != (currentBS & mask);
        }

        boolean mouseButtonDown(int mask) {
            return ((currentBS & mask) != 0);
        }

        boolean checkButtons(MotionEvent e) {
            boolean isHandled = false;
            currentBS = e.getButtonState();
            for (int[] button: buttons) {
                if (isMouseButtonChanged(button[0])) {
                    mInjector.sendMouseEvent(mRenderData.getCursorPosition(), button[1], mouseButtonDown(button[0]), false);
                    isHandled = true;
                }
            }
            savedBS = currentBS;
            return isHandled;
        }

        private boolean hasFlags(MotionEvent e, int flags) {
            return (e.getFlags() & flags) == flags;
        }

        @SuppressLint({"WrongConstant", "InlinedApi"})
        private boolean isScrollingEvent(MotionEvent e) {
            return hasFlags(e, 0x14000000) || (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q && e.getClassification() == MotionEvent.CLASSIFICATION_TWO_FINGER_SWIPE);
        }

        boolean onTouch(@SuppressWarnings("unused") View v, MotionEvent e) {
            boolean isButtonHandled;
            switch(e.getActionMasked()) {
                case MotionEvent.ACTION_BUTTON_PRESS:
                case MotionEvent.ACTION_BUTTON_RELEASE:
                    mScroller.onGenericMotionEvent(e);
                    handler.removeCallbacks(mouseDownRunnable);
                    onTap = e.getActionMasked() == MotionEvent.ACTION_BUTTON_PRESS;
                    mIsDragging = false;

                    checkButtons(e);
                    return true;
                case MotionEvent.ACTION_HOVER_MOVE: {
                    float scaledX = e.getX() * mRenderData.scale.x, scaledY = e.getY() * mRenderData.scale.y;
                    if (mRenderData.setCursorPosition(scaledX, scaledY))
                        mInjector.sendCursorMove(scaledX, scaledY, false);
                    return true;
                }
                case MotionEvent.ACTION_DOWN:
                    isButtonHandled = checkButtons(e);
                    if (isScrollingEvent(e)) {
                        mIsScrolling = true;
                        mScroller.onTouchEvent(e);
                    } else if (hasFlags(e, 0x4000000)) {
                        mIsDragging = true;
                        handler.postDelayed(mouseDownRunnable, 0);
                    } else if (!isButtonHandled) {
                        onTap = true;
                        mInjector.sendMouseEvent(mRenderData.getCursorPosition(), InputStub.BUTTON_LEFT, true, false);
                    }
                    return true;
                case MotionEvent.ACTION_UP:
                    isButtonHandled = checkButtons(e);
                    if (isScrollingEvent(e)) {
                        mScroller.onTouchEvent(e);
                        mIsScrolling = false;
                    }
                    else if (hasFlags(e, 0x4000000)) {
                        mInjector.sendMouseEvent(mRenderData.getCursorPosition(), InputStub.BUTTON_LEFT, false, false);
                        mIsDragging = false;
                    } else if (!isButtonHandled && onTap) {
                        mInjector.sendMouseEvent(mRenderData.getCursorPosition(), InputStub.BUTTON_LEFT, false, false);
                        onTap = false;
                    }

                    return true;
                case MotionEvent.ACTION_MOVE:
                    if (mIsScrolling && isScrollingEvent(e))
                        mScroller.onTouchEvent(e);
                    else if ((mIsDragging && hasFlags(e, 0x4000000)) || onTap) {
                        float scaledX = e.getX() * mRenderData.scale.x, scaledY = e.getY() * mRenderData.scale.y;
                        if (mRenderData.setCursorPosition(scaledX, scaledY))
                            mInjector.sendCursorMove(scaledX, scaledY, false);
                    }
                    return true;
                case MotionEvent.ACTION_HOVER_EXIT: // when the user removes their hand from the trackpad, all states should be reset
                case MotionEvent.ACTION_CANCEL:
                    onTap = false;
                    mIsScrolling = false;
                    mIsDragging = false;
                    return true;
            }
            return false;
        }

        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
            mInjector.sendMouseWheelEvent(distanceX, distanceY);
            return true;
        }

        @Override
        public boolean onDoubleTapEvent(MotionEvent e) {
            onSingleTapConfirmed(e);
            onSingleTapConfirmed(e);
            return true;
        }

        @Override
        public boolean onSingleTapConfirmed(MotionEvent e) {
            mInjector.sendMouseEvent(mRenderData.getCursorPosition(), InputStub.BUTTON_LEFT, true, false);
            mInjector.sendMouseEvent(mRenderData.getCursorPosition(), InputStub.BUTTON_LEFT, false, false);
            return true;
        }
    }


    /**
     * Interface with a set of functions to control the behavior of the remote host renderer.
     */
    public interface RenderStub {
        /**
         * Informs the stub that swipe was performed.
         */
        void swipeUp();

        /**
         * Informs the stub that swipe was performed.
         */
        void swipeDown();

        class NullStub implements RenderStub {
            @Override public void swipeUp() {}
            @Override public void swipeDown() {}
        }
    }
}
