package com.accessvascularinc.hydroguide.mma.db.dao;

import androidx.room.Dao;
import androidx.room.Delete;
import androidx.room.Insert;
import androidx.room.Query;
import androidx.room.Update;

import com.accessvascularinc.hydroguide.mma.db.entities.UsersEntity;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;

import java.util.List;

@Dao
public interface UsersDao {
    @Insert
    long insertUser(UsersEntity user);

    @Update
    void updateUser(UsersEntity user);

    @Delete
    void deleteUser(UsersEntity user);

    @Query("SELECT * FROM users WHERE userId = :userId")
    UsersEntity getUserById(long userId);

    @Query("SELECT * FROM users WHERE email = :email LIMIT 1")
    UsersEntity getUserByEmail(String email);

    @Query("SELECT * FROM users WHERE user_name = :username LIMIT 1")
    UsersEntity getUserByUsername(String username);

    @Query("SELECT * FROM users WHERE LOWER(email) = LOWER(:usernameOrEmail) OR user_name = :usernameOrEmail LIMIT 1")
    UsersEntity getUserByUsernameOrEmail(String usernameOrEmail);

    @Query("SELECT * FROM users")
    List<UsersEntity> getAllUsers();

    @Query("SELECT * FROM users WHERE is_active = 1")
    List<UsersEntity> fetchAllActiveUsers();

    @Query("SELECT * FROM users WHERE is_active = 1 AND is_local_user = 1 AND role = 'Clinician'")
    List<UsersEntity> fetchAllLocalActiveClinicians();

    @Query("UPDATE users SET encrypted_pin = :encryptedPin, pin_enabled = :pinEnabled WHERE user_name = :username")
    void updateUserPinByUsername(String username, String encryptedPin, boolean pinEnabled);

    @Query("UPDATE users SET encrypted_token = :encryptedToken, encrypted_refresh_token = :encryptedRefreshToken, token_expiry = :tokenExpiry WHERE user_name = :username OR LOWER(email) = LOWER(:username)")
    void updateUserTokensByUsername(String username, String encryptedToken, String encryptedRefreshToken, Long tokenExpiry);
}
