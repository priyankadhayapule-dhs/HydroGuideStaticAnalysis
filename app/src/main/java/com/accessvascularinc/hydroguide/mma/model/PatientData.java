package com.accessvascularinc.hydroguide.mma.model;

import android.util.Log;

import androidx.databinding.BaseObservable;
import androidx.databinding.Bindable;

import com.accessvascularinc.hydroguide.mma.BR;

import org.json.JSONArray;
import org.json.JSONException;

import java.util.OptionalDouble;

/**
 * The observable type containing the current patient's data, including biological data and
 * procedural data.
 */
public class PatientData extends BaseObservable {
  private Facility patient_facility = new Facility();
  private String patientName = "";
  private String patient_id = "";
  private String patient_dob = ""; // An example for the expected format is 01/01/1970.
  private String patient_notes = "";

  private OptionalDouble patient_arm_circumference = OptionalDouble.empty(); // Measured in cm.
  private String patient_insertion_vein = "";
  private String patient_insertion_side = ""; // Can either be the left or right side.
  private OptionalDouble patient_vein_size = OptionalDouble.empty(); // Perhaps measured in mm.
  private String patient_inserter = ""; // This is the name of one conducting the procedure.
  private String patient_reason_insertion = "";
  private String patient_no_of_lumens = ""; // Number of lumens
  private String patient_catheter_size = ""; // Catheter size

  public PatientData() {

  }

  /**
   * Parses patient data as a JSON string to create a new instance. This is used for reading data
   * from a text file.
   *
   * @param dataString the JSON string that represents patient data.
   *
   * @throws JSONException a JSON exception for when the string provided does not result in a
   *                       JSONArray.
   */
  public PatientData(final String dataString) throws JSONException {
    final JSONArray dataStrArray = new JSONArray(dataString);
    final String facilityJson = dataStrArray.getString(0);

    patient_facility = new Facility(facilityJson);
    patientName = dataStrArray.getString(1);
    patient_id = dataStrArray.getString(2);
    patient_dob = dataStrArray.getString(3);
    patient_notes = dataStrArray.getString(4);
    patient_arm_circumference = DataFiles.parseOptionalDouble(dataStrArray.getString(5));
    patient_insertion_vein = dataStrArray.getString(6);
    patient_insertion_side = dataStrArray.getString(7);
    patient_vein_size = DataFiles.parseOptionalDouble(dataStrArray.getString(8));
    patient_inserter = dataStrArray.getString(9);
    patient_reason_insertion = dataStrArray.getString(10);
  }

  /**
   * Gets the name of the facility the patient is located at.
   *
   * @return the patient facility
   */
  @Bindable
  public Facility getPatientFacility() {
    return patient_facility;
  }

  /**
   * Sets the name of the facility the patient is located at.
   *
   * @param newPatientFacility the patient facility
   */
  public void setPatientFacility(final Facility newPatientFacility) {
    this.patient_facility = newPatientFacility;
    notifyPropertyChanged(BR.patientFacility);
  }

  /**
   * Gets the patient's name.
   *
   * @return the patient name
   */
  @Bindable
  public String getPatientName() {
    return patientName;
  }

  /**
   * Sets the patient's name.
   *
   * @param newPatientName the new name
   */
  public void setPatientName(final String newPatientName) {
    this.patientName = newPatientName;
    notifyPropertyChanged(BR.patientName);
  }

  /**
   * Gets the patient's id.
   *
   * @return the patient id
   */
  @Bindable
  public String getPatientId() {
    return patient_id;
  }

  /**
   * Sets the patient's id.
   *
   * @param newPatientId the patient id
   */
  public void setPatientId(final String newPatientId) {
    this.patient_id = newPatientId;
    notifyPropertyChanged(BR.patientId);
  }

  /**
   * Gets the patient's date of birth with expected format 01/01/1970.
   *
   * @return the patient date of birth
   */
  @Bindable
  public String getPatientDob() {
    return patient_dob;
  }

  /**
   * Sets the patient's date of birth with expected format 01/01/1970.
   *
   * @param newPatientDob the patient date of birth
   */
  public void setPatientDob(final String newPatientDob) {
    this.patient_dob = newPatientDob;
    notifyPropertyChanged(BR.patientDob);
  }

  /**
   * Gets the patient's gender.
   *
   * @return the patient gender
   */
  @Bindable
  public String getPatientNotes() {
    return patient_notes;
  }

  /**
   * Sets the patient's notes.
   *
   * @param newPatientNotes the patient notes
   */
  public void setPatientNotes(final String newPatientNotes) {
    this.patient_notes = newPatientNotes;
    notifyPropertyChanged(BR.patientNotes);
  }

  /**
   * Gets the patient's arm circumference in centimeters.
   *
   * @return the patient arm circumference
   */
  @Bindable
  public OptionalDouble getPatientArmCircumference() {
    return patient_arm_circumference;
  }

  /**
   * Sets the patient's arm circumference in centimeters.
   *
   * @param newPatientArmCircumference the patient arm circumference
   */
  public void setPatientArmCircumference(final OptionalDouble newPatientArmCircumference) {
    this.patient_arm_circumference = newPatientArmCircumference;
    notifyPropertyChanged(BR.patientArmCircumference);
  }

  /**
   * Gets the patient's insertion vein.
   *
   * @return the patient insertion vein
   */
  @Bindable
  public String getPatientInsertionVein() {
    return patient_insertion_vein;
  }

  /**
   * Sets the patient's insertion vein.
   *
   * @param newPatientInsertionVein the patient insertion vein
   */
  public void setPatientInsertionVein(final String newPatientInsertionVein) {
    this.patient_insertion_vein = newPatientInsertionVein;
    notifyPropertyChanged(BR.patientInsertionVein);
  }

  /**
   * Gets the patient's insertion side, either left or right.
   *
   * @return the patient insertion side
   */
  @Bindable
  public String getPatientInsertionSide() {
    return patient_insertion_side;
  }

  /**
   * Sets the patient's insertion side, either left or right.
   *
   * @param newPatientInsertionSide the patient insertion side
   */
  public void setPatientInsertionSide(final String newPatientInsertionSide) {
    this.patient_insertion_side = newPatientInsertionSide;
    notifyPropertyChanged(BR.patientInsertionSide);
  }

  /**
   * Gets the patient's vein size in millimeters.
   *
   * @return the patient vein size
   */
  @Bindable
  public OptionalDouble getPatientVeinSize() {
    return patient_vein_size;
  }

  /**
   * Sets the patient's vein size in millimeters.
   *
   * @param newPatientVeinSize the patient vein size
   */
  public void setPatientVeinSize(final OptionalDouble newPatientVeinSize) {
    this.patient_vein_size = newPatientVeinSize;
    notifyPropertyChanged(BR.patientVeinSize);
  }

  /**
   * Gets the name of the person conducting the procedure.
   *
   * @return the name of the person conducting the procedure
   */
  @Bindable
  public String getPatientInserter() {
    return patient_inserter;
  }

  /**
   * Sets the name of the person conducting the procedure.
   *
   * @param newPatientInserter the name of the person conducting the procedure
   */
  public void setPatientInserter(final String newPatientInserter) {
    this.patient_inserter = newPatientInserter;
    notifyPropertyChanged(BR.patientInserter);
  }

  /**
   * Gets the reason for the patient's procedure.
   *
   * @return the patient's reason for insertion
   */
  @Bindable
  public String getPatientReasonInsertion() {
    return patient_reason_insertion;
  }

  /**
   * Sets the reason for the patient's procedure.
   *
   * @param newPatientReasonInsertion the patient's reason for insertion
   */
  public void setPatientReasonInsertion(final String newPatientReasonInsertion) {
    this.patient_reason_insertion = newPatientReasonInsertion;
    notifyPropertyChanged(BR.patientReasonInsertion);
  }

  /**
   * Gets the patient's number of lumens.
   *
   * @return the patient's number of lumens
   */
  @Bindable
  public String getPatientNoOfLumens() {
    return patient_no_of_lumens;
  }

  /**
   * Sets the patient's number of lumens.
   *
   * @param newPatientNoOfLumens the patient's number of lumens
   */
  public void setPatientNoOfLumens(final String newPatientNoOfLumens) {
    this.patient_no_of_lumens = newPatientNoOfLumens;
    notifyPropertyChanged(BR.patientNoOfLumens);
  }

  /**
   * Gets the patient's catheter size.
   *
   * @return the patient's catheter size
   */
  @Bindable
  public String getPatientCatheterSize() {
    return patient_catheter_size;
  }

  /**
   * Sets the patient's catheter size.
   *
   * @param newPatientCatheterSize the patient's catheter size
   */
  public void setPatientCatheterSize(final String newPatientCatheterSize) {
    this.patient_catheter_size = newPatientCatheterSize;
    notifyPropertyChanged(BR.patientCatheterSize);
  }

  /**
   * Get the patient's full procedural data as an array of strings in the following order: facility,
   * name, id, date of birth, gender, arm circumference (in cm), insertion vein, insertion side,
   * vein size (in mm), the name of the person completing the procedure, and the reason for the
   * procedure.
   *
   * @return the array of strings
   */
  public String[] getPatientDataText() throws JSONException {
    Log.i("ABC", "patient_id: " + patient_id);
    Log.i("ABC",
            "patient_facility.getFacilityDataText(): " + patient_facility.getFacilityDataText());
    Log.i("ABC", "patientName: " + patientName);
    Log.i("ABC", "patient_id MRN " + patient_id);
    Log.i("ABC", "patient_dob " + patient_dob);
    Log.i("ABC", "-----------------");
    Log.i("ABC",
            "patient_arm_circumference " + (patient_arm_circumference.isPresent() ? Double.toString(
                    patient_arm_circumference.getAsDouble()) : ""));
    Log.i("ABC", "patient_insertion_vein " + patient_insertion_vein);
    Log.i("ABC", "patient_insertion_side " + patient_insertion_side);
    Log.i("ABC", "patient_inserter " + patient_inserter);
    Log.i("ABC", "patient_reason_insertion " + patient_reason_insertion);
    Log.i("ABC",
            "patient_vein_size " + (patient_vein_size.isPresent() ?
                    Double.toString(patient_vein_size.getAsDouble()) : ""));

    return new String[] {
            patient_facility.getFacilityDataText(),
            patientName,
            patient_id,
            patient_dob,
            patient_notes,
            patient_arm_circumference.isPresent() ? Double.toString(
                    patient_arm_circumference.getAsDouble()) : "",
            patient_insertion_vein,
            patient_insertion_side,
            patient_vein_size.isPresent() ? Double.toString(patient_vein_size.getAsDouble()) : "",
            patient_inserter,
            patient_reason_insertion,
    };
  }

  /**
   * Check if any patient data field has been filled in.
   * @return true if at least one field has been modified from its default value.
   */
  public boolean hasAnyData() {
    return (patient_id != null && !patient_id.isEmpty())
        || (patientName != null && !patientName.isEmpty())
        || (patient_dob != null && !patient_dob.isEmpty())
        || (patient_notes != null && !patient_notes.isEmpty())
        || (patient_arm_circumference != null && patient_arm_circumference.isPresent())
        || (patient_vein_size != null && patient_vein_size.isPresent())
        || (patient_insertion_vein != null && !patient_insertion_vein.isEmpty())
        || (patient_insertion_side != null && !patient_insertion_side.isEmpty())
        || (patient_inserter != null && !patient_inserter.isEmpty())
        || (patient_reason_insertion != null && !patient_reason_insertion.isEmpty())
        || (patient_no_of_lumens != null && !patient_no_of_lumens.isEmpty())
        || (patient_catheter_size != null && !patient_catheter_size.isEmpty())
        || (patient_facility != null
            && patient_facility.getFacilityName() != null
            && !patient_facility.getFacilityName().isEmpty());
  }

  /**
   * Clear patient data.
   */
  public void clearPatientData() {
    patient_facility = new Facility();
    patientName = "";
    patient_id = "";
    patient_dob = "";
    patient_notes = "";
    patient_arm_circumference = OptionalDouble.empty();
    patient_insertion_vein = "";
    patient_insertion_side = "";
    patient_vein_size = OptionalDouble.empty();
    patient_inserter = "";
    patient_reason_insertion = "";
    patient_no_of_lumens = "";
    patient_catheter_size = "";
  }
}
