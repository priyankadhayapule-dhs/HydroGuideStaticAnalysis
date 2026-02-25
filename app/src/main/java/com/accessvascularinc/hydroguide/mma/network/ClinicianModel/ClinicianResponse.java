package com.accessvascularinc.hydroguide.mma.network.ClinicianModel;

import java.util.List;

public class ClinicianResponse {
    private ClinicianApiModel data;
    private boolean success;
    private String errorMessage;

    public ClinicianApiModel getData() {
        return data;
    }

    public void setData(ClinicianApiModel data) {
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
}
