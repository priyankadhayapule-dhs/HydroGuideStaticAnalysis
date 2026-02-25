package com.accessvascularinc.hydroguide.mma.db.entities;

import androidx.room.ColumnInfo;
import androidx.room.Entity;
import androidx.room.Ignore;
import androidx.room.PrimaryKey;
import androidx.room.ColumnInfo;

import com.accessvascularinc.hydroguide.mma.model.DataRecords;

/**
 * Represents a Facility in the HydroGuide system.
 * This entity is stored in the "facilities" table in the Room database.
 */
@Entity(tableName = "facilities")
public class FacilityEntity extends DataRecords {

  public Long getCloudDbPrimaryKey() {
    return cloudDbPrimaryKey;
  }

  public void setCloudDbPrimaryKey(Long cloudDbPrimaryKey) {
    this.cloudDbPrimaryKey = cloudDbPrimaryKey;
  }

  /** Unique ID of the facility (auto-generated primary key). */
  @PrimaryKey(autoGenerate = true)
  private Long facilityId = null;

  /** Name of the facility. */
  @ColumnInfo(name = "facility_name")
  private String facilityName;

  /** Organization ID associated with the facility. */
  @ColumnInfo(name = "organization_id")
  private String organizationId;

  /** Last used date of the facility, stored as a String. */
  @ColumnInfo(name = "date_last_used")
  private String dateLastUsed;

  /** User who created the record. */
  @ColumnInfo(name = "created_by")
  private Integer createdBy;

  /** Date and time when the record was created. */
  @ColumnInfo(name = "created_on")
  private String createdOn;

  /** Indicates whether the facility is active. */
  @ColumnInfo(name = "is_active")
  private boolean isActive;

  @ColumnInfo(name = "action_type")
  private String actionType; // "Added", "Updated", "Deleted"

  @ColumnInfo(name = "sync_status")
  private String syncStatus; // "Pending", "Synced"

  @ColumnInfo(name = "cloud_db_primary_key")
  private Long cloudDbPrimaryKey;

  @ColumnInfo(name = "show_patient_name")
  private boolean showPatientName;

  @ColumnInfo(name = "show_insertion_vein")
  private boolean showInsertionVein;

  @ColumnInfo(name = "show_patient_id")
  private boolean showPatientId;

  @ColumnInfo(name = "show_patient_side")
  private boolean showPatientSide;

  @ColumnInfo(name = "show_patient_dob")
  private boolean showPatientDob;

  @ColumnInfo(name = "show_arm_circumference")
  private boolean showArmCircumference;

  @ColumnInfo(name = "show_vein_size")
  private boolean showVeinSize;

  @ColumnInfo(name = "show_reason_for_insertion")
  private boolean showReasonForInsertion;

  @ColumnInfo(name = "show_no_of_lumens")
  private boolean showNoOfLumens;

  @ColumnInfo(name = "show_catheter_size")
  private boolean showCatheterSize;

  @ColumnInfo(name = "created_by_email")
  private String createdByEmail;

  public FacilityEntity(String facilityName, String organizationId, String dateLastUsed,
      Integer createdBy, String createdOn, boolean isActive,
      boolean showPatientName, boolean showInsertionVein, boolean showPatientId,
      boolean showPatientSide, boolean showPatientDob, boolean showArmCircumference,
      boolean showVeinSize, boolean showReasonForInsertion, boolean showNoOfLumens,
      boolean showCatheterSize, String createdByEmail) {
    this.facilityName = facilityName;
    this.organizationId = organizationId;
    this.dateLastUsed = dateLastUsed;
    this.createdBy = createdBy;
    this.createdOn = createdOn;
    this.isActive = isActive;
    this.showPatientName = showPatientName;
    this.showInsertionVein = showInsertionVein;
    this.showPatientId = showPatientId;
    this.showPatientSide = showPatientSide;
    this.showPatientDob = showPatientDob;
    this.showArmCircumference = showArmCircumference;
    this.showVeinSize = showVeinSize;
    this.showReasonForInsertion = showReasonForInsertion;
    this.showNoOfLumens = showNoOfLumens;
    this.showCatheterSize = showCatheterSize;
    this.createdByEmail = createdByEmail;
  }

  /**
   * Returns the facility ID.
   *
   * @return Facility ID.
   */
  public Long getFacilityId() {
    return facilityId;
  }

  /**
   * Sets the facility ID.
   *
   * @param facilityId Facility ID to set.
   */
  public void setFacilityId(Long facilityId) {
    this.facilityId = facilityId;
  }

  /**
   * Returns the facility name.
   *
   * @return Facility name.
   */
  public String getFacilityName() {
    return facilityName;
  }

  /**
   * Sets the facility name.
   * public String getActionType() {
   * return actionType;
   * }
   *
   * public void setActionType(String actionType) {
   * this.actionType = actionType;
   * }
   * 
   * public String getSyncStatus() {
   * return syncStatus;
   * }
   * 
   * public void setSyncStatus(String syncStatus) {
   * this.syncStatus = syncStatus;
   * }
   *
   * @param facilityName Facility name to set.
   */
  public void setFacilityName(String facilityName) {
    this.facilityName = facilityName;
  }

  /**
   * Returns the organization ID associated with the facility.
   *
   * @return Organization ID.
   */
  public String getOrganizationId() {
    return organizationId;
  }

  /**
   * Sets the organization ID.
   *
   * @param organizationId Organization ID to set.
   */
  public void setOrganizationId(String organizationId) {
    this.organizationId = organizationId;
  }

  /**
   * Returns the last used date of the facility.
   *
   * @return Last used date as a String.
   */
  public String getDateLastUsed() {
    return dateLastUsed;
  }

  /**
   * Sets the last used date of the facility.
   *
   * @param dateLastUsed Date to set.
   */
  public void setDateLastUsed(String dateLastUsed) {
    this.dateLastUsed = dateLastUsed;
  }

  /**
   * Returns the user who created the record.
   *
   * @return Created by.
   */
  public Integer getCreatedBy() {
    return createdBy;
  }

  /**
   * Sets the user who created the record.
   *
   * @param createdBy User to set.
   */
  public void setCreatedBy(Integer createdBy) {
    this.createdBy = createdBy;
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

  /**
   * Returns the creation date and time of the record.
   *
   * @return Creation date as a String.
   */
  public String getCreatedOn() {
    return createdOn;
  }

  /**
   * Sets the creation date and time of the record.
   *
   * @param createdOn Date and time to set.
   */
  public void setCreatedOn(String createdOn) {
    this.createdOn = createdOn;
  }

  /**
   * Returns whether the facility is active.
   *
   * @return True if active, false otherwise.
   */
  public boolean isActive() {
    return isActive;
  }

  /**
   * Sets whether the facility is active.
   *
   * @param active True if active, false otherwise.
   */
  public void setActive(boolean active) {
    isActive = active;
  }

  public boolean isShowPatientName() {
    return showPatientName;
  }

  public void setShowPatientName(final boolean showPatientName) {
    this.showPatientName = showPatientName;
  }

  public boolean isShowInsertionVein() {
    return showInsertionVein;
  }

  public void setShowInsertionVein(final boolean showInsertionVein) {
    this.showInsertionVein = showInsertionVein;
  }

  public boolean isShowPatientId() {
    return showPatientId;
  }

  public void setShowPatientId(final boolean showPatientId) {
    this.showPatientId = showPatientId;
  }

  public boolean isShowPatientSide() {
    return showPatientSide;
  }

  public void setShowPatientSide(final boolean showPatientSide) {
    this.showPatientSide = showPatientSide;
  }

  public boolean isShowPatientDob() {
    return showPatientDob;
  }

  public void setShowPatientDob(final boolean showPatientDob) {
    this.showPatientDob = showPatientDob;
  }

  public boolean isShowArmCircumference() {
    return showArmCircumference;
  }

  public void setShowArmCircumference(final boolean showArmCircumference) {
    this.showArmCircumference = showArmCircumference;
  }

  public boolean isShowVeinSize() {
    return showVeinSize;
  }

  public void setShowVeinSize(final boolean showVeinSize) {
    this.showVeinSize = showVeinSize;
  }

  public boolean isShowReasonForInsertion() {
    return showReasonForInsertion;
  }

  public void setShowReasonForInsertion(final boolean showReasonForInsertion) {
    this.showReasonForInsertion = showReasonForInsertion;
  }

  public boolean isShowNoOfLumens() {
    return showNoOfLumens;
  }

  public void setShowNoOfLumens(final boolean showNoOfLumens) {
    this.showNoOfLumens = showNoOfLumens;
  }

  public boolean isShowCatheterSize() {
    return showCatheterSize;
  }

  public void setShowCatheterSize(final boolean showCatheterSize) {
    this.showCatheterSize = showCatheterSize;
  }

  public String getCreatedByEmail() {
    return createdByEmail;
  }

  public void setCreatedByEmail(final String createdByEmail) {
    this.createdByEmail = createdByEmail;
  }
}
