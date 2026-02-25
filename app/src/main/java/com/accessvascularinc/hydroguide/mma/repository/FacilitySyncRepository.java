package com.accessvascularinc.hydroguide.mma.repository;

import androidx.annotation.NonNull;

import com.accessvascularinc.hydroguide.mma.network.ApiService;
import com.accessvascularinc.hydroguide.mma.network.FacilitySyncModel.FacilitySyncRequest;
import com.accessvascularinc.hydroguide.mma.network.FacilitySyncModel.FacilitySyncResponse;

import retrofit2.Call;

public class FacilitySyncRepository {
    private final ApiService apiService;

    public FacilitySyncRepository(@NonNull ApiService apiService) {
        this.apiService = apiService;
    }

    public Call<FacilitySyncResponse> syncFacility(String organizationId, FacilitySyncRequest request) {
        return apiService.syncFacility(organizationId, request);
    }
}
