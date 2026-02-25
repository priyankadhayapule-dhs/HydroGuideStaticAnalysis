package com.accessvascularinc.hydroguide.mma.network.OfflineSyncPayload;

import com.google.gson.annotations.SerializedName;

/**
 * Response from offline-to-online sync endpoint
 * Backend returns nested data structure with message
 * 
 * Response format:
 * {
 * "data": {
 * "message": "Sync completed. Facilities synced: X, Procedures synced: Y"
 * },
 * "success": true,
 * "errorMessage": "",
 * "additionalInfo": ""
 * }
 */
public class SyncResponseDTO {
    @SerializedName("data")
    public SyncDataDTO data;

    @SerializedName("success")
    public boolean success;

    @SerializedName("errorMessage")
    public String errorMessage;

    @SerializedName("additionalInfo")
    public String additionalInfo;

    public SyncResponseDTO() {
    }

    public SyncResponseDTO(SyncDataDTO data, boolean success, String errorMessage, String additionalInfo) {
        this.data = data;
        this.success = success;
        this.errorMessage = errorMessage;
        this.additionalInfo = additionalInfo;
    }

    public SyncDataDTO getData() {
        return data;
    }

    public void setData(SyncDataDTO data) {
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

    /**
     * Get message from nested data
     */
    public String getMessage() {
        return data != null ? data.getMessage() : null;
    }

    /**
     * Extract facility sync count from message
     */
    public int getFacilitiesSynced() {
        return data != null ? data.getFacilitiesSynced() : 0;
    }

    /**
     * Extract procedure sync count from message
     */
    public int getProceduresSynced() {
        return data != null ? data.getProceduresSynced() : 0;
    }
}
