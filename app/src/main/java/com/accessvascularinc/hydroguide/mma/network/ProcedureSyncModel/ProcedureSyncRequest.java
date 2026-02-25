package com.accessvascularinc.hydroguide.mma.network.ProcedureSyncModel;
import com.google.gson.annotations.SerializedName;
import java.util.List;

public class ProcedureSyncRequest {
    private List<ProcedureData> added;

    public List<ProcedureData> getAdded() {
        return added;
    }

    public void setAdded(List<ProcedureData> added) {
        this.added = added;
    }

    public static class ProcedureData {
        public String startTime;
        public double trimLengthCm;
        public double insertedCatheterCm;
        public double externalCatheterCm;
        public boolean hydroGuideConfirmed;
        public String state;
        public String appSoftwareVersion;
        public String controllerFirmwareVersion;
        public String organizationId;
        public String facilityOrLocation;
        public String clinicianId;
        public Double armCircumference;
        public Double veinSize;
        public String insertionVein;
        public String patientSide;
        @SerializedName("Nooflumens")
        public Double Nooflumens;
        @SerializedName("Cathetersize")
        public Double Cathetersize;
        public Boolean isUltrasoundUsed;
        public Boolean isUltrasoundCaptureSaved;
        public List<ProcedureCapture> procedureCaptures;
        public Patient patient;
    }

    public static class ProcedureCapture {
        public int captureId;
        public boolean isIntravascular;
        public boolean shownInReport;
        public double insertedLengthCm;
        public double exposedLengthCm;
        public String captureData; // JSON array string, e.g. "[0.1,0.2,null]"
        public String markedPoints; // JSON array string, e.g. "[null,null,null]"
        public double upperBound;
        public double lowerBound;
        public double increment;
    }

    public static class Patient {
        public String organizationId;
        public String patientName;
        public String patientMrn;
        public String patientDob;
    }
}
