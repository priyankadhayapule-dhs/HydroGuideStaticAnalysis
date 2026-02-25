package com.accessvascularinc.hydroguide.mma.network.ClinicianModel;

import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse;

import java.util.List;

public class ClinicianApiModel {
  private String userId;
  private String userEmail;
  private String userName;
  private String createdOn;
  private boolean isActive;
  private List<Role> roles;
  // Add other fields as needed from the API response

  public String getUserId() {
    return userId;
  }

  public void setUserId(String userId) {
    this.userId = userId;
  }

  public String getUserEmail() {
    return userEmail;
  }

  public void setUserEmail(String userEmail) {
    this.userEmail = userEmail;
  }

  public String getUserName() {
    return userName;
  }

  public void setRoles(List<Role> roles) {
    this.roles = roles;
  }

  public void setUserName(String userName) {
    this.userName = userName;
  }

  public String getCreatedOn() {
    return createdOn;
  }

  public void setCreatedOn(String createdOn) {
    this.createdOn = createdOn;
  }

  public boolean isActive() {
    return isActive;
  }

  public void setActive(boolean active) {
    isActive = active;
  }
  public List<Role> getRoles() {
    return roles;
  }

  public static class Role {
    private String roleName;

    public String getRole() {
      return roleName;
    }

    public String setRole(String roleName) {
      return this.roleName = roleName;
    }
  }
}
