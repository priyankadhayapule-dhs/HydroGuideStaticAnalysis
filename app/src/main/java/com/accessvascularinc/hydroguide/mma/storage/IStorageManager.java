package com.accessvascularinc.hydroguide.mma.storage;

import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel;
import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.CreateClinicianRequest;
import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.CreateClinicianResponse;
import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse;
import com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity;
import com.accessvascularinc.hydroguide.mma.db.entities.UsersEntity;

public interface IStorageManager {
  interface LoginCallback {
    void onResult(LoginResponse response, String errorMessage);
  }

  interface SyncCallback {
    void onResult(boolean success, String errorMessage);
  }

  interface CreateClinicianCallback {
    void onResult(CreateClinicianResponse response, String errorMessage);
  }

  interface RemoveClinicianCallback {
    void onResult(boolean success, String errorMessage);
  }

  interface ResetPasswordCallback {
    void onResult(boolean success, String errorMessage);
  }

  interface FetchClinicianByIdCallback {
    void onResult(ClinicianApiModel clinician, String errorMessage);
  }

  interface GetAllClinicianCallback {
    void onResult(java.util.List<ClinicianApiModel> clinicians, String errorMessage);
  }

  interface GetAllFacilitiesCallback {
    void onResult(java.util.List<FacilityEntity> facilities, String errorMessage);
  }

  StorageMode getStorageMode();

  void login(String username, String password, LoginCallback callback);

  void refreshToken(String refreshToken, LoginCallback callback);

  boolean isSyncAvailable();

  void syncAllData(SyncCallback callback);

  void createClinician(String organizationId, CreateClinicianRequest request,
      CreateClinicianCallback callback);

  void removeClinician(String organizationId, String clinicianId, RemoveClinicianCallback callback);

  void resetPassword(String organizationId, String userId, String plainPassword, String oldUserName, String newUsername,
      ResetPasswordCallback callback);

  void resetPassword(String organizationId, String userId, String plainPassword,String oldUsername, String newUsername, boolean isAdminReset,
      ResetPasswordCallback callback);

  void updateUserPin(String username, String encryptedPin, boolean pinEnabled);

  UsersEntity getUserByUsernameOrEmail(String usernameOrEmail);

  void updateUserTokens(String username, String encryptedToken, String encryptedRefreshToken, Long tokenExpiry);

  void fetchClinicianById(String organizationId, String clinicianId,
      FetchClinicianByIdCallback callback);

  void fetchUsersById(String organizationId, String userId,
                          FetchClinicianByIdCallback callback);

  void getAllClinicians(String organizationId, GetAllClinicianCallback callback);

  void getAllFacilities(String organizationId, GetAllFacilitiesCallback callback);

  void getAllOrganizationUsers(String organizationId, GetAllClinicianCallback callback);

  void createFacility(FacilityEntity facility);

  void updateFacility(FacilityEntity facility);

  void deleteFacility(FacilityEntity facility);

  enum StorageMode {
    OFFLINE,
    ONLINE,
    UNKNOWN
  }

  enum UserType {
    Organization_Admin,
    Clinician
  }
}
