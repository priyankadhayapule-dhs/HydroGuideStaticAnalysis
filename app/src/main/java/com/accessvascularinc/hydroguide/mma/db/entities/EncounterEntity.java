package com.accessvascularinc.hydroguide.mma.db.entities;

import androidx.room.Entity;
import androidx.room.PrimaryKey;

import com.accessvascularinc.hydroguide.mma.model.EncounterData;
import com.accessvascularinc.hydroguide.mma.model.EncounterState;
import com.accessvascularinc.hydroguide.mma.model.PatientData;
import com.accessvascularinc.hydroguide.mma.model.Capture;

import java.util.Date;
import java.util.OptionalDouble;

@Entity(tableName = "encounters")
public class EncounterEntity {

  @PrimaryKey(autoGenerate = true)
  private int id;

  private String uuid;
  private Long startTime;
  private Long endTime;

  private PatientData patient;
  private Capture externalCapture;
  private Capture[] intravCaptures;

  private EncounterState state;
  private Double trimLengthCm;
  private Double insertedCatheterCm;
  private Double externalCatheterCm;

  private Boolean hydroGuideConfirmed;
  private String appSoftwareVersion;
  private String controllerFirmwareVersion;
  private Boolean procedureStarted;
  private String dataDirPath;
  @androidx.room.ColumnInfo(name = "action_type")
  private String actionType; // "Added",

  @androidx.room.ColumnInfo(name = "sync_status")
  private String syncStatus; // "Pending", "Synced"

  @androidx.room.ColumnInfo(name = "cloud_db_primary_key")
  private String cloudDbPrimaryKey; // GUID from cloud

  @androidx.room.ColumnInfo(name = "organization_id")
  private String organizationId;

  @androidx.room.ColumnInfo(name = "facility_id")
  private Long facilityId;

  @androidx.room.ColumnInfo(name = "facility_or_location")
  private String facilityOrLocation;

  @androidx.room.ColumnInfo(name = "clinician_id")
  private String clinicianId;

  @androidx.room.ColumnInfo(name = "patient_id")
  private long patientId;

  @androidx.room.ColumnInfo(name = "device_id")
  private long deviceId;

  @androidx.room.ColumnInfo(name = "tablet_id")
  private long tabletId;

  @androidx.room.ColumnInfo(name = "conducted_by")
  private String conductedBy; // Email or string of the person who conducted the encounter

  @androidx.room.ColumnInfo(name = "patient_name")
  private String patientName;

  @androidx.room.ColumnInfo(name = "patient_mrn")
  private String patientMrn;

  @androidx.room.ColumnInfo(name = "patient_dob")
  private String patientDob;

  @androidx.room.ColumnInfo(name = "arm_circumference")
  private Double armCircumference;

  @androidx.room.ColumnInfo(name = "vein_size")
  private Double veinSize;

  @androidx.room.ColumnInfo(name = "insertion_vein")
  private String insertionVein;

  @androidx.room.ColumnInfo(name = "patient_side")
  private String patientSide;

  @androidx.room.ColumnInfo(name = "patient_no_of_lumens")
  private String patientNoOfLumens;

  @androidx.room.ColumnInfo(name = "patient_catheter_size")
  private String patientCatheterSize;

  @androidx.room.ColumnInfo(name = "is_ultrasound_used")
  private Boolean isUltrasoundUsed;

  @androidx.room.ColumnInfo(name = "is_ultrasound_capture_saved")
  private Boolean isUltrasoundCaptureSaved;

  public String getConductedBy() {
    return conductedBy;
  }

  public void setConductedBy(String conductedBy) {
    this.conductedBy = conductedBy;
  }

  public EncounterData toEncounterData() {
    final EncounterData data = new EncounterData();
    data.setStartTime(parseDate(this.startTime));
    data.setState(this.state);
    data.setPatient(this.patient);
    data.setExternalCapture(this.externalCapture);
    data.setIntravCaptures(this.intravCaptures);
    data.setTrimLengthCm(toOptionalDouble(this.trimLengthCm));
    data.setInsertedCatheterCm(toOptionalDouble(this.insertedCatheterCm));
    data.setExternalCatheterCm(toOptionalDouble(this.externalCatheterCm));
    data.setHydroGuideConfirmed(
        this.hydroGuideConfirmed != null ? this.hydroGuideConfirmed : Boolean.FALSE);
    data.setAppSoftwareVersion(this.appSoftwareVersion);
    data.setControllerFirmwareVersion(this.controllerFirmwareVersion);
    data.setProcedureStarted(this.procedureStarted != null ? this.procedureStarted : Boolean.FALSE);
    data.setDataDirPath(this.dataDirPath);
    return data;
  }

  public static OptionalDouble toOptionalDouble(double value) {
    return Double.isNaN(value) ? OptionalDouble.empty() : OptionalDouble.of(value);
  }

  public static OptionalDouble toOptionalDouble(Double value) {
    return value == null ? OptionalDouble.empty() : OptionalDouble.of(value);
  }

  public Date parseDate(long timestamp) throws IllegalArgumentException {
    if (timestamp <= 0) {
      throw new IllegalArgumentException("Invalid timestamp: " + timestamp);
    }

    return new Date(timestamp);
  }
  // -------------------------
  // Getters and Setters
  // -------------------------

  public int getId() {
    return id;
  }

  public void setId(int id) {
    this.id = id;
  }

  public String getUuid() {
    return uuid;
  }

  public void setUuid(String uuid) {
    this.uuid = uuid;
  }

  public Long getStartTime() {
    return startTime;
  }

  public void setStartTime(Long startTime) {
    this.startTime = startTime;
  }

  public Long getEndTime() {
    return endTime;
  }

  public void setEndTime(Long endTime) {
    this.endTime = endTime;
  }

  public PatientData getPatient() {
    return patient;
  }

  public void setPatient(PatientData patient) {
    this.patient = patient;
  }

  public Capture getExternalCapture() {
    return externalCapture;
  }

  public void setExternalCapture(Capture externalCapture) {
    this.externalCapture = externalCapture;
  }

  public Capture[] getIntravCaptures() {
    return intravCaptures;
  }

  public void setIntravCaptures(Capture[] intravCaptures) {
    this.intravCaptures = intravCaptures;
  }

  public EncounterState getState() {
    return state;
  }

  public void setState(EncounterState state) {
    this.state = state;
  }

  public Double getTrimLengthCm() {
    return trimLengthCm;
  }

  public void setTrimLengthCm(Double trimLengthCm) {
    this.trimLengthCm = trimLengthCm;
  }

  public Double getInsertedCatheterCm() {
    return insertedCatheterCm;
  }

  public void setInsertedCatheterCm(Double insertedCatheterCm) {
    this.insertedCatheterCm = insertedCatheterCm;
  }

  public Double getExternalCatheterCm() {
    return externalCatheterCm;
  }

  public void setExternalCatheterCm(Double externalCatheterCm) {
    this.externalCatheterCm = externalCatheterCm;
  }

  public Boolean getHydroGuideConfirmed() {
    return hydroGuideConfirmed;
  }

  public void setHydroGuideConfirmed(Boolean hydroGuideConfirmed) {
    this.hydroGuideConfirmed = hydroGuideConfirmed;
  }

  public String getAppSoftwareVersion() {
    return appSoftwareVersion;
  }

  public void setAppSoftwareVersion(String appSoftwareVersion) {
    this.appSoftwareVersion = appSoftwareVersion;
  }

  public String getControllerFirmwareVersion() {
    return controllerFirmwareVersion;
  }

  public void setControllerFirmwareVersion(String controllerFirmwareVersion) {
    this.controllerFirmwareVersion = controllerFirmwareVersion;
  }

  public Boolean getProcedureStarted() {
    return procedureStarted;
  }

  public void setProcedureStarted(Boolean procedureStarted) {
    this.procedureStarted = procedureStarted;
  }

  public String getDataDirPath() {
    return dataDirPath;
  }

  public void setDataDirPath(String dataDirPath) {
    this.dataDirPath = dataDirPath;
  }

  public String getActionType() {
    return actionType;
  }

  public void setActionType(String actionType) {
    this.actionType = actionType;
  }

  public String getSyncStatus() {
    return syncStatus;
  }

  public void setSyncStatus(String syncStatus) {
    this.syncStatus = syncStatus;
  }

  public String getCloudDbPrimaryKey() {
    return cloudDbPrimaryKey;
  }

  public void setCloudDbPrimaryKey(String cloudDbPrimaryKey) {
    this.cloudDbPrimaryKey = cloudDbPrimaryKey;
  }

  // --- Added getters and setters for new fields ---
  public String getOrganizationId() {
    return organizationId;
  }

  public void setOrganizationId(String organizationId) {
    this.organizationId = organizationId;
  }

  public Long getFacilityId() {
    return facilityId;
  }

  public void setFacilityId(Long facilityId) {
    this.facilityId = facilityId;
  }

  public String getFacilityOrLocation() {
    return facilityOrLocation;
  }

  public void setFacilityOrLocation(String facilityOrLocation) {
    this.facilityOrLocation = facilityOrLocation;
  }

  public String getClinicianId() {
    return clinicianId;
  }

  public void setClinicianId(String clinicianId) {
    this.clinicianId = clinicianId;
  }

  public long getPatientId() {
    return patientId;
  }

  public void setPatientId(long patientId) {
    this.patientId = patientId;
  }

  public long getDeviceId() {
    return deviceId;
  }

  public void setDeviceId(long deviceId) {
    this.deviceId = deviceId;
  }

  public long getTabletId() {
    return tabletId;
  }

  public void setTabletId(long tabletId) {
    this.tabletId = tabletId;
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

  public Double getArmCircumference() {
    return armCircumference;
  }

  public void setArmCircumference(Double armCircumference) {
    this.armCircumference = armCircumference;
  }

  public Double getVeinSize() {
    return veinSize;
  }

  public void setVeinSize(Double veinSize) {
    this.veinSize = veinSize;
  }

  public String getInsertionVein() {
    return insertionVein;
  }

  public void setInsertionVein(String insertionVein) {
    this.insertionVein = insertionVein;
  }

  public String getPatientSide() {
    return patientSide;
  }

  public void setPatientSide(String patientSide) {
    this.patientSide = patientSide;
  }

  public String getPatientNoOfLumens() {
    return patientNoOfLumens;
  }

  public void setPatientNoOfLumens(String patientNoOfLumens) {
    this.patientNoOfLumens = patientNoOfLumens;
  }

  public String getPatientCatheterSize() {
    return patientCatheterSize;
  }

  public void setPatientCatheterSize(String patientCatheterSize) {
    this.patientCatheterSize = patientCatheterSize;
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
