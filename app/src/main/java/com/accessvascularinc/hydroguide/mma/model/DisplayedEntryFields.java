package com.accessvascularinc.hydroguide.mma.model;

import androidx.databinding.BaseObservable;
import androidx.databinding.Bindable;

import com.accessvascularinc.hydroguide.mma.BR;

import org.json.JSONArray;
import org.json.JSONException;

public class DisplayedEntryFields extends BaseObservable {
  private boolean showName = true;
  private boolean showId = true;
  private boolean showDob = true;
  private boolean showReasonForInsertion = true;
  private boolean showInsertionVein = true;
  private boolean showPatientSide = true;
  private boolean showArmCircumference = true;
  private boolean showVeinSize = true;
  private boolean showNoOfLumens = false;
  private boolean showCatheterSize = false;

  public DisplayedEntryFields() {

  }

  public DisplayedEntryFields(final String dataString) throws JSONException {
    final JSONArray dataStrArray = new JSONArray(dataString);
    showName = Boolean.parseBoolean(dataStrArray.getString(0));
    showId = Boolean.parseBoolean(dataStrArray.getString(1));
    showDob = Boolean.parseBoolean(dataStrArray.getString(2));
    showReasonForInsertion = Boolean.parseBoolean(dataStrArray.getString(3));
    showInsertionVein = Boolean.parseBoolean(dataStrArray.getString(4));
    showPatientSide = Boolean.parseBoolean(dataStrArray.getString(5));
    showArmCircumference = Boolean.parseBoolean(dataStrArray.getString(6));
    showVeinSize = Boolean.parseBoolean(dataStrArray.getString(7));
  }

  @Bindable
  public boolean isShowName() {
    return showName;
  }

  public void setShowName(final boolean newShowName) {
    this.showName = newShowName;
    notifyPropertyChanged(BR.showName);
  }

  @Bindable
  public boolean isShowId() {
    return showId;
  }

  public void setShowId(final boolean newShowId) {
    this.showId = newShowId;
    notifyPropertyChanged(BR.showId);
  }

  @Bindable
  public boolean isShowDob() {
    return showDob;
  }

  public void setShowDob(final boolean newShowDob) {
    this.showDob = newShowDob;
    notifyPropertyChanged(BR.showDob);
  }

  @Bindable
  public boolean isShowReasonForInsertion() {
    return showReasonForInsertion;
  }

  public void setShowReasonForInsertion(final boolean newShowReasonForInsertion) {
    this.showReasonForInsertion = newShowReasonForInsertion;
    notifyPropertyChanged(BR.showReasonForInsertion);
  }

  @Bindable
  public boolean isShowInsertionVein() {
    return showInsertionVein;
  }

  public void setShowInsertionVein(final boolean newShowInsertionVein) {
    this.showInsertionVein = newShowInsertionVein;
    notifyPropertyChanged(BR.showInsertionVein);
  }

  @Bindable
  public boolean isShowPatientSide() {
    return showPatientSide;
  }

  public void setShowPatientSide(final boolean newShowPatientSide) {
    this.showPatientSide = newShowPatientSide;
    notifyPropertyChanged(BR.showPatientSide);
  }

  @Bindable
  public boolean isShowArmCircumference() {
    return showArmCircumference;
  }

  public void setShowArmCircumference(final boolean newShowArmCircumference) {
    this.showArmCircumference = newShowArmCircumference;
    notifyPropertyChanged(BR.showArmCircumference);
  }

  @Bindable
  public boolean isShowVeinSize() {
    return showVeinSize;
  }

  public void setShowVeinSize(final boolean newShowVeinSize) {
    this.showVeinSize = newShowVeinSize;
    notifyPropertyChanged(BR.showVeinSize);
  }

  @Bindable
  public boolean isShowNoOfLumens() {
    return showNoOfLumens;
  }

  public void setShowNoOfLumens(final boolean newShowNoOfLumens) {
    this.showNoOfLumens = newShowNoOfLumens;
    notifyPropertyChanged(BR.showNoOfLumens);
  }

  @Bindable
  public boolean isShowCatheterSize() {
    return showCatheterSize;
  }

  public void setShowCatheterSize(final boolean newShowCatheterSize) {
    this.showCatheterSize = newShowCatheterSize;
    notifyPropertyChanged(BR.showCatheterSize);
  }

  public String[] getDisplayedEntryFieldsDataText() {
    return new String[] {
        String.valueOf(showName),
        String.valueOf(showId),
        String.valueOf(showDob),
        String.valueOf(showReasonForInsertion),
        String.valueOf(showInsertionVein),
        String.valueOf(showPatientSide),
        String.valueOf(showArmCircumference),
        String.valueOf(showVeinSize),
    };
  }
}