package com.accessvascularinc.hydroguide.mma.network.ResetPasswordModel;

public class ResetPasswordModel {
    public static class ResetPasswordResponse {
        private boolean success;
        private String errorMessage;
        private String additionalInfo;

        public boolean isSuccess() {
            return success;
        }

        public String getErrorMessage() {
            return errorMessage;
        }

        public String getAdditionalInfo() {
            return additionalInfo;
        }
    }
}
