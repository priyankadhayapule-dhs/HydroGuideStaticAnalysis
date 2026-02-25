package com.accessvascularinc.hydroguide.mma.network.OfflineSyncPayload;

import com.google.gson.annotations.SerializedName;
import java.util.UUID;

/**
 * Offline Sync Facility DTO
 * Matches backend OfflineSyncFacilityDTO structure
 * Backend will auto-create user with Organization_Admin role if createdbyEmail
 * user doesn't exist
 */
public class OfflineSyncFacilityDTO {
    @SerializedName("facilityId")
    public UUID facilityId;

    @SerializedName("organizationId")
    public UUID organizationId;

    @SerializedName("facilityName")
    public String facilityName;

    @SerializedName("createdOn")
    public long createdOn;

    @SerializedName("createdbyEmail")
    public String createdbyEmail;

    public OfflineSyncFacilityDTO() {
    }

    public OfflineSyncFacilityDTO(UUID facilityId, UUID organizationId, String facilityName,
            long createdOn, String createdbyEmail) {
        this.facilityId = facilityId;
        this.organizationId = organizationId;
        this.facilityName = facilityName;
        this.createdOn = createdOn;
        this.createdbyEmail = createdbyEmail;
    }

    public UUID getFacilityId() {
        return facilityId;
    }

    public void setFacilityId(UUID facilityId) {
        this.facilityId = facilityId;
    }

    public UUID getOrganizationId() {
        return organizationId;
    }

    public void setOrganizationId(UUID organizationId) {
        this.organizationId = organizationId;
    }

    public String getFacilityName() {
        return facilityName;
    }

    public void setFacilityName(String facilityName) {
        this.facilityName = facilityName;
    }

    public long getCreatedOn() {
        return createdOn;
    }

    public void setCreatedOn(long createdOn) {
        this.createdOn = createdOn;
    }

    public String getCreatedbyEmail() {
        return createdbyEmail;
    }

    public void setCreatedbyEmail(String createdbyEmail) {
        this.createdbyEmail = createdbyEmail;
    }
}
