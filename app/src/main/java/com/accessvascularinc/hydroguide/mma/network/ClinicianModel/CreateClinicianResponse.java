package com.accessvascularinc.hydroguide.mma.network.ClinicianModel;

public class CreateClinicianResponse {
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

    public void setSuccess(boolean success) {
        this.success = success;
    }

    public void setErrorMessage(String errorMessage) {
        this.errorMessage = errorMessage;
    }

    public void setAdditionalInfo(String additionalInfo) {
        this.additionalInfo = additionalInfo;
    }
}
