// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.termux.x11.input;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.graphics.Matrix;
import android.graphics.PointF;
import android.view.GestureDetector;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.Surface;
import android.view.View;

import androidx.annotation.IntDef;
import androidx.core.math.MathUtils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

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

    private final RenderData mRenderData;
    private final RenderStub mRenderStub;
    private final GestureDetector mScroller;
    private final ScaleGestureDetector mZoomer;
    private final TapGestureDetector mTapDetector;
    private final StylusListener mStylusListener = new StylusListener();
    private final HardwareMouseListener mHMListener = new HardwareMouseListener();
    private final DexListener mDexListener;
    private final TouchInputHandler mTouchpadHandler;
    private final TouchpadListener mTouchpadListener = new TouchpadListener();

    /** Used to disambiguate a 2-finger gesture as a swipe or a pinch. */
    private final SwipeDetector mSwipePinchDetector;

    private InputStrategyInterface mInputStrategy;
    private final InputEventSender mInjector;
    private final Context mContext;

    /**
     * Used for tracking swipe gestures. Only the Y-direction is needed for responding to swipe-up
     * or swipe-down.
     */
    private float mTotalMotionY;

    /**
     * Distance in pixels beyond which a motion gesture is considered to be a swipe. This is
     * initialized using the Context passed into the ctor.
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
     * Set to true when a 1 finger pan gesture originates with a longpress.  This means the user
     * is performing a drag operation.
     */
    private boolean mIsDragging;

    private TouchInputHandler(Context ctx, RenderData renderData, RenderStub renderStub,
                              final InputEventSender injector, boolean isTouchpad) {
        if (renderStub == null || injector == null)
            throw new NullPointerException();

        mRenderStub = renderStub;
        mRenderData = renderData != null ? renderData :new RenderData();
        mInjector = injector;
        mContext = ctx;

        GestureListener listener = new GestureListener();
        mScroller = new GestureDetector(/*desktop*/ ctx, listener, null, false);

        // If long-press is enabled, the gesture-detector will not emit any further onScroll
        // notifications after the onLongPress notification. Since onScroll is being used for
        // moving the cursor, it means that the cursor would become stuck if the finger were held
        // down too long.
        mScroller.setIsLongpressEnabled(false);

        mZoomer = new ScaleGestureDetector(/*desktop*/ ctx, listener);
        mTapDetector = new TapGestureDetector(/*desktop*/ ctx, listener);
        mSwipePinchDetector = new SwipeDetector(/*desktop*/ ctx);

        // The threshold needs to be bigger than the ScaledTouchSlop used by the gesture-detectors,
        // so that a gesture cannot be both a tap and a swipe. It also needs to be small enough so
        // that intentional swipes are usually detected.
        float density = /*desktop*/ ctx.getResources().getDisplayMetrics().density;
        mSwipeThreshold = 40 * density;

//        mEdgeSlopInPx = ViewConfiguration.get(/*desktop*/ ctx).getScaledEdgeSlop();

        setInputMode(InputMode.TRACKPAD);
        mDexListener = new DexListener(ctx);
        mTouchpadHandler = isTouchpad ? null : new TouchInputHandler(ctx, mRenderData, renderStub, injector, true);
    }

    public TouchInputHandler(Context ctx, RenderStub renderStub, final InputEventSender injector) {
        this(ctx, null, renderStub, injector, false);
    }

    boolean isDexEvent(MotionEvent event) {
        int SOURCE_DEX = InputDevice.SOURCE_MOUSE | InputDevice.SOURCE_TOUCHSCREEN;
        return ((event.getSource() & SOURCE_DEX) == SOURCE_DEX) &&
                ((event.getSource() & InputDevice.SOURCE_TOUCHPAD) != InputDevice.SOURCE_TOUCHPAD);
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

        if (event.getToolType(event.getActionIndex()) == MotionEvent.TOOL_TYPE_STYLUS) {
            return mStylusListener.onTouch(view, event);
        }

        if (event.getToolType(event.getActionIndex()) == MotionEvent.TOOL_TYPE_MOUSE
              || (event.getSource() & InputDevice.SOURCE_MOUSE ) == InputDevice.SOURCE_MOUSE)
            return mHMListener.onTouch(view, event);

        if (event.getToolType(event.getActionIndex()) == MotionEvent.TOOL_TYPE_FINGER) {
            // Dex touchpad sends events as finger, but it should be considered as a mouse.
            if (isDexEvent(event))
                return mDexListener.onTouch(view, event);

            // Regular touchpads and Dex touchpad send events as finger too,
            // but they should be handled as touchscreens with trackpad mode.
            if (mTouchpadHandler != null && (event.getSource() & InputDevice.SOURCE_TOUCHPAD) == InputDevice.SOURCE_TOUCHPAD)
                return mTouchpadHandler.handleTouchEvent(view, view, event);

            // Give the underlying input strategy a chance to observe the current motion event before
            // passing it to the gesture detectors.  This allows the input strategy to react to the
            // event or save the payload for use in recreating the gesture remotely.
            mInputStrategy.onMotionEvent(event);

            // Avoid short-circuit logic evaluation - ensure all gesture detectors see all events so
            // that they generate correct notifications.
            mScroller.onTouchEvent(event);
            mZoomer.onTouchEvent(event);
            mTapDetector.onTouchEvent(event);
            mSwipePinchDetector.onTouchEvent(event);
            mTouchpadListener.onTouch(view, event);

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

    public boolean handleCapturedEvent(View v, MotionEvent e) {
        int button = mouseButtonFromMotionEvent(e);
        switch(e.getAction()) {
            case MotionEvent.ACTION_MOVE:
                if (e.getPointerCount() != 1 || (e.getAxisValue(MotionEvent.AXIS_RELATIVE_X) == 0 && e.getAxisValue(MotionEvent.AXIS_RELATIVE_Y) == 0))
                    break;

                int rot = (int) (e.getOrientation() * 2 / Math.PI);
                float x = 2 * e.getAxisValue(MotionEvent.AXIS_RELATIVE_X);
                float y = 2 * e.getAxisValue(MotionEvent.AXIS_RELATIVE_Y);

                /* Getting */
                rot = (rot == -1) ? Surface.ROTATION_90 : ((rot == 1) ? Surface.ROTATION_270 : rot);
                /* Get rotation of event relative to display rotation */
                rot = (4 + rot - v.getDisplay().getRotation()) % 4;
                /* Send events, flipped and transformed according to relative rotation */
                switch(rot) {
                    case Surface.ROTATION_0:
                        mInjector.sendCursorMove(x, y, true);
                        break;
                    case Surface.ROTATION_90:
                        mInjector.sendCursorMove(-y, -x, true);
                        break;
                    case Surface.ROTATION_180:
                        mInjector.sendCursorMove(-x, -y, true);
                        break;
                    case Surface.ROTATION_270:
                        //noinspection SuspiciousNameCombination
                        mInjector.sendCursorMove(y, -x, true);
                        break;
                }

                return true;
            case MotionEvent.ACTION_BUTTON_PRESS:
                mInjector.sendMouseDown(button, true);
                return true;
            case MotionEvent.ACTION_BUTTON_RELEASE:
                mInjector.sendMouseUp(button, true);
                return true;
            case MotionEvent.ACTION_SCROLL:
                float scrollY = -100 * e.getAxisValue(MotionEvent.AXIS_VSCROLL);
                float scrollX = -100 * e.getAxisValue(MotionEvent.AXIS_HSCROLL);

                mInjector.sendMouseWheelEvent(scrollX, scrollY);
                return true;
            default:
                break;
        }

        if (((e.getSource() & InputDevice.SOURCE_TOUCHPAD) == InputDevice.SOURCE_TOUCHPAD) || ((e.getSource() & InputDevice.SOURCE_BLUETOOTH_STYLUS) == InputDevice.SOURCE_BLUETOOTH_STYLUS)) {
            if (mTouchpadHandler != null)
                mTouchpadHandler.handleTouchEvent(v, v, e);
        }

        return true;
    }

    private void resetTransformation() {
        float sx = (float) mRenderData.imageWidth / mRenderData.screenWidth;
        float sy = (float) mRenderData.imageHeight / mRenderData.screenHeight;
        mRenderData.transform.setScale(sx, sy);
    }

    public void handleClientSizeChanged(int w, int h) {
        mRenderData.screenWidth = w;
        mRenderData.screenHeight = h;

        resetTransformation();

        if (mTouchpadHandler != null)
            mTouchpadHandler.handleClientSizeChanged(w, h);

        moveCursorToScreenPoint((float) w / 2, (float) h / 2);
    }

    public void handleHostSizeChanged(int w, int h) {
        mRenderData.imageWidth = w;
        mRenderData.imageHeight = h;
//        mPanGestureBounds = new Rect(mEdgeSlopInPx, mEdgeSlopInPx, w - mEdgeSlopInPx, h - mEdgeSlopInPx);
        moveCursorToScreenPoint((float) w/2, (float) h/2);

        resetTransformation();

        if (mTouchpadHandler != null)
            mTouchpadHandler.handleHostSizeChanged(w, h);
    }

    public void setInputMode(@InputMode int inputMode) {
        if (inputMode == InputMode.TOUCH)
            mInputStrategy = new TouchInputStrategy(mRenderData, mInjector);
        else if (inputMode == InputMode.SIMULATED_TOUCH)
            mInputStrategy = new SimulatedTouchInputStrategy(mRenderData, mInjector, mContext);
        else
            mInputStrategy = new TrackpadInputStrategy(mInjector);
    }

    public void setPreferScancodes(boolean enabled) {
        mInjector.preferScancodes = enabled;
    }

    public void setPointerCaptureEnabled(boolean enabled) {
        mInjector.pointerCapture = enabled;
    }

    private void moveCursorByOffset(float deltaX, float deltaY) {
        if (mInputStrategy instanceof TrackpadInputStrategy)
            mInjector.sendCursorMove((int) -deltaX, (int) -deltaY, true);
        else if (mInputStrategy instanceof SimulatedTouchInputStrategy) {
            PointF cursorPos = mRenderData.getCursorPosition();
            cursorPos.offset(-deltaX, -deltaY);
            cursorPos.set(MathUtils.clamp(cursorPos.x, 0, mRenderData.screenWidth), MathUtils.clamp(cursorPos.y, 0, mRenderData.screenHeight));
            if (mRenderData.setCursorPosition(cursorPos.x, cursorPos.y))
                mInjector.sendCursorMove((int) cursorPos.x, (int) cursorPos.y, false);
        }
    }

    /** Moves the cursor to the specified position on the screen. */
    private void moveCursorToScreenPoint(float screenX, float screenY) {
        if (mInputStrategy instanceof TrackpadInputStrategy || mInputStrategy instanceof SimulatedTouchInputStrategy) {
            float[] imagePoint = mapScreenPointToImagePoint(screenX, screenY);
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

    /** Translates a point in screen coordinates to a location on the desktop image. */
    private float[] mapScreenPointToImagePoint(float screenX, float screenY) {
        float[] mappedPoints = {screenX, screenY};
        Matrix screenToImage = new Matrix();

        mRenderData.transform.invert(screenToImage);
        screenToImage.mapPoints(mappedPoints);

        return mappedPoints;
    }
    private int mouseButtonFromMotionEvent(MotionEvent e) {
        switch (e.getActionButton()) {
            case MotionEvent.BUTTON_PRIMARY:
                return InputStub.BUTTON_LEFT;
            case MotionEvent.BUTTON_SECONDARY:
                return InputStub.BUTTON_RIGHT;
            case MotionEvent.BUTTON_TERTIARY:
                return InputStub.BUTTON_MIDDLE;
            default:
                return InputStub.BUTTON_UNDEFINED;
        }
    }

    /** Responds to touch events filtered by the gesture detectors.
     * @noinspection NullableProblems */
    private class GestureListener extends GestureDetector.SimpleOnGestureListener
            implements ScaleGestureDetector.OnScaleGestureListener,
                       TapGestureDetector.OnTapListener {
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
                if (!(mInputStrategy instanceof TrackpadInputStrategy)) {
                    // Ensure the cursor is located at the coordinates of the original event,
                    // otherwise the target window may not receive the scroll event correctly.
                    moveCursorToScreenPoint(e1.getX(), e1.getY());
                }
                mInputStrategy.onScroll(distanceX, distanceY);

                // Prevent the cursor being moved or flung by the gesture.
                mSuppressCursorMovement = true;
                return true;
            }

            if (pointerCount != 1 || mSuppressCursorMovement) {
                return false;
            }

            float[] delta = {distanceX, distanceY};

            Matrix canvasToImage = new Matrix();
            mRenderData.transform.invert(canvasToImage);
            canvasToImage.mapVectors(delta);

            if (mInputStrategy instanceof TrackpadInputStrategy)
                moveCursorByOffset(delta[0], delta[1]);
            if (!(mInputStrategy instanceof TrackpadInputStrategy) && mIsDragging) {
                // Ensure the cursor follows the user's finger when the user is dragging under
                // direct input mode.
                moveCursorToScreenPoint(e2.getX(), e2.getY());
            }
            return true;
        }

        /** Called when the user is in the process of pinch-zooming. */
        @Override
        public boolean onScale(ScaleGestureDetector detector) {
            return true;
        }

        /** Called whenever a gesture starts. Always accepts the gesture so it isn't ignored. */
        @Override
        public boolean onDown(MotionEvent e) {
            return true;
        }

        /**
         * Called when the user starts to zoom. Always accepts the zoom so that
         * onScale() can decide whether to respond to it.
         */
        @Override
        public boolean onScaleBegin(ScaleGestureDetector detector) {
            return true;
        }

        /** Called when the user is done zooming. Defers to onScale()'s judgement. */
        @Override
        public void onScaleEnd(ScaleGestureDetector detector) {
        }

        /**
         * Called when the user taps the screen with one or more fingers.
         */
        @Override
        public void onTap(int pointerCount, float x, float y) {
            int button = mouseButtonFromPointerCount(pointerCount);
            if (button == InputStub.BUTTON_UNDEFINED)
                return;

            if (!(mInputStrategy instanceof TrackpadInputStrategy)) {
                if (screenPointLiesOutsideImageBoundary(x, y))
                    return;

                moveCursorToScreenPoint(x, y);
            }

            mInputStrategy.onTap(button);
        }

        /** Called when a long-press is triggered for one or more fingers. */
        @Override
        public void onLongPress(int pointerCount, float x, float y) {
            int button = mouseButtonFromPointerCount(pointerCount);
            if (button == InputStub.BUTTON_UNDEFINED) {
                return;
            }

            if (!(mInputStrategy instanceof TrackpadInputStrategy)) {
                if (screenPointLiesOutsideImageBoundary(x, y))
                    return;
                moveCursorToScreenPoint(x, y);
            }

            if (mInputStrategy.onPressAndHold(button))
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
            float[] mappedPoints = mapScreenPointToImagePoint(screenX, screenY);

            float imageWidth = (float) mRenderData.imageWidth + EPSILON;
            float imageHeight = (float) mRenderData.imageHeight + EPSILON;

            return mappedPoints[0] < -EPSILON || mappedPoints[0] > imageWidth
                    || mappedPoints[1] < -EPSILON || mappedPoints[1] > imageHeight;
        }
    }

    public boolean sendKeyEvent(View view, KeyEvent event) {
        return mInjector.sendKeyEvent(view, event);
    }

    private class TouchpadListener {
        void onTouch(View v, MotionEvent e) {
            if (mInjector.pointerCapture && !v.hasPointerCapture() && e.getAction() == MotionEvent.ACTION_UP)
                v.requestPointerCapture();
            if ( (e.getActionMasked() == MotionEvent.ACTION_MOVE) ||
                    (e.getActionMasked() == MotionEvent.ACTION_HOVER_MOVE) &&
                    e.getPointerCount() == 1 &&
                    (e.getSource() & InputDevice.SOURCE_TOUCHPAD) == InputDevice.SOURCE_TOUCHPAD) {
                moveCursorByOffset(
                        -2 * e.getAxisValue(MotionEvent.AXIS_RELATIVE_X),
                        -2 * e.getAxisValue(MotionEvent.AXIS_RELATIVE_Y));
            }
        }
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

        @SuppressLint("ClickableViewAccessibility")
        boolean onTouch(View v, MotionEvent e) {
            if (mInjector.pointerCapture && !v.hasPointerCapture() && e.getAction() == MotionEvent.ACTION_UP)
                v.requestPointerCapture();

            if (e.getAction() == MotionEvent.ACTION_SCROLL) {
                float scrollY = -100 * e.getAxisValue(MotionEvent.AXIS_VSCROLL);
                float scrollX = -100 * e.getAxisValue(MotionEvent.AXIS_HSCROLL);

                mInjector.sendMouseWheelEvent(scrollX, scrollY);
                return true;
            }

            float[] imagePoint = mapScreenPointToImagePoint((int) e.getX(), (int) e.getY());
            if (mRenderData.setCursorPosition(imagePoint[0], imagePoint[1]))
                mInjector.sendCursorMove(imagePoint[0], imagePoint[1], false);

            currentBS = e.getButtonState();
            if (isMouseButtonChanged(MotionEvent.BUTTON_PRIMARY))
                mInjector.sendMouseEvent(mRenderData.getCursorPosition(), InputStub.BUTTON_LEFT, mouseButtonDown(MotionEvent.BUTTON_PRIMARY), true);
            if (isMouseButtonChanged(MotionEvent.BUTTON_TERTIARY))
                mInjector.sendMouseEvent(mRenderData.getCursorPosition(), InputStub.BUTTON_MIDDLE, mouseButtonDown(MotionEvent.BUTTON_TERTIARY), true);
            if (isMouseButtonChanged(MotionEvent.BUTTON_SECONDARY))
                mInjector.sendMouseEvent(mRenderData.getCursorPosition(), InputStub.BUTTON_RIGHT, mouseButtonDown(MotionEvent.BUTTON_SECONDARY), true);
            savedBS = currentBS;
            return true;
        }
    }

    private class StylusListener {
        private int button = 0;

        // I've got this on SM-N770F
        private static final int ACTION_PRIMARY_DOWN = 0xd3;
        private static final int ACTION_PRIMARY_UP = 0xd4;

        @SuppressLint("ClickableViewAccessibility")
        boolean onTouch(View v, MotionEvent e) {
            if (mInjector.pointerCapture && !v.hasPointerCapture() && e.getAction() == MotionEvent.ACTION_UP)
                v.requestPointerCapture();

            int action = e.getAction();
            float[] imagePoint = mapScreenPointToImagePoint((int) e.getX(e.getActionIndex()), (int) e.getY(e.getActionIndex()));
            if (mRenderData.setCursorPosition(imagePoint[0], imagePoint[1]))
                mInjector.sendCursorMove(imagePoint[0], imagePoint[1], false);


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
        private boolean mIsDragging = false;
        private boolean mIsScrolling = false;
        DexListener(Context ctx) {
            mScroller = new GestureDetector(ctx, this, null, false);
        }

        boolean isMouseButtonChanged(int mask) {
            return (savedBS & mask) != (currentBS & mask);
        }

        boolean mouseButtonDown(int mask) {
            return ((currentBS & mask) != 0);
        }

        boolean onTouch(@SuppressWarnings("unused") View v, MotionEvent e) {
            switch(e.getActionMasked()) {
                case MotionEvent.ACTION_BUTTON_PRESS:
                case MotionEvent.ACTION_BUTTON_RELEASE:
                    mScroller.onGenericMotionEvent(e);

                    currentBS = e.getButtonState();
                    if (isMouseButtonChanged(MotionEvent.BUTTON_PRIMARY))
                        mInjector.sendMouseEvent(mRenderData.getCursorPosition(), InputStub.BUTTON_LEFT, mouseButtonDown(MotionEvent.BUTTON_PRIMARY), false);
                    if (isMouseButtonChanged(MotionEvent.BUTTON_TERTIARY))
                        mInjector.sendMouseEvent(mRenderData.getCursorPosition(), InputStub.BUTTON_MIDDLE, mouseButtonDown(MotionEvent.BUTTON_TERTIARY), false);
                    if (isMouseButtonChanged(MotionEvent.BUTTON_SECONDARY))
                        mInjector.sendMouseEvent(mRenderData.getCursorPosition(), InputStub.BUTTON_RIGHT, mouseButtonDown(MotionEvent.BUTTON_SECONDARY), false);
                    savedBS = currentBS;
                    break;
                case MotionEvent.ACTION_HOVER_MOVE:
                    float[] imagePoint = mapScreenPointToImagePoint((int) e.getX(), (int) e.getY());
                    if (mRenderData.setCursorPosition(imagePoint[0], imagePoint[1]))
                        mInjector.sendCursorMove(imagePoint[0], imagePoint[1], false);
                    break;
                case MotionEvent.ACTION_DOWN:
                    if ((e.getFlags() & 0x14000000) == 0x14000000) {
                        mIsScrolling = true;
                        mScroller.onTouchEvent(e);
                    } else if ((e.getFlags() & 0x4000000) == 0x4000000) {
                        mIsDragging = true;
                        mInjector.sendMouseEvent(mRenderData.getCursorPosition(), InputStub.BUTTON_LEFT, true, false);
                    }
                    break;
                case MotionEvent.ACTION_UP:
                    if ((e.getFlags() & 0x14000000) == 0x14000000) {
                        mScroller.onTouchEvent(e);
                        mIsScrolling = false;
                    }
                    else if ((e.getFlags() & 0x4000000) == 0x4000000) {
                        mInjector.sendMouseEvent(mRenderData.getCursorPosition(), InputStub.BUTTON_LEFT, false, false);
                        mIsDragging = false;
                    }

                    if (mInjector.pointerCapture && !v.hasPointerCapture())
                        v.requestPointerCapture();

                    break;
                case MotionEvent.ACTION_MOVE:
                    if (mIsScrolling && (e.getFlags() & 0x14000000) == 0x14000000)
                        mScroller.onTouchEvent(e);
                    else if (mIsDragging && (e.getFlags() & 0x4000000) == 0x4000000) {
                        imagePoint = mapScreenPointToImagePoint((int) e.getX(), (int) e.getY());
                        if (mRenderData.setCursorPosition(imagePoint[0], imagePoint[1]))
                            mInjector.sendCursorMove(imagePoint[0], imagePoint[1], false);
                    }
                    break;
            }
            return true;
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
