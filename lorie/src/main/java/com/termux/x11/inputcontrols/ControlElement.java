package com.termux.x11.inputcontrols;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.view.MotionEvent;

import androidx.core.graphics.ColorUtils;

import com.termux.x11.widget.InputControlsView;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.Arrays;

public class ControlElement {
    public static final float STICK_DEAD_ZONE = 0.15f;
    public static final float DPAD_DEAD_ZONE = 0.3f;
    public static final float STICK_SENSITIVITY = 3.0f;
    public static final float TRACKPAD_MIN_SPEED = 0.8f;
    public static final float TRACKPAD_MAX_SPEED = 20.0f;
    public static final byte TRACKPAD_ACCELERATION_THRESHOLD = 4;
    public static final short BUTTON_MIN_TIME_TO_KEEP_PRESSED = 300;
    public static final int FLAG_SELECTED = 1<<0;
    public static final int FLAG_PRESSED = 1<<1;
    public static final int FLAG_VISIBLE = 1<<2;
    public static final int FLAG_TOGGLE_SWITCH = 1<<3;
    public static final int FLAG_BOUNDING_BOX_NEEDS_UPDATE = 1<<4;
    public static final int FLAG_MOUSE_MOVE_MODE = 1<<5;
    public enum Type {
        BUTTON, D_PAD, RANGE_BUTTON, STICK, TRACKPAD, MIDI_KEY, RADIAL_MENU;

        public static String[] names() {
            Type[] types = values();
            String[] names = new String[types.length];
            for (int i = 0; i < types.length; i++) names[i] = types[i].name().replace("_", "-");
            return names;
        }
    }
    public enum Shape {
        CIRCLE, RECT, ROUND_RECT, SQUARE;

        public static String[] names() {
            Shape[] shapes = values();
            String[] names = new String[shapes.length];
            for (int i = 0; i < shapes.length; i++) names[i] = shapes[i].name().replace("_", " ");
            return names;
        }
    }
    public enum Range {
        FROM_A_TO_Z(26), FROM_0_TO_9(10), FROM_F1_TO_F12(12), FROM_NP0_TO_NP9(10);
        public final byte max;

        Range(int max) {
            this.max = (byte)max;
        }

        public static String[] names() {
            Range[] ranges = values();
            String[] names = new String[ranges.length];
            for (int i = 0; i < ranges.length; i++) names[i] = ranges[i].name().replace("_", " ");
            return names;
        }
    }
    private final InputControlsView inputControlsView;
    private Type type = Type.BUTTON;
    private Shape shape = Shape.CIRCLE;
    private Binding[] bindings = {Binding.NONE, Binding.NONE, Binding.NONE, Binding.NONE};
    private float scale = 1.0f;
    private float opacity = 1.0f;
    private short x;
    private short y;
    private int currentPointerId = -1;
    private final Rect boundingBox = new Rect();
    private boolean[] states = new boolean[4];
    private final Bitmask propertyFlags = new Bitmask(new int[]{FLAG_BOUNDING_BOX_NEEDS_UPDATE});
    private String text = "";
    private byte iconId;
    private Range range;
    private byte orientation;
    private PointF currentPosition;
    private RangeScroller scroller;
    private CubicBezierInterpolator interpolator;
    private Object touchTime;
    private Path[] paths;

    public ControlElement(InputControlsView inputControlsView) {
        this.inputControlsView = inputControlsView;
    }

    private void reset() {
        bindings = new Binding[4];
        setBinding(Binding.NONE);
        scroller = null;
        text = "";

        switch (type) {
            case D_PAD:
            case STICK:
                bindings[0] = Binding.KEY_W;
                bindings[1] = Binding.KEY_D;
                bindings[2] = Binding.KEY_S;
                bindings[3] = Binding.KEY_A;
                break;
            case TRACKPAD:
                bindings[0] = Binding.MOUSE_MOVE_UP;
                bindings[1] = Binding.MOUSE_MOVE_RIGHT;
                bindings[2] = Binding.MOUSE_MOVE_DOWN;
                bindings[3] = Binding.MOUSE_MOVE_LEFT;
                break;
            case RANGE_BUTTON:
                scroller = new RangeScroller(inputControlsView, this);
                break;
            case MIDI_KEY:
                shape = Shape.SQUARE;
                text = "C1";
                break;
            case RADIAL_MENU:
                setBindingCount(3);
                break;
        }

        iconId = 0;
        range = null;
        propertyFlags.set(FLAG_BOUNDING_BOX_NEEDS_UPDATE);
    }

    public Type getType() {
        return type;
    }

    public void setType(Type type) {
        this.type = type;
        reset();
    }

    public byte getBindingCount() {
        return (byte)bindings.length;
    }

    public byte getFirstBindingIndex() {
        for (byte i = 0; i < bindings.length; i++) if (bindings[i] != Binding.NONE) return i;
        return 0;
    }

    public byte getLastBindingIndex() {
        byte last = 0;
        for (byte i = 0; i < bindings.length; i++) if (bindings[i] != Binding.NONE) last = i;
        return last;
    }

    public void setBindingCount(int bindingCount) {
        bindings = new Binding[bindingCount];
        setBinding(Binding.NONE);
        states = new boolean[bindingCount];
        propertyFlags.set(FLAG_BOUNDING_BOX_NEEDS_UPDATE);
    }

    public Shape getShape() {
        return shape;
    }

    public void setShape(Shape shape) {
        this.shape = shape;
        propertyFlags.set(FLAG_BOUNDING_BOX_NEEDS_UPDATE);
    }

    public Range getRange() {
        return range != null ? range : Range.FROM_A_TO_Z;
    }

    public void setRange(Range range) {
        this.range = range;
    }

    public byte getOrientation() {
        return orientation;
    }

    public void setOrientation(byte orientation) {
        this.orientation = orientation;
        propertyFlags.set(FLAG_BOUNDING_BOX_NEEDS_UPDATE);
    }

    public boolean isToggleSwitch() {
        return propertyFlags.isSet(FLAG_TOGGLE_SWITCH);
    }

    public void setToggleSwitch(boolean toggleSwitch) {
        propertyFlags.set(FLAG_TOGGLE_SWITCH, toggleSwitch);
    }

    public boolean isMouseMoveMode() {
        return propertyFlags.isSet(FLAG_MOUSE_MOVE_MODE);
    }

    public void setMouseMoveMode(boolean mouseMoveMode) {
        propertyFlags.set(FLAG_MOUSE_MOVE_MODE, mouseMoveMode);
    }

    public Binding getBindingAt(int index) {
        return index < bindings.length ? bindings[index] : Binding.NONE;
    }

    public void setBindingAt(int index, Binding binding) {
        if (index >= bindings.length) {
            int oldLength = bindings.length;
            bindings = Arrays.copyOf(bindings, index+1);
            Arrays.fill(bindings, oldLength, bindings.length, Binding.NONE);
            states = new boolean[bindings.length];
            propertyFlags.set(FLAG_BOUNDING_BOX_NEEDS_UPDATE);
        }
        bindings[index] = binding;
    }

    public void setBinding(Binding binding) {
        Arrays.fill(bindings, binding);
    }

    public float getScale() {
        return scale;
    }

    public void setScale(float scale) {
        this.scale = scale;
        propertyFlags.set(FLAG_BOUNDING_BOX_NEEDS_UPDATE);
    }

    public float getOpacity() {
        return opacity;
    }

    public void setOpacity(float opacity) {
        this.opacity = opacity;
    }

    public short getX() {
        return x;
    }

    public void setX(int x) {
        this.x = (short)x;
        propertyFlags.set(FLAG_BOUNDING_BOX_NEEDS_UPDATE);
    }

    public short getY() {
        return y;
    }

    public void setY(int y) {
        this.y = (short)y;
        propertyFlags.set(FLAG_BOUNDING_BOX_NEEDS_UPDATE);
    }

    public boolean isSelected() {
        return propertyFlags.isSet(FLAG_SELECTED);
    }

    public void setSelected(boolean selected) {
        if (type == Type.RADIAL_MENU) propertyFlags.set(FLAG_VISIBLE, selected);
        propertyFlags.set(FLAG_SELECTED, selected);
    }

    public String getText() {
        return text;
    }

    public void setText(String text) {
        this.text = text != null ? text : "";
    }

    public byte getIconId() {
        return iconId;
    }

    public void setIconId(int iconId) {
        this.iconId = (byte)iconId;
    }

    public Rect getBoundingBox() {
        if (propertyFlags.isSet(FLAG_BOUNDING_BOX_NEEDS_UPDATE)) computeBoundingBox();
        return boundingBox;
    }

    private Rect computeBoundingBox() {
        int snappingSize = inputControlsView.getSnappingSize();
        int halfWidth = 0;
        int halfHeight = 0;

        switch (type) {
            case BUTTON:
            case MIDI_KEY:
                switch (shape) {
                    case RECT:
                    case ROUND_RECT:
                        halfWidth = snappingSize * 4;
                        halfHeight = snappingSize * 2;
                        break;
                    case SQUARE:
                        halfWidth = (int)(snappingSize * 2.5f);
                        halfHeight = (int)(snappingSize * 2.5f);
                        break;
                    case CIRCLE:
                        halfWidth = snappingSize * 3;
                        halfHeight = snappingSize * 3;
                        break;
                }
                break;
            case D_PAD: {
                halfWidth = snappingSize * 7;
                halfHeight = snappingSize * 7;
                break;
            }
            case TRACKPAD:
            case STICK: {
                halfWidth = snappingSize * 6;
                halfHeight = snappingSize * 6;
                break;
            }
            case RANGE_BUTTON: {
                halfWidth = snappingSize * ((bindings.length * 4) / 2);
                halfHeight = snappingSize * 2;

                if (orientation == 1) {
                    int tmp = halfWidth;
                    halfWidth = halfHeight;
                    halfHeight = tmp;
                }
                break;
            }
            case RADIAL_MENU:
                halfWidth = snappingSize * 3;
                halfHeight = snappingSize * 3;
                break;
        }

        halfWidth *= scale;
        halfHeight *= scale;
        boundingBox.set(x - halfWidth, y - halfHeight, x + halfWidth, y + halfHeight);
        propertyFlags.unset(FLAG_BOUNDING_BOX_NEEDS_UPDATE);
        paths = null;
        return boundingBox;
    }

    private String getBindingTextAt(int index) {
        Binding binding = getBindingAt(index);
        String text = binding.toString().replace("NUMPAD ", "NP").replace("BUTTON ", "");
        if (text.length() > 7) {
            String[] parts = text.split(" ");
            StringBuilder sb = new StringBuilder();
            for (String part : parts) sb.append(part.charAt(0));
            return (binding.isMouse() ? "M" : "")+sb;
        }
        else return text;
    }

    private String getDisplayText() {
        if (text != null && !text.isEmpty()) {
            return text;
        }
        else {
            if (type == Type.BUTTON) {
                StringBuilder sb = new StringBuilder();
                for (byte i = 0; i < bindings.length; i++) {
                    if (bindings[i] != Binding.NONE) {
                        if (sb.length() > 0) sb.append("+");
                        sb.append(getBindingTextAt(i));
                    }
                }
                if (sb.length() > 0) return sb.toString();
            }

            return getBindingTextAt(0);
        }
    }

    private static float getTextSizeForWidth(Paint paint, String text, float desiredWidth) {
        final byte testTextSize = 48;
        paint.setTextSize(testTextSize);
        return testTextSize * desiredWidth / paint.measureText(text);
    }

    private static Binding getRangeBindingForIndex(Range range, int index) {
        switch (range) {
            case FROM_A_TO_Z:
                return Binding.valueOf("KEY_"+((char)(65 + index)));
            case FROM_0_TO_9:
                return Binding.valueOf("KEY_"+((index + 1) % 10));
            case FROM_F1_TO_F12:
                return Binding.valueOf("KEY_F"+(index + 1));
            case FROM_NP0_TO_NP9:
                return Binding.valueOf("KEY_KP_"+((index + 1) % 10));
            default:
                return Binding.NONE;
        }
    }

    private static String getRangeTextForIndex(Range range, int index) {
        switch (range) {
            case FROM_A_TO_Z:
                return String.valueOf((char)(65 + index));
            case FROM_0_TO_9:
                return String.valueOf((index + 1) % 10);
            case FROM_F1_TO_F12:
                return  "F"+(index + 1);
            case FROM_NP0_TO_NP9:
                return  "NP"+((index + 1) % 10);
            default:
                return "";
        }
    }

    public void draw(Canvas canvas) {
        int snappingSize = inputControlsView.getSnappingSize();
        Paint paint = inputControlsView.getPaint();
        int lightColor = getLightColor();

        paint.setColor(propertyFlags.isSet(FLAG_SELECTED) ? getHighlightColor() : lightColor);
        paint.setStyle(Paint.Style.STROKE);
        float strokeWidth = snappingSize * 0.25f;
        paint.setStrokeWidth(strokeWidth);
        Rect boundingBox = getBoundingBox();

        switch (type) {
            case BUTTON:
            case MIDI_KEY: {
                if (propertyFlags.isSet(FLAG_PRESSED)) paint.setStyle(Paint.Style.FILL);

                float cx = boundingBox.centerX();
                float cy = boundingBox.centerY();

                switch (shape) {
                    case CIRCLE:
                        canvas.drawCircle(cx, cy, boundingBox.width() * 0.5f, paint);
                        break;
                    case RECT:
                        canvas.drawRect(boundingBox, paint);
                        break;
                    case ROUND_RECT: {
                        float radius = boundingBox.height() * 0.5f;
                        canvas.drawRoundRect(boundingBox.left, boundingBox.top, boundingBox.right, boundingBox.bottom, radius, radius, paint);
                        break;
                    }
                    case SQUARE: {
                        float radius = snappingSize * 0.75f * scale;
                        canvas.drawRoundRect(boundingBox.left, boundingBox.top, boundingBox.right, boundingBox.bottom, radius, radius, paint);
                        break;
                    }
                }

                if (iconId > 0) {
                    drawIcon(canvas, cx, cy, boundingBox.width(), boundingBox.height(), iconId, true);
                }
                else {
                    String text = getDisplayText();
                    paint.setTextSize(Math.min(getTextSizeForWidth(paint, text, boundingBox.width() - strokeWidth * 2), snappingSize * 2 * scale));
                    paint.setTextAlign(Paint.Align.CENTER);
                    paint.setStyle(Paint.Style.FILL);
                    paint.setColor(propertyFlags.isSet(FLAG_PRESSED) ? getDarkColor() : lightColor);
                    canvas.drawText(text, x, (y - ((paint.descent() + paint.ascent()) * 0.5f)), paint);
                }
                break;
            }
            case D_PAD: {
                float cx = boundingBox.centerX();
                float cy = boundingBox.centerY();
                float offsetX = snappingSize * 2 * scale;
                float offsetY = snappingSize * 3 * scale;
                float start = snappingSize * scale;

                if (paths == null) {
                    Path path = new Path();
                    path.moveTo(cx, cy - start);
                    path.lineTo(cx - offsetX, cy - offsetY);
                    path.lineTo(cx - offsetX, boundingBox.top);
                    path.lineTo(cx + offsetX, boundingBox.top);
                    path.lineTo(cx + offsetX, cy - offsetY);
                    path.close();

                    path.moveTo(cx - start, cy);
                    path.lineTo(cx - offsetY, cy - offsetX);
                    path.lineTo(boundingBox.left, cy - offsetX);
                    path.lineTo(boundingBox.left, cy + offsetX);
                    path.lineTo(cx - offsetY, cy + offsetX);
                    path.close();

                    path.moveTo(cx, cy + start);
                    path.lineTo(cx - offsetX, cy + offsetY);
                    path.lineTo(cx - offsetX, boundingBox.bottom);
                    path.lineTo(cx + offsetX, boundingBox.bottom);
                    path.lineTo(cx + offsetX, cy + offsetY);
                    path.close();

                    path.moveTo(cx + start, cy);
                    path.lineTo(cx + offsetY, cy - offsetX);
                    path.lineTo(boundingBox.right, cy - offsetX);
                    path.lineTo(boundingBox.right, cy + offsetX);
                    path.lineTo(cx + offsetY, cy + offsetX);
                    path.close();
                    paths = new Path[]{path};
                }

                canvas.drawPath(paths[0], paint);
                break;
            }
            case RANGE_BUTTON: {
                Range range = getRange();
                int oldColor = paint.getColor();
                int darkColor = getDarkColor();

                float radius = snappingSize * 0.75f * scale;
                float elementSize = scroller.getElementSize();
                float minTextSize = snappingSize * 2 * scale;
                float scrollOffset = scroller.getScrollOffset();
                byte[] rangeIndex = scroller.getRangeIndex();
                Binding selectedBinding = scroller.getBinding();

                boolean wasPressed = false;

                if (orientation == 0) {
                    float lineTop = boundingBox.top + strokeWidth * 0.5f;
                    float lineBottom = boundingBox.bottom - strokeWidth * 0.5f;
                    float startX = boundingBox.left;
                    canvas.drawRoundRect(startX, boundingBox.top, boundingBox.right, boundingBox.bottom, radius, radius, paint);

                    if (paths == null) {
                        Path path = new Path();
                        path.addRoundRect(startX, boundingBox.top, boundingBox.right, boundingBox.bottom, radius, radius, Path.Direction.CW);
                        paths = new Path[]{path};
                    }

                    canvas.save();
                    canvas.clipPath(paths[0]);
                    startX -= scrollOffset % elementSize;

                    for (byte i = rangeIndex[0]; i < rangeIndex[1]; i++) {
                        int index = i % range.max;
                        paint.setStyle(Paint.Style.STROKE);
                        paint.setColor(oldColor);
                        boolean pressed = propertyFlags.isSet(FLAG_PRESSED) && selectedBinding == getRangeBindingForIndex(range, index);

                        if (startX > boundingBox.left && startX  < boundingBox.right && !pressed && !wasPressed) {
                            canvas.drawLine(startX, lineTop, startX, lineBottom, paint);
                        }
                        String text = getRangeTextForIndex(range, index);

                        if (startX < boundingBox.right && startX + elementSize > boundingBox.left) {
                            paint.setStyle(Paint.Style.FILL);

                            if (pressed) {
                                paint.setColor(lightColor);
                                canvas.drawRect(startX, lineTop, startX + elementSize, lineBottom, paint);
                            }

                            paint.setColor(pressed ? darkColor : lightColor);
                            paint.setTextSize(Math.min(getTextSizeForWidth(paint, text, elementSize - strokeWidth * 2), minTextSize));
                            paint.setTextAlign(Paint.Align.CENTER);
                            canvas.drawText(text, startX + elementSize * 0.5f, (y - ((paint.descent() + paint.ascent()) * 0.5f)), paint);
                        }

                        startX += elementSize;
                        wasPressed = pressed;
                    }

                    paint.setStyle(Paint.Style.STROKE);
                    paint.setColor(oldColor);
                    canvas.restore();
                }
                else {
                    float lineLeft = boundingBox.left + strokeWidth * 0.5f;
                    float lineRight = boundingBox.right - strokeWidth * 0.5f;
                    float startY = boundingBox.top;
                    canvas.drawRoundRect(boundingBox.left, startY, boundingBox.right, boundingBox.bottom, radius, radius, paint);

                    if (paths == null) {
                        Path path = new Path();
                        path.addRoundRect(boundingBox.left, startY, boundingBox.right, boundingBox.bottom, radius, radius, Path.Direction.CW);
                        paths = new Path[]{path};
                    }

                    canvas.save();
                    canvas.clipPath(paths[0]);
                    startY -= scrollOffset % elementSize;

                    for (byte i = rangeIndex[0]; i < rangeIndex[1]; i++) {
                        int index = i % range.max;
                        paint.setStyle(Paint.Style.STROKE);
                        paint.setColor(oldColor);
                        boolean pressed = propertyFlags.isSet(FLAG_PRESSED) && selectedBinding == getRangeBindingForIndex(range, index);

                        if (startY > boundingBox.top && startY < boundingBox.bottom && !pressed && !wasPressed) {
                            canvas.drawLine(lineLeft, startY, lineRight, startY, paint);
                        }
                        String text = getRangeTextForIndex(range, index);

                        if (startY < boundingBox.bottom && startY + elementSize > boundingBox.top) {
                            paint.setStyle(Paint.Style.FILL);

                            if (pressed) {
                                paint.setColor(lightColor);
                                canvas.drawRect(lineLeft, startY, lineRight, startY + elementSize, paint);
                            }

                            paint.setColor(pressed ? darkColor : lightColor);
                            paint.setTextSize(Math.min(getTextSizeForWidth(paint, text, boundingBox.width() - strokeWidth * 2), minTextSize));
                            paint.setTextAlign(Paint.Align.CENTER);
                            canvas.drawText(text, x, startY + elementSize * 0.5f - ((paint.descent() + paint.ascent()) * 0.5f), paint);
                        }

                        startY += elementSize;
                        wasPressed = pressed;
                    }

                    paint.setStyle(Paint.Style.STROKE);
                    paint.setColor(oldColor);
                    canvas.restore();
                }
                break;
            }
            case STICK: {
                int cx = boundingBox.centerX();
                int cy = boundingBox.centerY();
                int oldColor = paint.getColor();
                canvas.drawCircle(cx, cy, boundingBox.height() * 0.5f, paint);

                float thumbstickX = currentPosition != null ? currentPosition.x : cx;
                float thumbstickY = currentPosition != null ? currentPosition.y : cy;

                short thumbRadius = (short) (snappingSize * 3.5f * scale);
                paint.setStyle(Paint.Style.FILL);
                paint.setColor(ColorUtils.setAlphaComponent(lightColor, 50));
                canvas.drawCircle(thumbstickX, thumbstickY, thumbRadius, paint);

                paint.setStyle(Paint.Style.STROKE);
                paint.setColor(oldColor);
                canvas.drawCircle(thumbstickX, thumbstickY, thumbRadius + strokeWidth * 0.5f, paint);
                break;
            }
            case TRACKPAD: {
                float radius = boundingBox.height() * 0.15f;
                canvas.drawRoundRect(boundingBox.left, boundingBox.top, boundingBox.right, boundingBox.bottom, radius, radius, paint);
                float offset = strokeWidth * 2.5f;
                float innerStrokeWidth = strokeWidth * 2;
                float innerHeight = boundingBox.height() - offset * 2;
                radius = (innerHeight / boundingBox.height()) * radius - (innerStrokeWidth * 0.5f + strokeWidth * 0.5f);
                paint.setStrokeWidth(innerStrokeWidth);
                canvas.drawRoundRect(boundingBox.left + offset, boundingBox.top + offset, boundingBox.right - offset, boundingBox.bottom - offset, radius, radius, paint);
                break;
            }
            case RADIAL_MENU: {
                float startAngle = 0;
                float cx = boundingBox.centerX();
                float cy = boundingBox.centerY();
                float innerRadius = boundingBox.width() * 0.5f + snappingSize * 0.5f;
                float outerRadius = boundingBox.width() + snappingSize * scale;
                float radius = boundingBox.width() * 0.5f;
                int oldColor = paint.getColor();

                if (paths == null) {
                    Path path0 = new Path();
                    Path path1 = new Path();
                    RectF outerOval = new RectF(cx - outerRadius, cy - outerRadius, cx + outerRadius, cy + outerRadius);
                    RectF innerOval = new RectF(cx - innerRadius, cy - innerRadius, cx + innerRadius, cy + innerRadius);
                    float outerMargin = (float)Math.toRadians(2);
                    float innerMargin = outerMargin * (outerRadius / innerRadius);

                    for (int i = 0; i <= bindings.length; i++) {
                        float t = (float)i / bindings.length;
                        float endAngle = (float)(t * Math.PI * 2 + Math.PI * 1.5f);

                        if (i > 0) {
                            path0.moveTo((float)(cx + Math.cos(startAngle + innerMargin) * innerRadius), (float)(cy + Math.sin(startAngle + innerMargin) * innerRadius));
                            path0.lineTo((float)(cx + Math.cos(startAngle + outerMargin) * outerRadius), (float)(cy + Math.sin(startAngle + outerMargin) * outerRadius));
                            path0.arcTo(outerOval, (float)Math.toDegrees(startAngle + outerMargin), (float)Math.toDegrees(endAngle - startAngle - outerMargin * 2));
                            path0.lineTo((float)(cx + Math.cos(endAngle - innerMargin) * innerRadius), (float)(cy + Math.sin(endAngle - innerMargin) * innerRadius));
                            path0.arcTo(innerOval, (float)Math.toDegrees(endAngle - innerMargin), (float)-Math.toDegrees(endAngle - startAngle - innerMargin * 2));

                            float middleAngle = (startAngle + endAngle) * 0.5f;
                            float endX = (float)(cx + Math.cos(middleAngle) * radius * 0.5f);
                            float endY = (float)(cy + Math.sin(middleAngle) * radius * 0.5f);
                            path1.moveTo(cx, cy);
                            path1.lineTo(endX, endY);
                            path1.addCircle(endX, endY, snappingSize * 0.4f, Path.Direction.CW);
                        }

                        startAngle = endAngle;
                    }

                    paths = new Path[]{path0, path1};
                }

                if (propertyFlags.isSet(FLAG_VISIBLE)) {
                    float minTextSize = snappingSize * 2 * scale;
                    int darkColor = getDarkColor();
                    paint.setStrokeCap(Paint.Cap.SQUARE);
                    canvas.drawPath(paths[0], paint);
                    paint.setStrokeCap(Paint.Cap.BUTT);

                    paint.setStyle(Paint.Style.FILL);
                    paint.setTextAlign(Paint.Align.CENTER);
                    float touchAreaRadius = (outerRadius - innerRadius) * 0.5f * 1.25f;

                    for (int i = 0, j = 0; i <= bindings.length; i++) {
                        float t = (float)i / bindings.length;
                        float endAngle = (float)(t * Math.PI * 2 + Math.PI * 1.5f);

                        if (i > 0) {
                            float middleAngle = (startAngle + endAngle) * 0.5f;
                            float touchAreaCenter = (innerRadius + outerRadius) * 0.5f;
                            float touchAreaX = (short)(cx + Math.cos(middleAngle) * touchAreaCenter);
                            float touchAreaY = (short)(cy + Math.sin(middleAngle) * touchAreaCenter);

                            canvas.save();
                            canvas.translate(touchAreaX, touchAreaY);
                            float textAngle = (float)Math.toDegrees(middleAngle + Math.PI * 0.5f) % 360;
                            if (textAngle > 90 && textAngle <= 270) textAngle += 180;
                            canvas.rotate(textAngle);
                            String text = getBindingTextAt(j++);
                            paint.setTextSize(Math.min(getTextSizeForWidth(paint, text, touchAreaRadius * 2), minTextSize));
                            paint.setColor(propertyFlags.isSet(FLAG_PRESSED) ? darkColor : lightColor);
                            canvas.drawText(text, 0, -((paint.descent() + paint.ascent()) * 0.5f), paint);
                            canvas.restore();
                        }

                        startAngle = endAngle;
                    }

                    drawIcon(canvas, cx, cy, boundingBox.width() * 0.4f, boundingBox.width() * 0.4f, 17, false);
                }
                else {
                    paint.setStyle(Paint.Style.FILL_AND_STROKE);
                    paint.setColor(oldColor);
                    canvas.drawPath(paths[1], paint);
                }

                paint.setStyle(Paint.Style.STROKE);
                paint.setColor(oldColor);
                canvas.drawCircle(cx, cy, radius, paint);
                break;
            }
        }
    }

    private void drawIcon(Canvas canvas, float cx, float cy, float width, float height, int iconId, boolean automargin) {
        Paint paint = inputControlsView.getPaint();
        Bitmap icon = inputControlsView.getIcon((byte)iconId);
        paint.setColorFilter(propertyFlags.isSet(FLAG_PRESSED) ? inputControlsView.getDarkColorFilter() : inputControlsView.getLightColorFilter());
        float snappingSize = inputControlsView.getSnappingSize();
        int margin = automargin ? (int)(snappingSize * (shape == Shape.CIRCLE || shape == Shape.SQUARE ? 2.0f : 1.0f) * scale) : 0;
        int halfSize = (int)((Math.min(width, height) - margin) * 0.5f);

        Rect srcRect = new Rect(0, 0, icon.getWidth(), icon.getHeight());
        Rect dstRect = new Rect((int)(cx - halfSize), (int)(cy - halfSize), (int)(cx + halfSize), (int)(cy + halfSize));
        canvas.drawBitmap(icon, srcRect, dstRect, paint);
        paint.setColorFilter(null);
    }

    public JSONObject toJSONObject() {
        try {
            JSONObject elementJSONObject = new JSONObject();
            elementJSONObject.put("type", type.name());
            elementJSONObject.put("shape", shape.name());

            JSONArray bindingsJSONArray = new JSONArray();
            for (Binding binding : bindings) bindingsJSONArray.put(binding.name());

            elementJSONObject.put("bindings", bindingsJSONArray);
            elementJSONObject.put("scale", Float.valueOf(scale));
            if (opacity < 1.0f) elementJSONObject.put("opacity", Float.valueOf(opacity));
            elementJSONObject.put("x", (float)x / inputControlsView.getMaxWidth());
            elementJSONObject.put("y", (float)y / inputControlsView.getMaxHeight());
            elementJSONObject.put("toggleSwitch", propertyFlags.isSet(FLAG_TOGGLE_SWITCH));
            elementJSONObject.put("text", text);
            elementJSONObject.put("iconId", iconId);

            if (type == Type.RANGE_BUTTON && range != null) {
                elementJSONObject.put("range", range.name());
                if (orientation != 0) elementJSONObject.put("orientation", orientation);
            }
            if (propertyFlags.isSet(FLAG_MOUSE_MOVE_MODE)) elementJSONObject.put("mouseMoveMode", true);

            return elementJSONObject;
        }
        catch (JSONException e) {
            return null;
        }
    }

    public boolean containsPoint(float x, float y) {
        if (type == Type.RADIAL_MENU && propertyFlags.isSet(FLAG_VISIBLE)) {
            float outerRadius = boundingBox.width() + inputControlsView.getSnappingSize() * scale;
            return Mathf.distance(boundingBox.centerX(), boundingBox.centerY(), x, y) < outerRadius;
        }
        else return getBoundingBox().contains((int)(x + 0.5f), (int)(y + 0.5f));
    }

    private boolean isKeepButtonPressedAfterMinTime() {
        Binding binding = getBindingAt(0);
        return !propertyFlags.isSet(FLAG_TOGGLE_SWITCH) && (binding == Binding.GAMEPAD_BUTTON_L3 || binding == Binding.GAMEPAD_BUTTON_R3);
    }

    public boolean handleTouchDown(int pointerId, float x, float y) {
        if (currentPointerId == -1 && containsPoint(x, y)) {
            currentPointerId = pointerId;
            if (type == Type.BUTTON) {
                if (isKeepButtonPressedAfterMinTime()) touchTime = System.currentTimeMillis();
                if (!propertyFlags.isSet(FLAG_TOGGLE_SWITCH) || !propertyFlags.isSet(FLAG_SELECTED)) inputControlsView.handleInputEvent(bindings, true);
                if (propertyFlags.isSet(FLAG_MOUSE_MOVE_MODE)) inputControlsView.mouseMove(x, y, MotionEvent.ACTION_DOWN);

                propertyFlags.set(FLAG_PRESSED);
                inputControlsView.invalidate();
                return true;
            }
            else if (type == Type.RANGE_BUTTON) {
                scroller.handleTouchDown(x, y);
                propertyFlags.set(FLAG_PRESSED);
                inputControlsView.invalidate();
                return true;
            }
            else if (type == Type.MIDI_KEY) {
                // No MIDI synth target exists in termux-x11 yet; track pressed state only.
                propertyFlags.set(FLAG_PRESSED);
                inputControlsView.invalidate();
                return true;
            }
            else if (type == Type.RADIAL_MENU) {
                if (propertyFlags.isSet(FLAG_VISIBLE)) {
                    if (Mathf.distance(boundingBox.centerX(), boundingBox.centerY(), x, y) < (boundingBox.width() * 0.5f)) {
                        propertyFlags.unset(FLAG_VISIBLE);
                    }
                }
                else propertyFlags.set(FLAG_VISIBLE);
                inputControlsView.invalidate();
                return true;
            }
            else {
                if (type == Type.TRACKPAD) {
                    if (currentPosition == null) currentPosition = new PointF();
                    currentPosition.set(x, y);
                }
                return handleTouchMove(pointerId, x, y);
            }
        }
        else return false;
    }

    public boolean handleTouchMove(int pointerId, float x, float y) {
        if (pointerId == currentPointerId && (type == Type.D_PAD || type == Type.STICK || type == Type.TRACKPAD)) {
            float deltaX, deltaY;
            Rect boundingBox = getBoundingBox();
            float radius = boundingBox.width() * 0.5f;

            if (type == Type.TRACKPAD) {
                if (currentPosition == null) currentPosition = new PointF();
                float[] deltaPoint = inputControlsView.computeDeltaPoint(currentPosition.x, currentPosition.y, x, y);
                deltaX = deltaPoint[0];
                deltaY = deltaPoint[1];
                currentPosition.set(x, y);
            }
            else {
                float localX = x - boundingBox.left;
                float localY = y - boundingBox.top;
                float offsetX = localX - radius;
                float offsetY = localY - radius;

                float distance = Mathf.lengthSq(radius - localX, radius - localY);
                if (distance > radius * radius) {
                    float angle = (float)Math.atan2(offsetY, offsetX);
                    offsetX = (float)(Math.cos(angle) * radius);
                    offsetY = (float)(Math.sin(angle) * radius);
                }

                deltaX = Mathf.clamp(offsetX / radius, -1, 1);
                deltaY = Mathf.clamp(offsetY / radius, -1, 1);
            }

            if (type == Type.STICK) {
                if (currentPosition == null) currentPosition = new PointF();
                currentPosition.x = boundingBox.left + deltaX * radius + radius;
                currentPosition.y = boundingBox.top + deltaY * radius + radius;
                final boolean[] states = {deltaY <= -STICK_DEAD_ZONE, deltaX >= STICK_DEAD_ZONE, deltaY >= STICK_DEAD_ZONE, deltaX <= -STICK_DEAD_ZONE};

                for (byte i = 0; i < 4; i++) {
                    float value = i == 1 || i == 3 ? deltaX : deltaY;
                    Binding binding = getBindingAt(i);
                    if (binding.isGamepad()) {
                        value = Mathf.clamp(Math.max(0, Math.abs(value) - 0.01f) * Mathf.sign(value) * STICK_SENSITIVITY, -1, 1);
                        inputControlsView.handleInputEvent(binding, true, value);
                        this.states[i] = true;
                    }
                    else {
                        boolean state = binding.isMouseMove() ? (states[i] || states[(i+2)%4]) : states[i];
                        inputControlsView.handleInputEvent(binding, state, value);
                        this.states[i] = state;
                    }
                }

                inputControlsView.invalidate();
            }
            else if (type == Type.TRACKPAD) {
                final boolean[] states = {deltaY <= -TRACKPAD_MIN_SPEED, deltaX >= TRACKPAD_MIN_SPEED, deltaY >= TRACKPAD_MIN_SPEED, deltaX <= -TRACKPAD_MIN_SPEED};
                int cursorDx = 0;
                int cursorDy = 0;

                for (byte i = 0; i < 4; i++) {
                    float value = (i == 1 || i == 3 ? deltaX : deltaY);
                    Binding binding = getBindingAt(i);
                    if (binding.isGamepad()) {
                        if (interpolator == null) interpolator = new CubicBezierInterpolator();
                        if (Math.abs(value) > TRACKPAD_ACCELERATION_THRESHOLD) value *= STICK_SENSITIVITY;
                        interpolator.set(0.075f, 0.95f, 0.45f, 0.95f);
                        float interpolatedValue = interpolator.getInterpolation(Math.min(1.0f, Math.abs(value / TRACKPAD_MAX_SPEED)));
                        inputControlsView.handleInputEvent(binding, true, Mathf.clamp(interpolatedValue * Mathf.sign(value), -1, 1));
                        this.states[i] = true;
                    }
                    else {
                        if (Math.abs(value) > InputControlsView.CURSOR_ACCELERATION_THRESHOLD) value *= InputControlsView.CURSOR_ACCELERATION;
                        if (binding == Binding.MOUSE_MOVE_LEFT || binding == Binding.MOUSE_MOVE_RIGHT) {
                            cursorDx = Mathf.roundPoint(value);
                        }
                        else if (binding == Binding.MOUSE_MOVE_UP || binding == Binding.MOUSE_MOVE_DOWN) {
                            cursorDy = Mathf.roundPoint(value);
                        }
                        else {
                            inputControlsView.handleInputEvent(binding, states[i], value);
                            this.states[i] = states[i];
                        }
                    }
                }

                if (cursorDx != 0 || cursorDy != 0) inputControlsView.injectPointerMoveDelta(cursorDx, cursorDy);
            }
            else {
                final boolean[] states = {deltaY <= -DPAD_DEAD_ZONE, deltaX >= DPAD_DEAD_ZONE, deltaY >= DPAD_DEAD_ZONE, deltaX <= -DPAD_DEAD_ZONE};

                for (byte i = 0; i < 4; i++) {
                    float value = i == 1 || i == 3 ? deltaX : deltaY;
                    Binding binding = getBindingAt(i);
                    boolean state = binding.isMouseMove() ? (states[i] || states[(i+2)%4]) : states[i];
                    inputControlsView.handleInputEvent(binding, state, value);
                    this.states[i] = state;
                }
            }

            return true;
        }
        else if (pointerId == currentPointerId && type == Type.RANGE_BUTTON) {
            scroller.handleTouchMove(x, y);
            if (scroller.isScrolling()) {
                propertyFlags.unset(FLAG_PRESSED);
                inputControlsView.invalidate();
            }
            return true;
        }
        else if (pointerId == currentPointerId && type == Type.BUTTON && propertyFlags.isSet(FLAG_MOUSE_MOVE_MODE)) {
            inputControlsView.mouseMove(x, y, MotionEvent.ACTION_MOVE);
            return true;
        }
        else return false;
    }

    public boolean handleTouchUp(int pointerId, float x, float y) {
        if (pointerId == currentPointerId) {
            if (type == Type.BUTTON) {
                boolean selected = propertyFlags.isSet(FLAG_SELECTED);
                if (isKeepButtonPressedAfterMinTime() && touchTime != null) {
                    selected = (System.currentTimeMillis() - (long)touchTime) > BUTTON_MIN_TIME_TO_KEEP_PRESSED;
                    if (!selected) inputControlsView.handleInputEvent(bindings, false);
                    propertyFlags.set(FLAG_SELECTED, selected);
                    touchTime = null;
                }
                else if (!propertyFlags.isSet(FLAG_TOGGLE_SWITCH) || propertyFlags.isSet(FLAG_SELECTED)) {
                    inputControlsView.handleInputEvent(bindings, false);
                }

                if (propertyFlags.isSet(FLAG_TOGGLE_SWITCH)) propertyFlags.set(FLAG_SELECTED, !selected);
                if (propertyFlags.isSet(FLAG_MOUSE_MOVE_MODE)) inputControlsView.mouseMove(0, 0, MotionEvent.ACTION_UP);

                propertyFlags.unset(FLAG_PRESSED);
                inputControlsView.invalidate();
            }
            else if (type == Type.MIDI_KEY) {
                // No MIDI synth target exists in termux-x11 yet; track pressed state only.
                propertyFlags.unset(FLAG_PRESSED);
                inputControlsView.invalidate();
            }
            else if (type == Type.RADIAL_MENU) {
                if (propertyFlags.isSet(FLAG_VISIBLE)) handleRadialMenuClick(x, y);
                inputControlsView.invalidate();
            }
            else if (type == Type.RANGE_BUTTON || type == Type.D_PAD || type == Type.STICK || type == Type.TRACKPAD) {
                for (byte i = 0; i < states.length; i++) {
                    if (states[i]) inputControlsView.handleInputEvent(getBindingAt(i), false);
                    states[i] = false;
                }

                if (type == Type.RANGE_BUTTON) {
                    scroller.handleTouchUp();
                    propertyFlags.unset(FLAG_PRESSED);
                    inputControlsView.invalidate();
                }
                else if (type == Type.STICK) {
                    inputControlsView.invalidate();
                }

                if (currentPosition != null) currentPosition = null;
            }
            currentPointerId = -1;
            return true;
        }
        return false;
    }

    private void handleRadialMenuClick(float x, float y) {
        int snappingSize = inputControlsView.getSnappingSize();
        float cx = boundingBox.centerX();
        float cy = boundingBox.centerY();
        float innerRadius = boundingBox.width() * 0.5f + snappingSize * 0.5f;
        float outerRadius = boundingBox.width() + snappingSize * scale;
        float startAngle = 0;
        Binding clickedBinding = Binding.NONE;

        for (int i = 0, j = 0; i <= bindings.length; i++) {
            float t = (float)i / bindings.length;
            float endAngle = (float)(t * Math.PI * 2 + Math.PI * 1.5f);

            if (i > 0) {
                float middleAngle = (startAngle + endAngle) * 0.5f;
                float touchAreaCenter = (innerRadius + outerRadius) * 0.5f;
                float touchAreaX = (short)(cx + Math.cos(middleAngle) * touchAreaCenter);
                float touchAreaY = (short)(cy + Math.sin(middleAngle) * touchAreaCenter);

                float lineAx = (float)(cx + Math.cos(startAngle) * touchAreaCenter);
                float lineAy = (float)(cy + Math.sin(startAngle) * touchAreaCenter);
                float lineBx = (float)(cx + Math.cos(endAngle) * touchAreaCenter);
                float lineBy = (float)(cy + Math.sin(endAngle) * touchAreaCenter);

                if (Mathf.distance(touchAreaX, touchAreaY, x, y) <= Mathf.distance(lineAx, lineAy, lineBx, lineBy) * 0.5f &&
                    Mathf.distance(cx, cy, x, y) > innerRadius &&
                    Mathf.distance(cx, cy, x, y) <= outerRadius) {
                    clickedBinding = bindings[j];
                    break;
                }
                j++;
            }

            startAngle = endAngle;
        }

        if (clickedBinding != Binding.NONE) {
            propertyFlags.unset(FLAG_VISIBLE);
            final Binding finalBinding = clickedBinding;
            inputControlsView.handleInputEvent(finalBinding, true);
            inputControlsView.postDelayed(() -> inputControlsView.handleInputEvent(finalBinding, false), 30);
        }
    }

    public int getLightColor() {
        float opacity = inputControlsView.isEditMode() ? Math.max(0.15f, this.opacity) : this.opacity;
        return Color.argb((int)(opacity * inputControlsView.getOverlayOpacity() * 255), 255, 255, 255);
    }

    public int getDarkColor() {
        float opacity = inputControlsView.isEditMode() ? Math.max(0.15f, this.opacity) : this.opacity;
        return Color.argb((int)(opacity * inputControlsView.getOverlayOpacity() * 255), 0, 0, 0);
    }

    public int getHighlightColor() {
        return Color.argb((int)(inputControlsView.getOverlayOpacity() * 255), 2, 119, 189);
    }
}
