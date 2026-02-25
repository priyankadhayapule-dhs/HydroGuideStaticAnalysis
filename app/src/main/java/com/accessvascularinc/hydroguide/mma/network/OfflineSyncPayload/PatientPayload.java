package com.accessvascularinc.hydroguide.mma.network.OfflineSyncPayload;

import com.google.gson.annotations.SerializedName;
import java.util.UUID;

/**
 * Patient Payload DTO
 * Used for auto-creating patients on backend during sync
 * If patient doesn't exist on cloud, backend will create it with this data
 */
public class PatientPayload {
    @SerializedName("patientId")
    public UUID patientId;

    @SerializedName("organizationId")
    public UUID organizationId;

    @SerializedName("patientName")
    public String patientName;

    @SerializedName("patientMrn")
    public String patientMrn;

    @SerializedName("patientDob")
    public String patientDob;

    @SerializedName("createdOn")
    public long createdOn;

    public PatientPayload() {
    }

    public PatientPayload(UUID patientId, UUID organizationId, String patientName,
            String patientMrn, String patientDob, long createdOn) {
        this.patientId = patientId;
        this.organizationId = organizationId;
        this.patientName = patientName;
        this.patientMrn = patientMrn;
        this.patientDob = patientDob;
        this.createdOn = createdOn;
    }

    public UUID getPatientId() {
        return patientId;
    }

    public void setPatientId(UUID patientId) {
        this.patientId = patientId;
    }

    public UUID getOrganizationId() {
        return organizationId;
    }

    public void setOrganizationId(UUID organizationId) {
        this.organizationId = organizationId;
    }

    public String getPatientName() {
        return patientName;
    }

    public void setPatientName(String patientName) {
        this.patientName = patientName;
    }

    public String getPatientMrn() {
        return patientMrn;
    }

    public void setPatientMrn(String patientMrn) {
        this.patientMrn = patientMrn;
    }

    public String getPatientDob() {
        return patientDob;
    }

    public void setPatientDob(String patientDob) {
        this.patientDob = patientDob;
    }

    public long getCreatedOn() {
        return createdOn;
    }

    public void setCreatedOn(long createdOn) {
        this.createdOn = createdOn;
    }
}
