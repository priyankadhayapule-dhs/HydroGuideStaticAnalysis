package com.accessvascularinc.hydroguide.mma.network.ProcedureSyncModel;

import com.google.gson.*;
import java.lang.reflect.Type;

import java.util.List;

public class ProcedureSyncResponse {
    private List<ProcedureData> data;
    private boolean success;
    private String errorMessage;
    private String additionalInfo;

    public List<ProcedureData> getData() {
        return data;
    }

    public void setData(List<ProcedureData> data) {
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

    public static class ProcedureData {
        public String procedureId;
        public String startTime;
        public double trimLengthCm;
        public double insertedCatheterCm;
        public double externalCatheterCm;
        public boolean hydroGuideConfirmed;
        public String state;
        public String appSoftwareVersion;
        public String controllerFirmwareVersion;
        public String organizationId;
        public int facilityId;
        public String facilityOrLocation;
        public String clinicianId;
        // Added for GET /procedure endpoint
        public String clinicianName;
        public String clinicianEmail;
        public String patientId;
        public String deviceId;
        public String tabletId;
        public List<ProcedureCapture> procedureCaptures;
        public Patient patient;
        public String fileUrl;
    }

    public static class ProcedureCapture {
        public int captureLocalId;
        public String procedureId;
        public int captureId;
        public boolean isIntravascular;
        public boolean shownInReport;
        public double insertedLengthCm;
        public double exposedLengthCm;
        public String captureData;
        public String markedPoints;
        public double upperBound;
        public double lowerBound;
        public double increment;
    }

    public static class Patient {
        public String patientId;
        public String organizationId;
        public String patientName;
        public String patientMrn;
        public String patientDob;
        public String createdBy;
        public String createdOn;
    }
}
