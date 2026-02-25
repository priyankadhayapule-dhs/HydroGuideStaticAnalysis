package com.accessvascularinc.hydroguide.mma.model;

import android.util.Log;

import androidx.databinding.BaseObservable;
import androidx.databinding.Bindable;

import com.accessvascularinc.hydroguide.mma.BR;
import com.accessvascularinc.hydroguide.mma.ui.input.InputUtils;
import com.accessvascularinc.hydroguide.mma.ui.plot.ImpedanceDataSeries;

import org.json.JSONArray;
import org.json.JSONException;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.OptionalDouble;

/**
 * A data binding object for UI elements that keeps track of all the data relating to an encounter,
 * also known as a procedure. Namely, it tracks data about the patient having the procedure
 * performed on, state of the catheter being used, and all the captures taken during the procedure.
 */
public class EncounterData extends BaseObservable {
  // --- Added fields for ProcedureEntity compatibility ---
  private String organizationId;
  private Long facilityId;
  private String facilityOrLocation;
  private String clinicianId;
  private long patientId;
  private long deviceId;
  private long tabletId;
  /**
   * The minimum length of the catheter after being trimmed in cm.
   */
  public static final double MIN_TRIM_LENGTH_CM = 15.0;
  /**
   * The maximum length of the catheter after being trimmed in cm.
   */
  public static final double MAX_TRIM_LENGTH_CM = 55.0;
  /**
   * A standard or average length of the catheter after being trimmed in cm to serve as a default
   * starting point of values to choose from.
   */
  public static final double STANDARD_TRIM_LENGTH_CM = 40.0;
  /**
   * A standard or average length of the portion of the catheter inserted in the patient in cm to
   * serve as a default starting point of values to choose from.
   */
  public static final double DEFAULT_INSERT_LENGTH = 30.0;
  private Date endTime = null;
  private String uuid = null;
  private Date startTime = null;
  private OptionalDouble trimLengthCm = OptionalDouble.empty();
  private OptionalDouble insertedCatheterCm = OptionalDouble.empty();
  private OptionalDouble externalCatheterCm = OptionalDouble.empty();
  private PatientData patient = new PatientData();
  private Capture externalCapture = null;
  private Capture[] intravCaptures = new Capture[0];
  private String[] ultrasoundCapturePaths = new String[0];
  private String[] selectedUltrasoundPathsForReport = null; // Temporary storage for PDF generation
  private Boolean hydroGuideConfirmed = null;
  private String appSoftwareVersion = "";
  private String controllerFirmwareVersion = "";
  private Boolean procedureStarted = false;
  private String dataDirPath = null;
  private EncounterState state = EncounterState.Uninitialized;
  private Capture latestCapture = null;
  private Capture selectedCapture = null;

  // --- Setup screen fields (for electrode distances) ---
  private OptionalDouble electrodeDistance1Cm = OptionalDouble.empty();
  private OptionalDouble electrodeDistance2Cm = OptionalDouble.empty();
  private OptionalDouble electrodeDistance3Cm = OptionalDouble.empty();
  private ImpedanceDataSeries impedanceData = null;

  // --- Ultrasound tracking fields ---
  private Boolean isUltrasoundUsed = false;
  private Boolean isUltrasoundCaptureSaved = false;

  @Bindable
  public Capture getLatestCapture() {
    return latestCapture;
  }

  public void setLatestCapture(Capture latestCapture) {
    Log.d("EncounterData",
            "setLatestCapture: captureId=" +
                    (latestCapture != null ? latestCapture.getCaptureId() : "null") +
                    ", isIntravascular=" +
                    (latestCapture != null ? latestCapture.getIsIntravascular() : "N/A"));
    this.latestCapture = latestCapture;
    notifyPropertyChanged(BR.latestCapture);
  }

  @Bindable
  public Capture getSelectedCapture() {
    return selectedCapture;
  }

  public void setSelectedCapture(Capture selectedCapture) {
    Log.d("EncounterData",
            "setSelectedCapture: captureId=" +
                    (selectedCapture != null ? selectedCapture.getCaptureId() : "null") +
                    ", isIntravascular=" +
                    (selectedCapture != null ? selectedCapture.getIsIntravascular() : "N/A"));
    this.selectedCapture = selectedCapture;
    notifyPropertyChanged(BR.selectedCapture);
  }

  public EncounterData() {
    initEncounter();
  }

  // --- Getters and setters for new fields ---
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

  private void initEncounter() {
    startTime = null;
    endTime = null;
    uuid = null;
    trimLengthCm = OptionalDouble.empty();
    insertedCatheterCm = OptionalDouble.empty();
    externalCatheterCm = OptionalDouble.empty();
    patient = new PatientData();
    externalCapture = null;
    intravCaptures = new Capture[0];
    ultrasoundCapturePaths = new String[0];
    hydroGuideConfirmed = null;
    appSoftwareVersion = "";
    controllerFirmwareVersion = "";
    procedureStarted = false;
    dataDirPath = null;
    state = EncounterState.Uninitialized;
    latestCapture = null;
    selectedCapture = null;
    electrodeDistance1Cm = OptionalDouble.empty();
    electrodeDistance2Cm = OptionalDouble.empty();
    electrodeDistance3Cm = OptionalDouble.empty();
    impedanceData = new ImpedanceDataSeries();
    isUltrasoundUsed = false;
    isUltrasoundCaptureSaved = false;
  }

  public void initializeState() {
    state = (state == EncounterState.Uninitialized) ? EncounterState.Idle : state;
  }

  @Bindable
  public Date getStartTime() {
    return startTime;
  }

  public void setStartTime(final Date newTime) {
    startTime = newTime;
    notifyPropertyChanged(BR.startTime);
  }

  public String getStartTimeText() {
    if (startTime == null) {
      return "";
    }
    return (new SimpleDateFormat(InputUtils.FILE_DATE_TIME_PATTERN, Locale.US)).format(startTime);
  }

  public String getStartTimeShortText() {
    if (startTime == null) {
      return "";
    }
    return (new SimpleDateFormat(InputUtils.SHORT_DATE_PATTERN, Locale.US)).format(startTime);
  }

  @Bindable
  public PatientData getPatient() {
    return patient;
  }

  public void setPatient(final PatientData newPatientData) {
    this.patient = newPatientData;
    notifyPropertyChanged(BR.patient);
  }

  @Bindable
  public Capture getExternalCapture() {
    return externalCapture;
  }

  public void setExternalCapture(final Capture newCapture) {
    externalCapture = newCapture;
    notifyPropertyChanged(BR.externalCapture);
  }

  @Bindable
  public Capture[] getIntravCaptures() {
    return intravCaptures;
  }

  public void setIntravCaptures(final Capture[] newCaptures) {
    intravCaptures = newCaptures;
    notifyPropertyChanged(BR.intravCaptures);
  }

  @Bindable
  public String[] getUltrasoundCapturePaths() {
    return ultrasoundCapturePaths;
  }

  public void setUltrasoundCapturePaths(final String[] paths) {
    ultrasoundCapturePaths = paths;
    notifyPropertyChanged(BR.ultrasoundCapturePaths);
  }

  /**
   * Get impedance data series for plotting
   *
   * @return the impedance data series
   */
  public ImpedanceDataSeries getImpedanceData() {
    return impedanceData;
  }

  /**
   * Add an ultrasound capture path to the list
   *
   * @param path the file path to add
   */
  public void addUltrasoundCapturePath(final String path) {
    String[] newPaths = new String[ultrasoundCapturePaths.length + 1];
    System.arraycopy(ultrasoundCapturePaths, 0, newPaths, 0, ultrasoundCapturePaths.length);
    newPaths[ultrasoundCapturePaths.length] = path;
    setUltrasoundCapturePaths(newPaths);
  }

  /**
   * Get the count of ultrasound captures
   *
   * @return the number of ultrasound captures
   */
  public int getUltrasoundCaptureCount() {
    return ultrasoundCapturePaths.length;
  }

  /**
   * Set selected ultrasound paths for report generation (temporary storage)
   *
   * @param paths the selected paths
   */
  public void setSelectedUltrasoundPathsForReport(final String[] paths) {
    selectedUltrasoundPathsForReport = paths;
  }

  /**
   * Get ultrasound paths for report generation Returns selected paths if set, otherwise returns all
   * paths
   *
   * @return the ultrasound paths to use in report
   */
  public String[] getUltrasoundPathsForReport() {
    return (selectedUltrasoundPathsForReport != null) ? selectedUltrasoundPathsForReport :
            ultrasoundCapturePaths;
  }

  /**
   * Gets the full length of the trimmed catheter in cm.
   *
   * @return the full length of the trimmed catheter in cm.
   */
  @Bindable
  public OptionalDouble getTrimLengthCm() {
    return trimLengthCm;
    // return OptionalDouble.of(40.0);

  }

  /**
   * Sets the length of the catheter after being trimmed in cm.
   *
   * @param newLength_cm the new length of the catheter after being trimmed in cm.
   */
  public void setTrimLengthCm(final OptionalDouble newLength_cm) {
    trimLengthCm = newLength_cm;
    notifyPropertyChanged(BR.trimLengthCm);
  }

  @Bindable
  public OptionalDouble getInsertedCatheterCm() {
    return insertedCatheterCm;
  }

  /**
   * Sets the length of the portion of the catheter inserted in the patient in cm.
   *
   * @param newLength_cm the new length of the inserted portion of the catheter in cm
   */
  public void setInsertedCatheterCm(final OptionalDouble newLength_cm) {
    insertedCatheterCm = newLength_cm;
    notifyPropertyChanged(BR.insertedCatheterCm);
  }

  /**
   * Gets the remaining length of the catheter still outside of the patient in cm.
   *
   * @return the length of the catheter still outside the patient in cm.
   */
  @Bindable
  public OptionalDouble getExternalCatheterCm() {
    return externalCatheterCm;
  }

  /**
   * Sets the remaining length of the catheter still outside of the patient in cm.
   *
   * @param newLength_cm the new length of the catheter still outside the patient in cm.
   */
  public void setExternalCatheterCm(final OptionalDouble newLength_cm) {
    externalCatheterCm = newLength_cm;
    notifyPropertyChanged(BR.externalCatheterCm);
  }

  /**
   * Gets whether the catheter tip's location was confirmed using this app.
   *
   * @return whether or not catheter tip's location was confirmed using this app. This is null until
   * the user chooses "Yes" or "No" on the report summary screen.
   */
  @Bindable
  public Boolean getHydroGuideConfirmed() {
    return hydroGuideConfirmed;
  }

  /**
   * Sets whether the catheter tip's location was confirmed using this app.
   *
   * @param hgConfirmed whether or not catheter tip's location was confirmed using this app. True
   *                    means "Yes" and false means "No".
   */
  public void setHydroGuideConfirmed(final Boolean hgConfirmed) {
    hydroGuideConfirmed = hgConfirmed;
    notifyPropertyChanged(BR.hydroGuideConfirmed);
  }

  public String getAppSoftwareVersion() {
    return appSoftwareVersion;
  }

  public void setAppSoftwareVersion(final String appSwVer) {
    this.appSoftwareVersion = appSwVer;
  }

  public String getControllerFirmwareVersion() {
    return controllerFirmwareVersion;
  }

  public void setControllerFirmwareVersion(final String controllerFwVer) {
    this.controllerFirmwareVersion = controllerFwVer;
  }

  /**
   * Gets the path on the tablet's storage that is being used to save data files for the current
   * procedure.
   *
   * @return the full path as a string to the directory being used to save procedure data files.
   */
  @Bindable
  public String getDataDirPath() {
    return dataDirPath;
  }

  /**
   * Sets the path on the tablet's storage that will be used to save data files for the current
   * procedure.
   *
   * @param path the full path as a string to the directory to be used to save procedure data
   *             files.
   */
  public void setDataDirPath(final String path) {
    dataDirPath = path;
    notifyPropertyChanged(BR.dataDirPath);
  }

  @Bindable
  public EncounterState getState() {
    return state;
  }

  public void setState(final EncounterState encounterState) {
    state = encounterState;
    notifyPropertyChanged(BR.state);
  }

  @Bindable
  public Boolean getProcedureStarted() {
    return procedureStarted;
  }

  public void setProcedureStarted(final boolean started) {
    procedureStarted = started;
  }

  @Override
  public void notifyPropertyChanged(final int fieldId) {
    if (state == EncounterState.Uninitialized) {
      state = EncounterState.Idle;
    }

    super.notifyPropertyChanged(fieldId);
  }

  /**
   * Get readable text string of data about the patient having the procedure performed on, state of
   * the catheter being used, and the external capture taken.
   *
   * @return a readable text string of this procedure's data, minus the intravascular captures.
   *
   * @throws JSONException a JSON exception when trying to format an array into a string.
   */
  public String[] getProcedureDataText() throws JSONException {
    // TODO: Improve this to a JSON object to facilitate parsing and reading data
    // from file.
    Log.v("ABC", "startTime: " + startTime);
    Log.v("ABC", "trimLengthCm: " +
            (trimLengthCm.isPresent() ? Double.toString(trimLengthCm.getAsDouble()) : ""));
    Log.v("ABC", "insertedCatheterCm: "
            + (insertedCatheterCm.isPresent() ? Double.toString(insertedCatheterCm.getAsDouble()) :
            ""));
    Log.v("ABC", "externalCatheterCm: "
            + (externalCatheterCm.isPresent() ? Double.toString(externalCatheterCm.getAsDouble()) :
            ""));
    Log.v("ABC", "hydroGuideConfirmed: " +
            (hydroGuideConfirmed != null ? hydroGuideConfirmed.toString() : ""));
    Log.v("ABC", "state: " + state.toString());
    Log.v("ABC", "appSoftwareVersion: " + appSoftwareVersion);
    Log.v("ABC", "controllerFirmwareVersion: " + controllerFirmwareVersion);
    // prasanna
    return new String[] {
            (startTime == null) ? "" : startTime.toString(),
            trimLengthCm.isPresent() ? Double.toString(trimLengthCm.getAsDouble()) : "",
            insertedCatheterCm.isPresent() ? Double.toString(insertedCatheterCm.getAsDouble()) : "",
            externalCatheterCm.isPresent() ? Double.toString(externalCatheterCm.getAsDouble()) : "",
            new JSONArray(patient.getPatientDataText()).toString(),
            hydroGuideConfirmed != null ? hydroGuideConfirmed.toString() : "",
            externalCapture != null ? new JSONArray(
                    externalCapture.getCaptureDataText()).toString() : "",
            new JSONArray(getIntravCapsText()).toString(),
            state.toString(),
            appSoftwareVersion,
            controllerFirmwareVersion,
            new JSONArray(ultrasoundCapturePaths).toString(),
            isUltrasoundUsed != null ? isUltrasoundUsed.toString() : "false",
            isUltrasoundCaptureSaved != null ? isUltrasoundCaptureSaved.toString() : "false",
    };
  }

  /**
   * Get readable text string of this procedure's intravascular captures in JSON format.
   *
   * @return a JSON formatted string of this procedure's intravascular captures.
   *
   * @throws JSONException a JSON exception when trying to format an array into a string.
   */
  public String[] getIntravCapsText() throws JSONException {
    final String[] intravCaps = new String[intravCaptures.length];
    for (int i = 0; i < intravCaptures.length; i++) {
      intravCaps[i] = new JSONArray(intravCaptures[i].getCaptureDataText()).toString();
    }
    return intravCaps;
  }

  /**
   * Clears or resets this procedure data instance to a default state.
   */
  public void clearProcedureData() {
    setStartTime(null);
    setTrimLengthCm(OptionalDouble.empty());
    setInsertedCatheterCm(OptionalDouble.empty());
    setExternalCatheterCm(OptionalDouble.empty());
    setExternalCapture(null);
    setIntravCaptures(new Capture[0]);
    setUltrasoundCapturePaths(new String[0]);
    setHydroGuideConfirmed(null);
    setAppSoftwareVersion("");
    setControllerFirmwareVersion("");
    setDataDirPath(null);
  }

  public void clearEncounterData() {
    initEncounter();
    notifyPropertyChanged(BR._all);
  }

  public boolean isLive() {
    return (getState() == EncounterState.Active || getState() == EncounterState.Suspended ||
            getState() == EncounterState.IntraSuspended ||
            getState() == EncounterState.ExternalSuspended ||
            getState() == EncounterState.DualSuspended);
  }

  public boolean isConcluded() {
    return (getState() == EncounterState.ConcludedIncomplete ||
            getState() == EncounterState.CompletedUnsuccessful ||
            getState() == EncounterState.CompletedSuccessful);
  }

  // --- Getters and setters for electrode distance fields ---

  /**
   * Gets the first electrode distance measurement in cm.
   *
   * @return OptionalDouble containing the distance, or empty if not set
   */
  @Bindable
  public OptionalDouble getElectrodeDistance1Cm() {
    return electrodeDistance1Cm;
  }

  /**
   * Sets the first electrode distance measurement in cm.
   *
   * @param distance1 the distance value, or empty OptionalDouble
   */
  public void setElectrodeDistance1Cm(final OptionalDouble distance1) {
    this.electrodeDistance1Cm = distance1;
    notifyPropertyChanged(BR.electrodeDistance1Cm);
  }

  /**
   * Gets the second electrode distance measurement in cm.
   *
   * @return OptionalDouble containing the distance, or empty if not set
   */
  @Bindable
  public OptionalDouble getElectrodeDistance2Cm() {
    return electrodeDistance2Cm;
  }

  /**
   * Sets the second electrode distance measurement in cm.
   *
   * @param distance2 the distance value, or empty OptionalDouble
   */
  public void setElectrodeDistance2Cm(final OptionalDouble distance2) {
    this.electrodeDistance2Cm = distance2;
    notifyPropertyChanged(BR.electrodeDistance2Cm);
  }

  /**
   * Gets the third electrode distance measurement in cm.
   *
   * @return OptionalDouble containing the distance, or empty if not set
   */
  @Bindable
  public OptionalDouble getElectrodeDistance3Cm() {
    return electrodeDistance3Cm;
  }

  /**
   * Sets the third electrode distance measurement in cm.
   *
   * @param distance3 the distance value, or empty OptionalDouble
   */
  public void setElectrodeDistance3Cm(final OptionalDouble distance3) {
    this.electrodeDistance3Cm = distance3;
    notifyPropertyChanged(BR.electrodeDistance3Cm);
  }

  /**
   * Gets whether ultrasound was used during the procedure.
   *
   * @return true if ultrasound was available and used, false otherwise
   */
  @Bindable
  public Boolean getIsUltrasoundUsed() {
    return isUltrasoundUsed;
  }

  /**
   * Sets whether ultrasound was used during the procedure.
   *
   * @param isUltrasoundUsed true if ultrasound was available and used
   */
  public void setIsUltrasoundUsed(final Boolean isUltrasoundUsed) {
    this.isUltrasoundUsed = isUltrasoundUsed;
    notifyPropertyChanged(BR.isUltrasoundUsed);
  }

  /**
   * Gets whether an ultrasound capture was saved during the procedure.
   *
   * @return true if an ultrasound capture was saved, false otherwise
   */
  @Bindable
  public Boolean getIsUltrasoundCaptureSaved() {
    return isUltrasoundCaptureSaved;
  }

  /**
   * Sets whether an ultrasound capture was saved during the procedure.
   *
   * @param isUltrasoundCaptureSaved true if an ultrasound capture was saved
   */
  public void setIsUltrasoundCaptureSaved(final Boolean isUltrasoundCaptureSaved) {
    this.isUltrasoundCaptureSaved = isUltrasoundCaptureSaved;
    notifyPropertyChanged(BR.isUltrasoundCaptureSaved);
  }

}
