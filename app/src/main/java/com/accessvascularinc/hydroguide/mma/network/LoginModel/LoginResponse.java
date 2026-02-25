package com.accessvascularinc.hydroguide.mma.network.LoginModel;

import java.util.List;

public class LoginResponse {
  private Data data;
  private boolean success;
  private String errorMessage;
  private String additionalInfo;

  public Data getData() {
    return data;
  }

  public boolean isSuccess() {
    return success;
  }

  public String getErrorMessage() {
    return errorMessage;
  }

  public String getAdditionalInfo() {
    return additionalInfo;
  }

  /**
   * Returns the organization_id from the first role, or null if not available.
   */
  public String getOrganizationId() {
    if (data != null && data.getRoles() != null && !data.getRoles().isEmpty()) {
      return data.getRoles().get(0).getOrganization_id();
    }
    return null;
  }

  public static class Data {
    private String email;
    private String token;
    private String userId;
    private String userName;
    private List<Role> roles;
    private boolean isSystemGeneratedPassword;
    private String refreshToken;

    public String getUserName() {
      return userName;
    }

    public String getEmail() {
      return email;
    }

    public boolean getFirstTimeLogin() {
      return isSystemGeneratedPassword;
    }

    public String getToken() {
      return token;
    }

    public String getRefereshToken() {
      return refreshToken;
    }

    public String getUserId() {
      return userId;
    }

    public List<Role> getRoles() {
      return roles;
    }
  }

  public static class Role {
    private String role;
    private String organization_id;

    public String getRole() {
      return role;
    }

    public String getOrganization_id() {
      return organization_id;
    }
  }
}
