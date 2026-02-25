package com.accessvascularinc.hydroguide.mma.model;

import android.content.Context;

import androidx.databinding.BaseObservable;
import androidx.databinding.Bindable;

import com.accessvascularinc.hydroguide.mma.BR;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.stream.IntStream;

public class ProfileState extends BaseObservable {
  private UserProfile selectedProfile = null;
  private String[] savedProfilesList = null;
  private Facility[] facilityList = new Facility[0];



  @Bindable
  public UserProfile getSelectedProfile() {
    return selectedProfile;
  }

  public void setSelectedProfile(final UserProfile newSelectedProfile) {
    this.selectedProfile = newSelectedProfile;
    notifyPropertyChanged(BR.selectedProfile);
  }

  @Bindable
  public String[] getSavedProfilesList() {
    return savedProfilesList.clone();
  }

  public void setSavedProfilesList(final String[] newSavedProfilesList) {
    this.savedProfilesList = newSavedProfilesList.clone();
    notifyPropertyChanged(BR.savedProfilesList);
  }

  @Bindable
  public Facility[] getFacilityList() {
    return facilityList.clone();
  }

  public void setFacilityList(final Facility[] newFacilityList) {
    if (!facilityList.equals(newFacilityList)) {
      this.facilityList = newFacilityList.clone();
      notifyPropertyChanged(BR.facilityList);
    }
  }

  public String getFacilitiesDataText() throws JSONException {
    final JSONArray json = new JSONArray();
    for (final Facility facility : facilityList) {
      json.put(new JSONObject(facility.getFacilityDataText()));
    }
    return json.toString();
  }

  public void updateFacilityRecency(final Facility selectedFacility, final Context context) {
    // Find facility by ID, update with latest time, and save changes to file.
    final String facilityId = selectedFacility.getId();
    // Manually set facilities have their Id set to an empty string.
    if (facilityId.isEmpty()) {
      return;
    }

    final List<Facility> facilities = new ArrayList<>(Arrays.asList(getFacilityList()));
    final int facilityIdx = IntStream.range(0, facilities.size())
            .filter(i -> (Objects.equals(facilityId, facilities.get(i).getId())))
            .findFirst().orElse(-1);
    if (facilityIdx >= 0) {
      facilities.set(facilityIdx, selectedFacility);
      setFacilityList(facilities.toArray(new Facility[0]));

      // Write to file.
      try {
        final String writeData = getFacilitiesDataText();
        //prasanna first save table for Patient
        DataFiles.writeFacilitiesToFile(writeData, context);
      } catch (final JSONException e) {
        throw new RuntimeException(e);
      }
    }
  }

}
