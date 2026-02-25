
package com.accessvascularinc.hydroguide.mma.network;

import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianListResponse;
import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianResponse;
import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginRequest;
import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse;

import retrofit2.Call;
import retrofit2.http.Body;
import retrofit2.http.POST;
import retrofit2.http.GET;
import retrofit2.http.PUT;
import retrofit2.http.Path;
import retrofit2.http.DELETE;

import com.accessvascularinc.hydroguide.mma.network.FacilitySyncModel.FacilitySyncRequest;
import com.accessvascularinc.hydroguide.mma.network.FacilitySyncModel.FacilitySyncResponse;
import com.accessvascularinc.hydroguide.mma.network.OfflineToOnlineSyncModel.OfflineToOnlineSyncModel;
import com.accessvascularinc.hydroguide.mma.network.ProcedureSyncModel.ProcedureSyncRequest;
import com.accessvascularinc.hydroguide.mma.network.ProcedureSyncModel.ProcedureSyncResponse;
import com.accessvascularinc.hydroguide.mma.network.ResetPasswordModel.UpdatePasswordRequest;

import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.CreateClinicianRequest;
import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.CreateClinicianResponse;

import retrofit2.http.Multipart;
import retrofit2.http.Part;
import okhttp3.MultipartBody;
import retrofit2.http.Url; // (unused after fix, can remove later)
import retrofit2.http.Headers;
import retrofit2.http.Query;
import okhttp3.ResponseBody;
import retrofit2.http.Streaming;

public interface ApiService {
        @Headers("Cache-Control: no-cache")
        @POST("api/user/{organizationId}/{userId}/reset-password")
        Call<com.accessvascularinc.hydroguide.mma.network.ClinicianModel.CreateClinicianResponse> resetPasswordbyOrgAdmin(
                        @Path("organizationId") String organizationId,
                        @Path("userId") String userId);

        @Headers("Cache-Control: no-cache")
        @POST("api/user/login")
        Call<LoginResponse> login(@Body LoginRequest request);

        @Headers("Cache-Control: no-cache")
        @POST("api/user/send-verification-code")
        Call<com.accessvascularinc.hydroguide.mma.network.ResetPasswordModel.ResetPasswordModel.ResetPasswordResponse> sendVerificationCode(
                        @Body String email);

        @Headers("Cache-Control: no-cache")
        @POST("api/user/verify-otp")
        Call<com.accessvascularinc.hydroguide.mma.network.VerifyOtpModel.VerifyOtpResponse> verifyOtp(
                        @Body com.accessvascularinc.hydroguide.mma.network.VerifyOtpModel.VerifyOtpRequest request);

        @Headers("Cache-Control: no-cache")
        @POST("api/user/{userId}/update-password")
        Call<com.accessvascularinc.hydroguide.mma.network.ResetPasswordModel.ResetPasswordModel.ResetPasswordResponse> updatePassword(
                        @retrofit2.http.Header("Authorization") String authHeader,
                        @Path("userId") String userId, @Body UpdatePasswordRequest request);

        @Headers("Cache-Control: no-cache")
        @PUT("api/{organizationId}/facility/sync")
        Call<FacilitySyncResponse> syncFacility(
                        @Path("organizationId") String organizationId,
                        @Body FacilitySyncRequest request);

        @Headers("Cache-Control: no-cache")
        @PUT("api/{organizationId}/procedure/sync")
        Call<ProcedureSyncResponse> syncProcedure(
                        @Path("organizationId") String organizationId,
                        @Body ProcedureSyncRequest request);

        @Headers("Cache-Control: no-cache")
        @PUT("api/user/{userId}/user-settings")
        Call<com.accessvascularinc.hydroguide.mma.network.UserSettingsModel.UserSettingsSyncResponse> syncUserSettings(
                        @Path("userId") String userId,
                        @Body com.accessvascularinc.hydroguide.mma.network.UserSettingsModel.UserSettingsSyncRequest request);
        @Headers("Cache-Control: no-cache")
        @GET("api/user/{userId}/user-settings")
        Call<com.accessvascularinc.hydroguide.mma.network.UserSettingsModel.UserSettingsSyncResponse> getUserSettings(
                @Path("userId") String userId);

        // Fetch procedures from cloud (read-only) - paginated with filters
        @Headers("Cache-Control: no-cache")
        @GET("api/{organizationId}/procedure")
        Call<ProcedureSyncResponse> getProcedures(
                        @Path("organizationId") String organizationId,
                        @Query("searchTerm") String searchTerm,
                        @Query("fromDate") String fromDate,
                        @Query("toDate") String toDate,
                        @Query("offset") int offset,
                        @Query("limit") int limit,
                        @Query("facilityId") int facilityId,
                        @Query("PatientSide") String patientSide,
                        @Query("dobFromDate") String dobFromDate,
                        @Query("dobToDate") String dobToDate,
                        @Query("userId") String userId);

        // Fetch procedures without pagination (for backward compatibility)
        @Headers("Cache-Control: no-cache")
        @GET("api/{organizationId}/procedure")
        Call<ProcedureSyncResponse> getProceduresSimple(
                        @Path("organizationId") String organizationId);

        // Get total count of procedures for pagination
        @Headers("Cache-Control: no-cache")
        @GET("api/{organizationId}/procedure/GetAllProcedureCount")
        Call<com.accessvascularinc.hydroguide.mma.network.ProcedureSyncModel.ProcedureCountResponse> getProcedureCount(
                        @Path("organizationId") String organizationId,
                        @Query("searchTerm") String searchTerm,
                        @Query("fromDate") String fromDate,
                        @Query("toDate") String toDate,
                        @Query("facilityId") int facilityId,
                        @Query("PatientSide") String patientSide,
                        @Query("dobFromDate") String dobFromDate,
                        @Query("dobToDate") String dobToDate,
                        @Query("userId") String userId);

        // Get total count as raw response (in case API returns plain integer)
        @Headers("Cache-Control: no-cache")
        @GET("api/{organizationId}/procedure/GetAllProcedureCount")
        Call<ResponseBody> getProcedureCountRaw(
                        @Path("organizationId") String organizationId,
                        @Query("searchTerm") String searchTerm,
                        @Query("fromDate") String fromDate,
                        @Query("toDate") String toDate,
                        @Query("facilityId") int facilityId,
                        @Query("PatientSide") String patientSide,
                        @Query("dobFromDate") String dobFromDate,
                        @Query("dobToDate") String dobToDate,
                        @Query("userId") String userId);

        // Multipart variant (form-data field parts + file parts) for controller
        // expecting [FromForm] binding.
        @Headers("Cache-Control: no-cache")
        @Multipart
        @PUT("api/{organizationId}/procedure/sync")
        Call<ProcedureSyncResponse> syncProcedureMultipart(
                        @Path("organizationId") String organizationId,
                        @Part java.util.List<MultipartBody.Part> parts);

        // Download a blob/storage file by passing the blob URL as a query parameter
        // Endpoint signature server-side: getBlobStorageFile(string url)
        // Example final request:
        // /api/procedure/getBlobStorageFile?url=https://.../blob.pdf
        @Headers("Cache-Control: no-cache")
        @Streaming
        @GET("api/{organizationId}/procedure/getBlobStorageFile")
        Call<ResponseBody> getBlobStorageFile(
                        @Path("organizationId") String organizationId,
                        @Query("url") String fileUrl);

        @Headers("Cache-Control: no-cache")
        @POST("api/user/create/{organizationId}")
        Call<CreateClinicianResponse> createClinician(
                        @Path("organizationId") String organizationId,
                        @Body CreateClinicianRequest request);

        // Fetch clinicians by ID (with query param)
        @Headers("Cache-Control: no-cache")
        @GET("api/user/{organizationId}/clinician/all")
        Call<ClinicianListResponse> getCliniciansById(@Path("organizationId") String organizationId,
                        @Query("clinicianId") String clinicianId);

        @Headers("Cache-Control: no-cache")
        @GET("api/user/{organizationId}/user/{userId}")
        Call<ClinicianResponse> getUsersById(@Path("organizationId") String organizationId,
                        @Path("userId") String userId);

        // Fetch all clinicians for an organization
        @Headers("Cache-Control: no-cache")
        @GET("api/user/{organizationId}/clinician/all")
        Call<ClinicianListResponse> getAllClinicians(@Path("organizationId") String organizationId);

        // Fetch all organization admins for an organization
        @Headers("Cache-Control: no-cache")
        @GET("api/user/{organizationId}/user/all")
        Call<ClinicianListResponse> getAllOrganizationAdmins(
                        @Path("organizationId") String organizationId,
                        @Query("searchTerm") String searchTerm,
                        @Query("isOnlyOrganizationAdmin") boolean isOnlyOrganizationAdmin);

        // Delete clinician by ID
        @Headers("Cache-Control: no-cache")
        @DELETE("api/user/{organizationId}/clinician/delete/{clinicianId}")
        Call<ClinicianListResponse> deleteClinician(@Path("organizationId") String organizationId,
                        @Path("clinicianId") String clinicianId);

        @Headers("Cache-Control: no-cache")
        @POST("api/user/refresh-token")
        Call<LoginResponse> refreshToken(@Body String refreshToken);

        @Headers("Cache-Control: no-cache")
        @GET("api/user/{organizationId}/organizationusers")
        Call<ClinicianListResponse> getAllOrganizationUsers(
                        @Path("organizationId") String organizationId,
                        @Query("searchTerm") String searchTerm);

        // Sync offline data to cloud during Offline → Online mode switch
        // Sends multipart/form-data with facilitiesJson, proceduresJson, and files
        // Backend handles: user existence check, user creation, mapping, and record
        // insertion
        @Multipart
        @POST("api/{organizationId}/procedure/offline-switch")
        Call<com.accessvascularinc.hydroguide.mma.network.OfflineSyncPayload.SyncResponseDTO> syncOfflineDataToCloud(
                        @Path("organizationId") String organizationId,
                        @retrofit2.http.Header("Authorization") String authHeader,
                        @Part("facilitiesJson") okhttp3.RequestBody facilitiesJson,
                        @Part("proceduresJson") okhttp3.RequestBody proceduresJson,
                        @Part java.util.List<MultipartBody.Part> files);

}
