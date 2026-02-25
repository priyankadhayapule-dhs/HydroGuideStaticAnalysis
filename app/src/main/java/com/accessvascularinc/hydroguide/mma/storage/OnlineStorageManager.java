package com.accessvascularinc.hydroguide.mma.storage;

import dagger.hilt.android.qualifiers.ApplicationContext;

import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianResponse;
import com.accessvascularinc.hydroguide.mma.repository.ClinicianApiRepository;
import com.accessvascularinc.hydroguide.mma.repository.LoginRepository;
import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse;
import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.CreateClinicianRequest;
import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.CreateClinicianResponse;
import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianListResponse;

import android.content.Context;

import com.accessvascularinc.hydroguide.mma.sync.SyncManager;

import com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity;
import com.accessvascularinc.hydroguide.mma.db.entities.UsersEntity;
import com.accessvascularinc.hydroguide.mma.db.repository.FacilityRepository;
import com.accessvascularinc.hydroguide.mma.db.repository.UsersRepository;
import com.accessvascularinc.hydroguide.mma.utils.DbHelper;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import javax.inject.Inject;

import retrofit2.Callback;
import retrofit2.Call;
import retrofit2.Response;

/**
 * Online Storage Manager handles cloud-connected operations with local caching for offline
 * resilience.
 * <p>
 * Architecture: - Primary: Cloud API calls via LoginRepository, ClinicianApiRepository for all user
 * and clinician operations - Secondary: Local database caching via UsersRepository and
 * FacilityRepository for offline fallback - Sync: SyncManager handles bidirectional sync of data
 * between cloud and local database
 * <p>
 * User Model (synchronized with cloud): - Only cloud-authenticated users are stored locally
 * (isLocalUser = false) - Users have cloud IDs and tokens for API access - User data is kept in
 * sync through periodic sync operations
 * <p>
 * Use Cases: 1. CLOUD LOGIN: User authenticates against cloud API. Token is returned and user is
 * synced to local DB. 2. OFFLINE FALLBACK: If connection is lost, app can still access cached user
 * and facility data from local DB. 3. CLINICIAN MGMT: Org Admin creates/updates/removes clinicians
 * via cloud API. Changes sync back to local DB. 4. DATA SYNC: Background sync keeps local database
 * updated with cloud changes (facilities, clinicians, etc). 5. TOKEN REFRESH: Tokens are refreshed
 * via cloud API to maintain authentication.
 */
public class OnlineStorageManager implements IStorageManager {
  private final LoginRepository loginRepository;
  private final Context context;
  private final SyncManager syncManager;
  private final UsersRepository usersRepository;
  private final ClinicianApiRepository clinicianApiRepository;
  private final FacilityRepository facilityRepository;

  @Inject
  public OnlineStorageManager(@ApplicationContext Context context, SyncManager syncManager,
                              UsersRepository usersRepository,
                              ClinicianApiRepository clinicianApiRepository,
                              FacilityRepository facilityRepository) {
    this.context = context;
    this.loginRepository = new LoginRepository(context);
    this.syncManager = syncManager;
    this.usersRepository = usersRepository;
    this.clinicianApiRepository = clinicianApiRepository;
    this.facilityRepository = facilityRepository;
  }

  @Override
  public StorageMode getStorageMode() {
    return StorageMode.ONLINE;
  }

  @Override
  public void login(String username, String password, LoginCallback callback) {
      MedDevLog.info("Login", "Attempting online login");
      if (loginRepository != null) {
      loginRepository.login(username, password, (LoginResponse response, String errorMessage) -> {
        if (response != null && response.isSuccess() && response.getData() != null) {
          // Sync cloud login response to local database for offline access and login
          // history.
          // This enables the login screen to display previously-used email addresses for
          // quick selection
          // on returning logins, improving user experience.
          //
          // Logic:
          // - If user previously logged in on this tablet: Update their cloud user data
          // (tokens, roles)
          // - If first-time login on this tablet: Create new local database entry with
          // is_local_user=false
          // and cloud user details for future offline access
          //
          // This runs asynchronously to avoid blocking the login response callback.
          new Thread(() -> {
            String email = response.getData().getEmail();
            String userId = response.getData().getUserId();
            String userName = response.getData().getUserName();
            String role = null;
            String orgId = null;
            if (response.getData().getRoles() != null && !response.getData().getRoles().isEmpty()) {
              role = response.getData().getRoles().get(0).getRole();
              orgId = response.getData().getRoles().get(0).getOrganization_id();
            }
            // Check if user already exists in local database
            UsersEntity existingUser = usersRepository.getUserByEmail(email);
            if (existingUser != null) {
              // Update existing cloud user with new token/role info
              usersRepository.updateCloudLoginUser(existingUser, userName, userId, orgId, role);
            } else {
              // Create new cloud user in local database
              usersRepository.createCloudLoginUser(userName, email, userId, orgId, role);
            }

            MedDevLog.audit("Login", "Logged in " + userId);
            MedDevLog.setLoggedInUserId(userId);
          }).start();
        }
        // Return cloud login response immediately; local sync happens asynchronously
        callback.onResult(response, errorMessage);
      });
    } else {
      callback.onResult(null, "LoginRepository not initialized");
    }
  }

  public void refreshToken(String refreshToken, LoginCallback callback) {
    if (loginRepository != null) {
      loginRepository.refreshToken(refreshToken, (LoginResponse response, String errorMessage) -> {
        if (response != null && response.isSuccess() && response.getData() != null) {
          callback.onResult(response, errorMessage);
        }
      });
    } else {
      callback.onResult(null, "LoginRepository not initialized");
    }
  }

  @Override
  public boolean isSyncAvailable() {
    return true;
  }

  @Override
  public void syncAllData(SyncCallback callback) {
    if (syncManager != null) {
      syncManager.syncAll();
      if (callback != null) {
        callback.onResult(true, null);
      }
    } else {
      if (callback != null) {
        callback.onResult(false, "SyncManager not available");
      }
    }
  }

  @Override
  public void createClinician(String organizationId, CreateClinicianRequest request,
                              CreateClinicianCallback callback) {
    if (clinicianApiRepository != null) {
      clinicianApiRepository.createClinician(organizationId, request,
              new Callback<CreateClinicianResponse>() {
                @Override
                public void onResponse(Call<CreateClinicianResponse> call,
                                       Response<CreateClinicianResponse> response) {
                  if (response.isSuccessful() && response.body() != null) {
                    callback.onResult(response.body(), null);
                  } else {
                    callback.onResult(null, response.message());
                  }
                }

                @Override
                public void onFailure(Call<CreateClinicianResponse> call, Throwable t) {
                  callback.onResult(null, t.getMessage());
                }
              });
    } else {
      if (callback != null) {
        callback.onResult(null, "ClinicianApiRepository not initialized");
      }
    }
  }

  @Override
  public void removeClinician(String organizationId, String clinicianId,
                              RemoveClinicianCallback callback) {
    if (clinicianApiRepository != null) {
      clinicianApiRepository.deleteClinician(organizationId, clinicianId,
              new Callback<ClinicianListResponse>() {
                @Override
                public void onResponse(Call<ClinicianListResponse> call,
                                       Response<ClinicianListResponse> response) {
                  if (response.isSuccessful() && response.body() != null &&
                          response.body().isSuccess()) {
                    callback.onResult(true, null);
                  } else {
                    callback.onResult(false, response.message());
                  }
                }

                @Override
                public void onFailure(Call<ClinicianListResponse> call, Throwable t) {
                  callback.onResult(false, t.getMessage());
                }
              });
    } else {
      if (callback != null) {
        callback.onResult(false, "ClinicianApiRepository not initialized");
      }
    }
  }

  @Override
  public void resetPassword(String organizationId, String userId, String plainPassword,String oldUserName, String newUserName,
                            ResetPasswordCallback callback) {
    resetPassword(organizationId, userId, plainPassword,oldUserName,newUserName, false, callback);
  }

  @Override
  public void resetPassword(String organizationId, String userId, String plainPassword,
                            String oldUserName, String newUsername,
                            boolean isAdminReset,
                            ResetPasswordCallback callback) {
    if (clinicianApiRepository != null) {
      clinicianApiRepository.resetPasswordbyOrgAdmin(organizationId, userId,
              new Callback<CreateClinicianResponse>() {
                @Override
                public void onResponse(Call<CreateClinicianResponse> call,
                                       Response<CreateClinicianResponse> response) {
                  if (response.isSuccessful() && response.body() != null &&
                          response.body().isSuccess()) {
                    callback.onResult(true, null);
                  } else {
                    callback.onResult(false, response.message());
                  }
                }

                @Override
                public void onFailure(Call<CreateClinicianResponse> call, Throwable t) {
                  callback.onResult(false, t.getMessage());
                }
              });
    } else {
      if (callback != null) {
        callback.onResult(false, "ClinicianApiRepository not initialized");
      }
    }
  }

  @Override
  public void fetchClinicianById(String organizationId, String clinicianId,
                                 FetchClinicianByIdCallback callback) {
    if (clinicianApiRepository != null) {
      clinicianApiRepository.getClinicianById(organizationId, clinicianId,
              new Callback<ClinicianListResponse>() {
                @Override
                public void onResponse(Call<ClinicianListResponse> call,
                                       Response<ClinicianListResponse> response) {
                  if (response.isSuccessful() && response.body() != null &&
                          response.body().isSuccess()
                          && response.body().getData() != null &&
                          !response.body().getData().isEmpty()) {
                    callback.onResult(response.body().getData().get(0), null);
                  } else {
                    callback.onResult(null, response.message());
                  }
                }

                @Override
                public void onFailure(Call<ClinicianListResponse> call, Throwable t) {
                  callback.onResult(null, t.getMessage());
                }
              });
    } else {
      if (callback != null) {
        callback.onResult(null, "ClinicianApiRepository not initialized");
      }
    }
  }

  @Override
  public void fetchUsersById(String organizationId, String userId,
                             FetchClinicianByIdCallback callback) {
    if (clinicianApiRepository != null) {
      clinicianApiRepository.getUsersById(organizationId, userId,
              new Callback<ClinicianResponse>() {
                @Override
                public void onResponse(Call<ClinicianResponse> call,
                                       Response<ClinicianResponse> response) {
                  if (response.isSuccessful() && response.body() != null &&
                          response.body().isSuccess()
                          && response.body().getData() != null) {
                    callback.onResult(response.body().getData(), null);
                  } else {
                    callback.onResult(null, response.message());
                  }
                }

                @Override
                public void onFailure(Call<ClinicianResponse> call, Throwable t) {
                  callback.onResult(null, t.getMessage());
                }
              });
    } else {
      if (callback != null) {
        callback.onResult(null, "ClinicianApiRepository not initialized");
      }
    }
  }
  @Override
  public void getAllClinicians(String organizationId, GetAllClinicianCallback callback) {
    if (clinicianApiRepository != null) {
      clinicianApiRepository.getAllClinicians(organizationId,
              new Callback<ClinicianListResponse>() {
                @Override
                public void onResponse(Call<ClinicianListResponse> call,
                                       Response<ClinicianListResponse> response) {
                  if (response.isSuccessful() && response.body() != null &&
                          response.body().isSuccess()) {
                    callback.onResult(response.body().getData(), null);
                  } else {
                    callback.onResult(null, response.message());
                  }
                }

                @Override
                public void onFailure(Call<ClinicianListResponse> call, Throwable t) {
                  callback.onResult(null, t.getMessage());
                }
              });
    } else {
      if (callback != null) {
        callback.onResult(null, "ClinicianApiRepository not initialized");
      }
    }
  }

  @Override
  public void getAllOrganizationUsers(String organizationId, GetAllClinicianCallback callback) {
    if (clinicianApiRepository != null) {
      clinicianApiRepository.getAllOrganizationUsers(organizationId,
              new Callback<>() {
                @Override
                public void onResponse(Call<ClinicianListResponse> call,
                                       Response<ClinicianListResponse> response) {
                  if (response.isSuccessful() && response.body() != null &&
                          response.body().isSuccess()) {
                    callback.onResult(response.body().getData(), null);
                  } else {
                    callback.onResult(null, response.message());
                  }
                }

                @Override
                public void onFailure(Call<ClinicianListResponse> call, Throwable t) {
                  callback.onResult(null, t.getMessage());
                }
              });
    } else {
      if (callback != null) {
        callback.onResult(null, "ClinicianApiRepository not initialized");
      }
    }
  }

  @Override
  public void getAllFacilities(String organizationId, GetAllFacilitiesCallback callback) {
    if (facilityRepository == null) {
      if (callback != null) {
        callback.onResult(null, "FacilityRepository not initialized");
      }
      return;
    }

    android.os.Handler mainHandler = new android.os.Handler(android.os.Looper.getMainLooper());
    new Thread(() -> {
      try {
        java.util.List<FacilityEntity> facilities;
        if (organizationId != null && !organizationId.trim().isEmpty()) {
          facilities = facilityRepository.getFacilitiesByOrganization(organizationId);
        } else {
          facilities = facilityRepository.getAllFacilities();
        }
        if (callback != null) {
          mainHandler.post(() -> callback.onResult(facilities, null));
        }
      } catch (Exception e) {
        if (callback != null) {
          mainHandler.post(() -> callback.onResult(null,
                  "Error retrieving facilities: " + e.getMessage()));
        }
      }
    }).start();
  }

  @Override
  public void createFacility(FacilityEntity facility) {
    if (facilityRepository != null && facility != null) {
      DbHelper.createFacility(facilityRepository, facility);
    }
  }

  @Override
  public void updateFacility(FacilityEntity facility) {
    if (facilityRepository != null && facility != null) {
      DbHelper.updateFacility(facilityRepository, facility);
    }
  }

  @Override
  public void deleteFacility(FacilityEntity facility) {
    if (facilityRepository != null && facility != null) {
      DbHelper.deleteFacility(facilityRepository, facility);
    }
  }

  @Override
  public void updateUserPin(String username, String encryptedPin, boolean pinEnabled) {
    // For online mode, PIN is stored locally in offline database
    // Use usersRepository directly since we don't have offlineStorageManager reference
    if (usersRepository != null) {
      new Thread(() -> {
        usersRepository.updateUserPinByUsername(username, encryptedPin, pinEnabled);
      }).start();
    }
  }

  @Override
  public UsersEntity getUserByUsernameOrEmail(String usernameOrEmail) {
    // For online mode, get from local database
    if (usersRepository != null) {
      return usersRepository.getUserByUsernameOrEmail(usernameOrEmail);
    }
    return null;
  }

  @Override
  public void updateUserTokens(String username, String encryptedToken, String encryptedRefreshToken, Long tokenExpiry) {
    if (usersRepository != null) {
      usersRepository.updateUserTokens(username, encryptedToken, encryptedRefreshToken, tokenExpiry);
    }
  }
}
