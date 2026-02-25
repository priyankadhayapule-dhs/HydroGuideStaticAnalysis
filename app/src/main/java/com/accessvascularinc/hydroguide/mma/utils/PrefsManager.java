package com.accessvascularinc.hydroguide.mma.utils;

import android.content.Context;
import android.content.SharedPreferences;

import androidx.security.crypto.EncryptedSharedPreferences;
import androidx.security.crypto.MasterKey;

import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.google.gson.Gson;

public class PrefsManager {
    private static final String KEY_TABLET_CONFIGURATION_STATE_IS_OFFLINE = "tablet_configuration_state_is_offline";
    private static final String PREFS_NAME = "avi_prefs";
    private static final String TOKEN_SAVED_TIME = "token_saved_time";
    private static final String KEY_LOGIN_RESPONSE = "login_response";
    private static final long TOKEN_EXPIRY_DURATION_MS = 18 * 60 * 60 * 1000L;
    private static final String KEY_IS_SYNC_IN_PROGRESS = "isSyncInProgress";
    private static final String KEY_SELECTED_DEPTH_INDEX = "selected_depth_index";
    private static final String KEY_NEAR_GAIN = "near_gain";
    private static final String KEY_FAR_GAIN = "far_gain";
    private static final String KEY_USER_ID = "user_id";
    private static final String KEY_USER_EMAIL = "user_email";
    private static final String KEY_FIRMWARE_VERSION = "FirmwareVersion";
    /**
     * Get the auth token from shared preferences, or null if not found.
     */
    public static String getTokenFromPrefs(Context context) {
        LoginResponse loginResponse = getLoginResponse(context);
        if (loginResponse != null && loginResponse.getData() != null) {
            return loginResponse.getData().getToken();
        }
        return null;
    }

    /**
     * Get the organization ID from shared preferences, or null if not found.
     */
    public static String getOrganizationId(Context context) {
        LoginResponse loginResponse = getLoginResponse(context);
        return (loginResponse != null) ? loginResponse.getOrganizationId() : null;
    }

    /**
     * Returns true if the tablet is in offline configuration state, false
     * otherwise.
     * Default is false if not set.
     */
    public static boolean getTabletConfigurationStateIsOffline(Context context) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        return prefs.getBoolean(KEY_TABLET_CONFIGURATION_STATE_IS_OFFLINE, true);
    }

    /**
     * Returns the configured storage mode for the tablet.
     * <p>
     * - UNKNOWN: mode has never been configured (fresh install).
     * - OFFLINE / ONLINE: mode has been explicitly set.
     */
    public static IStorageManager.StorageMode getConfiguredStorageMode(Context context) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        if (!prefs.contains(KEY_TABLET_CONFIGURATION_STATE_IS_OFFLINE)) {
            return IStorageManager.StorageMode.UNKNOWN;
        }
        boolean isOffline = prefs.getBoolean(KEY_TABLET_CONFIGURATION_STATE_IS_OFFLINE, true);
        return isOffline ? IStorageManager.StorageMode.OFFLINE : IStorageManager.StorageMode.ONLINE;
    }

    /**
     * Sets the tablet configuration offline state flag.
     */
    public static void setTabletConfigurationStateIsOffline(Context context, boolean isOffline) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        prefs.edit().putBoolean(KEY_TABLET_CONFIGURATION_STATE_IS_OFFLINE, isOffline).apply();
        MedDevLog.audit("Online Offline switch", "Tablet configuration mode set to " + (isOffline ? "OFFLINE" : "ONLINE"));
    }

    public static void saveLoginResponse(Context context, LoginResponse response) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        prefs.edit().putString(KEY_LOGIN_RESPONSE, new Gson().toJson(response)).apply();
    }

    public static LoginResponse getLoginResponse(Context context) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        String json = prefs.getString(KEY_LOGIN_RESPONSE, null);
        if (json == null)
            return null;
        return new Gson().fromJson(json, LoginResponse.class);
    }

    public static void clearLoginResponse(Context context) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        prefs.edit().remove(KEY_LOGIN_RESPONSE).apply();
    }

    private static SharedPreferences getEncryptedPrefs(Context context) {
        try {
            MasterKey masterKey = new MasterKey.Builder(context)
                    .setKeyScheme(MasterKey.KeyScheme.AES256_GCM)
                    .build();
            return EncryptedSharedPreferences.create(
                    context,
                    PREFS_NAME,
                    masterKey,
                    EncryptedSharedPreferences.PrefKeyEncryptionScheme.AES256_SIV,
                    EncryptedSharedPreferences.PrefValueEncryptionScheme.AES256_GCM);
        } catch (Exception e) {
            throw new RuntimeException("Failed to create encrypted shared preferences", e);
        }
    }

    public static void setSyncStatus(Context context, boolean status) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        prefs.edit().putBoolean(KEY_IS_SYNC_IN_PROGRESS, status).apply();
    }

    public static boolean getSyncStatus(Context context) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        return prefs.getBoolean(KEY_IS_SYNC_IN_PROGRESS, false);
    }
    
    /**
     * Save the selected depth index to preferences.
     */
    public static void setSelectedDepthIndex(Context context, int depthIndex) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        prefs.edit().putInt(KEY_SELECTED_DEPTH_INDEX, depthIndex).apply();
    }
    
    /**
     * Get the selected depth index from preferences.
     * Returns 0 (first depth) as default if not set.
     */
    public static int getSelectedDepthIndex(Context context) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        return prefs.getInt(KEY_SELECTED_DEPTH_INDEX, 0);
    }
    
    /**
     * Save the near gain value to preferences.
     */
    public static void setNearGain(Context context, int nearGain) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        prefs.edit().putInt(KEY_NEAR_GAIN, nearGain).apply();
    }
    
    /**
     * Get the near gain value from preferences.
     * Returns 100 as default if not set.
     */
    public static int getNearGain(Context context) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        return prefs.getInt(KEY_NEAR_GAIN, 100);
    }
    
    /**
     * Save the far gain value to preferences.
     */
    public static void setFarGain(Context context, int farGain) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        prefs.edit().putInt(KEY_FAR_GAIN, farGain).apply();
    }
    
    /**
     * Get the far gain value from preferences.
     * Returns 100 as default if not set.
     */
    public static int getFarGain(Context context) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        return prefs.getInt(KEY_FAR_GAIN, 100);
    }
    
    public static String getPrefsName(){
        return PREFS_NAME;
    }

    public static String getTokenSavedTime() {
        return TOKEN_SAVED_TIME;
    }

    public static long getTokenExpiryDurationMs() {
        return TOKEN_EXPIRY_DURATION_MS;
    }

    /**
     * Save cloud user ID to shared preferences.
     */
    public static void setUserId(Context context, String cloudUserId) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        prefs.edit().putString(KEY_USER_ID, cloudUserId).apply();
    }

    /**
     * Get cloud user ID from shared preferences, or null if not found.
     */
    public static String getUserId(Context context) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        return prefs.getString(KEY_USER_ID, null);
    }

    /**
     * Save user email to shared preferences.
     */
    public static void setUserEmail(Context context, String email) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        prefs.edit().putString(KEY_USER_EMAIL, email).apply();
    }

    /**
     * Get user email from shared preferences, or null if not found.
     */
    public static String getUserEmail(Context context) {
        SharedPreferences prefs = getEncryptedPrefs(context);
        return prefs.getString(KEY_USER_EMAIL, null);
    }

    public static void updateFirmwareVersion(Context context, String firmwareVersion)
    {
        SharedPreferences prefs = getEncryptedPrefs(context);
        prefs.edit().putString(KEY_FIRMWARE_VERSION, firmwareVersion).apply();
    }
    public static String getFirmwareVersion(Context context)
    {
        SharedPreferences prefs = getEncryptedPrefs(context);
        return prefs.getString(KEY_FIRMWARE_VERSION, null);
    }
}
