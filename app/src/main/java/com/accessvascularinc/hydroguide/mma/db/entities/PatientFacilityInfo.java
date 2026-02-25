package com.accessvascularinc.hydroguide.mma.db.entities;

public class PatientFacilityInfo {
  public String patientName;
  public Long patientId;   // assuming auto-generated primary key in PatientEntity
  public String patientDob;
  public String createdOn;
  public String createdBy;
  public String facilityName;
}