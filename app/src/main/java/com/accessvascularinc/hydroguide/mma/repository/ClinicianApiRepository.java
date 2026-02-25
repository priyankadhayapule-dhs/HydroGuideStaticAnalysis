package com.accessvascularinc.hydroguide.mma.repository;

import com.accessvascularinc.hydroguide.mma.network.ApiService;
import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.CreateClinicianRequest;
import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.CreateClinicianResponse;


import retrofit2.Call;
import retrofit2.Callback;

public class ClinicianApiRepository {
    private final ApiService apiService;

    public ClinicianApiRepository(ApiService apiService) {
        this.apiService = apiService;
    }

    public void createClinician(String organizationId, CreateClinicianRequest request,
            Callback<CreateClinicianResponse> callback) {
        Call<CreateClinicianResponse> call = apiService.createClinician(organizationId, request);
        call.enqueue(callback);
    }

    public void getAllClinicians(String organizationId,
            Callback<com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianListResponse> callback) {
        Call<com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianListResponse> call = apiService
                .getAllClinicians(organizationId);
        call.enqueue(callback);
    }

    public void getAllOrganizationAdmins(String organizationId,
            Callback<com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianListResponse> callback) {
        Call<com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianListResponse> call = apiService
                .getAllOrganizationAdmins(organizationId, "", true);
        call.enqueue(callback);
    }

    public void getAllOrganizationUsers(String organizationId,
                                         Callback<com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianListResponse> callback) {
        Call<com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianListResponse> call = apiService
                .getAllOrganizationUsers(organizationId, "");
        call.enqueue(callback);
    }

    public void getUsersById(String organizationId, String userId,
            Callback<com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianResponse> callback) {
        Call<com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianResponse> call = apiService
                .getUsersById(organizationId, userId);
        call.enqueue(callback);
    }
    public void getClinicianById(String organizationId, String clinicianId,
                                 Callback<com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianListResponse> callback) {
        Call<com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianListResponse> call = apiService
                .getCliniciansById(organizationId, clinicianId);
        call.enqueue(callback);
    }

    public void deleteClinician(String organizationId, String clinicianId,
            Callback<com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianListResponse> callback) {
        Call<com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianListResponse> call = apiService
                .deleteClinician(organizationId, clinicianId);
        call.enqueue(callback);
    }

    public void resetPasswordbyOrgAdmin(String organizationId, String userId,
            Callback<com.accessvascularinc.hydroguide.mma.network.ClinicianModel.CreateClinicianResponse> callback) {
        Call<com.accessvascularinc.hydroguide.mma.network.ClinicianModel.CreateClinicianResponse> call = apiService
                .resetPasswordbyOrgAdmin(organizationId, userId);
        call.enqueue(callback);
    }

}
