package com.accessvascularinc.hydroguide.mma.network.ClinicianModel;

import java.util.List;

public class ClinicianListResponse {
    private List<ClinicianApiModel> data;
    private boolean success;
    private String errorMessage;
    private String additionalInfo;

    public List<ClinicianApiModel> getData() {
        return data;
    }

    public void setData(List<ClinicianApiModel> data) {
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
