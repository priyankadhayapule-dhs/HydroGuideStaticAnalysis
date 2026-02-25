package com.accessvascularinc.hydroguide.mma.storage;

import android.content.Context;

import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;

import javax.inject.Inject;

import dagger.hilt.android.qualifiers.ApplicationContext;

/**
 * Wrapper storage manager that dynamically routes calls to the
 * online or offline implementation based on the current tablet
 * configuration flag in PrefsManager.
 *
 * This allows switching between modes at runtime without needing
 * to recreate the dependency graph or restart the app.
 */
public class ModeAwareStorageManager implements IStorageManager {

    private final Context context;
    private final OnlineStorageManager onlineStorageManager;
    private final OfflineStorageManager offlineStorageManager;

    @Inject
    public ModeAwareStorageManager(@ApplicationContext Context context,
                                   OnlineStorageManager onlineStorageManager,
                                   OfflineStorageManager offlineStorageManager) {
        this.context = context;
        this.onlineStorageManager = onlineStorageManager;
        this.offlineStorageManager = offlineStorageManager;
    }

    private IStorageManager getActiveManager() {
        boolean isOffline = PrefsManager.getTabletConfigurationStateIsOffline(context);
        return isOffline ? offlineStorageManager : onlineStorageManager;
    }

    @Override
    public StorageMode getStorageMode() {
        return PrefsManager.getTabletConfigurationStateIsOffline(context)
                ? StorageMode.OFFLINE
                : StorageMode.ONLINE;
    }

    @Override
    public void login(String username, String password, LoginCallback callback) {
        getActiveManager().login(username, password, callback);
    }

    @Override
    public void refreshToken(String refreshToken, LoginCallback callback) {
        getActiveManager().refreshToken(refreshToken, callback);
    }

    @Override
    public boolean isSyncAvailable() {
        return getActiveManager().isSyncAvailable();
    }

    @Override
    public void syncAllData(SyncCallback callback) {
        getActiveManager().syncAllData(callback);
    }

    @Override
    public void createClinician(String organizationId,
                                com.accessvascularinc.hydroguide.mma.network.ClinicianModel.CreateClinicianRequest request,
                                CreateClinicianCallback callback) {
        getActiveManager().createClinician(organizationId, request, callback);
    }

    @Override
    public void removeClinician(String organizationId, String clinicianId, RemoveClinicianCallback callback) {
        getActiveManager().removeClinician(organizationId, clinicianId, callback);
    }

    @Override
    public void resetPassword(String organizationId, String userId, String plainPassword, String oldUserName,String newUserName,
                              ResetPasswordCallback callback) {
        getActiveManager().resetPassword(organizationId, userId, plainPassword, oldUserName,newUserName, callback);
    }

    @Override
    public void resetPassword(String organizationId, String userId, String plainPassword,String oldUserName, String newUsername,
                              boolean isAdminReset, ResetPasswordCallback callback) {
        getActiveManager().resetPassword(organizationId, userId, plainPassword,oldUserName, newUsername, isAdminReset, callback);
    }

    @Override
    public void fetchClinicianById(String organizationId, String clinicianId,
                                   FetchClinicianByIdCallback callback) {
        getActiveManager().fetchClinicianById(organizationId, clinicianId, callback);
    }

    @Override
    public void fetchUsersById(String organizationId, String clinicianId,
                               FetchClinicianByIdCallback callback) {
        getActiveManager().fetchUsersById(organizationId, clinicianId, callback);
    }

    @Override
    public void getAllClinicians(String organizationId, GetAllClinicianCallback callback) {
        getActiveManager().getAllClinicians(organizationId, callback);
    }
    @Override
    public void getAllOrganizationUsers(String organizationId, GetAllClinicianCallback callback) {
        getActiveManager().getAllOrganizationUsers(organizationId, callback);
    }

    @Override
    public void getAllFacilities(String organizationId, GetAllFacilitiesCallback callback) {
        getActiveManager().getAllFacilities(organizationId, callback);
    }

    @Override
    public void createFacility(com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity facility) {
        getActiveManager().createFacility(facility);
    }

    @Override
    public void updateFacility(com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity facility) {
        getActiveManager().updateFacility(facility);
    }

    @Override
    public void deleteFacility(com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity facility) {
        getActiveManager().deleteFacility(facility);
    }

    @Override
    public void updateUserPin(String username, String encryptedPin, boolean pinEnabled) {
        getActiveManager().updateUserPin(username, encryptedPin, pinEnabled);
    }

    @Override
    public com.accessvascularinc.hydroguide.mma.db.entities.UsersEntity getUserByUsernameOrEmail(
            String usernameOrEmail) {
        return getActiveManager().getUserByUsernameOrEmail(usernameOrEmail);
    }

    @Override
    public void updateUserTokens(String username, String encryptedToken, String encryptedRefreshToken, Long tokenExpiry) {
        getActiveManager().updateUserTokens(username, encryptedToken, encryptedRefreshToken, tokenExpiry);
    }
}

