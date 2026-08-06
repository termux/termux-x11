package com.termux.x11.extrakeys;

import static com.termux.x11.MainActivity.*;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.DrawableWrapper;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.util.AttributeSet;
import android.util.TypedValue;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.ScheduledExecutorService;

import java.util.Map;
import java.util.stream.Collectors;

import android.view.Gravity;
import android.view.HapticFeedbackConstants;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.GridLayout;
import android.widget.PopupWindow;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.x11.MainActivity;
import com.termux.x11.R;
import com.termux.x11.utils.TermuxX11ExtraKeys;

/**
 * A {@link View} showing extra keys (such as Escape, Ctrl, Alt) not normally available on an Android soft
 * keyboards.
 * <p>
 * To use it, add following to a layout file and import it in your activity layout file or inflate
 * it with a {@link androidx.viewpager.widget.ViewPager}.:
 * {@code
 * <?xml version="1.0" encoding="utf-8"?>
 * <com.termux.x11.extrakeys.ExtraKeysView xmlns:android="http://schemas.android.com/apk/res/android"
 *     android:id="@+id/extra_keys"
 *     style="?android:attr/buttonBarStyle"
 *     android:layout_width="match_parent"
 *     android:layout_height="match_parent"
 *     android:layout_alignParentBottom="true"
 *     android:orientation="horizontal" />
 * }
 *
 * Then in your activity, get its reference by a call to {@link android.app.Activity#findViewById(int)}
 * or {@link LayoutInflater#inflate(int, ViewGroup)} if using {@link androidx.viewpager.widget.ViewPager}.
 * Then call {@link #setExtraKeysViewClient(IExtraKeysView)} and pass it the implementation of
 * {@link IExtraKeysView} so that you can receive callbacks. You can also override other values set
 * in {@link ExtraKeysView#ExtraKeysView(Context, AttributeSet)} by calling the respective functions.
 * If you extend {@link ExtraKeysView}, you can also set them in the constructor, but do call super().
 * <p>
 * After this you will have to make a call to {@link ExtraKeysView#reload() and pass
 * it the {@link ExtraKeysInfo} to load and display the extra keys. Read its class javadocs for more
 * info on how to create it.
 * <p>
 * Termux app defines the view in res/layout/view_terminal_toolbar_extra_keys and
 * inflates it in TerminalToolbarViewPager.instantiateItem() and sets the {@link ExtraKeysView} client
 * and calls {@link ExtraKeysView#reload(ExtraKeysInfo).
 * The {@link ExtraKeysInfo} is created by TermuxAppSharedProperties.setExtraKeys().
 * Then its got and the view height is adjusted in TermuxActivity.setTerminalToolbarHeight().
 * The client used is TermuxTerminalExtraKeys, which extends
 * {@link com.termux.x11.utils.TermuxX11ExtraKeys } to handle Termux app specific logic and
 * leave the rest to the super class.
 */
public final class ExtraKeysView extends GridLayout {
    /** The client for the {@link ExtraKeysView}. */
    public interface IExtraKeysView {
        /**
         * This is called by {@link ExtraKeysView} when a button is clicked. This is also called
         * for {@link #mRepetitiveKeys} and {@link ExtraKeyButton} that have a popup set.
         * However, this is not called for {@link #mSpecialButtons}, whose state can instead be read
         * via a call to {@link #readSpecialButton(SpecialButton, boolean)}.
         *
         * @param view The view that was clicked.
         * @param buttonInfo The {@link ExtraKeyButton} for the button that was clicked.
         *                   The button may be a {@link ExtraKeyButton#KEY_MACRO} set which can be
         *                   checked with a call to {@link ExtraKeyButton#macro}.
         * @param button The {@link Button} that was clicked.
         */
        void onExtraKeyButtonClick(View view, ExtraKeyButton buttonInfo, Button button);

        /**
         * This is called by {@link ExtraKeysView} when a button is clicked so that the client
         * can perform any hepatic feedback. This is only called in the {@link Button.OnClickListener}
         * and not for every repeat. Its also called for {@link #mSpecialButtons}.
         *
         * @param view The view that was clicked.
         * @param buttonInfo The {@link ExtraKeyButton} for the button that was clicked.
         * @param button The {@link Button} that was clicked.
         * @return Return {@code true} if the client handled the feedback, otherwise {@code false}
         * so that {@link ExtraKeysView#performExtraKeyButtonHapticFeedback(View, ExtraKeyButton, Button)}
         * can handle it depending on system settings.
         */
        boolean performExtraKeyButtonHapticFeedback(View view, ExtraKeyButton buttonInfo, Button button);
    }

    /** Defines the default value for {@link #mButtonTextColor} */
    public static final int DEFAULT_BUTTON_TEXT_COLOR = 0xFFFFFFFF;
    /** Defines the default value for {@link #mButtonActiveTextColor} */
    public static final int DEFAULT_BUTTON_ACTIVE_TEXT_COLOR = 0xFF80DEEA;
    /** Defines the default value for {@link #mButtonBackgroundColor} */
    public static final int DEFAULT_BUTTON_BACKGROUND_COLOR = 0x00000000;
    /** Defines the default value for {@link #mButtonActiveBackgroundColor} */
    public static final int DEFAULT_BUTTON_ACTIVE_BACKGROUND_COLOR = 0xFF7F7F7F;

    /** Defines the minimum allowed duration in milliseconds for {@link #mLongPressTimeout}. */
    public static final int MIN_LONG_PRESS_DURATION = 200;
    /** Defines the maximum allowed duration in milliseconds for {@link #mLongPressTimeout}. */
    public static final int MAX_LONG_PRESS_DURATION = 3000;
    /** Defines the fallback duration in milliseconds for {@link #mLongPressTimeout}. */
    public static final int FALLBACK_LONG_PRESS_DURATION = 400;

    /** Defines the minimum allowed duration in milliseconds for {@link #mLongPressRepeatDelay}. */
    public static final int MIN_LONG_PRESS__REPEAT_DELAY = 5;
    /** Defines the maximum allowed duration in milliseconds for {@link #mLongPressRepeatDelay}. */
    public static final int MAX_LONG_PRESS__REPEAT_DELAY = 2000;
    /** Defines the default duration in milliseconds for {@link #mLongPressRepeatDelay}. */
    public static final int DEFAULT_LONG_PRESS_REPEAT_DELAY = 80;



    /** The implementation of the {@link IExtraKeysView} that acts as a client for the {@link ExtraKeysView}. */
    private IExtraKeysView mExtraKeysViewClient;

    /** The map for the {@link SpecialButton} and their {@link SpecialButtonState}. Defaults to
     * the one returned by {@link #getDefaultSpecialButtons(ExtraKeysView)}. */
    private Map<SpecialButton, SpecialButtonState> mSpecialButtons;

    /** The keys for the {@link SpecialButton} added to {@link #mSpecialButtons}. This is automatically
     * set when the call to {@link #setSpecialButtons(Map)} is made. */
    private Set<String> mSpecialButtonsKeys;


    /**
     * The list of keys for which auto repeat of key should be triggered if its extra keys button
     * is long pressed. This is done by calling {@link IExtraKeysView#onExtraKeyButtonClick(View, ExtraKeyButton, Button)}
     * every {@link #mLongPressRepeatDelay} seconds after {@link #mLongPressTimeout} has passed.
     * The default keys are defined by {@link ExtraKeysConstants#PRIMARY_REPETITIVE_KEYS}.
     */
    private List<String> mRepetitiveKeys;


    /** The text color for the extra keys button. Defaults to {@link #DEFAULT_BUTTON_TEXT_COLOR}. */
    private int mButtonTextColor;
    /** The text color for the extra keys button when its active.
     * Defaults to {@link #DEFAULT_BUTTON_ACTIVE_TEXT_COLOR}. */
    private int mButtonActiveTextColor;
    /** The background color for the extra keys button. Defaults to {@link #DEFAULT_BUTTON_BACKGROUND_COLOR}. */
    private int mButtonBackgroundColor;
    /** The background color for the extra keys button when its active. Defaults to
     * {@link #DEFAULT_BUTTON_ACTIVE_BACKGROUND_COLOR}. */
    private int mButtonActiveBackgroundColor;


    /**
     * Defines the duration in milliseconds before a press turns into a long press. The default
     * duration used is the one returned by a call to {@link ViewConfiguration#getLongPressTimeout()}
     * which will return the system defined duration which can be changed in accessibility settings.
     * The duration must be in between {@link #MIN_LONG_PRESS_DURATION} and {@link #MAX_LONG_PRESS_DURATION},
     * otherwise {@link #FALLBACK_LONG_PRESS_DURATION} is used.
     */
    private int mLongPressTimeout;

    /**
     * Defines the duration in milliseconds for the delay between trigger of each repeat of
     * {@link #mRepetitiveKeys}. The default value is defined by {@link #DEFAULT_LONG_PRESS_REPEAT_DELAY}.
     * The duration must be in between {@link #MIN_LONG_PRESS__REPEAT_DELAY} and
     * {@link #MAX_LONG_PRESS__REPEAT_DELAY}, otherwise {@link #DEFAULT_LONG_PRESS_REPEAT_DELAY} is used.
     */
    private int mLongPressRepeatDelay;


    /** The popup window shown if {@link ExtraKeyButton#popup} returns a {@code non-null} value
     * and a swipe up action is done on an extra key. */
    private PopupWindow mPopupWindow;

    private ScheduledExecutorService mScheduledExecutor;
    private Handler mHandler;
    private SpecialButtonsLongHoldRunnable mSpecialButtonsLongHoldRunnable;
    private int mLongPressCount;

    /** How many characters of a label a button is expected to fit, most preset keys are this short. */
    private static final int FITTED_LABEL_LENGTH = 4;

    private final Paint mLabelPaint = new Paint();
    private final Paint mHintPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    /** The text size the buttons come with from the theme, the upper bound for {@link #mTextSize}. */
    private float mBaseTextSize;
    /** The text size currently shared by every button, {@code 0} until it has been computed. */
    private float mTextSize;


    public ExtraKeysView(Context context, AttributeSet attrs) {
        super(context, attrs);

        setRepetitiveKeys(ExtraKeysConstants.PRIMARY_REPETITIVE_KEYS);
        setSpecialButtons(getDefaultSpecialButtons(this));

        setButtonColors(DEFAULT_BUTTON_TEXT_COLOR, DEFAULT_BUTTON_ACTIVE_TEXT_COLOR, DEFAULT_BUTTON_BACKGROUND_COLOR, DEFAULT_BUTTON_ACTIVE_BACKGROUND_COLOR);

        setLongPressTimeout(ViewConfiguration.getLongPressTimeout());
        setLongPressRepeatDelay(DEFAULT_LONG_PRESS_REPEAT_DELAY);
    }


    /** Set {@link #mExtraKeysViewClient}. */
    public void setExtraKeysViewClient(IExtraKeysView extraKeysViewClient) {
        mExtraKeysViewClient = extraKeysViewClient;
    }

    /** Set {@link #mRepetitiveKeys}. Must not be {@code null}. */
    public void setRepetitiveKeys(@NonNull List<String> repetitiveKeys) {
        mRepetitiveKeys = repetitiveKeys;
    }

    /** Set {@link #mSpecialButtonsKeys}. Must not be {@code null}. */
    public void setSpecialButtons(@NonNull Map<SpecialButton, SpecialButtonState> specialButtons) {
        mSpecialButtons = specialButtons;
        mSpecialButtonsKeys = this.mSpecialButtons.keySet().stream().map(SpecialButton::getKey).collect(Collectors.toSet());
    }


    /**
     * Set the {@link ExtraKeysView} button colors.
     *
     * @param buttonTextColor The value for {@link #mButtonTextColor}.
     * @param buttonActiveTextColor The value for {@link #mButtonActiveTextColor}.
     * @param buttonBackgroundColor The value for {@link #mButtonBackgroundColor}.
     * @param buttonActiveBackgroundColor The value for {@link #mButtonActiveBackgroundColor}.
     */
    public void setButtonColors(int buttonTextColor, int buttonActiveTextColor, int buttonBackgroundColor, int buttonActiveBackgroundColor) {
        mButtonTextColor = buttonTextColor;
        mButtonActiveTextColor = buttonActiveTextColor;
        mButtonBackgroundColor = buttonBackgroundColor;
        mButtonActiveBackgroundColor = buttonActiveBackgroundColor;
    }


    /** Get {@link #mButtonTextColor}. */
    public int getButtonTextColor() {
        return mButtonTextColor;
    }

    /** Get {@link #mButtonActiveTextColor}. */
    public int getButtonActiveTextColor() {
        return mButtonActiveTextColor;
    }

    /** Set {@link #mLongPressTimeout}. */
    public void setLongPressTimeout(int longPressDuration) {
        if (longPressDuration >= MIN_LONG_PRESS_DURATION && longPressDuration <= MAX_LONG_PRESS_DURATION) {
            mLongPressTimeout = longPressDuration;
        } else {
            mLongPressTimeout = FALLBACK_LONG_PRESS_DURATION;
        }
    }

    /** Set {@link #mLongPressRepeatDelay}. */
    public void setLongPressRepeatDelay(int longPressRepeatDelay) {
        if (mLongPressRepeatDelay >= MIN_LONG_PRESS__REPEAT_DELAY && mLongPressRepeatDelay <= MAX_LONG_PRESS__REPEAT_DELAY) {
            mLongPressRepeatDelay = longPressRepeatDelay;
        } else {
            mLongPressRepeatDelay = DEFAULT_LONG_PRESS_REPEAT_DELAY;
        }
    }

    /** Get the default map that can be used for {@link #mSpecialButtons}. */
    @NonNull
    public Map<SpecialButton, SpecialButtonState> getDefaultSpecialButtons(ExtraKeysView extraKeysView) {
        return new HashMap<>() {{
            put(SpecialButton.CTRL, new SpecialButtonState(extraKeysView));
            put(SpecialButton.ALT, new SpecialButtonState(extraKeysView));
            put(SpecialButton.SHIFT, new SpecialButtonState(extraKeysView));
            put(SpecialButton.META, new SpecialButtonState(extraKeysView));
            put(SpecialButton.FN, new SpecialButtonState(extraKeysView));
        }};
    }

    /**
     * Reload this instance of {@link ExtraKeysView} with the info passed in {@code extraKeysInfo}.
     */
    @SuppressLint("ClickableViewAccessibility")
    public void reload() {
        TermuxX11ExtraKeys.setExtraKeys();
        ExtraKeysInfo extraKeysInfo = TermuxX11ExtraKeys.getExtraKeysInfo();
        if (extraKeysInfo == null)
            return;

        for(SpecialButtonState state : mSpecialButtons.values())
            state.buttons = new ArrayList<>();

        removeAllViews();
        mTextSize = 0;
        if (mPopupWindow != null)
            dismissPopup();

        ExtraKeyButton[][] buttons = extraKeysInfo.getMatrix();

        setRowCount(buttons.length);
        setColumnCount(maximumLength(buttons));

        boolean reverseRows = MainActivity.getInstance().getPagerPosition() == PAGER_POSITION_TOP;

        for (int row = 0; row < buttons.length; row++) {
            int actualRow = reverseRows ? buttons.length - 1 - row : row;
            for (int col = 0; col < buttons[actualRow].length; col++) {
                final ExtraKeyButton buttonInfo = buttons[actualRow][col];

                KeyButton button;
                if (isSpecialButton(buttonInfo)) {
                    button = createSpecialButton(buttonInfo.key, true);
                    if (button == null) return;
                } else {
                    button = new KeyButton();
                }
                if (mBaseTextSize <= 0)
                    mBaseTextSize = button.getTextSize();

                button.setBackground(new ColorDrawable(Color.BLACK) {
                    public boolean isStateful() {
                        return true;
                    }
                    public boolean hasFocusStateSpecified() {
                        return true;
                    }
                });
                if (!setIcon(button, buttonInfo.key))
                    button.setText(buttonInfo.display);
                if (buttonInfo.popup != null)
                    button.setPopupHint(buttonInfo.popup);
                if (!isSpecialButton(buttonInfo))
                    button.setTextColor(mButtonTextColor);
                button.setAllCaps(true);
                button.setPadding(0, 0, 0, 0);

                button.setOnClickListener(view -> {
                    performExtraKeyButtonHapticFeedback(view, buttonInfo, button);
                    onAnyExtraKeyButtonClick(view, buttonInfo, button);
                });

                button.setOnTouchListener((view, event) -> {
                    switch (event.getAction()) {
                        case MotionEvent.ACTION_DOWN:
                            view.setBackgroundColor(mButtonActiveBackgroundColor);
                            // Start long press scheduled executors which will be stopped in next MotionEvent
                            startScheduledExecutors(view, buttonInfo, button);
                            return true;

                        case MotionEvent.ACTION_MOVE:
                            if (buttonInfo.popup != null) {
                                // Show popup on swipe up
                                if (mPopupWindow == null && (reverseRows ? (event.getY() > 0) : (event.getY() < 0))) {
                                    stopScheduledExecutors();
                                    view.setBackgroundColor(mButtonBackgroundColor);
                                    showPopup(view, buttonInfo.popup);
                                }
                                if (mPopupWindow != null && (reverseRows ? (event.getY() < 0) : (event.getY() > 0))) {
                                    view.setBackgroundColor(mButtonActiveBackgroundColor);
                                    dismissPopup();
                                }
                            }
                            return true;

                        case MotionEvent.ACTION_CANCEL:
                            view.setBackgroundColor(mButtonBackgroundColor);
                            stopScheduledExecutors();
                            return true;

                        case MotionEvent.ACTION_UP:
                            view.setBackgroundColor(mButtonBackgroundColor);
                            stopScheduledExecutors();
                            // If ACTION_UP up was not from a repetitive key or was with a key with a popup button
                            if (mLongPressCount == 0 || mPopupWindow != null) {
                                // Trigger popup button click if swipe up complete
                                if (mPopupWindow != null) {
                                    dismissPopup();
                                    if (buttonInfo.popup != null) {
                                        onAnyExtraKeyButtonClick(view, buttonInfo.popup, button);
                                    }
                                } else {
                                    view.performClick();
                                }
                            }
                            return true;

                        default:
                            return true;
                    }
                });

                LayoutParams param = new GridLayout.LayoutParams();
                param.width = 0;
                param.height = 0;
                param.setMargins(0, 0, 0, 0);
                param.columnSpec = GridLayout.spec(col, GridLayout.FILL, 1.f);
                param.rowSpec = GridLayout.spec(row, GridLayout.FILL, 1.f);
                button.setLayoutParams(param);

                addView(button);
            }
        }

        fitLabels();
    }

    public void onExtraKeyButtonClick(View view, ExtraKeyButton buttonInfo, Button button) {
        if (mExtraKeysViewClient != null)
            mExtraKeysViewClient.onExtraKeyButtonClick(view, buttonInfo, button);
    }

    public void performExtraKeyButtonHapticFeedback(View view, ExtraKeyButton buttonInfo, Button button) {
        if (mExtraKeysViewClient != null) {
            // If client handled the feedback, then just return
            if (mExtraKeysViewClient.performExtraKeyButtonHapticFeedback(view, buttonInfo, button))
                return;
        }

        if (Settings.System.getInt(getContext().getContentResolver(),
            Settings.System.HAPTIC_FEEDBACK_ENABLED, 0) != 0) {

            if (Build.VERSION.SDK_INT >= 28) {
                button.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
            } else {
                // Perform haptic feedback only if no total silence mode enabled.
                if (Settings.Global.getInt(getContext().getContentResolver(), "zen_mode", 0) != 2) {
                    button.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
                }
            }
        }
    }

    public void onAnyExtraKeyButtonClick(View view, @NonNull ExtraKeyButton buttonInfo, Button button) {
        if (isSpecialButton(buttonInfo)) {
            if (mLongPressCount > 0) return;
            SpecialButtonState state = mSpecialButtons.get(SpecialButton.valueOf(buttonInfo.key));
            if (state == null) return;

            // Toggle active state and disable lock state if new state is not active
            state.setIsActive(!state.isActive);
            if (!state.isActive)
                state.setIsLocked(false);
        } else {
            onExtraKeyButtonClick(view, buttonInfo, button);
        }
    }

    public void startScheduledExecutors(View view, ExtraKeyButton buttonInfo, Button button) {
        stopScheduledExecutors();
        mLongPressCount = 0;
        if (mRepetitiveKeys.contains(buttonInfo.key)) {
            // Auto repeat key if long pressed until ACTION_UP stops it by calling stopScheduledExecutors.
            // Currently, only one (last) repeat key can run at a time. Old ones are stopped.
            mScheduledExecutor = Executors.newSingleThreadScheduledExecutor();
            mScheduledExecutor.scheduleWithFixedDelay(() -> {
                mLongPressCount++;
                onExtraKeyButtonClick(view, buttonInfo, button);
            }, mLongPressTimeout, mLongPressRepeatDelay, TimeUnit.MILLISECONDS);
        } else if (isSpecialButton(buttonInfo)) {
            // Lock the key if long pressed by running mSpecialButtonsLongHoldRunnable after
            // waiting for mLongPressTimeout milliseconds. If user does not long press, then the
            // ACTION_UP triggered will cancel the runnable by calling stopScheduledExecutors before
            // it has a chance to run.
            SpecialButtonState state = mSpecialButtons.get(SpecialButton.valueOf(buttonInfo.key));
            if (state == null) return;
            if (mHandler == null)
                mHandler = new Handler(Looper.getMainLooper());
            mSpecialButtonsLongHoldRunnable = new SpecialButtonsLongHoldRunnable(state);
            mHandler.postDelayed(mSpecialButtonsLongHoldRunnable, mLongPressTimeout);
        }
    }

    public void stopScheduledExecutors() {
        if (mScheduledExecutor != null) {
            mScheduledExecutor.shutdownNow();
            mScheduledExecutor = null;
        }

        if (mSpecialButtonsLongHoldRunnable != null && mHandler != null) {
            mHandler.removeCallbacks(mSpecialButtonsLongHoldRunnable);
            mSpecialButtonsLongHoldRunnable = null;
        }
    }

    public class SpecialButtonsLongHoldRunnable implements Runnable {
        public final SpecialButtonState mState;

        public SpecialButtonsLongHoldRunnable(SpecialButtonState state) {
            mState = state;
        }

        public void run() {
            // Toggle active and lock state
            mState.setIsLocked(!mState.isActive);
            mState.setIsActive(!mState.isActive);
            mLongPressCount++;
        }
    }

    void showPopup(View view, ExtraKeyButton extraButton) {
        int pos = MainActivity.getInstance().getPagerPosition();
        int width = pos == PAGER_POSITION_TOP || pos == PAGER_POSITION_BOTTOM ? view.getMeasuredWidth() : view.getMeasuredHeight();
        int height = pos == PAGER_POSITION_TOP || pos == PAGER_POSITION_BOTTOM ? view.getMeasuredHeight() : view.getMeasuredWidth();
        Button button;
        if (isSpecialButton(extraButton)) {
            button = createSpecialButton(extraButton.key, false);
            if (button == null) return;
        } else {
            button = new Button(getContext(), null, android.R.attr.buttonBarButtonStyle);
            button.setTextColor(mButtonTextColor);
        }
        if (!setIcon(button, extraButton.key)) {
            button.setText(extraButton.display);
            if (mTextSize > 0)
                button.setTextSize(TypedValue.COMPLEX_UNIT_PX, mTextSize);
        }
        button.setAllCaps(true);
        button.setPadding(0, 0, 0, 0);
        button.setMinHeight(0);
        button.setMinWidth(0);
        button.setMinimumWidth(0);
        button.setMinimumHeight(0);
        button.setWidth(width);
        button.setHeight(height);
        button.setBackgroundColor(mButtonActiveBackgroundColor);
        mPopupWindow = new PopupWindow(this);
        mPopupWindow.setWidth(LayoutParams.WRAP_CONTENT);
        mPopupWindow.setHeight(LayoutParams.WRAP_CONTENT);
        mPopupWindow.setContentView(button);
        mPopupWindow.setOutsideTouchable(true);
        mPopupWindow.setFocusable(false);
        switch (pos) {
            case PAGER_POSITION_TOP: mPopupWindow.showAsDropDown(view, 0, 0); break;
            case PAGER_POSITION_BOTTOM: mPopupWindow.showAsDropDown(view, 0, -2 * height); break;
            case PAGER_POSITION_LEFT: mPopupWindow.showAsDropDown(view, 0, -width); break;
            case PAGER_POSITION_RIGHT: mPopupWindow.showAsDropDown(view, -width, -height -width); break;
        }
    }

    public void dismissPopup() {
        mPopupWindow.setContentView(null);
        mPopupWindow.dismiss();
        mPopupWindow = null;
    }

    /** Check whether a {@link ExtraKeyButton} is a {@link SpecialButton}. */
    public boolean isSpecialButton(ExtraKeyButton button) {
        return mSpecialButtonsKeys.contains(button.key);
    }

    /**
     * Read whether {@link SpecialButton} registered in {@link #mSpecialButtons} is active or not.
     *
     * @param specialButton The {@link SpecialButton} to read.
     * @param autoSetInActive Set to {@code true} if {@link SpecialButtonState#isActive} should be
     *                        set {@code false} if button is not locked.
     * @return Returns {@code null} if button does not exist in {@link #mSpecialButtons}. If button
     *         exists, then returns {@code true} if the button is created in {@link ExtraKeysView}
     *         and is active, otherwise {@code false}.
     */
    @Nullable
    public Boolean readSpecialButton(SpecialButton specialButton, boolean autoSetInActive) {
        SpecialButtonState state = mSpecialButtons.get(specialButton);
        if (state == null) return null;

        if (!state.isCreated || !state.isActive)
            return false;

        // Disable active state only if not locked
        if (autoSetInActive && !state.isLocked)
            state.setIsActive(false);

        return true;
    }

    public KeyButton createSpecialButton(String buttonKey, boolean needUpdate) {
        SpecialButtonState state = mSpecialButtons.get(SpecialButton.valueOf(buttonKey));
        if (state == null) return null;
        state.setIsCreated(true);
        KeyButton button = new KeyButton();
        button.setTextColor(state.isActive ? mButtonActiveTextColor : mButtonTextColor);
        if (needUpdate) {
            state.buttons.add(button);
        }
        return button;
    }

    private boolean setIcon(Button button, String key) {
        int id = iconResource(key);
        if (id == 0)
            return false;

        Drawable icon = getResources().getDrawable(id, getContext().getTheme());
        button.setText(null);
        button.setForeground(new ScaledIcon(icon, shrinkFactor()));
        button.setForegroundGravity(Gravity.CENTER);
        button.setContentDescription(key);
        return true;
    }

    /** The icon a key is drawn with instead of its label, {@code 0} for the keys that have none. */
    private static int iconResource(String key) {
        int id;
        switch (key) {
            case "LEFT":
                id = R.drawable.ic_extra_key_arrow_left;
                break;
            case "RIGHT":
                id = R.drawable.ic_extra_key_arrow_right;
                break;
            case "UP":
                id = R.drawable.ic_extra_key_arrow_up;
                break;
            case "DOWN":
                id = R.drawable.ic_extra_key_arrow_down;
                break;
            case "PREFERENCES":
                id = R.drawable.ic_extra_key_settings;
                break;
            case "KEYBOARD":
                id = R.drawable.ic_extra_key_keyboard;
                break;
            case "ZOOM_IN":
                id = R.drawable.ic_zoom_in;
                break;
            case "ZOOM_OUT":
                id = R.drawable.ic_zoom_out;
                break;
            case "ZOOM_RESET":
                id = R.drawable.ic_zoom_reset;
                break;
            default:
                return 0;
        }

        return id;
    }

    @Override
    protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        super.onSizeChanged(width, height, oldWidth, oldHeight);
        fitLabels();
    }

    /**
     * Shrinks every button by the same factor, the least one at which the widest label still fits
     * its cell on a single line, scaling icons along with the labels. Labels longer than
     * {@link #FITTED_LABEL_LENGTH} only count up to that length, so one long custom label can not
     * shrink the whole bar.
     */
    private void fitLabels() {
        int columns = getColumnCount();
        if (getWidth() <= 0 || columns < 1 || getChildCount() < 1 || mBaseTextSize <= 0)
            return;

        // Every cell is the same width, and every button draws with the typeface of the same style.
        float cellWidth = (float) (getWidth() - getPaddingLeft() - getPaddingRight()) / columns;
        mLabelPaint.setTypeface(((Button) getChildAt(0)).getTypeface());
        mLabelPaint.setTextSize(mBaseTextSize);

        float widest = 0;
        for (int i = 0; i < getChildCount(); i++) {
            Button button = (Button) getChildAt(i);
            if (!hasLabel(button))
                continue;

            // Buttons are all caps, and capitals are the wider ones, so measure the text as drawn.
            CharSequence label = button.getText();
            widest = Math.max(widest, mLabelPaint.measureText(
                    label.subSequence(0, Math.min(label.length(), FITTED_LABEL_LENGTH))
                            .toString().toUpperCase(Locale.ROOT)));
        }

        float textSize = widest > cellWidth ? mBaseTextSize * cellWidth / widest : mBaseTextSize;
        if (textSize == mTextSize)
            return;
        mTextSize = textSize;
        for (int i = 0; i < getChildCount(); i++) {
            Button button = (Button) getChildAt(i);
            // A button drawing an icon keeps the text size too, its popup hint scales with it.
            button.setTextSize(TypedValue.COMPLEX_UNIT_PX, textSize);
            if (!hasLabel(button) && button.getForeground() instanceof ScaledIcon)
                // A new instance, setForeground ignores the one it already holds.
                button.setForeground(new ScaledIcon(((ScaledIcon) button.getForeground()).getDrawable(), shrinkFactor()));
        }
    }

    /** Whether a button draws a label rather than an icon set by {@link #setIcon(Button, String)}. */
    private static boolean hasLabel(Button button) {
        CharSequence label = button.getText();
        return label != null && label.length() > 0;
    }

    /** How much smaller than the theme size the buttons currently draw, {@code 1} while they fit. */
    private float shrinkFactor() {
        return mTextSize > 0 && mBaseTextSize > 0 ? mTextSize / mBaseTextSize : 1f;
    }

    /**
     * A button of the bar, drawing what its popup key is in the top right corner, the way a keycap
     * hints at the symbols its key can also produce.
     */
    final class KeyButton extends Button {
        /** How much smaller than the button label the hint draws. */
        private static final float HINT_SCALE = 0.5f;
        /** An icon pads itself, so it needs more room than a label to read as the same size. */
        private static final float HINT_ICON_SCALE = 1.75f;

        @Nullable
        private Drawable mHintIcon;
        @Nullable
        private String mHintLabel;

        KeyButton() {
            super(ExtraKeysView.this.getContext(), null, android.R.attr.buttonBarButtonStyle);
        }

        /** Set the popup key to hint at, the same way {@link #setIcon(Button, String)} shows a key. */
        void setPopupHint(@NonNull ExtraKeyButton popup) {
            int id = iconResource(popup.key);
            mHintIcon = id != 0 ? getResources().getDrawable(id, getContext().getTheme()).mutate() : null;
            mHintLabel = popup.display.toUpperCase(Locale.ROOT);
            invalidate();
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            if (mHintLabel == null)
                return;

            float size = getTextSize() * HINT_SCALE, padding = size / 4;
            float right = getWidth() - padding;
            int alpha = Color.alpha(getCurrentTextColor()) * 2 / 3;
            if (mHintIcon != null) {
                size *= HINT_ICON_SCALE;
                mHintIcon.setAlpha(alpha);
                mHintIcon.setBounds(Math.round(right - size), Math.round(padding),
                        Math.round(right), Math.round(padding + size));
                mHintIcon.draw(canvas);
            } else {
                mHintPaint.setTypeface(getTypeface());
                mHintPaint.setTextSize(size);
                mHintPaint.setTextAlign(Paint.Align.RIGHT);
                mHintPaint.setColor(getCurrentTextColor());
                mHintPaint.setAlpha(alpha);
                // A macro label can be arbitrarily long, so it is cut to what the corner holds.
                int fits = mHintPaint.breakText(mHintLabel, true, getWidth() - 2 * padding, null);
                canvas.drawText(mHintLabel, 0, fits, right, padding - mHintPaint.ascent(), mHintPaint);
            }
        }
    }

    /**
     * An icon reporting a scaled intrinsic size, because a foreground is drawn at that size and so
     * would otherwise keep the size it has at the theme's text size.
     */
    private static final class ScaledIcon extends DrawableWrapper {
        private final float mScale;

        ScaledIcon(Drawable icon, float scale) {
            super(icon);
            mScale = scale;
        }

        @Override
        public int getIntrinsicWidth() {
            return Math.round(getDrawable().getIntrinsicWidth() * mScale);
        }

        @Override
        public int getIntrinsicHeight() {
            return Math.round(getDrawable().getIntrinsicHeight() * mScale);
        }
    }

    /**
     * General util function to compute the longest column length in a matrix.
     */
    public static int maximumLength(Object[][] matrix) {
        int m = 0;
        for (Object[] row : matrix)
            m = Math.max(m, row.length);
        return m;
    }
}
