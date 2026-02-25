package com.accessvascularinc.hydroguide.mma.model;

import android.util.Log;

import androidx.databinding.BaseObservable;
import androidx.databinding.Bindable;

import com.accessvascularinc.hydroguide.mma.BR;

import org.json.JSONArray;
import org.json.JSONException;

import java.util.Arrays;

public class UserPreferences extends BaseObservable {
  private DisplayedEntryFields displayedEntryFields = new DisplayedEntryFields();
  private WaveformCaptureLabel waveformCaptureLabel = WaveformCaptureLabel.InsertedLength;
  private boolean enableAudio = true;
  private FacilitySelectionMode facilitySelectionMode = FacilitySelectionMode.Dropdown;
  private String[] favoriteFacilitiesIds = new String[0];
  private Facility manualFacility = new Facility();

  public UserPreferences() {

  }

  public UserPreferences(final String dataString) throws JSONException {
    final JSONArray dataStrArray = new JSONArray(dataString);
    displayedEntryFields = new DisplayedEntryFields(dataStrArray.getString(0));
    waveformCaptureLabel = WaveformCaptureLabel.valueOf(dataStrArray.getString(1));
    enableAudio = Boolean.parseBoolean(dataStrArray.getString(2));
    facilitySelectionMode = FacilitySelectionMode.valueOf(dataStrArray.getString(3));
    favoriteFacilitiesIds = DataFiles.getArrayFromString(dataStrArray.getString(4));
    manualFacility = new Facility(dataStrArray.getString(5));
  }

  @Bindable
  public WaveformCaptureLabel getWaveformCaptureLabel() {
    return waveformCaptureLabel;
  }

  public void setWaveformCaptureLabel(final WaveformCaptureLabel newWaveformCaptureLabel) {
    this.waveformCaptureLabel = newWaveformCaptureLabel;
    notifyPropertyChanged(BR.waveformCaptureLabel);
  }

  @Bindable
  public DisplayedEntryFields getDisplayedEntryFields() {
    return displayedEntryFields;
  }

  public void setDisplayedEntryFields(final DisplayedEntryFields newDisplayedEntryFields) {
    this.displayedEntryFields = newDisplayedEntryFields;
    notifyPropertyChanged(BR.displayedEntryFields);
  }

  @Bindable
  public boolean isEnableAudio() {
    return enableAudio;
  }

  public void setEnableAudio(final boolean newEnableAudio) {
    this.enableAudio = newEnableAudio;
    notifyPropertyChanged(BR.enableAudio);
  }

  @Bindable
  public FacilitySelectionMode getFacilitySelectionMode() {
    return facilitySelectionMode;
  }

  public void setFacilitySelectionMode(final FacilitySelectionMode newFacilitySelectionMode) {
    this.facilitySelectionMode = newFacilitySelectionMode;
    notifyPropertyChanged(BR.facilitySelectionMode);
  }

  @Bindable
  public String[] getFavoriteFacilitiesIds() {
    return favoriteFacilitiesIds.clone();
  }

  public void setFavoriteFacilitiesIds(final String[] newFavFacilities) {
    this.favoriteFacilitiesIds = newFavFacilities.clone();
    notifyPropertyChanged(BR.favoriteFacilitiesIds);
  }

  @Bindable
  public Facility getManualFacility() {
    return manualFacility;
  }

  public void setManualFacility(final Facility newManualFacility) {
    this.manualFacility = newManualFacility;
    notifyPropertyChanged(BR.manualFacility);
  }

  public String[] getUserPreferencesDataText() throws JSONException {

    for(int i=0;i<displayedEntryFields.getDisplayedEntryFieldsDataText().length;i++){
      Log.d("ABC", "displayedEntryFields.getDisplayedEntryFieldsDataText(): "+displayedEntryFields.getDisplayedEntryFieldsDataText()[i]);
    }
    Log.d("ABC", " waveformCaptureLabel.toString(): "+ waveformCaptureLabel.toString());
    Log.d("ABC", "String.valueOf(enableAudio): "+String.valueOf(enableAudio));
    Log.d("ABC", "facilitySelectionMode: "+facilitySelectionMode);
    Log.d("ABC", "Arrays.toString(favoriteFacilitiesIds): "+Arrays.toString(favoriteFacilitiesIds));
    Log.d("ABC", "manualFacility.getFacilityDataText(): "+manualFacility.getFacilityDataText());
    return new String[] {
            new JSONArray(displayedEntryFields.getDisplayedEntryFieldsDataText()).toString(),
            waveformCaptureLabel.toString(),
            String.valueOf(enableAudio),
            facilitySelectionMode.toString(),
            Arrays.toString(favoriteFacilitiesIds),
            manualFacility.getFacilityDataText(),
    };
  }

  public enum WaveformCaptureLabel {
    ExposedLength,
    InsertedLength,
  }

  public enum FacilitySelectionMode {
    OnceManual,
    Dropdown,
    ManualEachProcedure,
  }
}
