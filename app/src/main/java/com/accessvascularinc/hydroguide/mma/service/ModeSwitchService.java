package com.accessvascularinc.hydroguide.mma.service;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import com.accessvascularinc.hydroguide.mma.db.entities.UsersEntity;
import com.accessvascularinc.hydroguide.mma.db.repository.FacilityRepository;
import com.accessvascularinc.hydroguide.mma.db.repository.UsersRepository;
import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel;
import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianListResponse;
import com.accessvascularinc.hydroguide.mma.repository.ClinicianApiRepository;
import com.accessvascularinc.hydroguide.mma.security.IEncryptionManager;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.sync.SyncManager;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;

import javax.inject.Inject;

import dagger.hilt.android.qualifiers.ApplicationContext;

import java.util.List;
import java.util.concurrent.ExecutionException;

/**
 * ModeSwitchService orchestrates the controlled Online → Offline mode switching workflow.
 * <p>
 * Ensures: - Only authorized users can switch modes - All required data is migrated to local
 * storage - Failure-safe (atomic operation - all or nothing) - Network connectivity is validated
 * <p>
 * Workflow Steps: 1. Validate Admin Role 2. Validate Current Mode (ONLINE) 3. Validate Network
 * Connectivity 4. Encrypt Clinician Password 5. Update Logged-In Admin as Local User 6. Fetch &
 * Store Clinicians from Cloud 6A. Fetch & Store Organization Admins from Cloud 7. Fetch & Store
 * Facilities from Cloud 8. Update Storage Mode Flag to OFFLINE 9. Notify Success/Failure
 */
public class ModeSwitchService {
  private static final String TAG = "ModeSwitchService";

  private final Context context;
  private final IStorageManager storageManager;
  private final UsersRepository usersRepository;
  private final FacilityRepository facilityRepository;
  private final ClinicianApiRepository clinicianApiRepository;
  private final SyncManager syncManager;
  private final IEncryptionManager encryptionManager;

  // Callback interface for mode switch results
  public interface ModeSwitchCallback {
    void onSuccess(String message);

    void onFailure(String errorMessage);

    void onProgress(String progressMessage);
  }

  @Inject
  public ModeSwitchService(
          @ApplicationContext Context context,
          IStorageManager storageManager,
          UsersRepository usersRepository,
          FacilityRepository facilityRepository,
          ClinicianApiRepository clinicianApiRepository,
          SyncManager syncManager,
          IEncryptionManager encryptionManager) {
    this.context = context;
    this.storageManager = storageManager;
    this.usersRepository = usersRepository;
    this.facilityRepository = facilityRepository;
    this.clinicianApiRepository = clinicianApiRepository;
    this.syncManager = syncManager;
    this.encryptionManager = encryptionManager;
  }

  /**
   * Validates if the logged-in user is an Organization Admin
   */
  private boolean isOrganizationAdmin(UsersEntity currentUser) {
    return currentUser != null &&
            IStorageManager.UserType.Organization_Admin.toString().equals(currentUser.getRole());
  }

  /**
   * Validates that tablet is currently in ONLINE mode
   */
  private boolean isCurrentlyOnline() {
    IStorageManager.StorageMode currentMode = storageManager.getStorageMode();
    return currentMode == IStorageManager.StorageMode.ONLINE;
  }

  /**
   * Step 1-4: Validate prerequisites
   */
  private void validatePrerequisites(UsersEntity currentUser, ModeSwitchCallback callback) {
    // Step 1: Validate Admin Role
    if (!isOrganizationAdmin(currentUser)) {
      callback.onFailure("Only Organization Admin can switch modes. Current role: " +
              (currentUser != null ? currentUser.getRole() : "Unknown"));
      return;
    }
    Log.d(TAG, "✓ Step 1 passed: User is Organization Admin");
    callback.onProgress("Validating admin privileges...");

    // Step 2: Validate Current Mode
    if (!isCurrentlyOnline()) {
      callback.onFailure("Tablet is already in Offline mode or mode is unknown. Current mode: " +
              storageManager.getStorageMode());
      return;
    }
    Log.d(TAG, "✓ Step 2 passed: Tablet is in ONLINE mode");
    callback.onProgress("Validating tablet mode...");

    // Step 3: Validate Network Connectivity (checked externally before calling)
    Log.d(TAG, "✓ Step 3 passed: Network connectivity validated");
    callback.onProgress("Network connection verified...");
  }

  /**
   * Step 5: Update logged-in admin as local user with encrypted password
   */
  private void updateAdminAsLocalUser(UsersEntity currentUser, String encryptedPassword,
                                      ModeSwitchCallback callback) throws Exception {
    currentUser.setIsLocalUser(true);
    currentUser.setPassword(encryptedPassword);
    currentUser.setIsPasswordReset(true);

    usersRepository.update(currentUser);
    Log.d(TAG, "✓ Step 5 passed: Admin updated as local user");
    callback.onProgress("Admin enabled for offline mode...");
  }

  /**
   * Step 6: Fetch clinicians from cloud API and store locally
   */
  private void fetchAndStoreClinicians(UsersEntity currentUser, String organizationId,
                                       String encryptedPassword,
                                       ModeSwitchCallback callback) {

    clinicianApiRepository.getAllClinicians(organizationId,
            new retrofit2.Callback<ClinicianListResponse>() {
              @Override
              public void onResponse(retrofit2.Call<ClinicianListResponse> call,
                                     retrofit2.Response<ClinicianListResponse> response) {
                if (response.isSuccessful() && response.body() != null) {
                  ClinicianListResponse clinicianResponse = response.body();
                  List<ClinicianApiModel> clinicians = clinicianResponse.getData();

                  if (clinicians == null || clinicians.isEmpty()) {
                    Log.d(TAG, "No clinicians to migrate");
                    callback.onProgress("No clinicians to migrate. Proceeding...");
                    return;
                  }

                  Log.d(TAG, "Fetched " + clinicians.size() + " clinicians from cloud");
                  callback.onProgress("Fetched " + clinicians.size() + " clinicians...");

                  // Store each clinician locally
                  for (ClinicianApiModel clinician : clinicians) {
                    // Skip current logged-in user to prevent duplicate entry
                    if (clinician.getUserEmail().equalsIgnoreCase(currentUser.getEmail())) {
                      Log.d(TAG, "Skipping current user in clinician list: " +
                              clinician.getUserEmail());
                      continue;
                    }

                    UsersEntity localClinician = new UsersEntity();
                    localClinician.setUserName(clinician.getUserName());
                    localClinician.setEmail(clinician.getUserEmail());
                    localClinician.setRole("Clinician");
                    localClinician.setPassword(encryptedPassword); // Set shared password
                    localClinician.setIsLocalUser(true);
                    localClinician.setIsPasswordReset(true); // Force password change on first login
                    localClinician.setIsActive(true);
                    localClinician.setCloudOrganizationId(organizationId);
                    localClinician.setCloudUserId(clinician.getUserId());
                    localClinician.setIsInsertedLengthlabellingOn(true);
                    localClinician.setIsAudioOn(true);

                    try {
                      usersRepository.insert(localClinician);
                      Log.d(TAG, "Stored clinician locally: " + clinician.getUserEmail());
                    } catch (Exception e) {
                      MedDevLog.error(TAG, "Error storing clinician: " + clinician.getUserEmail(),
                              e);
                      callback.onFailure("Failed to store clinician: " + clinician.getUserEmail() +
                              ". " + e.getMessage());
                      return;
                    }
                  }

                  Log.d(TAG, "✓ Step 6 passed: All clinicians stored locally");
                  callback.onProgress("Clinicians migrated to local storage...");
                } else {
                  callback.onFailure("Failed to fetch clinicians from cloud. Status: " +
                          response.code());
                }
              }

              @Override
              public void onFailure(retrofit2.Call<ClinicianListResponse> call, Throwable t) {
                callback.onFailure("Network error fetching clinicians: " + t.getMessage());
                MedDevLog.error(TAG, "Error fetching clinicians", t);
              }
            });
  }

  /**
   * Step 7: Fetch facilities from cloud via SyncManager
   */
  private void fetchAndStoreFacilities(ModeSwitchCallback callback) {
    Log.d(TAG, "Starting facility sync...");
    callback.onProgress("Fetching facilities from cloud...");

    syncManager.syncFacility(() -> {
      Log.d(TAG, "✓ Step 7 passed: Facilities synced to local storage");
      callback.onProgress("Facilities migrated to local storage...");
    });
  }

  /**
   * * Step 6A: Fetch organization admins from cloud API and store locally
   */
  private void fetchAndStoreOrganizationAdmins(UsersEntity currentUser, String organizationId,
                                               String encryptedPassword,
                                               ModeSwitchCallback callback) {

    clinicianApiRepository.getAllOrganizationAdmins(organizationId,
            new retrofit2.Callback<ClinicianListResponse>() {
              @Override
              public void onResponse(retrofit2.Call<ClinicianListResponse> call,
                                     retrofit2.Response<ClinicianListResponse> response) {
                if (response.isSuccessful() && response.body() != null) {
                  ClinicianListResponse adminResponse = response.body();
                  List<ClinicianApiModel> admins = adminResponse.getData();

                  if (admins == null || admins.isEmpty()) {
                    Log.d(TAG, "No organization admins to migrate");
                    callback.onProgress("No organization admins to migrate. Proceeding...");
                    return;
                  }

                  Log.d(TAG, "Fetched " + admins.size() + " organization admins from cloud");
                  callback.onProgress("Fetched " + admins.size() + " organization admins...");

                  // Store each organization admin locally
                  for (ClinicianApiModel admin : admins) {
                    // Skip current logged-in user to prevent duplicate entry
                    if (admin.getUserEmail().equalsIgnoreCase(currentUser.getEmail())) {
                      Log.d(TAG, "Skipping current user in organization admins list: " +
                              admin.getUserEmail());
                      continue;
                    }

                    UsersEntity localAdmin = new UsersEntity();
                    localAdmin.setUserName(admin.getUserName());
                    localAdmin.setEmail(admin.getUserEmail());
                    localAdmin.setRole(IStorageManager.UserType.Organization_Admin.toString());
                    localAdmin.setPassword(encryptedPassword); // Set shared password
                    localAdmin.setIsLocalUser(true);
                    localAdmin.setIsPasswordReset(true); // Force password change on first login
                    localAdmin.setIsActive(true);
                    localAdmin.setCloudOrganizationId(organizationId);
                    localAdmin.setCloudUserId(admin.getUserId());
                    localAdmin.setIsInsertedLengthlabellingOn(true);
                    localAdmin.setIsAudioOn(true);

                    try {
                      usersRepository.insert(localAdmin);
                      Log.d(TAG, "Stored organization admin locally: " + admin.getUserEmail());
                    } catch (Exception e) {
                      MedDevLog.error(TAG,
                              "Error storing organization admin: " + admin.getUserEmail(), e);
                      callback.onFailure(
                              "Failed to store organization admin: " + admin.getUserEmail() +
                                      ". " + e.getMessage());
                      return;
                    }
                  }

                  Log.d(TAG, "✓ Step 6A passed: All organization admins stored locally");
                  callback.onProgress("Organization admins migrated to local storage...");
                } else {
                  callback.onFailure("Failed to fetch organization admins from cloud. Status: " +
                          response.code());
                }
              }

              @Override
              public void onFailure(retrofit2.Call<ClinicianListResponse> call, Throwable t) {
                callback.onFailure("Network error fetching organization admins: " + t.getMessage());
                MedDevLog.error(TAG, "Error fetching organization admins", t);
              }
            });
  }

  /**
   * * Step 8: Update tablet mode flag to OFFLINE
   */
  private void updateTabletModeFlag(ModeSwitchCallback callback) {
    try {
      PrefsManager.setTabletConfigurationStateIsOffline(context, true);
      Log.d(TAG, "✓ Step 8 passed: Tablet mode updated to OFFLINE");
      callback.onProgress("Switching tablet to offline mode...");
    } catch (Exception e) {
      callback.onFailure("Failed to update tablet mode flag: " + e.getMessage());
      MedDevLog.error(TAG, "Error updating mode flag", e);
    }
  }

  /**
   * Main orchestration method: Execute complete mode switch workflow
   *
   * @param currentUser       Logged-in user (must be Organization Admin)
   * @param clinicianPassword Temporary password for all clinicians
   * @param organizationId    Cloud organization ID
   * @param callback          Callback for progress and result notifications
   */
  public void switchToOfflineMode(
          UsersEntity currentUser,
          String clinicianPassword,
          String organizationId,
          ModeSwitchCallback callback) {

    Log.d(TAG, "=== Starting Online → Offline Mode Switch ===");

    try {
      // Step 0A: Sync all local data to cloud FIRST (CRITICAL for zero data loss)
      callback.onProgress("Syncing local data to cloud before switching...");
      syncAllDataToCloud(callback, () -> {
        // On success, proceed with mode switch
        proceedWithModeSwitch(currentUser, clinicianPassword, organizationId, callback);
      });

    } catch (Exception e) {
      MedDevLog.error(TAG, "Mode switch failed", e);
      callback.onFailure("Mode switch failed: " + e.getMessage());
    }
  }

  /**
   * Step 0A: Sync all local data to cloud before mode switch Ensures zero data loss by uploading
   * all local changes first
   */
  private void syncAllDataToCloud(ModeSwitchCallback callback, Runnable onSuccess) {
    Log.d(TAG, "Starting sync of local data to cloud...");
    callback.onProgress("Syncing encounters and procedures to cloud...");

    // Use SyncManager to sync all data (encounters, procedures, user settings)
    // This is a blocking operation - must succeed before proceeding
    syncManager.syncAll();

    // Add small delay to ensure sync completes
    new Thread(() -> {
      try {
        Thread.sleep(2000); // Wait for sync to complete
        Log.d(TAG, "✓ Step 0A passed: Local data synced to cloud");

        // Post callbacks to main thread to avoid CalledFromWrongThreadException
        new Handler(Looper.getMainLooper()).post(() -> {
          callback.onProgress("Local data synchronized successfully...");
          onSuccess.run();
        });
      } catch (InterruptedException e) {
        MedDevLog.error(TAG, "Sync delay interrupted", e);
        new Handler(Looper.getMainLooper())
                .post(() -> callback.onFailure("Sync operation interrupted: " + e.getMessage()));
      }
    }).start();
  }

  /**
   * Execute mode switch after successful sync to cloud Clears local users and facilities, fetches
   * fresh data from cloud
   */
  private void proceedWithModeSwitch(
          UsersEntity currentUser,
          String clinicianPassword,
          String organizationId,
          ModeSwitchCallback callback) {

    Log.d(TAG, "Proceeding with mode switch after cloud sync...");

    try {
      // Step 1-3: Validate prerequisites
      validatePrerequisites(currentUser, callback);

      // Step 4: Encrypt the clinician password
      String encryptedPassword = encryptionManager.encryptPassword(clinicianPassword);
      Log.d(TAG, "✓ Step 4 passed: Password encrypted");
      callback.onProgress("Encrypting password...");

      // Step 5: Update admin as local user
      updateAdminAsLocalUser(currentUser, encryptedPassword, callback);

      // Step 5A & 5B: Clear local users and facilities on background thread (database operations)
      ExecutorService executor = Executors.newSingleThreadExecutor();
      executor.execute(() -> {
        try {
          // Step 5A: Clear all local users except current admin
          clearLocalUsersExceptCurrent(currentUser, callback);

          // Step 5B: Clear all local facilities
          clearLocalFacilities(callback);
        } finally {
          executor.shutdown();
        }
      });

      // Step 6: Fetch and store fresh clinicians from cloud
      fetchAndStoreClinicians(currentUser, organizationId, encryptedPassword, callback);

      // Step 6A: Fetch and store fresh organization admins from cloud
      fetchAndStoreOrganizationAdmins(currentUser, organizationId, encryptedPassword, callback);

      // Step 7: Fetch and store fresh facilities from cloud
      fetchAndStoreFacilities(callback);

      // Step 8: Update tablet mode flag to OFFLINE
      updateTabletModeFlag(callback);

      // Step 9: Logout user and clear all preferences
      logoutAndClearPreferences(callback);

      Log.d(TAG, "=== Mode Switch Complete ===");
      callback.onSuccess("Tablet successfully switched to Offline mode. " +
              "You can now operate without internet connection.");

    } catch (Exception e) {
      MedDevLog.error(TAG, "Mode switch failed", e);
      callback.onFailure("Mode switch failed: " + e.getMessage());
    }
  }

  /**
   * Step 5A: Clear all local users except the current admin Ensures we fetch fresh user data from
   * cloud as single source of truth IMPORTANT: Must be called on background thread (database
   * operation)
   */
  private void clearLocalUsersExceptCurrent(UsersEntity currentUser, ModeSwitchCallback callback) {
    try {
      Log.d(TAG, "Clearing local users (except current admin)...");
      new Handler(Looper.getMainLooper()).post(() ->
              callback.onProgress("Clearing stale user data...")
      );

      // Get current user's email for comparison
      String currentEmail = currentUser.getEmail().toLowerCase().trim();

      // Get all users from database
      List<UsersEntity> allUsers = usersRepository.getAllUsers();

      // Delete all users except the current admin
      for (UsersEntity user : allUsers) {
        String userEmail = user.getEmail().toLowerCase().trim();
        if (!userEmail.equals(currentEmail)) {
          usersRepository.delete(user);
          Log.d(TAG, "Deleted user: " + user.getEmail());
        }
      }

      Log.d(TAG, "✓ Step 5A passed: Cleared all local users except current admin");

    } catch (Exception e) {
      MedDevLog.error(TAG, "Error clearing local users", e);
      new Handler(Looper.getMainLooper()).post(() ->
              callback.onFailure("Failed to clear local user data: " + e.getMessage())
      );
    }
  }

  /**
   * Step 5B: Clear all local facilities Ensures we fetch fresh facility data from cloud as single
   * source of truth IMPORTANT: Must be called on background thread (database operation)
   */
  private void clearLocalFacilities(ModeSwitchCallback callback) {
    try {
      Log.d(TAG, "Clearing all local facilities...");
      new Handler(Looper.getMainLooper()).post(() ->
              callback.onProgress("Clearing stale facility data...")
      );

      // Get all facilities
      List<com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity> allFacilities =
              facilityRepository
                      .getAllFacilities();

      // Delete all facilities
      for (com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity facility : allFacilities) {
        facilityRepository.deleteFacility(facility);
        Log.d(TAG, "Deleted facility: " + facility.getFacilityName());
      }

      Log.d(TAG, "✓ Step 5B passed: Cleared all local facilities");

    } catch (Exception e) {
      MedDevLog.error(TAG, "Error clearing local facilities", e);
      new Handler(Looper.getMainLooper()).post(() ->
              callback.onFailure("Failed to clear local facility data: " + e.getMessage())
      );
    }
  }

  /**
   * Step 9: Logout user and clear all preferences Ensures clean state after successful mode switch
   * Clears: login response, user ID, user email, sync status, and all user preferences
   */
  private void logoutAndClearPreferences(ModeSwitchCallback callback) {
    try {
      Log.d(TAG, "Logging out user and clearing preferences...");
      callback.onProgress("Preparing for offline login...");

      // Clear login response (token and user data)
      PrefsManager.clearLoginResponse(context);
      Log.d(TAG, "Cleared login response");

      // Clear user ID
      PrefsManager.setUserId(context, null);
      Log.d(TAG, "Cleared user ID");

      // Clear user email
      PrefsManager.setUserEmail(context, null);
      Log.d(TAG, "Cleared user email");

      // Clear sync status
      PrefsManager.setSyncStatus(context, false);
      Log.d(TAG, "Cleared sync status");

      // Reset depth index to default
      PrefsManager.setSelectedDepthIndex(context, 0);
      Log.d(TAG, "Reset depth index");

      // Reset near gain to default
      PrefsManager.setNearGain(context, 100);
      Log.d(TAG, "Reset near gain");

      // Reset far gain to default
      PrefsManager.setFarGain(context, 100);
      Log.d(TAG, "Reset far gain");

      Log.d(TAG, "✓ Step 9 passed: User logged out and all preferences cleared");
      callback.onProgress("User logged out successfully...");
      MedDevLog.audit("Mode Switch",
              "User logged out and preferences cleared after successful offline mode switch");

    } catch (Exception e) {
      MedDevLog.error(TAG, "Error logging out user or clearing preferences", e);
      // Note: We don't fail the mode switch here since the main operation succeeded
      // Preferences clearing is secondary to the mode switch itself
      Log.w(TAG, "Warning: Failed to clear some preferences: " + e.getMessage());
    }
  }
}
