package com.accessvascularinc.hydroguide.mma.db.entities;

import androidx.room.ColumnInfo;
import androidx.room.Entity;
import androidx.room.PrimaryKey;
import androidx.room.Index;

import java.util.Date;

@Entity(tableName = "users", indices = {
        @Index(value = "user_name", unique = true),
        @Index(value = "email", unique = true)
})
public class UsersEntity {

    @PrimaryKey(autoGenerate = true)
    @ColumnInfo(name = "userId")
    private Long userId;

    @ColumnInfo(name = "user_name")
    private String userName;

    @ColumnInfo(name = "email")
    private String email;

    @ColumnInfo(name = "is_local_user")
    private Boolean isLocalUser;

    @ColumnInfo(name = "password")
    private String password;

    @ColumnInfo(name = "is_password_reset")
    private Boolean isPasswordReset;

    @ColumnInfo(name = "is_active")
    private Boolean isActive;

    @ColumnInfo(name = "role")
    private String role;

    @ColumnInfo(name = "cloud_organizationId")
    private String cloudOrganizationId;

    @ColumnInfo(name = "cloud_userId")
    private String cloudUserId;

    @ColumnInfo(name = "isInsertedLengthlabellingOn")
    private Boolean isInsertedLengthlabellingOn;

    @ColumnInfo(name = "isAudioOn")
    private Boolean isAudioOn;

    @ColumnInfo(name = "encrypted_pin")
    private String encryptedPin;

    @ColumnInfo(name = "pin_enabled")
    private Boolean pinEnabled;

    @ColumnInfo(name = "encrypted_token")
    private String encryptedToken;

    @ColumnInfo(name = "encrypted_refresh_token")
    private String encryptedRefreshToken;

    @ColumnInfo(name = "token_expiry")
    private Long tokenExpiry;

    @ColumnInfo(name = "createOn")
    private String createdOn;

    // Getters and Setters
    public Long getUserId() {
        return userId;
    }
    public String getCreatedOn(){
        return createdOn;
    }
    public void setCreatedOn(String createdOn){
        this.createdOn = createdOn;
    }

    public void setUserId(Long userId) {
        this.userId = userId;
    }

    public String getUserName() {
        return userName;
    }

    public void setUserName(String userName) {
        this.userName = userName;
    }

    public String getEmail() {
        return email;
    }

    public void setEmail(String email) {
        this.email = email;
    }

    public Boolean getIsLocalUser() {
        return isLocalUser;
    }

    public void setIsLocalUser(Boolean isLocalUser) {
        this.isLocalUser = isLocalUser;
    }

    public String getPassword() {
        return password;
    }

    public void setPassword(String password) {
        this.password = password;
    }

    public Boolean getIsPasswordReset() {
        return isPasswordReset;
    }

    public void setIsPasswordReset(Boolean isPasswordReset) {
        this.isPasswordReset = isPasswordReset;
    }

    public Boolean getIsActive() {
        return isActive;
    }

    public void setIsActive(Boolean isActive) {
        this.isActive = isActive;
    }

    public String getRole() {
        return role;
    }

    public void setRole(String role) {
        this.role = role;
    }

    public String getCloudOrganizationId() {
        return cloudOrganizationId;
    }

    public void setCloudOrganizationId(String cloudOrganizationId) {
        this.cloudOrganizationId = cloudOrganizationId;
    }

    public String getCloudUserId() {
        return cloudUserId;
    }

    public void setCloudUserId(String cloudUserId) {
        this.cloudUserId = cloudUserId;
    }

    public Boolean getIsInsertedLengthlabellingOn() {
        return isInsertedLengthlabellingOn;
    }

    public void setIsInsertedLengthlabellingOn(Boolean isInsertedLengthlabellingOn) {
        this.isInsertedLengthlabellingOn = isInsertedLengthlabellingOn;
    }

    public Boolean getIsAudioOn() {
        return isAudioOn;
    }

    public void setIsAudioOn(Boolean isAudioOn) {
        this.isAudioOn = isAudioOn;
    }

    public String getEncryptedPin() {
        return encryptedPin;
    }

    public void setEncryptedPin(String encryptedPin) {
        this.encryptedPin = encryptedPin;
    }

    public Boolean getPinEnabled() {
        return pinEnabled;
    }

    public void setPinEnabled(Boolean pinEnabled) {
        this.pinEnabled = pinEnabled;
    }

    public String getEncryptedToken() {
        return encryptedToken;
    }

    public void setEncryptedToken(String encryptedToken) {
        this.encryptedToken = encryptedToken;
    }

    public String getEncryptedRefreshToken() {
        return encryptedRefreshToken;
    }

    public void setEncryptedRefreshToken(String encryptedRefreshToken) {
        this.encryptedRefreshToken = encryptedRefreshToken;
    }

    public Long getTokenExpiry() {
        return tokenExpiry;
    }

    public void setTokenExpiry(Long tokenExpiry) {
        this.tokenExpiry = tokenExpiry;
    }
}