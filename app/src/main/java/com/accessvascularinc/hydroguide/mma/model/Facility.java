package com.accessvascularinc.hydroguide.mma.model;

import androidx.annotation.NonNull;
import androidx.databinding.BaseObservable;
import androidx.databinding.Bindable;

import com.accessvascularinc.hydroguide.mma.BR;

import org.json.JSONException;
import org.json.JSONObject;

public class Facility extends BaseObservable {
  private String id = ""; // A manually entered facility will have no unique Id.
  private String facilityName = "";
  private String dateLastUsed = "";

  public Facility() {
  }

  public Facility(final String newId, final String name, final String lastUsed) {
    this.id = newId;
    this.facilityName = name;
    this.dateLastUsed = lastUsed;
  }

  public Facility(final String jsonStr) throws JSONException {
    final JSONObject json = new JSONObject(jsonStr);

    this.id = json.getString("id");
    this.facilityName = json.getString("facilityName");
    this.dateLastUsed = json.getString("dateLastUsed");
  }

  public String getId() {
    return id;
  }

  @Bindable
  public String getFacilityName() {
    return facilityName;
  }

  public void setFacilityName(final String newProfileName) {
    this.facilityName = newProfileName;
    notifyPropertyChanged(BR.profileName);
  }

  @Bindable
  public String getDateLastUsed() {
    return dateLastUsed;
  }

  public void setDateLastUsed(final String lastUsed) {
    this.dateLastUsed = lastUsed;
    notifyPropertyChanged(BR.fileName);
  }

  public String getFacilityDataText() throws JSONException {
    final JSONObject json = new JSONObject();
    json.put("id", id);
    json.put("facilityName", facilityName);
    json.put("dateLastUsed", dateLastUsed);
    return json.toString();
  }

  @NonNull
  @Override
  public String toString() {
    return facilityName;
  }
}
