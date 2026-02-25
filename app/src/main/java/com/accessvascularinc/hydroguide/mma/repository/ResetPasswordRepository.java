package com.accessvascularinc.hydroguide.mma.repository;

import android.content.Context;
import com.accessvascularinc.hydroguide.mma.network.ResetPasswordModel.ResetPasswordModel.ResetPasswordResponse;
import com.accessvascularinc.hydroguide.mma.network.ResetPasswordModel.UpdatePasswordRequest;
import com.accessvascularinc.hydroguide.mma.network.RetrofitClient;
import com.accessvascularinc.hydroguide.mma.network.ApiService;
import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class ResetPasswordRepository {
    private final Context context;

    public ResetPasswordRepository(Context context) {
        this.context = context;
    }

    public interface ResetPasswordCallback {
        void onComplete(ResetPasswordResponse response, String errorMessage);
    }

    public void sendResetPasswordCode(String email, ResetPasswordCallback callback) {
        ApiService apiService = RetrofitClient.getClient(context).create(ApiService.class);
        Call<ResetPasswordResponse> call = apiService.sendVerificationCode(email);
        call.enqueue(new Callback<ResetPasswordResponse>() {
            @Override
            public void onResponse(Call<ResetPasswordResponse> call, Response<ResetPasswordResponse> response) {
                if (response.isSuccessful() && response.body() != null) {
                    callback.onComplete(response.body(), null);
                } else {
                    callback.onComplete(null, "Reset password failed");
                }
            }

            @Override
            public void onFailure(Call<ResetPasswordResponse> call, Throwable t) {
                callback.onComplete(null, t.getMessage());
            }
        });
    }

    public void updatePassword(String userId, String email, String newPassword, String currentPassword,
            String token, String oldUserName, String newUserName, ResetPasswordCallback callback) {
        ApiService apiService = RetrofitClient.getClient(context).create(ApiService.class);
        UpdatePasswordRequest request = new UpdatePasswordRequest(email, newPassword, currentPassword, oldUserName, newUserName);
        Call<ResetPasswordResponse> call = apiService.updatePassword("Bearer " + token, userId, request);
        call.enqueue(new Callback<ResetPasswordResponse>() {
            @Override
            public void onResponse(Call<ResetPasswordResponse> call, Response<ResetPasswordResponse> response) {
                if (response.isSuccessful() && response.body() != null) {
                    callback.onComplete(response.body(), null);
                } else {
                    callback.onComplete(null, "Password reset failed");
                }
            }

            @Override
            public void onFailure(Call<ResetPasswordResponse> call, Throwable t) {
                callback.onComplete(null, t.getMessage());
            }
        });
    }
}