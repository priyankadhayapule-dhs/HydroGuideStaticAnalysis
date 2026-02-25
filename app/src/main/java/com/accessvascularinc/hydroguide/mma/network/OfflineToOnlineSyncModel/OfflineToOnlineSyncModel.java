package com.accessvascularinc.hydroguide.mma.network.OfflineToOnlineSyncModel;

import com.google.gson.annotations.SerializedName;

import java.util.List;
import java.util.UUID;

/**
 * Data models for Offline → Online synchronization requests and responses.
 */
public class OfflineToOnlineSyncModel {

    /**
     * Request model for syncing offline data to cloud
     * 
     * Backend Responsibilities:
     * - Check if users with createdByEmail / conductedByEmail exist
     * - Create missing users with appropriate roles:
     * - createdByEmail → Organization_Admin role
     * - conductedByEmail → Clinician role
     * - Map facilities to createdByEmail users
     * - Map encounters to conductedByEmail users
     * - Insert all records atomically
     */
    public static class OfflineToOnlineSyncRequest {
        @SerializedName("organizationId")
        public String organizationId;

        @SerializedName("facilities")
        public List<FacilityDto> facilities;

        @SerializedName("encounters")
        public List<EncounterDto> encounters;

        public OfflineToOnlineSyncRequest() {
        }

        public OfflineToOnlineSyncRequest(String organizationId) {
            this.organizationId = organizationId;
            this.facilities = new java.util.ArrayList<>();
            this.encounters = new java.util.ArrayList<>();
        }

        /**
         * Facility Data Transfer Object
         * Backend will use createdByEmail to check/create user and map records
         */
        public static class FacilityDto {
            @SerializedName("facilityName")
            public String facilityName;

            @SerializedName("createdByEmail")
            public String createdByEmail; // Backend uses this to find/create user and assign Organization_Admin role

            @SerializedName("organizationId")
            public UUID organizationId;

            @SerializedName("isActive")
            public Boolean isActive;

            @SerializedName("isPatientNameRequired")
            public Boolean isPatientNameRequired;

            @SerializedName("isInsertionVeinRequired")
            public Boolean isInsertionVeinRequired;

            @SerializedName("isPatientIdRequired")
            public Boolean isPatientIdRequired;

            @SerializedName("isPatientSideRequired")
            public Boolean isPatientSideRequired;

            @SerializedName("isPatientDobRequired")
            public Boolean isPatientDobRequired;

            @SerializedName("isArmCircumferenceRequired")
            public Boolean isArmCircumferenceRequired;

            @SerializedName("isVeinSizeRequired")
            public Boolean isVeinSizeRequired;

            @SerializedName("isReasonForInsertionRequired")
            public Boolean isReasonForInsertionRequired;

            @SerializedName("isCatheterSizeRequired")
            public Boolean isCatheterSizeRequired;

            @SerializedName("isNumberOfLumensRequired")
            public Boolean isNumberOfLumensRequired;

            public FacilityDto() {
            }
        }

        /**
         * Encounter Data Transfer Object
         * Backend will use conductedByEmail to check/create user and map records
         */
        public static class EncounterDto {
            @SerializedName("uuid")
            public String uuid;

            @SerializedName("startTime")
            public String startTime;

            @SerializedName("conductedByEmail")
            public String conductedByEmail; // Backend uses this to find/create user and assign Clinician role

            @SerializedName("patientName")
            public String patientName;

            @SerializedName("patientMrn")
            public String patientMrn;

            @SerializedName("patientDob")
            public String patientDob;

            @SerializedName("armCircumference")
            public Double armCircumference;

            @SerializedName("veinSize")
            public Double veinSize;

            @SerializedName("insertionVein")
            public String insertionVein;

            @SerializedName("patientSide")
            public String patientSide;

            @SerializedName("patientNoOfLumens")
            public String patientNoOfLumens;

            @SerializedName("patientCatheterSize")
            public String patientCatheterSize;

            @SerializedName("facilityOrLocation")
            public String facilityOrLocation;

            @SerializedName("trimLengthCm")
            public Double trimLengthCm;

            @SerializedName("insertedCatheterCm")
            public Double insertedCatheterCm;

            @SerializedName("externalCatheterCm")
            public Double externalCatheterCm;

            @SerializedName("hydroGuideConfirmed")
            public Boolean hydroGuideConfirmed;

            @SerializedName("appSoftwareVersion")
            public String appSoftwareVersion;

            @SerializedName("controllerFirmwareVersion")
            public String controllerFirmwareVersion;

            @SerializedName("procedureCaptures")
            public List<ProcedureCaptureDto> procedureCaptures;

            @SerializedName("isUltrasoundUsed")
            public Boolean isUltrasoundUsed;

            @SerializedName("isUltrasoundCaptureSaved")
            public Boolean isUltrasoundCaptureSaved;

            public EncounterDto() {
            }
        }

        /**
         * Procedure Capture Data Transfer Object
         */
        public static class ProcedureCaptureDto {
            @SerializedName("captureId")
            public Integer captureId;

            @SerializedName("isIntravascular")
            public Boolean isIntravascular;

            @SerializedName("shownInReport")
            public Boolean shownInReport;

            @SerializedName("insertedLengthCm")
            public Double insertedLengthCm;

            @SerializedName("exposedLengthCm")
            public Double exposedLengthCm;

            @SerializedName("captureData")
            public String captureData;

            @SerializedName("markedPoints")
            public String markedPoints;

            @SerializedName("upperBound")
            public Double upperBound;

            @SerializedName("lowerBound")
            public Double lowerBound;

            @SerializedName("increment")
            public Double increment;

            public ProcedureCaptureDto() {
            }
        }

    }

    /**
     * Response model for Offline → Online sync
     */
    public static class OfflineToOnlineSyncResponse {
        @SerializedName("success")
        public boolean success;

        @SerializedName("message")
        public String message;

        @SerializedName("errorMessage")
        public String errorMessage;

        @SerializedName("data")
        public SyncResultData data;

        public boolean isSuccess() {
            return success;
        }

        public String getMessage() {
            return message;
        }

        public String getErrorMessage() {
            return errorMessage;
        }

        public SyncResultData getData() {
            return data;
        }

        /**
         * Sync result details
         */
        public static class SyncResultData {
            @SerializedName("facilitiesSynced")
            public Integer facilitiesSynced;

            @SerializedName("encountersSynced")
            public Integer encountersSynced;

            @SerializedName("usersCreated")
            public Integer usersCreated;

            @SerializedName("usersUpdated")
            public Integer usersUpdated;

            @SerializedName("syncErrors")
            public List<SyncError> syncErrors;

            public Integer getFacilitiesSynced() {
                return facilitiesSynced;
            }

            public Integer getEncountersSynced() {
                return encountersSynced;
            }

            public Integer getUsersCreated() {
                return usersCreated;
            }

            public Integer getUsersUpdated() {
                return usersUpdated;
            }

            public List<SyncError> getSyncErrors() {
                return syncErrors;
            }

            /**
             * Represents a sync error for a specific record
             */
            public static class SyncError {
                @SerializedName("type")
                public String type; // "FACILITY" or "ENCOUNTER"

                @SerializedName("identifier")
                public String identifier;

                @SerializedName("errorMessage")
                public String errorMessage;

                public String getType() {
                    return type;
                }

                public String getIdentifier() {
                    return identifier;
                }

                public String getErrorMessage() {
                    return errorMessage;
                }
            }
        }
    }

}
