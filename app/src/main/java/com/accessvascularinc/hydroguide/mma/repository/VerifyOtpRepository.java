package com.accessvascularinc.hydroguide.mma.repository;

import android.content.Context;
import com.accessvascularinc.hydroguide.mma.network.VerifyOtpModel.VerifyOtpRequest;
import com.accessvascularinc.hydroguide.mma.network.VerifyOtpModel.VerifyOtpResponse;
import com.accessvascularinc.hydroguide.mma.network.RetrofitClient;
import com.accessvascularinc.hydroguide.mma.network.ApiService;
import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class VerifyOtpRepository {
    private final Context context;

    public VerifyOtpRepository(Context context) {
        this.context = context;
    }

    public interface VerifyOtpCallback {
        void onComplete(VerifyOtpResponse response, String errorMessage);
    }

    public void verifyOtp(String email, String verificationCode, VerifyOtpCallback callback) {
        ApiService apiService = RetrofitClient.getClient(context).create(ApiService.class);
        VerifyOtpRequest request = new VerifyOtpRequest(email, verificationCode);
        Call<VerifyOtpResponse> call = apiService.verifyOtp(request);
        call.enqueue(new Callback<VerifyOtpResponse>() {
            @Override
            public void onResponse(Call<VerifyOtpResponse> call, Response<VerifyOtpResponse> response) {
                if (response.isSuccessful() && response.body() != null) {
                    callback.onComplete(response.body(), null);
                } else {
                    callback.onComplete(null, "OTP verification failed");
                }
            }

            @Override
            public void onFailure(Call<VerifyOtpResponse> call, Throwable t) {
                callback.onComplete(null, t.getMessage());
            }
        });
    }
}
