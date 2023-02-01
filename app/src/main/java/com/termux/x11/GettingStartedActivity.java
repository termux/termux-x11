package com.termux.x11;

import android.content.Intent;
import android.os.Bundle;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

import com.termux.shared.android.PermissionUtils;
import com.termux.shared.data.IntentUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.logger.Logger;
import com.termux.x11.utils.ViewUtils;

import java.util.Arrays;

public class GettingStartedActivity extends AppCompatActivity {

    private TextView mStoragePermissionNotGrantedWarning;
    private TextView mBatteryOptimizationNotDisabledWarning;
    private TextView mDisplayOverOtherAppsPermissionNotGrantedWarning;

    private Button mGrantStoragePermission;
    private Button mDisableBatteryOptimization;
    private Button mGrantDisplayOverOtherAppsPermission;

    private static final String LOG_TAG = "TermuxX11Activity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.getting_started_activity);

        Toolbar toolbar = findViewById(R.id.toolbar);
        if (toolbar != null)
            setSupportActionBar(toolbar);
        final ActionBar actionBar = getSupportActionBar();
        if (actionBar != null)
            actionBar.setTitle("Termux:X11");

        TextView pluginInfo = findViewById(R.id.textview_plugin_info);
        pluginInfo.setText(getString(R.string.plugin_info, "https://github.com/termux/termux-x11", "Termux"));

        mStoragePermissionNotGrantedWarning = findViewById(R.id.textview_storage_permission_not_granted_warning);
        mGrantStoragePermission = findViewById(R.id.btn_grant_storage_permission);
        mGrantStoragePermission.setOnClickListener(v -> checkAndRequestStoragePermission(true, false));

        mBatteryOptimizationNotDisabledWarning = findViewById(R.id.textview_battery_optimization_not_disabled_warning);
        mDisableBatteryOptimization = findViewById(R.id.btn_disable_battery_optimizations);
        mDisableBatteryOptimization.setOnClickListener(v -> requestDisableBatteryOptimizations());

        mDisplayOverOtherAppsPermissionNotGrantedWarning = findViewById(R.id.textview_display_over_other_apps_not_granted_warning);
        mGrantDisplayOverOtherAppsPermission = findViewById(R.id.btn_grant_display_over_other_apps_permission);
        mGrantDisplayOverOtherAppsPermission.setOnClickListener(v -> requestDisplayOverOtherAppsPermission());
    }

    @Override
    protected void onResume() {
        super.onResume();

        checkAndRequestStoragePermission(false, false);
        checkIfBatteryOptimizationNotDisabled();
        checkIfDisplayOverOtherAppsPermissionNotGranted();
    }

    /**
     * For processes to access primary external storage (/sdcard, /storage/emulated/0, ~/storage/shared),
     * termux needs to be granted legacy WRITE_EXTERNAL_STORAGE or MANAGE_EXTERNAL_STORAGE permissions
     * if targeting targetSdkVersion 30 (android 11) and running on sdk 30 (android 11) and higher.
     */
    public void checkAndRequestStoragePermission(boolean requestPermission, boolean isPermissionCallback) {
        if (mStoragePermissionNotGrantedWarning == null) return;

        // Do not ask for permission again
        int requestCode = requestPermission ? PermissionUtils.REQUEST_GRANT_STORAGE_PERMISSION : -1;

        if (requestPermission)
            Logger.logInfo(LOG_TAG, "Requesting external storage permission");

        // If permission is granted
        if(PermissionUtils.checkAndRequestLegacyOrManageExternalStoragePermission(
                this, requestCode, false)) {
            if (isPermissionCallback)
                Logger.logInfoAndShowToast(this, LOG_TAG,
                        getString(com.termux.shared.R.string.msg_storage_permission_granted_on_request));

            ViewUtils.setWarningTextViewAndButtonState(this, mStoragePermissionNotGrantedWarning,
                    mGrantStoragePermission, false, getString(R.string.action_already_granted));
        } else {
            if (isPermissionCallback)
                Logger.logInfoAndShowToast(this, LOG_TAG,
                        getString(com.termux.shared.R.string.msg_storage_permission_not_granted_on_request));

            ViewUtils.setWarningTextViewAndButtonState(this, mStoragePermissionNotGrantedWarning,
                    mGrantStoragePermission, true, getString(R.string.action_grant_storage_permission));
        }
    }



    private void checkIfBatteryOptimizationNotDisabled() {
        if (mBatteryOptimizationNotDisabledWarning == null) return;

        // If battery optimizations not disabled
        if (!PermissionUtils.checkIfBatteryOptimizationsDisabled(this)) {
            ViewUtils.setWarningTextViewAndButtonState(this, mBatteryOptimizationNotDisabledWarning,
                    mDisableBatteryOptimization, true, getString(R.string.action_disable_battery_optimizations));
        } else {
            ViewUtils.setWarningTextViewAndButtonState(this, mBatteryOptimizationNotDisabledWarning,
                    mDisableBatteryOptimization, false, getString(R.string.action_already_disabled));
        }
    }

    private void requestDisableBatteryOptimizations() {
        Logger.logDebug(LOG_TAG, "Requesting to disable battery optimizations");
        PermissionUtils.requestDisableBatteryOptimizations(this, PermissionUtils.REQUEST_DISABLE_BATTERY_OPTIMIZATIONS);
    }



    private void checkIfDisplayOverOtherAppsPermissionNotGranted() {
        if (mDisplayOverOtherAppsPermissionNotGrantedWarning == null) return;

        // If display over other apps permission not granted
        if (!PermissionUtils.checkDisplayOverOtherAppsPermission(this)) {
            ViewUtils.setWarningTextViewAndButtonState(this, mDisplayOverOtherAppsPermissionNotGrantedWarning,
                    mGrantDisplayOverOtherAppsPermission, true, getString(R.string.action_grant_display_over_other_apps_permission));
        } else {
            ViewUtils.setWarningTextViewAndButtonState(this, mDisplayOverOtherAppsPermissionNotGrantedWarning,
                    mGrantDisplayOverOtherAppsPermission, false, getString(R.string.action_already_granted));
        }
    }

    private void requestDisplayOverOtherAppsPermission() {
        Logger.logDebug(LOG_TAG, "Requesting to grant display over other apps permission");
        PermissionUtils.requestDisplayOverOtherAppsPermission(this, PermissionUtils.REQUEST_GRANT_DISPLAY_OVER_OTHER_APPS_PERMISSION);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        Logger.logVerbose(LOG_TAG, "onActivityResult: requestCode: " + requestCode + ", resultCode: "  + resultCode + ", data: "  + IntentUtils.getIntentString(data));

        switch (requestCode) {
            case PermissionUtils.REQUEST_GRANT_STORAGE_PERMISSION:
                checkAndRequestStoragePermission(false, true);
            case PermissionUtils.REQUEST_DISABLE_BATTERY_OPTIMIZATIONS:
                if(PermissionUtils.checkIfBatteryOptimizationsDisabled(this))
                    Logger.logDebug(LOG_TAG, "Battery optimizations disabled by user on request.");
                else
                    Logger.logDebug(LOG_TAG, "Battery optimizations not disabled by user on request.");
                break;
            case PermissionUtils.REQUEST_GRANT_DISPLAY_OVER_OTHER_APPS_PERMISSION:
                if(PermissionUtils.checkDisplayOverOtherAppsPermission(this))
                    Logger.logDebug(LOG_TAG, "Display over other apps granted by user on request.");
                else
                    Logger.logDebug(LOG_TAG, "Display over other apps denied by user on request.");
                break;
            default:
                Logger.logError(LOG_TAG, "Unknown request code \"" + requestCode + "\" passed to onRequestPermissionsResult");
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        Logger.logVerbose(LOG_TAG, "onRequestPermissionsResult: requestCode: " + requestCode +
                ", permissions: "  + Arrays.toString(permissions) + ", grantResults: "  + Arrays.toString(grantResults));
        if (requestCode == PermissionUtils.REQUEST_GRANT_STORAGE_PERMISSION) {
            checkAndRequestStoragePermission(false, true);
        }
    }
}