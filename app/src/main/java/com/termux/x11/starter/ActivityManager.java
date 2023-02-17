package com.termux.x11.starter;

/**
 * \@hide-hidden constants
 */
public class ActivityManager {
    private static final int FIRST_START_FATAL_ERROR_CODE = -100;
    private static final int LAST_START_FATAL_ERROR_CODE = -1;
    private static final int FIRST_START_SUCCESS_CODE = 0;
    private static final int LAST_START_SUCCESS_CODE = 99;
    private static final int FIRST_START_NON_FATAL_ERROR_CODE = 100;
    private static final int LAST_START_NON_FATAL_ERROR_CODE = 199;

    /**
     * Result for IActivityManager.startVoiceActivity: active session is currently hidden.
     * @hide
     */
    public static final int START_VOICE_HIDDEN_SESSION = FIRST_START_FATAL_ERROR_CODE;

    /**
     * Result for IActivityManager.startVoiceActivity: active session does not match
     * the requesting token.
     * @hide
     */
    public static final int START_VOICE_NOT_ACTIVE_SESSION = FIRST_START_FATAL_ERROR_CODE + 1;

    /**
     * Result for IActivityManager.startActivity: trying to start a background user
     * activity that shouldn't be displayed for all users.
     * @hide
     */
    public static final int START_NOT_CURRENT_USER_ACTIVITY = FIRST_START_FATAL_ERROR_CODE + 2;

    /**
     * Result for IActivityManager.startActivity: trying to start an activity under voice
     * control when that activity does not support the VOICE category.
     * @hide
     */
    public static final int START_NOT_VOICE_COMPATIBLE = FIRST_START_FATAL_ERROR_CODE + 3;

    /**
     * Result for IActivityManager.startActivity: an error where the
     * start had to be canceled.
     * @hide
     */
    public static final int START_CANCELED = FIRST_START_FATAL_ERROR_CODE + 4;

    /**
     * Result for IActivityManager.startActivity: an error where the
     * thing being started is not an activity.
     * @hide
     */
    public static final int START_NOT_ACTIVITY = FIRST_START_FATAL_ERROR_CODE + 5;

    /**
     * Result for IActivityManager.startActivity: an error where the
     * caller does not have permission to start the activity.
     * @hide
     */
    public static final int START_PERMISSION_DENIED = FIRST_START_FATAL_ERROR_CODE + 6;

    /**
     * Result for IActivityManager.startActivity: an error where the
     * caller has requested both to forward a result and to receive
     * a result.
     * @hide
     */
    public static final int START_FORWARD_AND_REQUEST_CONFLICT = FIRST_START_FATAL_ERROR_CODE + 7;

    /**
     * Result for IActivityManager.startActivity: an error where the
     * requested class is not found.
     * @hide
     */
    public static final int START_CLASS_NOT_FOUND = FIRST_START_FATAL_ERROR_CODE + 8;

    /**
     * Result for IActivityManager.startActivity: an error where the
     * given Intent could not be resolved to an activity.
     * @hide
     */
    public static final int START_INTENT_NOT_RESOLVED = FIRST_START_FATAL_ERROR_CODE + 9;

    /**
     * Result for IActivityManager.startAssistantActivity: active session is currently hidden.
     * @hide
     */
    public static final int START_ASSISTANT_HIDDEN_SESSION = FIRST_START_FATAL_ERROR_CODE + 10;

    /**
     * Result for IActivityManager.startAssistantActivity: active session does not match
     * the requesting token.
     * @hide
     */
    public static final int START_ASSISTANT_NOT_ACTIVE_SESSION = FIRST_START_FATAL_ERROR_CODE + 11;

    /**
     * Result for IActivityManaqer.startActivity: the activity was started
     * successfully as normal.
     * @hide
     */
    public static final int START_SUCCESS = FIRST_START_SUCCESS_CODE;

    /**
     * Result for IActivityManaqer.startActivity: the caller asked that the Intent not
     * be executed if it is the recipient, and that is indeed the case.
     * @hide
     */
    public static final int START_RETURN_INTENT_TO_CALLER = FIRST_START_SUCCESS_CODE + 1;

    /**
     * Result for IActivityManaqer.startActivity: activity wasn't really started, but
     * a task was simply brought to the foreground.
     * @hide
     */
    public static final int START_TASK_TO_FRONT = FIRST_START_SUCCESS_CODE + 2;

    /**
     * Result for IActivityManaqer.startActivity: activity wasn't really started, but
     * the given Intent was given to the existing top activity.
     * @hide
     */
    public static final int START_DELIVERED_TO_TOP = FIRST_START_SUCCESS_CODE + 3;

    /**
     * Result for IActivityManaqer.startActivity: request was canceled because
     * app switches are temporarily canceled to ensure the user's last request
     * (such as pressing home) is performed.
     * @hide
     */
    public static final int START_SWITCHES_CANCELED = FIRST_START_NON_FATAL_ERROR_CODE;

    /**
     * Result for IActivityManaqer.startActivity: a new activity was attempted to be started
     * while in Lock Task Mode.
     * @hide
     */
    public static final int START_RETURN_LOCK_TASK_MODE_VIOLATION =
            FIRST_START_NON_FATAL_ERROR_CODE + 1;

    /**
     * Result for IActivityManaqer.startActivity: a new activity start was aborted. Never returned
     * externally.
     * @hide
     */
    public static final int START_ABORTED = FIRST_START_NON_FATAL_ERROR_CODE + 2;

    /**
     * Flag for IActivityManaqer.startActivity: do special start mode where
     * a new activity is launched only if it is needed.
     * @hide
     */
    public static final int START_FLAG_ONLY_IF_NEEDED = 1<<0;

    /**
     * Flag for IActivityManaqer.startActivity: launch the app for
     * debugging.
     * @hide
     */
    public static final int START_FLAG_DEBUG = 1<<1;

    /**
     * Flag for IActivityManaqer.startActivity: launch the app for
     * allocation tracking.
     * @hide
     */
    public static final int START_FLAG_TRACK_ALLOCATION = 1<<2;

    /**
     * Flag for IActivityManaqer.startActivity: launch the app with
     * native debugging support.
     * @hide
     */
    public static final int START_FLAG_NATIVE_DEBUGGING = 1<<3;

    /**
     * Result for IActivityManaqer.broadcastIntent: success!
     * @hide
     */
    public static final int BROADCAST_SUCCESS = 0;

    /**
     * Result for IActivityManaqer.broadcastIntent: attempt to broadcast
     * a sticky intent without appropriate permission.
     * @hide
     */
    public static final int BROADCAST_STICKY_CANT_HAVE_PERMISSION = -1;

    /**
     * Result for IActivityManager.broadcastIntent: trying to send a broadcast
     * to a stopped user. Fail.
     * @hide
     */
    public static final int BROADCAST_FAILED_USER_STOPPED = -2;


}
