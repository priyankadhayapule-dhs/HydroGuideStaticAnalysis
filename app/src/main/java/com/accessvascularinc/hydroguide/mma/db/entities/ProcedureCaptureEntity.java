package com.accessvascularinc.hydroguide.mma.db.entities;

import androidx.room.ColumnInfo;
import androidx.room.Entity;
import androidx.room.PrimaryKey;

import androidx.room.ForeignKey;

@Entity(tableName = "procedure_captures", foreignKeys = {
    @ForeignKey(entity = EncounterEntity.class, parentColumns = "id", childColumns = "encounter_id", onDelete = ForeignKey.CASCADE)
})
public class ProcedureCaptureEntity {

  @PrimaryKey(autoGenerate = true)
  @ColumnInfo(name = "capture_local_id")
  private Long captureLocalId;

  @ColumnInfo(name = "procedure_id")
  private Long procedureId; // foreign key reference to ProcedureEntity

  @ColumnInfo(name = "encounter_id")
  private Long encounterId; // foreign key reference to EncounterEntity

  @ColumnInfo(name = "capture_id")
  private String captureId;

  @ColumnInfo(name = "is_intravascular")
  private String isIntravascular;

  @ColumnInfo(name = "shown_in_report")
  private String shownInReport;

  @ColumnInfo(name = "inserted_length_cm")
  private String insertedLengthCm;

  @ColumnInfo(name = "exposed_length_cm")
  private String exposedLengthCm;

  @ColumnInfo(name = "capture_data")
  private String captureData;

  @ColumnInfo(name = "marked_points")
  private String markedPoints;

  @ColumnInfo(name = "upper_bound")
  private String upperBound;

  @ColumnInfo(name = "lower_bound")
  private String lowerBound;

  @ColumnInfo(name = "increment")
  private String increment;

  // --- Constructor ---
  public ProcedureCaptureEntity( Long encounterId, String captureId,
      String isIntravascular,
      String shownInReport, String insertedLengthCm,
      String exposedLengthCm,
      String captureData, String markedPoints, String upperBound,
      String lowerBound, String increment) {
    this.encounterId = encounterId;
    this.captureId = captureId;
    this.isIntravascular = isIntravascular;
    this.shownInReport = shownInReport;
    this.insertedLengthCm = insertedLengthCm;
    this.exposedLengthCm = exposedLengthCm;
    this.captureData = captureData;
    this.markedPoints = markedPoints;
    this.upperBound = upperBound;
    this.lowerBound = lowerBound;
    this.increment = increment;
  }

  public Long getEncounterId() {
    return encounterId;
  }

  public void setEncounterId(Long encounterId) {
    this.encounterId = encounterId;
  }

  // --- Getters and Setters ---
  public Long getCaptureLocalId() {
    return captureLocalId;
  }

  public void setCaptureLocalId(Long captureLocalId) {
    this.captureLocalId = captureLocalId;
  }

  public Long getProcedureId() {
    return procedureId;
  }

  public void setProcedureId(Long procedureId) {
    this.procedureId = procedureId;
  }

  public String getCaptureId() {
    return captureId;
  }

  public void setCaptureId(String captureId) {
    this.captureId = captureId;
  }

  public String getIsIntravascular() {
    return isIntravascular;
  }

  public void setIsIntravascular(String isIntravascular) {
    this.isIntravascular = isIntravascular;
  }

  public String getShownInReport() {
    return shownInReport;
  }

  public void setShownInReport(String shownInReport) {
    this.shownInReport = shownInReport;
  }

  public String getInsertedLengthCm() {
    return insertedLengthCm;
  }

  public void setInsertedLengthCm(String insertedLengthCm) {
    this.insertedLengthCm = insertedLengthCm;
  }

  public String getExposedLengthCm() {
    return exposedLengthCm;
  }

  public void setExposedLengthCm(String exposedLengthCm) {
    this.exposedLengthCm = exposedLengthCm;
  }

  public String getCaptureData() {
    return captureData;
  }

  public void setCaptureData(String captureData) {
    this.captureData = captureData;
  }

  public String getMarkedPoints() {
    return markedPoints;
  }

  public void setMarkedPoints(String markedPoints) {
    this.markedPoints = markedPoints;
  }

  public String getUpperBound() {
    return upperBound;
  }

  public void setUpperBound(String upperBound) {
    this.upperBound = upperBound;
  }

  public String getLowerBound() {
    return lowerBound;
  }

  public void setLowerBound(String lowerBound) {
    this.lowerBound = lowerBound;
  }

  public String getIncrement() {
    return increment;
  }

  public void setIncrement(String increment) {
    this.increment = increment;
  }
}
