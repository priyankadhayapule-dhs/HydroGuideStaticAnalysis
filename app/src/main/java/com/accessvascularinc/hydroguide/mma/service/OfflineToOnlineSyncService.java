package com.accessvascularinc.hydroguide.mma.service;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;

import com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity;
import com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity;
import com.accessvascularinc.hydroguide.mma.db.entities.UsersEntity;
import com.accessvascularinc.hydroguide.mma.db.repository.EncounterRepository;
import com.accessvascularinc.hydroguide.mma.db.repository.FacilityRepository;
import com.accessvascularinc.hydroguide.mma.db.repository.UsersRepository;
import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse;
import com.accessvascularinc.hydroguide.mma.repository.OfflineToOnlineSyncRepository;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;

import javax.inject.Inject;
import dagger.hilt.android.qualifiers.ApplicationContext;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * OfflineToOnlineSyncService orchestrates the controlled Offline → Online mode
 * switching workflow.
 *
 * Tablet Responsibilities:
 * - Validate local admin role
 * - Authenticate with cloud
 * - Validate cloud user is Organization Admin
 * - Collect all offline records (Facilities & Encounters)
 * - Send records to cloud with email mappings
 * - Clear local database on success (fresh start for ONLINE mode)
 *
 * Backend Responsibilities (Cloud API):
 * - Check user existence by email
 * - Create missing users with appropriate roles
 * - Map records to users
 * - Insert records atomically
 *
 * Workflow Steps:
 * 1. Validate Admin Role (offline user)
 * 2. Validate Current Mode (OFFLINE)
 * 3. Authenticate with Cloud
 * 4. Validate Cloud User Role (must be Organization Admin)
 * 5. Fetch All Facilities & Encounters from Local DB
 * 6. Send Sync Request to Cloud with Email Mappings
 * 7. Clear Local Database (all users, facilities, encounters)
 * 8. Update Mode Flag to ONLINE
 * 9. Notify Success/Failure
 */
public class OfflineToOnlineSyncService {
    private static final String TAG = "OfflineToOnlineSyncService";

    private final Context context;
    private final IStorageManager storageManager;
    private final UsersRepository usersRepository;
    private final FacilityRepository facilityRepository;
    private final EncounterRepository encounterRepository;
    private final OfflineToOnlineSyncRepository syncRepository;
    private final com.accessvascularinc.hydroguide.mma.db.repository.ProcedureCaptureRepository procedureCaptureRepository;

    // Callback interface for sync results
    public interface OfflineToOnlineSyncCallback {
        void onSuccess(String message);

        void onFailure(String errorMessage);

        void onProgress(String progressMessage);
    }

    @Inject
    public OfflineToOnlineSyncService(
            @ApplicationContext Context context,
            IStorageManager storageManager,
            UsersRepository usersRepository,
            FacilityRepository facilityRepository,
            EncounterRepository encounterRepository,
            OfflineToOnlineSyncRepository syncRepository,
            com.accessvascularinc.hydroguide.mma.db.repository.ProcedureCaptureRepository procedureCaptureRepository) {
        this.context = context;
        this.storageManager = storageManager;
        this.usersRepository = usersRepository;
        this.facilityRepository = facilityRepository;
        this.encounterRepository = encounterRepository;
        this.syncRepository = syncRepository;
        this.procedureCaptureRepository = procedureCaptureRepository;
    }

    /**
     * Validates if the logged-in user is an Organization Admin
     */
    private boolean isOrganizationAdmin(UsersEntity currentUser) {
        return currentUser != null &&
                IStorageManager.UserType.Organization_Admin.toString().equals(currentUser.getRole());
    }

    /**
     * Validates that tablet is currently in OFFLINE mode
     */
    private boolean isCurrentlyOffline() {
        IStorageManager.StorageMode currentMode = storageManager.getStorageMode();
        return currentMode == IStorageManager.StorageMode.OFFLINE;
    }

    /**
     * Step 3: Authenticate with Cloud
     */
    private void authenticateWithCloud(String email, String password,
            OfflineToOnlineSyncCallback callback,
            CloudAuthCallback cloudCallback) {
        MedDevLog.info("OfflineToOnlineSync", "Authenticating with cloud server for: " + email);
        callback.onProgress("Authenticating with cloud server...");

        syncRepository.loginToCloud(email, password, (response, error) -> {
            if (error != null || response == null) {
                MedDevLog.warn("OfflineToOnlineSync", "Cloud authentication failed: " +
                        (error != null ? error : "Response is null"));
                callback.onFailure("Cloud authentication failed: " + (error != null ? error : "Unknown error"));
                return;
            }

            MedDevLog.audit("OfflineToOnlineSync", "Cloud authentication successful for: " + email);
            cloudCallback.onCloudAuthSuccess(response);
        });
    }

    /**
     * Step 4: Validate Cloud User and Organization
     */
    private void validateCloudUser(LoginResponse cloudResponse,
            OfflineToOnlineSyncCallback callback) {
        if (cloudResponse == null) {
            MedDevLog.error("OfflineToOnlineSync", "Cloud response is null");
            callback.onFailure("Cloud response is null");
            return;
        }

        // Validate user role is Organization Admin
        String cloudRole = null;
        if (cloudResponse.getData() != null && cloudResponse.getData().getRoles() != null
                && !cloudResponse.getData().getRoles().isEmpty()) {
            cloudRole = cloudResponse.getData().getRoles().get(0).getRole();
        }
        if (cloudRole == null || !IStorageManager.UserType.Organization_Admin.toString().equals(cloudRole)) {
            MedDevLog.warn("OfflineToOnlineSync", "Cloud user is not an Organization Admin. Role: " + cloudRole);
            callback.onFailure("Cloud user is not an Organization Admin. Role: " + cloudRole);
            return;
        }

        // Validate organization ID exists
        String orgId = cloudResponse.getOrganizationId();
        if (orgId == null || orgId.isEmpty()) {
            MedDevLog.error("OfflineToOnlineSync", "Organization ID is missing from cloud response");
            callback.onFailure("Organization ID is missing from cloud response");
            return;
        }

        MedDevLog.debug("OfflineToOnlineSync", "Cloud user validation passed for org: " + orgId);
    }

    /**
     * Step 5: Fetch all Facilities and Encounters from local DB
     */
    private void fetchOfflineRecords(OfflineToOnlineSyncCallback callback,
            RecordsFetchCallback recordsCallback) {
        try {
            List<FacilityEntity> facilities = facilityRepository.getAllFacilities();
            List<EncounterEntity> encounters = encounterRepository.getAllEncounterDataList();

            MedDevLog.debug("OfflineToOnlineSync", "Fetched offline records: " +
                    (facilities != null ? facilities.size() : 0) + " facilities, " +
                    (encounters != null ? encounters.size() : 0) + " encounters");

            callback.onProgress("Preparing " + facilities.size() + " facilities and " +
                    encounters.size() + " encounters for sync...");

            recordsCallback.onRecordsFetched(facilities, encounters);

        } catch (Exception e) {
            MedDevLog.error("OfflineToOnlineSync", "Error fetching offline records: " + e.getMessage(), e);
            callback.onFailure("Error fetching offline records: " + e.getMessage());
        }
    }

    /**
     * Step 6: Send sync request to cloud with email mappings
     * Backend will handle user existence check, user creation, and record mapping
     */
    private void syncRecordsToCloud(List<FacilityEntity> facilities,
            List<EncounterEntity> encounters,
            String organizationId,
            LoginResponse cloudResponse,
            OfflineToOnlineSyncCallback callback) {
        MedDevLog.info("OfflineToOnlineSync", "Syncing " + facilities.size() + " facilities and " +
                encounters.size() + " encounters to cloud");
        callback.onProgress("Uploading data to cloud server...");

        // Extract auth token from cloud response (used for subsequent API calls)
        String authToken = null;
        if (cloudResponse != null && cloudResponse.getData() != null && cloudResponse.getData().getToken() != null) {
            authToken = cloudResponse.getData().getToken();
        }

        syncRepository.syncOfflineDataToCloud(organizationId, facilities, encounters, authToken,
                (success, error) -> {
                    if (success) {
                        MedDevLog.audit("OfflineToOnlineSync", "Offline-to-online sync successful - " +
                                facilities.size() + " facilities, " + encounters.size() + " procedures");
                        callback.onProgress("Records synced successfully...");
                        callback.onSuccess("Records synced to cloud");
                    } else {
                        MedDevLog.warn("OfflineToOnlineSync", "Failed to sync records to cloud: " + error);
                        callback.onFailure("Failed to sync records to cloud: " + error);
                    }
                });
    }

    /**
     * Step 7: Clear local database (all users, facilities, encounters)
     * After successful sync to cloud, tablet operates in ONLINE mode.
     * All data will be fetched from cloud via sync operations.
     * 
     * This operation runs on a background thread to avoid blocking the main thread.
     */
    private void clearLocalDatabase(OfflineToOnlineSyncCallback callback) {
        Handler mainHandler = new Handler(Looper.getMainLooper());

        new Thread(() -> {
            try {
                List<UsersEntity> allUsers = usersRepository.getAllUsers();
                for (UsersEntity user : allUsers) {
                    usersRepository.delete(user);
                }

                List<FacilityEntity> allFacilities = facilityRepository.getAllFacilities();
                for (FacilityEntity facility : allFacilities) {
                    facilityRepository.deleteFacility(facility);
                }

                List<EncounterEntity> allEncounters = encounterRepository.getAllEncounterDataList();
                if (allEncounters != null && !allEncounters.isEmpty()) {
                    for (EncounterEntity encounter : allEncounters) {
                        encounterRepository.deleteEncounter(encounter);
                    }
                }

                try {
                    List<com.accessvascularinc.hydroguide.mma.db.entities.ProcedureCaptureEntity> allCaptures = procedureCaptureRepository
                            .getAllCaptures();
                    if (allCaptures != null) {
                        for (com.accessvascularinc.hydroguide.mma.db.entities.ProcedureCaptureEntity capture : allCaptures) {
                            procedureCaptureRepository.deleteCapture(capture);
                        }
                    }
                } catch (Exception e) {
                    MedDevLog.error("OfflineToOnlineSync", "Error clearing procedure captures: " + e.getMessage(), e);
                }

                MedDevLog.debug("OfflineToOnlineSync", "Local database cleared successfully");
                callback.onProgress("Cleaning up local data...");
                mainHandler.post(() -> {
                    callback.onSuccess("Database cleared successfully");
                });

            } catch (Exception e) {
                MedDevLog.error("OfflineToOnlineSync", "Error clearing local database: " + e.getMessage(), e);
                mainHandler.post(() -> {
                    callback.onFailure("Failed to clear local database: " + e.getMessage());
                });
            }
        }).start();
    }

    /**
     * Step 8: Update mode flag to ONLINE
     */
    private void updateTabletModeFlag(OfflineToOnlineSyncCallback callback) {
        Handler mainHandler = new Handler(Looper.getMainLooper());

        try {
            PrefsManager.setTabletConfigurationStateIsOffline(context, false);
            IStorageManager.StorageMode newMode = storageManager.getStorageMode();

            if (newMode == IStorageManager.StorageMode.ONLINE) {
                MedDevLog.info("OfflineToOnlineSync", "Tablet mode updated to ONLINE");
                callback.onProgress("Switching tablet to online mode...");
                mainHandler.post(() -> {
                    callback.onSuccess("Mode flag updated successfully");
                });
            } else {
                MedDevLog.warn("OfflineToOnlineSync",
                        "Mode update verification failed - expected ONLINE, got: " + newMode);
                mainHandler.post(() -> {
                    callback.onSuccess("Mode flag update attempted (status: " + newMode + ")");
                });
            }
        } catch (Exception e) {
            MedDevLog.error("OfflineToOnlineSync", "Error updating tablet mode flag: " + e.getMessage(), e);
            mainHandler.post(() -> {
                callback.onFailure("Failed to update tablet mode flag: " + e.getMessage());
            });
        }
    }

    /**
     * Main orchestration method: Execute complete Offline → Online mode switch
     * workflow
     *
     * @param currentUser   Logged-in offline user (must be Organization Admin)
     * @param cloudEmail    Cloud user email
     * @param cloudPassword Cloud user password
     * @param callback      Callback for progress and result notifications
     */
    public void switchToOnlineMode(
            UsersEntity currentUser,
            String cloudEmail,
            String cloudPassword,
            OfflineToOnlineSyncCallback callback) {

        MedDevLog.info("OfflineToOnlineSync", "Starting Offline → Online mode switch for user: " +
                (currentUser != null ? currentUser.getEmail() : "Unknown"));

        try {
            java.util.List<FacilityEntity> initialFacilities = facilityRepository.getAllFacilities();
            java.util.List<EncounterEntity> initialEncounters = encounterRepository.getAllEncounterDataList();

            int facilityCount = initialFacilities != null ? initialFacilities.size() : 0;
            int encounterCount = initialEncounters != null ? initialEncounters.size() : 0;

            boolean hasDataToSync = !((initialFacilities == null || initialFacilities.isEmpty())
                    && (initialEncounters == null || initialEncounters.isEmpty()));

            if (!hasDataToSync) {
                MedDevLog.info("OfflineToOnlineSync", "No data to sync - proceeding with authentication only");
                callback.onProgress("No data to sync. Proceeding with cloud authentication...");
            } else {
                MedDevLog.debug("OfflineToOnlineSync",
                        "Data found: " + facilityCount + " facilities, " + encounterCount + " encounters");
                callback.onProgress("Data validation passed. Preparing for sync...");
            }

            // Step 1-2: Validate prerequisites
            if (!isOrganizationAdmin(currentUser)) {
                callback.onFailure("Only Organization Admin can switch modes. Current role: " +
                        (currentUser != null ? currentUser.getRole() : "Unknown"));
                return;
            }
            callback.onProgress("Validating admin privileges...");

            if (!isCurrentlyOffline()) {
                callback.onFailure("Tablet is already in Online mode or mode is unknown. Current mode: " +
                        storageManager.getStorageMode());
                return;
            }
            callback.onProgress("Validating tablet mode...");

            // Step 3: Authenticate with Cloud
            authenticateWithCloud(cloudEmail, cloudPassword, callback,
                    cloudResponse -> {
                        try {
                            if (cloudResponse == null) {
                                callback.onFailure("Cloud response is null");
                                return;
                            }

                            String cloudRole = null;
                            if (cloudResponse.getData() != null && cloudResponse.getData().getRoles() != null
                                    && !cloudResponse.getData().getRoles().isEmpty()) {
                                cloudRole = cloudResponse.getData().getRoles().get(0).getRole();
                            }

                            if (cloudRole == null
                                    || !IStorageManager.UserType.Organization_Admin.toString().equals(cloudRole)) {
                                callback.onFailure("Cloud user is not an Organization Admin. Role: " + cloudRole);
                                return;
                            }

                            String orgId = cloudResponse.getOrganizationId();
                            if (orgId == null || orgId.isEmpty()) {
                                callback.onFailure("Organization ID is missing from cloud response");
                                return;
                            }

                            // Conditional Step 5-6: Only sync if data exists
                            if (hasDataToSync) {
                                fetchOfflineRecords(callback, (facilities, encounters) -> {
                                    try {
                                        syncRecordsToCloud(facilities, encounters,
                                                cloudResponse.getOrganizationId(), cloudResponse,
                                                new OfflineToOnlineSyncCallback() {
                                                    @Override
                                                    public void onSuccess(String message) {
                                                        try {
                                                            clearLocalDatabase(new OfflineToOnlineSyncCallback() {
                                                                @Override
                                                                public void onSuccess(String message) {
                                                                    try {
                                                                        updateTabletModeFlag(
                                                                                new OfflineToOnlineSyncCallback() {
                                                                                    @Override
                                                                                    public void onSuccess(
                                                                                            String message) {
                                                                                        MedDevLog.audit(
                                                                                                "OfflineToOnlineSync",
                                                                                                "Offline → Online mode switch completed. "
                                                                                                        +
                                                                                                        facilities
                                                                                                                .size()
                                                                                                        + " facilities, "
                                                                                                        +
                                                                                                        encounters
                                                                                                                .size()
                                                                                                        + " procedures synced");
                                                                                        callback.onSuccess(
                                                                                                "Tablet successfully switched to Online mode. "
                                                                                                        +
                                                                                                        "You can now sync data with the cloud server.");
                                                                                    }

                                                                                    @Override
                                                                                    public void onFailure(
                                                                                            String errorMessage) {
                                                                                        callback.onFailure(
                                                                                                errorMessage);
                                                                                    }

                                                                                    @Override
                                                                                    public void onProgress(
                                                                                            String progressMessage) {
                                                                                        callback.onProgress(
                                                                                                progressMessage);
                                                                                    }
                                                                                });
                                                                    } catch (Exception e) {
                                                                        MedDevLog.error("OfflineToOnlineSync",
                                                                                "Mode flag update failed: "
                                                                                        + e.getMessage(),
                                                                                e);
                                                                        callback.onFailure(
                                                                                "Failed to update mode flag: "
                                                                                        + e.getMessage());
                                                                    }
                                                                }

                                                                @Override
                                                                public void onFailure(String errorMessage) {
                                                                    MedDevLog.error("OfflineToOnlineSync",
                                                                            "Database clear failed: " + errorMessage);
                                                                    callback.onFailure(errorMessage);
                                                                }

                                                                @Override
                                                                public void onProgress(String progressMessage) {
                                                                    callback.onProgress(progressMessage);
                                                                }
                                                            });
                                                        } catch (Exception e) {
                                                            MedDevLog.error("OfflineToOnlineSync",
                                                                    "Sync process failed: " + e.getMessage(), e);
                                                            callback.onFailure(
                                                                    "Failed to clear database: " + e.getMessage());
                                                        }
                                                    }

                                                    @Override
                                                    public void onFailure(String errorMessage) {
                                                        MedDevLog.warn("OfflineToOnlineSync",
                                                                "Sync to cloud failed: " + errorMessage);
                                                        callback.onFailure("Sync failed: " + errorMessage);
                                                    }

                                                    @Override
                                                    public void onProgress(String progressMessage) {
                                                        callback.onProgress(progressMessage);
                                                    }
                                                });
                                    } catch (Exception e) {
                                        MedDevLog.error("OfflineToOnlineSync",
                                                "Sync process exception: " + e.getMessage(), e);
                                        callback.onFailure("Sync process failed: " + e.getMessage());
                                    }
                                });
                            } else {
                                MedDevLog.info("OfflineToOnlineSync",
                                        "No data available - skipping sync, proceeding with cleanup");
                                callback.onProgress("Cloud authentication successful. No data to sync...");

                                clearLocalDatabase(callback);
                                updateTabletModeFlag(callback);

                                MedDevLog.audit("OfflineToOnlineSync",
                                        "Offline → Online mode switch completed (no data to migrate)");
                                callback.onSuccess(
                                        "Tablet switched to Online mode. No data was available for migration.");
                            }

                        } catch (Exception e) {
                            MedDevLog.error("OfflineToOnlineSync",
                                    "Unexpected error in cloud auth callback: " + e.getMessage(), e);
                            callback.onFailure("Mode switch failed: " + e.getMessage());
                        }
                    });

        } catch (Exception e) {
            MedDevLog.error("OfflineToOnlineSync",
                    "Unexpected error in mode switch: " + e.getMessage(), e);
            callback.onFailure("Mode switch failed: " + e.getMessage());
        }
    }

    // Callback interfaces for nested async operations
    private interface CloudAuthCallback {
        void onCloudAuthSuccess(LoginResponse response);
    }

    private interface RecordsFetchCallback {
        void onRecordsFetched(List<FacilityEntity> facilities, List<EncounterEntity> encounters);
    }
}
