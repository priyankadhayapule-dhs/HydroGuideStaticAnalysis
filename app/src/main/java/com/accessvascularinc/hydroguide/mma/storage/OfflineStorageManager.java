package com.accessvascularinc.hydroguide.mma.storage;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.Toast;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.CreateClinicianRequest;
import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel;
import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.CreateClinicianResponse;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager.LoginCallback;
import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse;

import android.content.Context;

import com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity;
import com.accessvascularinc.hydroguide.mma.db.entities.UsersEntity;
import com.accessvascularinc.hydroguide.mma.db.repository.FacilityRepository;
import com.accessvascularinc.hydroguide.mma.db.repository.UsersRepository;
import com.accessvascularinc.hydroguide.mma.security.IEncryptionManager;
import com.accessvascularinc.hydroguide.mma.utils.DbHelper;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import dagger.hilt.android.qualifiers.ApplicationContext;

import javax.inject.Inject;

import java.util.ArrayList;
import java.util.List;

public class OfflineStorageManager implements IStorageManager {
  private static final String TAG = "OfflineStorageManager";

  /**
   * Offline Storage Manager handles local-only authentication and user management
   * for tablet mode.
   * <p>
   * User Model: - Organization_Admin: The first user created during offline
   * setup. Has permissions
   * to manage other users (create clinicians, reset passwords, remove users).
   * This is a bootstrap
   * user. - Clinician: Created by the Organization_Admin. These are standard
   * users who log in and
   * perform clinical work. They cannot manage other users.
   * <p>
   * Use Cases: 1. OFFLINE SETUP: Tablet is configured with no internet. An admin
   * creates the first
   * Organization_Admin account via OfflineAdminSetupFragment to bootstrap the
   * system. 2. CLINICIAN
   * CREATION: The Organization_Admin logs in and creates one or more Clinician
   * accounts via the
   * clinician management UI. 3. CLINICIAN LOGIN: Clinicians log in using their
   * email and password.
   * Authentication is performed against the local database (encrypted password
   * verification). 4.
   * PASSWORD RESET: Organization_Admin can reset any clinician's password.
   * Clinicians can change
   * their own password if not system-generated.
   */
  private final Context context;
  private final UsersRepository usersRepository;
  private final IEncryptionManager encryptionManager;
  private final FacilityRepository facilityRepository;

  @Inject
  public OfflineStorageManager(@ApplicationContext Context context,
      UsersRepository usersRepository,
      IEncryptionManager encryptionManager,
      FacilityRepository facilityRepository) {
    this.context = context;
    this.usersRepository = usersRepository;
    this.encryptionManager = encryptionManager;
    this.facilityRepository = facilityRepository;
  }

  /**
   * Safely executes callback, logging a warning if callback is null. This helps
   * catch cases where
   * callers forget to provide a callback.
   */
  private void safeCallback(String method, Runnable callbackAction) {
    if (callbackAction != null) {
      callbackAction.run();
    } else {
      MedDevLog.warn(TAG, "Callback is null in method: " + method);
    }
  }

  @Override
  public StorageMode getStorageMode() {
    return StorageMode.OFFLINE;
  }

  private UsersEntity currentOfflineUser; // Store authenticated user for role-based navigation

  /**
   * Authenticates a user for offline login. Only allows locally created users.
   * Supports login with
   * either username or email address. Performs password validation against
   * encrypted password
   * stored in local database. This is only used in offline mode where cloud
   * authentication is not
   * available.
   *
   * @param username User's username or email address (can be either)
   * @param password User's password in plain text
   * @param callback Returns LoginResponse with user details if successful, or
   *                 error message
   */
  @Override
  public void login(String username, String password, LoginCallback callback) {
    MedDevLog.info("Login", "Attempting offline login");
    if (usersRepository == null) {
      safeCallback("login",
          () -> callback.onResult(null, context.getString(R.string.error_repository_null)));
      return;
    }

    // Support login with either username or email
    String usernameOrEmail = username != null ? username.trim() : "";
    password = password != null ? password : "";

    if (usernameOrEmail.isEmpty() || password.isEmpty()) {
      safeCallback("login",
          () -> callback.onResult(null, "Username/Email and password are required"));
      return;
    }

    // Perform async login validation - check if user exists (by username or email)
    // and password is correct
    usersRepository.loginAsyncWithUsernameOrEmail(usernameOrEmail, password,
        (user, errorMessage) -> {
          if (user != null) {
            // User exists and password is valid - store user and build response
            currentOfflineUser = user;
            LoginResponse response = buildOfflineLoginResponse(user);
            MedDevLog.audit("Login", "Logged in " + response.getData().getUserId());
            MedDevLog.setLoggedInUserId(response.getData().getUserId());
            safeCallback("login", () -> callback.onResult(response, null));
          } else {
            // Return error message from repository
            currentOfflineUser = null;
            safeCallback("login", () -> callback.onResult(null, errorMessage));
          }
        });
  }

  /**
   * Builds a complete LoginResponse for offline users matching cloud login
   * response structure
   */
  private LoginResponse buildOfflineLoginResponse(UsersEntity user) {
    try {
      LoginResponse response = new LoginResponse();

      // Create Data object
      LoginResponse.Data data = new LoginResponse.Data();
      setFieldValue(data, "email", user.getEmail());
      setFieldValue(data, "userId", String.valueOf(user.getUserId()));
      setFieldValue(data, "userName", user.getUserName());
      setFieldValue(data, "token", generateOfflineToken(user));
      boolean requiresPasswordChange = Boolean.TRUE.equals(user.getIsPasswordReset());
      setFieldValue(data, "isSystemGeneratedPassword", requiresPasswordChange);

      // Create Role object
      LoginResponse.Role role = new LoginResponse.Role();
      setFieldValue(role, "role", user.getRole() != null ? user.getRole() : "Clinician");
      setFieldValue(role, "organization_id",
          user.getCloudOrganizationId() != null ? user.getCloudOrganizationId() : "");

      // Add role to list
      java.util.List<LoginResponse.Role> roles = new java.util.ArrayList<>();
      roles.add(role);
      setFieldValue(data, "roles", roles);

      // Set response fields
      setFieldValue(response, "data", data);
      setFieldValue(response, "success", true);
      setFieldValue(response, "errorMessage", "");
      setFieldValue(response, "additionalInfo", "");

      return response;
    } catch (Exception e) {
      return null;
    }
  }

  /**
   * Sets private field values using reflection
   */
  private void setFieldValue(Object obj, String fieldName, Object value) throws Exception {
    java.lang.reflect.Field field = obj.getClass().getDeclaredField(fieldName);
    field.setAccessible(true);
    field.set(obj, value);
  }

  /**
   * Generates offline token - mock JWT token for offline mode Format:
   * offline_{userId}_{timestamp}
   */
  private String generateOfflineToken(UsersEntity user) {
    return "offline_" + user.getUserId() + "_" + System.currentTimeMillis();
  }

  @Override
  public boolean isSyncAvailable() {
    return false;
  }

  @Override
  public void syncAllData(SyncCallback callback) {
    if (callback != null) {
      callback.onResult(false, context.getString(R.string.error_sync_not_available));
    }
  }

  @Override
  public void refreshToken(String refereshToken, LoginCallback callback) {
    if (callback != null) {
      callback.onResult(null, "Refresh not available in offline mode");
    }
  }

  @Override
  public void createClinician(String organizationId, CreateClinicianRequest request,
      CreateClinicianCallback callback) {
    // Creates a new Clinician user account. Only Organization_Admin users can call
    // this.
    // In offline mode, clinician details are persisted to the local database.
    // The request contains: name, email, password, and role (should be
    // "Clinician").
    if (request != null && usersRepository != null) {
      try {
        String email = request.getEmail().toLowerCase();
        String userName = request.getName();
        String password = request.getPassword();
        String role = request.getRoleName();
        String OrganizationId = request.getOrganizationId();

        usersRepository.createLocalTabletUser(userName, email, password, role, OrganizationId,
            (success, message) -> {
              if (callback != null) {
                if (success) {
                  CreateClinicianResponse response = new CreateClinicianResponse();
                  response.setSuccess(true);
                  callback.onResult(response, null);
                } else {
                  callback.onResult(null, message);
                }
              }
            });
      } catch (Exception e) {
        if (callback != null) {
          callback.onResult(null, "Error creating clinician: " + e.getMessage());
        }
      }
    } else {
      if (callback != null) {
        callback.onResult(null, context.getString(R.string.error_invalid_request));
      }
    }
  }

  @Override
  public void removeClinician(String organizationId, String clinicianId,
      RemoveClinicianCallback callback) {
    if (usersRepository == null || clinicianId == null) {
      if (callback != null) {
        callback.onResult(false, context.getString(R.string.error_repository_null));
      }
      return;
    }

    long userId = parseClinicianId(clinicianId);
    if (userId == -1) {
      if (callback != null) {
        callback.onResult(false, context.getString(R.string.error_invalid_id));
      }
      return;
    }

    usersRepository.deleteUserAsync(userId, (success, errorMessage) -> {
      if (callback != null) {
        callback.onResult(success,
            errorMessage != null ? errorMessage
                : (success ? context.getString(R.string.error_clinician_removed) : "Failed to remove clinician"));
      }
    });
  }

  @Override
  public void resetPassword(String organizationId, String userId, String plainPassword, String oldUserName, String newUserName,
      ResetPasswordCallback callback) {
    resetPassword(organizationId, userId, plainPassword, oldUserName, newUserName, false, callback);
  }

  @Override
  public void resetPassword(String organizationId, String userId, String plainPassword, String oldUserName, String newUserName,
      boolean isAdminReset,
      ResetPasswordCallback callback) {
    if (usersRepository == null || userId == null || plainPassword == null) {
      if (callback != null) {
        callback.onResult(false, context.getString(R.string.error_reset_password));
      }
      return;
    }

    // Parse userId to Long
    long parsedUserId = parseClinicianId(userId);
    if (parsedUserId == -1) {
      if (callback != null) {
        callback.onResult(false, "Invalid user ID");
      }
      return;
    }

    // Update password in local database
    usersRepository.updatePasswordAsync(parsedUserId, plainPassword, isAdminReset,oldUserName,newUserName,
        (success, message) -> {
          if (callback != null) {
            callback.onResult(success, message);
          }
        });
  }

  @Override
  public void fetchClinicianById(String organizationId, String clinicianId,
      FetchClinicianByIdCallback callback) {
    if (usersRepository == null || clinicianId == null) {
      if (callback != null) {
        callback.onResult(null, context.getString(R.string.error_repository_null));
      }
      return;
    }

    long userId = parseClinicianId(clinicianId);
    if (userId == -1) {
      if (callback != null) {
        callback.onResult(null, context.getString(R.string.error_invalid_id));
      }
      return;
    }

    usersRepository.fetchUserByIdAsync(userId, (user, errorMessage) -> {
      if (callback != null) {
        if (user != null) {
          callback.onResult(convertUserToClinician(user), null);
        } else {
          callback.onResult(null, errorMessage != null ? errorMessage : context.getString(R.string.error_not_found));
        }
      }
    });
  }

  @Override
  public void fetchUsersById(String organizationId, String clinicianId,
      FetchClinicianByIdCallback callback) {
    fetchClinicianById(organizationId, clinicianId, callback);
  }

  /**
   * Converts UsersEntity to ClinicianApiModel for offline mode.
   * <p>
   * This transforms a database entity (which could be any user type) into an
   * API-safe
   * representation for clinician data. Only clinician-relevant fields are
   * exposed: userId,
   * userName, and userEmail. Sensitive fields like password hashes are excluded.
   * <p>
   * Use Case: When listing or fetching clinicians (getAllClinicians,
   * fetchClinicianById), we
   * retrieve UsersEntity records from the database and convert them to
   * ClinicianApiModel for
   * returning to the UI/API callers. This provides clean separation between the
   * database layer and
   * the API/UI layer.
   */
  private ClinicianApiModel convertUserToClinician(UsersEntity user) {
    if (user == null) {
      return null;
    }

    ClinicianApiModel clinician = new ClinicianApiModel();
    clinician.setUserId(String.valueOf(user.getUserId()));
    clinician.setUserName(user.getUserName());
    clinician.setUserEmail(user.getEmail());
    clinician.setCreatedOn(user.getCreatedOn());
    List<ClinicianApiModel.Role> roles = new ArrayList<>();
    ClinicianApiModel.Role role = new ClinicianApiModel.Role();
    role.setRole(user.getRole());
    roles.add(role);
    clinician.setRoles(roles);
    return clinician;
  }

  /**
   * Converts UsersEntity list to ClinicianApiModel list - reuses single
   * conversion method
   */
  private List<ClinicianApiModel> convertUsersToClinicians(List<UsersEntity> users) {
    List<ClinicianApiModel> clinicians = new ArrayList<>();
    for (UsersEntity user : users) {
      ClinicianApiModel clinician = convertUserToClinician(user);
      if (clinician != null) {
        clinicians.add(clinician);
      }
    }
    return clinicians;
  }

  /**
   * Safely converts String ID to Long, returns -1 on error
   */
  private long parseClinicianId(String clinicianId) {
    try {
      return Long.parseLong(clinicianId);
    } catch (NumberFormatException e) {
      return -1;
    }
  }

  @Override
  public void getAllClinicians(String organizationId, GetAllClinicianCallback callback) {
    if (usersRepository != null) {
      usersRepository.fetchAllLocalActiveClinicians((users, errorMessage) -> {
        if (callback != null) {
          if (users != null && !users.isEmpty()) {
            List<ClinicianApiModel> clinicians = convertUsersToClinicians(users);
            callback.onResult(clinicians, null);
          } else if (users != null && users.isEmpty()) {
            callback.onResult(new ArrayList<>(), null);
          } else {
            callback.onResult(null,
                errorMessage != null ? errorMessage : "Error retrieving clinicians");
          }
        }
      });
    } else {
      if (callback != null) {
        callback.onResult(null, context.getString(R.string.error_repository_null));
      }
    }
  }

  @Override
  public void getAllOrganizationUsers(String organizationId, GetAllClinicianCallback callback) {
    if (usersRepository != null) {
      usersRepository.fetchAllLocalActiveUsers((users, errorMessage) -> {
        if (callback != null) {
          if (users != null && !users.isEmpty()) {
            List<ClinicianApiModel> clinicians = convertUsersToClinicians(users);
            callback.onResult(clinicians, null);
          } else if (users != null && users.isEmpty()) {
            callback.onResult(new ArrayList<>(), null);
          } else {
            callback.onResult(null,
                errorMessage != null ? errorMessage : "Error retrieving clinicians");
          }
        }
      });
    } else {
      if (callback != null) {
        callback.onResult(null, context.getString(R.string.error_repository_null));
      }
    }
  }

  @Override
  public void getAllFacilities(String organizationId, GetAllFacilitiesCallback callback) {
    if (facilityRepository == null) {
      if (callback != null) {
        callback.onResult(null, context.getString(R.string.facility_repository_null));
      }
      return;
    }

    Handler mainHandler = new Handler(Looper.getMainLooper());
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
    if (usersRepository != null && username != null) {
      new Thread(() -> {
        try {
          usersRepository.updateUserPinByUsername(username, encryptedPin, pinEnabled);
          Log.d(TAG, "PIN updated for user: " + username);
        } catch (Exception e) {
          Log.e(TAG, "Error updating PIN", e);
        }
      }).start();
    }
  }

  @Override
  public UsersEntity getUserByUsernameOrEmail(String usernameOrEmail) {
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
