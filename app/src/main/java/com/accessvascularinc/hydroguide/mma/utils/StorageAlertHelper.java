package com.accessvascularinc.hydroguide.mma.utils;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.db.repository.EncounterRepository;
import com.accessvascularinc.hydroguide.mma.model.TabletState;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Helper class for checking storage/sync status and determining alert levels.
 * Used by HomeFragment to show appropriate warnings or block procedure initiation.
 */
public class StorageAlertHelper {
    
    // Online mode thresholds (number of un-uploaded records)
    public static final int ONLINE_WARNING_THRESHOLD = 50;
    public static final int ONLINE_BLOCK_THRESHOLD = 100;
    
    // Offline mode thresholds (storage usage percentage)
    public static final int OFFLINE_WARNING_THRESHOLD = 50;
    public static final int OFFLINE_BLOCK_THRESHOLD = 90;
    
    /**
     * Alert level indicating the severity of storage/sync status.
     */
    public enum AlertLevel {
        NONE,       // No alert needed
        WARNING,    // Show warning but allow proceed
        BLOCKED     // Block procedure initiation
    }
    
    /**
     * Callback interface for receiving storage check results.
     */
    public interface StorageCheckCallback {
        void onResult(AlertLevel level, String message);
    }
    
    /**
     * Check storage/sync status and determine alert level.
     * Called from HomeFragment on screen entry and button click.
     * 
     * @param context Application or activity context
     * @param storageManager Storage manager to determine online/offline mode
     * @param tabletState Tablet state containing storage bytes info
     * @param encounterRepository Repository to query pending encounter count
     * @param callback Callback to receive alert level and message
     */
    public static void checkStorageStatus(
            Context context,
            IStorageManager storageManager,
            TabletState tabletState,
            EncounterRepository encounterRepository,
            StorageCheckCallback callback) {
        
        IStorageManager.StorageMode mode = storageManager.getStorageMode();
        
        if (mode == IStorageManager.StorageMode.ONLINE) {
            checkOnlineMode(context, encounterRepository, callback);
        } else {
            checkOfflineMode(context, tabletState, callback);
        }
    }
    
    /**
     * Check online mode: count un-uploaded (Pending) records.
     */
    private static void checkOnlineMode(
            Context context,
            EncounterRepository encounterRepository, 
            StorageCheckCallback callback) {
        
        ExecutorService executor = Executors.newSingleThreadExecutor();
        executor.execute(() -> {
            try {
                int pendingCount = encounterRepository.getPendingEncounterCount();
                
                AlertLevel level;
                String message;
                
                if (pendingCount >= ONLINE_BLOCK_THRESHOLD) {
                    level = AlertLevel.BLOCKED;
                    message = context.getString(
                        R.string.alert_storage_blocked_online, pendingCount);
                } else if (pendingCount > ONLINE_WARNING_THRESHOLD) {
                    level = AlertLevel.WARNING;
                    message = context.getString(
                        R.string.alert_storage_warning_online, pendingCount);
                } else {
                    level = AlertLevel.NONE;
                    message = null;
                }
                
                // Return result on main thread
                new Handler(Looper.getMainLooper())
                    .post(() -> callback.onResult(level, message));
                    
            } catch (Exception e) {
                MedDevLog.error("StorageAlertHelper", "Error checking pending count", e);
                new Handler(Looper.getMainLooper())
                    .post(() -> callback.onResult(AlertLevel.NONE, null));
            } finally {
                executor.shutdown();
            }
        });
    }
    
    /**
     * Check offline mode: calculate storage usage percentage.
     */
    private static void checkOfflineMode(
            Context context,
            TabletState tabletState,
            StorageCheckCallback callback) {
        
        long total = tabletState.getTotalSpaceBytes();
        long free = tabletState.getFreeSpaceBytes();
        
        // Handle case where storage hasn't been polled yet
        if (total == 0) {
            callback.onResult(AlertLevel.NONE, null);
            return;
        }
        
        long used = total - free;
        int usagePercentage = (int) ((used * 100) / total);
        
        AlertLevel level;
        String message;
        
        if (usagePercentage >= OFFLINE_BLOCK_THRESHOLD) {
            level = AlertLevel.BLOCKED;
            message = context.getString(
                R.string.alert_storage_blocked_offline, usagePercentage);
        } else if (usagePercentage >= OFFLINE_WARNING_THRESHOLD) {
            level = AlertLevel.WARNING;
            message = context.getString(
                R.string.alert_storage_warning_offline, usagePercentage);
        } else {
            level = AlertLevel.NONE;
            message = null;
        }
        
        callback.onResult(level, message);
    }
    
    /**
     * Get current storage usage percentage (for offline mode).
     * 
     * @param tabletState Tablet state containing storage info
     * @return Storage usage percentage (0-100)
     */
    public static int getStorageUsagePercentage(TabletState tabletState) {
        long total = tabletState.getTotalSpaceBytes();
        long free = tabletState.getFreeSpaceBytes();
        if (total == 0) return 0;
        return (int) (((total - free) * 100) / total);
    }
}
