package com.accessvascularinc.hydroguide.mma.network.OfflineSyncPayload;

import com.google.gson.annotations.SerializedName;

/**
 * Response from offline-to-online sync endpoint
 * Backend returns summary of sync operation
 */
public class SyncResponseMessageDTO {
    @SerializedName("Message")
    public String Message;

    public SyncResponseMessageDTO() {
    }

    public SyncResponseMessageDTO(String message) {
        this.Message = message;
    }

    public String getMessage() {
        return Message;
    }

    public void setMessage(String message) {
        Message = message;
    }

    /**
     * Check if sync was successful (no errors in message)
     */
    public boolean isSuccess() {
        return Message != null && !Message.contains("Errors:");
    }

    /**
     * Extract sync counts from message
     * Format: "Sync completed. Facilities synced: X, Procedures synced: Y"
     */
    public int getFacilitiesSynced() {
        if (Message == null)
            return 0;
        try {
            int idx = Message.indexOf("Facilities synced: ");
            if (idx >= 0) {
                String sub = Message.substring(idx + "Facilities synced: ".length());
                return Integer.parseInt(sub.split(",")[0].trim());
            }
        } catch (Exception e) {
            // Return 0 if parsing fails
        }
        return 0;
    }

    public int getProceduresSynced() {
        if (Message == null)
            return 0;
        try {
            int idx = Message.indexOf("Procedures synced: ");
            if (idx >= 0) {
                String sub = Message.substring(idx + "Procedures synced: ".length());
                return Integer.parseInt(sub.split(",")[0].trim());
            }
        } catch (Exception e) {
            // Return 0 if parsing fails
        }
        return 0;
    }

    /**
     * Extract error message if sync had errors
     */
    public String getErrorMessages() {
        if (Message == null)
            return null;
        int idx = Message.indexOf("Errors: ");
        if (idx >= 0) {
            return Message.substring(idx + "Errors: ".length());
        }
        return null;
    }
}
