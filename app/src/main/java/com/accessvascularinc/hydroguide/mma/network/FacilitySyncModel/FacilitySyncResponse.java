package com.accessvascularinc.hydroguide.mma.network.FacilitySyncModel;

import java.util.List;

public class FacilitySyncResponse {
    private List<FacilityData> data;
    private boolean success;
    private String errorMessage;
    private String additionalInfo;

    public List<FacilityData> getData() {
        return data;
    }

    public void setData(List<FacilityData> data) {
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

    public static class FacilityData {
        private int facilityId;
        private String facilityName;
        private String organizationId;
        private String dateLastUsed;
        private String createdBy;
        private String createdOn;
        private boolean isActive;
        @com.google.gson.annotations.SerializedName("ispatientNameRequired")
        private Boolean isPatientNameRequired;
        @com.google.gson.annotations.SerializedName("isinsertionVeinRequired")
        private Boolean isInsertionVeinRequired;
        @com.google.gson.annotations.SerializedName("ispatientIDRequired")
        private Boolean isPatientIDRequired;
        @com.google.gson.annotations.SerializedName("ispatientSideRequired")
        private Boolean isPatientSideRequired;
        @com.google.gson.annotations.SerializedName("ispatientDobRequired")
        private Boolean isPatientDobRequired;
        @com.google.gson.annotations.SerializedName("isarmCircumferenceRequired")
        private Boolean isArmCircumferenceRequired;
        @com.google.gson.annotations.SerializedName("isveinSizeRequired")
        private Boolean isVeinSizeRequired;
        @com.google.gson.annotations.SerializedName("isreasonForInsertionRequired")
        private Boolean isReasonForInsertionRequired;
        @com.google.gson.annotations.SerializedName("isCatherSizeRequired")
        private Boolean isCatherSizeRequired;
        @com.google.gson.annotations.SerializedName("isNumberOfLumensRequired")
        private Boolean isNumberOfLumensRequired;

        public int getFacilityId() {
            return facilityId;
        }

        public void setFacilityId(int facilityId) {
            this.facilityId = facilityId;
        }

        public String getFacilityName() {
            return facilityName;
        }

        public void setFacilityName(String facilityName) {
            this.facilityName = facilityName;
        }

        public String getOrganizationId() {
            return organizationId;
        }

        public void setOrganizationId(String organizationId) {
            this.organizationId = organizationId;
        }

        public String getDateLastUsed() {
            return dateLastUsed;
        }

        public void setDateLastUsed(String dateLastUsed) {
            this.dateLastUsed = dateLastUsed;
        }

        public String getCreatedBy() {
            return createdBy;
        }

        public void setCreatedBy(String createdBy) {
            this.createdBy = createdBy;
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

        public Boolean getIsPatientNameRequired() {
            return isPatientNameRequired;
        }

        public void setIsPatientNameRequired(Boolean isPatientNameRequired) {
            this.isPatientNameRequired = isPatientNameRequired;
        }

        public Boolean getIsInsertionVeinRequired() {
            return isInsertionVeinRequired;
        }

        public void setIsInsertionVeinRequired(Boolean isInsertionVeinRequired) {
            this.isInsertionVeinRequired = isInsertionVeinRequired;
        }

        public Boolean getIsPatientIDRequired() {
            return isPatientIDRequired;
        }

        public void setIsPatientIDRequired(Boolean isPatientIDRequired) {
            this.isPatientIDRequired = isPatientIDRequired;
        }

        public Boolean getIsPatientSideRequired() {
            return isPatientSideRequired;
        }

        public void setIsPatientSideRequired(Boolean isPatientSideRequired) {
            this.isPatientSideRequired = isPatientSideRequired;
        }

        public Boolean getIsPatientDobRequired() {
            return isPatientDobRequired;
        }

        public void setIsPatientDobRequired(Boolean isPatientDobRequired) {
            this.isPatientDobRequired = isPatientDobRequired;
        }

        public Boolean getIsArmCircumferenceRequired() {
            return isArmCircumferenceRequired;
        }

        public void setIsArmCircumferenceRequired(Boolean isArmCircumferenceRequired) {
            this.isArmCircumferenceRequired = isArmCircumferenceRequired;
        }

        public Boolean getIsVeinSizeRequired() {
            return isVeinSizeRequired;
        }

        public void setIsVeinSizeRequired(Boolean isVeinSizeRequired) {
            this.isVeinSizeRequired = isVeinSizeRequired;
        }

        public Boolean getIsReasonForInsertionRequired() {
            return isReasonForInsertionRequired;
        }

        public void setIsReasonForInsertionRequired(Boolean isReasonForInsertionRequired) {
            this.isReasonForInsertionRequired = isReasonForInsertionRequired;
        }

        public Boolean getIsCatherSizeRequired() {
            return isCatherSizeRequired;
        }

        public void setIsCatherSizeRequired(Boolean isCatherSizeRequired) {
            this.isCatherSizeRequired = isCatherSizeRequired;
        }

        public Boolean getIsNumberOfLumensRequired() {
            return isNumberOfLumensRequired;
        }

        public void setIsNumberOfLumensRequired(Boolean isNumberOfLumensRequired) {
            this.isNumberOfLumensRequired = isNumberOfLumensRequired;
        }
    }
}