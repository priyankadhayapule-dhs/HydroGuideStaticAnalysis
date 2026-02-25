package com.accessvascularinc.hydroguide.mma.network;

public class UserSettingsModel {

    public static class UserSettingsSyncRequest {
        private Boolean isInsertedLengthlabellingOn;
        private Boolean isAudioOn;

        public UserSettingsSyncRequest(Boolean isInsertedLengthlabellingOn, Boolean isAudioOn) {
            this.isInsertedLengthlabellingOn = isInsertedLengthlabellingOn;
            this.isAudioOn = isAudioOn;
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
    }

    public static class UserSettingsSyncResponse {
        private UserSettingsData data;
        private boolean success;
        private String errorMessage;
        private String additionalInfo;

        public UserSettingsData getData() {
            return data;
        }

        public void setData(UserSettingsData data) {
            this.data = data;
        }

        public boolean isSuccess() {
            return success;
        }

        public void setSuccess(boolean success) {
            this.success = success;
        }

        public String getErrorMessage() {
            return errorMessage;
        }

        public void setErrorMessage(String errorMessage) {
            this.errorMessage = errorMessage;
        }

        public String getAdditionalInfo() {
            return additionalInfo;
        }

        public void setAdditionalInfo(String additionalInfo) {
            this.additionalInfo = additionalInfo;
        }
    }

    public static class UserSettingsData {
        private String userId;
        private String userEmail;
        private String userName;
        private Boolean isInsertedLengthlabellingOn;
        private Boolean isAudioOn;

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

        public void setUserName(String userName) {
            this.userName = userName;
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
    }
}
