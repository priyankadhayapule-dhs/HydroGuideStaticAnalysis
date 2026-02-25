package com.accessvascularinc.hydroguide.mma.network.OfflineSyncPayload;

import com.google.gson.annotations.SerializedName;
import java.util.UUID;

/**
 * Offline Sync Procedure DTO
 * Matches backend OfflineSyncProcedureDTO structure
 * Backend will auto-create user with Clinician role if createdbyEmail user
 * doesn't exist
 */
public class OfflineSyncProcedureDTO {
    @SerializedName("procedureId")
    public UUID procedureId;

    @SerializedName("organizationId")
    public UUID organizationId;

    @SerializedName("facilityId")
    public UUID facilityId;

    @SerializedName("patientId")
    public UUID patientId;

    @SerializedName("clinicianId")
    public UUID clinicianId;

    @SerializedName("deviceId")
    public String deviceId;

    @SerializedName("startTime")
    public long startTime;

    @SerializedName("createdOn")
    public long createdOn;

    @SerializedName("createdbyEmail")
    public String createdbyEmail;

    @SerializedName("procedureData")
    public String procedureData;

    @SerializedName("patient")
    public PatientPayload patient;

    @SerializedName("isUltrasoundUsed")
    public Boolean isUltrasoundUsed;

    @SerializedName("isUltrasoundCaptureSaved")
    public Boolean isUltrasoundCaptureSaved;

    public OfflineSyncProcedureDTO() {
    }

    public OfflineSyncProcedureDTO(UUID procedureId, UUID organizationId, UUID facilityId,
            UUID patientId, UUID clinicianId, String deviceId,
            long startTime, long createdOn, String createdbyEmail,
            String procedureData, PatientPayload patient, Boolean isUltrasoundUsed,
            Boolean isUltrasoundCaptureSaved) {
        this.procedureId = procedureId;
        this.organizationId = organizationId;
        this.facilityId = facilityId;
        this.patientId = patientId;
        this.clinicianId = clinicianId;
        this.deviceId = deviceId;
        this.startTime = startTime;
        this.createdOn = createdOn;
        this.createdbyEmail = createdbyEmail;
        this.procedureData = procedureData;
        this.patient = patient;
        this.isUltrasoundUsed = isUltrasoundUsed;
        this.isUltrasoundCaptureSaved = isUltrasoundCaptureSaved;
    }

    public UUID getProcedureId() {
        return procedureId;
    }

    public void setProcedureId(UUID procedureId) {
        this.procedureId = procedureId;
    }

    public UUID getOrganizationId() {
        return organizationId;
    }

    public void setOrganizationId(UUID organizationId) {
        this.organizationId = organizationId;
    }

    public UUID getFacilityId() {
        return facilityId;
    }

    public void setFacilityId(UUID facilityId) {
        this.facilityId = facilityId;
    }

    public UUID getPatientId() {
        return patientId;
    }

    public void setPatientId(UUID patientId) {
        this.patientId = patientId;
    }

    public UUID getClinicianId() {
        return clinicianId;
    }

    public void setClinicianId(UUID clinicianId) {
        this.clinicianId = clinicianId;
    }

    public String getDeviceId() {
        return deviceId;
    }

    public void setDeviceId(String deviceId) {
        this.deviceId = deviceId;
    }

    public long getStartTime() {
        return startTime;
    }

    public void setStartTime(long startTime) {
        this.startTime = startTime;
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

    public String getProcedureData() {
        return procedureData;
    }

    public void setProcedureData(String procedureData) {
        this.procedureData = procedureData;
    }

    public PatientPayload getPatient() {
        return patient;
    }

    public void setPatient(PatientPayload patient) {
        this.patient = patient;
    }

    public Boolean getIsUltrasoundUsed() {
        return isUltrasoundUsed;
    }

    public void setIsUltrasoundUsed(Boolean isUltrasoundUsed) {
        this.isUltrasoundUsed = isUltrasoundUsed;
    }

    public Boolean getIsUltrasoundCaptureSaved() {
        return isUltrasoundCaptureSaved;
    }

    public void setIsUltrasoundCaptureSaved(Boolean isUltrasoundCaptureSaved) {
        this.isUltrasoundCaptureSaved = isUltrasoundCaptureSaved;
    }
}
