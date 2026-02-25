package com.accessvascularinc.hydroguide.mma.repository;

import com.accessvascularinc.hydroguide.mma.network.ApiService;
import com.accessvascularinc.hydroguide.mma.network.UserSettingsModel;

import retrofit2.Call;
import retrofit2.Callback;

import javax.inject.Inject;

public class UserSettingsSyncRepository {
    private final ApiService apiService;

    @Inject
    public UserSettingsSyncRepository(ApiService apiService) {
        this.apiService = apiService;
    }

    public void syncUserSettings(String userId, UserSettingsModel.UserSettingsSyncRequest request,
            Callback<UserSettingsModel.UserSettingsSyncResponse> callback) {
        Call<UserSettingsModel.UserSettingsSyncResponse> call = apiService.syncUserSettings(userId, request);
        call.enqueue(callback);
    }

    public void getUserSettings(String userId,
            Callback<UserSettingsModel.UserSettingsSyncResponse> callback) {
        Call<UserSettingsModel.UserSettingsSyncResponse> call = apiService.getUserSettings(userId);
        call.enqueue(callback);
    }
}
