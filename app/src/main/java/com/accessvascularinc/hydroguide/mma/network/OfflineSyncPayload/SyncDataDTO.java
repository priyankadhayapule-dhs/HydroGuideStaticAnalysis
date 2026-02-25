package com.accessvascularinc.hydroguide.mma.network.OfflineSyncPayload;

import com.google.gson.annotations.SerializedName;

/**
 * Nested data object in offline-to-online sync response
 * Contains the message with sync counts
 */
public class SyncDataDTO {
    @SerializedName("message")
    public String message;

    public SyncDataDTO() {
    }

    public SyncDataDTO(String message) {
        this.message = message;
    }

    public String getMessage() {
        return message;
    }

    public void setMessage(String message) {
        this.message = message;
    }

    /**
     * Extract facility sync count from message
     * Format: "Sync completed. Facilities synced: X, Procedures synced: Y"
     */
    public int getFacilitiesSynced() {
        if (message == null)
            return 0;
        try {
            int idx = message.indexOf("Facilities synced: ");
            if (idx >= 0) {
                String sub = message.substring(idx + "Facilities synced: ".length());
                return Integer.parseInt(sub.split(",")[0].trim());
            }
        } catch (Exception e) {
            // Return 0 if parsing fails
        }
        return 0;
    }

    /**
     * Extract procedure sync count from message
     * Format: "Sync completed. Facilities synced: X, Procedures synced: Y"
     */
    public int getProceduresSynced() {
        if (message == null)
            return 0;
        try {
            int idx = message.indexOf("Procedures synced: ");
            if (idx >= 0) {
                String sub = message.substring(idx + "Procedures synced: ".length());
                return Integer.parseInt(sub.split(",")[0].trim());
            }
        } catch (Exception e) {
            // Return 0 if parsing fails
        }
        return 0;
    }
}
