package com.accessvascularinc.hydroguide.mma.model;

/**
 * Model for displaying EncounterEntity data in the records table.
 */
public class EncounterTableRow {
    public String patientName;
    public String patientId;
    public String patientDob;
    public String createdOn;
    public String createdBy;
    public String facilityName;
    public int encounterId;
    public String uuid;
    public String state;
    public String dataDirPath;
    public String syncStatus;
    public String fileName;

    public EncounterTableRow() {
    }

    public EncounterTableRow(String patientName, String patientId, String patientDob, String createdOn,
            String createdBy,
            String facilityName, int encounterId, String uuid, String state, String dataDirPath, String syncStatus) {
        this.patientName = patientName;
        this.patientId = patientId;
        this.patientDob = patientDob;
        this.createdOn = createdOn;
        this.createdBy = createdBy;
        this.facilityName = facilityName;
        this.encounterId = encounterId;
        this.uuid = uuid;
        this.state = state;
        this.dataDirPath = dataDirPath;
        this.syncStatus = syncStatus;
    }
}
