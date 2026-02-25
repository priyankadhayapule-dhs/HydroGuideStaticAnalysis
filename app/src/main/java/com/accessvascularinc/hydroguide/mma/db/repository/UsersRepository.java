package com.accessvascularinc.hydroguide.mma.db.repository;

import android.app.Application;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;

import androidx.lifecycle.LiveData;

import com.accessvascularinc.hydroguide.mma.db.dao.UsersDao;
import com.accessvascularinc.hydroguide.mma.db.entities.UsersEntity;
import com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase;
import com.accessvascularinc.hydroguide.mma.security.IEncryptionManager;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import java.text.SimpleDateFormat;

import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class UsersRepository {
  private final UsersDao usersDao;
  private final ExecutorService executorService;
  private static final Handler mainHandler = new Handler(Looper.getMainLooper());
  private final IEncryptionManager encryptionManager;

  public interface CreateLocalUserCallback {
    void onResult(boolean success, String message);
  }

  public interface FetchAllActiveCliniciansCallback {
    void onResult(List<UsersEntity> users, String errorMessage);
  }

  public interface FetchUserByIdCallback {
    void onResult(UsersEntity user, String errorMessage);
  }

  public interface DeleteUserCallback {
    void onResult(boolean success, String errorMessage);
  }

  public interface LoginCallback {
    void onResult(UsersEntity user, String errorMessage);
  }

  public interface ResetPasswordCallback {
    void onResult(boolean success, String errorMessage);
  }

  public UsersRepository(HydroGuideDatabase db, IEncryptionManager encryptionManager) {
    this.usersDao = db.usersDao();
    this.executorService = Executors.newSingleThreadExecutor();
    this.encryptionManager = encryptionManager;
  }

  public void insert(final UsersEntity user) {
    executorService.execute(() -> usersDao.insertUser(user));
    MedDevLog.audit("Users", "Created user: " + user.getUserName() +
        ", Email: " + user.getEmail());
  }

  public void update(final UsersEntity user) {
    executorService.execute(() -> usersDao.updateUser(user));
  }

  public void delete(final UsersEntity user) {
    executorService.execute(() -> usersDao.deleteUser(user));
  }

  public UsersEntity getUserById(long userId) {
    return usersDao.getUserById(userId);
  }

  public UsersEntity getUserByEmail(String email) {
    String normalizedEmail = email != null ? email.toLowerCase().trim() : "";
    return usersDao.getUserByEmail(normalizedEmail);
  }

  public void fetchUserByEmailAsync(String email, FetchUserByIdCallback callback) {
    executorService.execute(() -> {
      try {
        String normalizedEmail = email != null ? email.toLowerCase().trim() : "";
        android.util.Log.d("UsersRepository",
            "[DB_QUERY_ASYNC] Searching for user by email: " + normalizedEmail);
        UsersEntity user = usersDao.getUserByEmail(normalizedEmail);
        postToMainThread(() -> {
          if (user != null) {
            android.util.Log.d("UsersRepository",
                "[DB_QUERY_ASYNC_SUCCESS] Found user: " + user.getUserName() + ", Role: " +
                    user.getRole());
            callback.onResult(user, null);
          } else {
            MedDevLog.error("UsersRepository",
                "[DB_QUERY_ASYNC_FAILED] No user found with email: " + normalizedEmail);
            callback.onResult(null, "User not found by email");
          }
        });
      } catch (Exception e) {
        postToMainThread(() -> callback.onResult(null,
            "Error fetching user: " + e.getMessage()));
      }
    });
  }

  public boolean userExistsByEmail(String email) {
    String normalizedEmail = email != null ? email.toLowerCase().trim() : "";
    return usersDao.getUserByEmail(normalizedEmail) != null;
  }

  public void createCloudLoginUser(String userName, String email, String userId, String orgId,
      String role) {
    MedDevLog.info("User", "Creating cloud login user: " + userName +
        ", UserID: " + userId + ", OrgID: " + orgId + ", Role: " + role);
    UsersEntity newUser = new UsersEntity();
    newUser.setUserName(userName);
    // Normalize email to lowercase for consistency
    newUser.setEmail(email != null ? email.toLowerCase().trim() : "");
    newUser.setCloudUserId(userId);
    newUser.setCloudOrganizationId(orgId);
    newUser.setRole(role);
    newUser.setIsLocalUser(false);
    newUser.setIsActive(true);
    newUser.setIsInsertedLengthlabellingOn(true);
    newUser.setIsAudioOn(true);
    newUser.setCreatedOn(new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS", java.util.Locale.US).format(new java.util.Date()));
    insert(newUser);
  }

  public void createLocalTabletUser(String userName, String email, String password, String role,
      String organizationId,
      CreateLocalUserCallback callback) {
    MedDevLog.info("User", "Creating local tablet user: " + userName + ", Role: " + role);
    executorService.execute(() -> {
      try {
        // Normalize email to lowercase for consistency
        String normalizedEmail = email != null ? email.toLowerCase().trim() : "";

        // Check if user with same email already exists
        UsersEntity existingUser = usersDao.getUserByEmail(normalizedEmail);
        if (existingUser != null) {
          postToMainThread(() -> callback.onResult(false, "A user with this email already exists"));
          return;
        }

        // Create new user
        UsersEntity newUser = new UsersEntity();
        newUser.setUserName(userName);
        newUser.setEmail(normalizedEmail);

        // Encrypt password before storing
        String plainPassword = password;
        String encryptedPassword = encryptionManager.encryptPassword(plainPassword);
        newUser.setPassword(encryptedPassword);

        newUser.setRole(role);
        newUser.setIsLocalUser(true);
        newUser.setIsActive(true);
        newUser.setIsPasswordReset(true);
        newUser.setIsInsertedLengthlabellingOn(true);
        newUser.setIsAudioOn(true);
        newUser.setCloudOrganizationId(organizationId);
        newUser.setCreatedOn(new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS", java.util.Locale.US).format(new java.util.Date()));

        usersDao.insertUser(newUser);
        postToMainThread(() -> callback.onResult(true, "Clinician created successfully"));
      } catch (Exception e) {
        postToMainThread(() -> callback.onResult(false,
            "Error creating clinician: " + e.getMessage()));
      }
    });
  }

  public void updateCloudLoginUser(UsersEntity user, String userName, String userId, String orgId,
      String role) {
    user.setUserName(userName);
    user.setCloudUserId(userId);
    user.setCloudOrganizationId(orgId);
    user.setRole(role);
    update(user);
  }

  public List<UsersEntity> getAllUsers() {
    return usersDao.getAllUsers();
  }

  public List<UsersEntity> fetchAllActiveUsers() {
    return usersDao.fetchAllActiveUsers();
  }

  /**
   * Posts a runnable to the main thread
   */
  private void postToMainThread(Runnable runnable) {
    mainHandler.post(runnable);
  }

  public void fetchAllLocalActiveClinicians(FetchAllActiveCliniciansCallback callback) {
    executorService.execute(() -> {
      try {
        List<UsersEntity> activeUsers = usersDao.fetchAllLocalActiveClinicians();
        postToMainThread(() -> callback.onResult(activeUsers, null));
      } catch (Exception e) {
        postToMainThread(() -> callback.onResult(null,
            "Error fetching active users: " + e.getMessage()));
      }
    });
  }

  public void fetchAllLocalActiveUsers(FetchAllActiveCliniciansCallback callback) {
    executorService.execute(() -> {
      try {
        List<UsersEntity> activeUsers = usersDao.fetchAllActiveUsers();
        postToMainThread(() -> callback.onResult(activeUsers, null));
      } catch (Exception e) {
        postToMainThread(() -> callback.onResult(null,
            "Error fetching active users: " + e.getMessage()));
      }
    });
  }

  public void fetchUserByIdAsync(long userId, FetchUserByIdCallback callback) {
    executorService.execute(() -> {
      try {
        UsersEntity user = usersDao.getUserById(userId);
        postToMainThread(() -> {
          if (user != null) {
            callback.onResult(user, null);
          } else {
            callback.onResult(null, "User not found by ID");
          }
        });
      } catch (Exception e) {
        postToMainThread(() -> callback.onResult(null,
            "Error fetching user: " + e.getMessage()));
      }
    });
  }

  public void deleteUserAsync(long userId, DeleteUserCallback callback) {
    executorService.execute(() -> {
      try {
        UsersEntity user = usersDao.getUserById(userId);
        if (user != null) {
          user.setIsActive(false);
          usersDao.updateUser(user);
          postToMainThread(() -> callback.onResult(true, null));
        } else {
          postToMainThread(() -> callback.onResult(false, "User not found by ID"));
        }
      } catch (Exception e) {
        postToMainThread(() -> callback.onResult(false,
            "Error deleting user: " + e.getMessage()));
      }
    });
  }

  /**
   * Authenticates a user for offline login. Only allows locally created users to
   * log in. This
   * method is only used in offline mode where cloud authentication is not
   * available.
   *
   * @param email    The user's email
   * @param password The user's password in plain text
   * @param callback Callback to invoke with login result
   */
  public void loginAsync(String email, String password, LoginCallback callback) {
    executorService.execute(() -> {
      try {
        if (email == null || email.trim().isEmpty() || password == null || password.isEmpty()) {
          postToMainThread(() -> callback.onResult(null, "Email and password are required"));
          return;
        }

        String normalizedEmail = email.toLowerCase().trim();
        UsersEntity user = usersDao.getUserByEmail(normalizedEmail);

        if (user == null) {
          postToMainThread(() -> callback.onResult(null, "User not found by email"));
          return;
        }
        if (user.getIsActive() == null || !user.getIsActive()) {
          postToMainThread(() -> callback.onResult(null, "User account is inactive"));
          return;
        }
        if (user.getIsLocalUser() == null || !user.getIsLocalUser()) {
          postToMainThread(() -> callback.onResult(null,
              "Only locally created users can login in offline mode"));
          return;
        }
        String storedPassword = user.getPassword();

        // Check if password was stored before attempting verification
        if (storedPassword == null) {
          postToMainThread(() -> callback.onResult(null, "Invalid password"));
          return;
        }

        boolean passwordMatches = encryptionManager.verifyPassword(password, storedPassword);
        if (!passwordMatches) {
          postToMainThread(() -> callback.onResult(null, "Invalid password"));
          return;
        }
        postToMainThread(() -> callback.onResult(user, null));
      } catch (Exception e) {
        postToMainThread(() -> callback.onResult(null,
            "Error during login: " + e.getMessage()));
      }
    });
  }

  /**
   * Authenticates a user for offline login using username OR email. Only allows
   * locally created
   * users to log in. This method supports flexible authentication with either
   * username or email
   * address. Username matching is CASE-SENSITIVE, email matching is
   * CASE-INSENSITIVE. This is only
   * used in offline mode where cloud authentication is not available.
   *
   * @param usernameOrEmail The user's username or email address
   * @param password        The user's password in plain text
   * @param callback        Callback to invoke with login result
   */
  public void loginAsyncWithUsernameOrEmail(String usernameOrEmail, String password,
      LoginCallback callback) {
    executorService.execute(() -> {
      try {
        if (usernameOrEmail == null || usernameOrEmail.trim().isEmpty() || password == null ||
            password.isEmpty()) {
          postToMainThread(
              () -> callback.onResult(null, "Username/Email and password are required"));
          return;
        }

        String input = usernameOrEmail.trim();
        UsersEntity user = usersDao.getUserByUsernameOrEmail(input);

        if (user == null) {
          postToMainThread(() -> callback.onResult(null, "User not found"));
          return;
        }
        if (user.getIsActive() == null || !user.getIsActive()) {
          postToMainThread(() -> callback.onResult(null, "User account is inactive"));
          return;
        }
        if (user.getIsLocalUser() == null || !user.getIsLocalUser()) {
          postToMainThread(() -> callback.onResult(null,
              "Only locally created users can login in offline mode"));
          return;
        }
        String storedPassword = user.getPassword();

        // Check if password was stored before attempting verification
        if (storedPassword == null) {
          postToMainThread(() -> callback.onResult(null, "Invalid password"));
          return;
        }

        boolean passwordMatches = encryptionManager.verifyPassword(password, storedPassword);
        if (!passwordMatches) {
          postToMainThread(() -> callback.onResult(null, "Invalid password"));
          return;
        }
        postToMainThread(() -> callback.onResult(user, null));
      } catch (Exception e) {
        postToMainThread(() -> callback.onResult(null,
            "Error during login: " + e.getMessage()));
      }
    });
  }

  /**
   * Updates password for a user asynchronously This overload delegates to the
   * version with
   * isAdminReset parameter
   */
  public void updatePasswordAsync(long userId, String plainPassword,
      ResetPasswordCallback callback) {
    updatePasswordAsync(userId, plainPassword, false, null,null,callback);
  }

  /**
   * Updates password for a user asynchronously
   *
   * @param userId        The ID of the user whose password is being updated
   * @param plainPassword The new password in plain text
   * @param isAdminReset  true if this is an admin-initiated reset, false if user
   *                      is changing their
   *                      own password
   * @param callback      Callback to invoke when operation completes
   */
  public void updatePasswordAsync(long userId, String plainPassword, boolean isAdminReset, String oldUsername, String newUsername,
      ResetPasswordCallback callback) {
    executorService.execute(() -> {
      try {
        // Retrieve user by ID
        UsersEntity user = usersDao.getUserById(userId);

        if (user == null) {
          postToMainThread(() -> callback.onResult(false, "User not found"));
          return;
        }

        // Validate username uniqueness if username is being changed
        if (oldUsername != null && !oldUsername.trim().isEmpty() &&
                newUsername != null && !newUsername.trim().isEmpty()) {
          if (!oldUsername.equals(newUsername)) {
            // Check if new username already exists
            UsersEntity existingUser = usersDao.getUserByUsername(newUsername);
            if (existingUser != null && existingUser.getUserId() != userId) {
              postToMainThread(() -> callback.onResult(false, "Username should be unique"));
              return;
            }
            // Update username if validation passes
            user.setUserName(newUsername);
          }
        }

        // Encrypt and update password
        String encryptedPassword = encryptionManager.encryptPassword(plainPassword);
        user.setPassword(encryptedPassword);
        // Set is_password_reset based on who initiated the reset
        // true if admin reset (user must change password on first login)
        // false if user changed their own password
        user.setIsPasswordReset(isAdminReset);

        usersDao.updateUser(user);
        postToMainThread(() -> callback.onResult(true, "Password updated successfully"));
      } catch (Exception e) {
        postToMainThread(() -> callback.onResult(false,
            "Error updating password: " + e.getMessage()));
      }
    });
  }

  /**
   * Update user's encrypted PIN and PIN enabled status by username
   */
  public void updateUserPinByUsername(String username, String encryptedPin, boolean pinEnabled) {
    usersDao.updateUserPinByUsername(username, encryptedPin, pinEnabled);
  }

  /**
   * Get user by username or email
   */
  public UsersEntity getUserByUsernameOrEmail(String usernameOrEmail) {
    return usersDao.getUserByUsernameOrEmail(usernameOrEmail);
  }

  /**
   * Update user's encrypted tokens and expiry
   */
  public void updateUserTokens(String username, String encryptedToken, String encryptedRefreshToken, Long tokenExpiry) {
    executorService.execute(() -> {
      usersDao.updateUserTokensByUsername(username, encryptedToken, encryptedRefreshToken, tokenExpiry);
    });
  }
}
