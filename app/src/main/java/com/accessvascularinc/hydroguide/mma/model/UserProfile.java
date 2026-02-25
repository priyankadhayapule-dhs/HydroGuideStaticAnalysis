package com.accessvascularinc.hydroguide.mma.model;

import androidx.databinding.BaseObservable;
import androidx.databinding.Bindable;

import com.accessvascularinc.hydroguide.mma.BR;

import org.json.JSONArray;
import org.json.JSONException;

public class UserProfile extends BaseObservable {
  private UserPreferences userPreferences = new UserPreferences();
  private String profileName = "";
  private String fileName = "";

  public UserProfile() {

  }

  public UserProfile(final String prefsDataString, final String profName, final String profFileName)
          throws JSONException {
    final JSONArray dataStrArray = new JSONArray(prefsDataString);
    userPreferences = new UserPreferences(dataStrArray.toString());
    profileName = profName;
    fileName = profFileName;
  }

  @Bindable
  public String getProfileName() {
    return profileName;
  }

  public void setProfileName(final String newProfileName) {
    this.profileName = newProfileName;
    notifyPropertyChanged(BR.profileName);
  }

  @Bindable
  public UserPreferences getUserPreferences() {
    return userPreferences;
  }

  public void setUserPreferences(final UserPreferences newUserPreferences) {
    this.userPreferences = newUserPreferences;
    notifyPropertyChanged(BR.userPreferences);
  }

  @Bindable
  public String getFileName() {
    return fileName;
  }

  public void setFileName(final String newFileName) {
    this.fileName = newFileName;
    notifyPropertyChanged(BR.fileName);
  }

  public String[] getProfileDataText() throws JSONException {
    // TODO: Improve this to a JSON object to facilitate parsing and reading data from file.
    return new String[] {
            new JSONArray(userPreferences.getUserPreferencesDataText()).toString(),
            profileName,
            fileName,
    };
  }
}
