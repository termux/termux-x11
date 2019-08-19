/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.termux.x11;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Point;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;

@SuppressWarnings("unused")
public class TouchParser {

    private static final int WL_STATE_PRESSED = 1;
    private static final int WL_STATE_RELEASED = 0;

    private static final int WL_POINTER_AXIS_VERTICAL_SCROLL = 0;
    private static final int WL_POINTER_AXIS_HORIZONTAL_SCROLL = 1;

    static final int TOUCH_MODE_DIRECT = 1;
    static final int TOUCH_MODE_MOUSE = 2;
    static final int TOUCH_MODE_TOUCHPAD = 3;

    static final int BTN_LEFT = 0x110;
    static final int BTN_RIGHT = 0x111;
    static final int BTN_MIDDLE = 0x112;

    static final int ACTION_DOWN = WL_STATE_PRESSED;
    static final int ACTION_UP = WL_STATE_RELEASED;

    private static final int AXIS_X = WL_POINTER_AXIS_HORIZONTAL_SCROLL;
    private static final int AXIS_Y = WL_POINTER_AXIS_VERTICAL_SCROLL;

    public interface OnTouchParseListener {
        void onPointerButton(int button, int state);
        void onPointerMotion(int x, int y);
        void onPointerScroll(int axis, float value);

        void onTouchDown(int id, float x, float y);
        void onTouchMotion(int id, float x, float y);
        void onTouchUp(int id);
        void onTouchFrame();
    }

    private int mTouchSlopSquare;
    private int mDoubleTapTouchSlopSquare;
    private int mDoubleTapSlopSquare;

    private static final int LONGPRESS_TIMEOUT = ViewConfiguration.getLongPressTimeout();
    private static final int TAP_TIMEOUT = ViewConfiguration.getTapTimeout();
    private static final int DOUBLE_TAP_TIMEOUT = ViewConfiguration.getDoubleTapTimeout();
    private static final int DOUBLE_TAP_MIN_TIME = 40;
    private static final int FLAG_IS_GENERATED_GESTURE = 0x8;
    private static final int TOUCH_SLOP = 8;
    private static final int DOUBLE_TAP_SLOP = 100;
    private static final int DOUBLE_TAP_TOUCH_SLOP = TOUCH_SLOP;

    // constants for Message.what used by GestureHandler below
    private static final int SHOW_PRESS = 1;
    private static final int LONG_PRESS = 2;
    private static final int TAP = 3;
    private static final int RIGHT_TAP = 4;
    private static final int TOUCH_FRAME = 5;

    private final Handler mHandler;
    private final OnTouchParseListener mListener;
    private final HardwareMouseListener hmListener;

    private boolean mStillDown;
    private boolean mDeferConfirmSingleTap;
    private boolean mInLongPress;
    private boolean mInContextClick;
    private boolean mAlwaysInTapRegion;
    private boolean mAlwaysInBiggerTapRegion;

    private MotionEvent mCurrentDownEvent;
    private MotionEvent mPreviousUpEvent;

    private boolean mIsDoubleTapping;

    private float mLastFocusX;
    private float mLastFocusY;
    private float mDownFocusX;
    private float mDownFocusY;

    private boolean mIsLongpressEnabled;

    private View target;
    private Point cursor;
    private int mMode = TOUCH_MODE_DIRECT;

    private boolean mInDrag = false;
    private boolean mWasInDrag = false;

    @SuppressLint("HandlerLeak")
    private class GestureHandler extends Handler {
        GestureHandler() {
            super();
        }

        GestureHandler(Handler handler) {
            super(handler.getLooper());
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case SHOW_PRESS:
                    break;

                case LONG_PRESS:
                    dispatchLongPress();
                    break;

                case TAP:
                    // If the user's finger is still down, do not count it as a tap
                    if (!mStillDown) {
                        Log.e("tp", "tap");
                        mListener.onPointerButton(BTN_LEFT, ACTION_DOWN);
                        mListener.onPointerButton(BTN_LEFT, ACTION_UP);
                    } else {
                        mDeferConfirmSingleTap = true;
                    }
                    break;
                case RIGHT_TAP:
                    break;

                default:
                    throw new RuntimeException("Unknown message " + msg); //never
            }
        }
    }


    TouchParser(View view, OnTouchParseListener listener) {
        mListener = listener;
        hmListener = new HardwareMouseListener();
        mHandler = new GestureHandler();

        target = view;
        cursor = new Point(target.getWidth()/2, target.getHeight()/2);

        init();
    }

    private void init() {
        Context context;
        if (target == null) {
            Log.e("TouchParser","TouchParser did not receive target view");
            context = null;
        }
        else
            context = target.getRootView().findViewById(android.R.id.content).getContext();

        mIsLongpressEnabled = true;

        // Fallback to support pre-donuts releases
        int touchSlop, doubleTapSlop, doubleTapTouchSlop;
        if (context == null) {
            //noinspection deprecation
            touchSlop = ViewConfiguration.getTouchSlop();
            doubleTapTouchSlop = touchSlop; // Hack rather than adding a hiden method for this
            doubleTapSlop = DOUBLE_TAP_SLOP;
            //noinspection
        } else {
            final ViewConfiguration configuration = ViewConfiguration.get(context);
            touchSlop = configuration.getScaledTouchSlop();
            doubleTapTouchSlop = DOUBLE_TAP_TOUCH_SLOP;
            doubleTapSlop = configuration.getScaledDoubleTapSlop();
        }
        mTouchSlopSquare = touchSlop * touchSlop;
        mDoubleTapTouchSlopSquare = doubleTapTouchSlop * doubleTapTouchSlop;
        mDoubleTapSlopSquare = doubleTapSlop * doubleTapSlop;
    }

    void setMode(int mode) {
        mMode = mode;
    }

    public void setIsLongpressEnabled(boolean isLongpressEnabled) {
        mIsLongpressEnabled = isLongpressEnabled;
    }

    public boolean isLongpressEnabled() {
        return mIsLongpressEnabled;
    }

    boolean onTouchEvent(MotionEvent ev) {
        if ((ev.getSource() & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE) {
            return hmListener.onTouch(ev);
        }

        if (mMode == TOUCH_MODE_DIRECT) {
            // get pointer index from the event object
            int pointerIndex = ev.getActionIndex();

            // get pointer ID
            int pointerId = ev.getPointerId(pointerIndex);

            switch(ev.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                case MotionEvent.ACTION_POINTER_DOWN:
                    mListener.onTouchDown(pointerId, ev.getX(pointerIndex), ev.getY(pointerIndex));
                    break;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_POINTER_UP:
                    mListener.onTouchUp(pointerId);
                    break;
                case MotionEvent.ACTION_MOVE:
                    for (int i=0; i< ev.getPointerCount(); i++)
                    mListener.onTouchMotion(ev.getPointerId(i), ev.getX(i), ev.getY(i));
                    break;
            }

            mListener.onTouchFrame();
            return true;
        }

        final int action = ev.getAction();
        final boolean pointerUp =
                (action & MotionEvent.ACTION_MASK) == MotionEvent.ACTION_POINTER_UP;
        final int skipIndex = pointerUp ? ev.getActionIndex() : -1;
        final boolean isGeneratedGesture =
                (ev.getFlags() & FLAG_IS_GENERATED_GESTURE) != 0;

        // Determine focal point
        float sumX = 0, sumY = 0;
        final int count = ev.getPointerCount();
        for (int i = 0; i < count; i++) {
            if (skipIndex == i) continue;
            sumX += ev.getX(i);
            sumY += ev.getY(i);
        }
        final int div = pointerUp ? count - 1 : count;
        final float focusX = sumX / div;
        final float focusY = sumY / div;

        boolean handled = false;

        switch (action & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_POINTER_DOWN:
                mDownFocusX = mLastFocusX = focusX;
                mDownFocusY = mLastFocusY = focusY;
                // Cancel long press and taps
                stopDrag();
                cancelTaps();
                mHandler.sendEmptyMessageDelayed(RIGHT_TAP, TAP_TIMEOUT);

                break;

            case MotionEvent.ACTION_POINTER_UP:
                mDownFocusX = mLastFocusX = focusX;
                mDownFocusY = mLastFocusY = focusY;
                stopDrag();

                break;

            case MotionEvent.ACTION_DOWN:
                boolean hadTapMessage = mHandler.hasMessages(TAP);
                if (hadTapMessage) {
                    mHandler.removeMessages(TAP);
                    mHandler.removeMessages(RIGHT_TAP);
                }
                if ((mCurrentDownEvent != null) && (mPreviousUpEvent != null)
                        && hadTapMessage
                        && isConsideredDoubleTap(mCurrentDownEvent, mPreviousUpEvent, ev)) {
                    // This is a second tap
                    mIsDoubleTapping = true;
                } else {
                    // This is a first tap
                    mHandler.sendEmptyMessageDelayed(TAP, DOUBLE_TAP_TIMEOUT);
                }

                mDownFocusX = mLastFocusX = focusX;
                mDownFocusY = mLastFocusY = focusY;
                if (mCurrentDownEvent != null) {
                    mCurrentDownEvent.recycle();
                }
                mCurrentDownEvent = MotionEvent.obtain(ev);
                mAlwaysInTapRegion = true;
                mAlwaysInBiggerTapRegion = true;
                mStillDown = true;
                mInLongPress = false;
                mDeferConfirmSingleTap = false;

                if (mIsLongpressEnabled) {
                    mHandler.removeMessages(LONG_PRESS);
                    if (!mIsDoubleTapping)
                        mHandler.sendEmptyMessageAtTime(LONG_PRESS,
                            mCurrentDownEvent.getDownTime() + LONGPRESS_TIMEOUT);
                }
                if (!mIsDoubleTapping)
                    mHandler.sendEmptyMessageAtTime(SHOW_PRESS,
                        mCurrentDownEvent.getDownTime() + TAP_TIMEOUT);
                handled = true;

                if (mMode == TOUCH_MODE_MOUSE)
                    mListener.onPointerMotion((int) ev.getX(), (int) ev.getY());
                break;

            case MotionEvent.ACTION_MOVE:
                if (mInContextClick) {
                    break;
                }

                final float scrollX = mLastFocusX - focusX;
                final float scrollY = mLastFocusY - focusY;
                if (mAlwaysInTapRegion) {
                    final int deltaX = (int) (focusX - mDownFocusX);
                    final int deltaY = (int) (focusY - mDownFocusY);
                    int distance = (deltaX * deltaX) + (deltaY * deltaY);
                    int slopSquare = isGeneratedGesture ? 0 : mTouchSlopSquare;
                    if (distance > slopSquare) {
                        handled = true;
                        onScroll(ev, scrollX, scrollY);
                        mLastFocusX = focusX;
                        mLastFocusY = focusY;
                        mAlwaysInTapRegion = false;
                        mHandler.removeMessages(TAP);
                        mHandler.removeMessages(RIGHT_TAP);
                        mHandler.removeMessages(SHOW_PRESS);
                        mHandler.removeMessages(LONG_PRESS);
                    }
                    int doubleTapSlopSquare = isGeneratedGesture ? 0 : mDoubleTapTouchSlopSquare;
                    if (distance > doubleTapSlopSquare) {
                        mAlwaysInBiggerTapRegion = false;
                    }
                } else if ((Math.abs(scrollX) >= 1) || (Math.abs(scrollY) >= 1)) {
                    handled = true;
                    onScroll(ev, scrollX, scrollY);
                    mLastFocusX = focusX;
                    mLastFocusY = focusY;
                }
                break;

            case MotionEvent.ACTION_UP:
                mStillDown = false;
                MotionEvent currentUpEvent = MotionEvent.obtain(ev);
                if (mMode == TOUCH_MODE_MOUSE) {
                    stopDrag();
                }

                if (mIsDoubleTapping) {
                    Log.e("1", "1");
                    mListener.onPointerButton(BTN_LEFT, ACTION_DOWN);
                    mListener.onPointerButton(BTN_LEFT, ACTION_UP);
                    mListener.onPointerButton(BTN_LEFT, ACTION_DOWN);
                    mListener.onPointerButton(BTN_LEFT, ACTION_UP);
                    handled = true;
                } else if (mInLongPress) {
                    Log.e("2", "2");
                    mHandler.removeMessages(TAP);
                    mInLongPress = false;
                    mListener.onPointerButton(BTN_LEFT, ACTION_UP);
                } else if (mAlwaysInTapRegion) {
                    Log.e("3", "3");
                    //if (mDeferConfirmSingleTap) {
                        //handled = mListener.onSingleTapConfirmed(ev);
                    //}
                } else if (mHandler.hasMessages(RIGHT_TAP)) {
                    //Log.e("touch", "right tap confirmed");
                    mListener.onPointerButton(BTN_RIGHT, ACTION_DOWN);
                    mListener.onPointerButton(BTN_RIGHT, ACTION_UP);
                }

                if (mPreviousUpEvent != null) {
                    mPreviousUpEvent.recycle();
                }
                // Hold the event we obtained above - listeners may have changed the original.
                mPreviousUpEvent = currentUpEvent;
                mIsDoubleTapping = false;
                mInLongPress = false;
                mDeferConfirmSingleTap = false;
                mInDrag = false;
                mWasInDrag = false;
                mHandler.removeMessages(SHOW_PRESS);
                mHandler.removeMessages(LONG_PRESS);
                mHandler.removeMessages(RIGHT_TAP);
                break;

            case MotionEvent.ACTION_CANCEL:
                cancel();
                break;
        }

        return handled;
    }

    private void onScroll(MotionEvent ev, float scrollX, float scrollY) {
        if (ev.getPointerCount() == 1) {
            if (mMode == TOUCH_MODE_MOUSE) {
                if (!mWasInDrag) {
                    startDrag();
                    mListener.onPointerMotion((int) ev.getX(), (int) ev.getY());
                }
                return;
            }

            cursor.x -= (int) scrollX;
            cursor.y -= (int) scrollY;

            if (cursor.x < 0) cursor.x = 0;
            if (cursor.y < 0) cursor.y = 0;
            if (cursor.x > target.getWidth()) cursor.x = target.getWidth();
            if (cursor.y > target.getHeight()) cursor.y = target.getHeight();

            mListener.onPointerMotion(cursor.x, cursor.y);
        } else if (ev.getPointerCount() == 2) {
            if (scrollX != 0)
                mListener.onPointerScroll(MotionEvent.AXIS_Y, (int)scrollX);
            if (scrollY != 0)
                mListener.onPointerScroll(MotionEvent.AXIS_X, (int)scrollY);
        }
    }

    private void startDrag() {
        if (mWasInDrag) return;
        if (!mInDrag) {
            Log.e("drag", "start");
            mListener.onPointerButton(BTN_LEFT, ACTION_DOWN);
            mInDrag = true;
        }
    }

    private void stopDrag() {
        if (mInDrag) {
            Log.e("drag", "stop");
            mListener.onPointerButton(BTN_LEFT, ACTION_UP);
            mInDrag = false;
        }
        mWasInDrag = true;
    }

    private void cancel() {
        mHandler.removeMessages(SHOW_PRESS);
        mHandler.removeMessages(LONG_PRESS);
        mHandler.removeMessages(TAP);
        mHandler.removeMessages(RIGHT_TAP);
        mIsDoubleTapping = false;
        mStillDown = false;
        mAlwaysInTapRegion = false;
        mAlwaysInBiggerTapRegion = false;
        mDeferConfirmSingleTap = false;
        mInLongPress = false;
        mInContextClick = false;
    }

    private void cancelTaps() {
        mHandler.removeMessages(SHOW_PRESS);
        mHandler.removeMessages(LONG_PRESS);
        mHandler.removeMessages(TAP);
        mIsDoubleTapping = false;
        mAlwaysInTapRegion = false;
        mAlwaysInBiggerTapRegion = false;
        mDeferConfirmSingleTap = false;
        mInLongPress = false;
        mInContextClick = false;
    }

    private boolean isConsideredDoubleTap(MotionEvent firstDown, MotionEvent firstUp,
                                          MotionEvent secondDown) {
        if (!mAlwaysInBiggerTapRegion) {
            return false;
        }

        final long deltaTime = secondDown.getEventTime() - firstUp.getEventTime();
        if (deltaTime > DOUBLE_TAP_TIMEOUT || deltaTime < DOUBLE_TAP_MIN_TIME) {
            return false;
        }

        int deltaX = (int) firstDown.getX() - (int) secondDown.getX();
        int deltaY = (int) firstDown.getY() - (int) secondDown.getY();
        final boolean isGeneratedGesture =
                (firstDown.getFlags() & FLAG_IS_GENERATED_GESTURE) != 0;
        int slopSquare = isGeneratedGesture ? 0 : mDoubleTapSlopSquare;
        return (deltaX * deltaX + deltaY * deltaY < slopSquare);
    }

    private void dispatchLongPress() {
        mHandler.removeMessages(TAP);
        mDeferConfirmSingleTap = false;
        mInLongPress = true;
        Log.e("tp", "long press");
        mListener.onPointerButton(BTN_LEFT, ACTION_DOWN);
    }



    private class HardwareMouseListener {
        private int savedBS = 0;
        private int currentBS = 0;

        boolean isMouseButtonChanged(int mask) {
            return (savedBS & mask) != (currentBS & mask);
        }

        int mouseButtonState(int mask) {
            return ((currentBS & mask) != 0) ? ACTION_DOWN : ACTION_UP;
        }

        @SuppressLint("ClickableViewAccessibility")
        boolean onTouch(MotionEvent e) {
            if (e.getAction() == MotionEvent.ACTION_SCROLL) {
                float scrollY = -5 * e.getAxisValue(MotionEvent.AXIS_VSCROLL);
                float scrollX = -5 * e.getAxisValue(MotionEvent.AXIS_HSCROLL);

                if (scrollY != 0) mListener.onPointerScroll(AXIS_Y, scrollY);
                if (scrollX != 0) mListener.onPointerScroll(AXIS_X, scrollX);
                return true;
            }

            mListener.onPointerMotion((int) e.getX(), (int) e.getY());

            currentBS = e.getButtonState();
            if (isMouseButtonChanged(MotionEvent.BUTTON_PRIMARY)) {
                mListener.onPointerButton(BTN_LEFT, mouseButtonState(MotionEvent.BUTTON_PRIMARY));
            }
            if (isMouseButtonChanged(MotionEvent.BUTTON_TERTIARY)) {
                mListener.onPointerButton(BTN_MIDDLE, mouseButtonState(MotionEvent.BUTTON_TERTIARY));
            }
            if (isMouseButtonChanged(MotionEvent.BUTTON_SECONDARY)) {
                mListener.onPointerButton(BTN_RIGHT, mouseButtonState(MotionEvent.BUTTON_SECONDARY));
            }
            savedBS = currentBS;
            return true;
        }
    }
}
