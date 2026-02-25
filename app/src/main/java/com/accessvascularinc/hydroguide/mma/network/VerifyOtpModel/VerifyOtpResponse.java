package com.accessvascularinc.hydroguide.mma.network.VerifyOtpModel;

import java.util.List;

public class VerifyOtpResponse {
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

    public static class Data {
        private String email;
        private String token;
        private String userId;
        private List<Role> roles;
        private String userName;
        private boolean isSystemGeneratedPassword;

        public String getEmail() {
            return email;
        }

        public String getToken() {
            return token;
        }

        public String getUserId() {
            return userId;
        }

        public List<Role> getRoles() {
            return roles;
        }

        public String getUserName() {
            return userName;
        }

        public boolean isSystemGeneratedPassword() {
            return isSystemGeneratedPassword;
        }
    }

    public static class Role {
        private String role;
        private String organization_id;

        public String getRole() {
            return role;
        }

        public String getOrganizationId() {
            return organization_id;
        }
    }
}
