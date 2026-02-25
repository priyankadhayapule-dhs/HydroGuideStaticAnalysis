package com.accessvascularinc.hydroguide.mma.repository;

import android.content.Context;
import androidx.annotation.Nullable;

import com.accessvascularinc.hydroguide.mma.network.ApiService;
import com.accessvascularinc.hydroguide.mma.network.RetrofitClient;
import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginRequest;
import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class LoginRepository {
  private final ApiService apiService;
  private final Context context;

  public interface LoginCallback {
    void onResult(@Nullable LoginResponse response, @Nullable String errorMessage);
  }

  public LoginRepository(Context context) {
    this.context = context.getApplicationContext();
    this.apiService = RetrofitClient.getClient(context).create(ApiService.class);
  }

  public void login(String email, String password, LoginCallback callback) {
    LoginRequest request = new LoginRequest(email, password);
    apiService.login(request).enqueue(new Callback<LoginResponse>() {
      @Override
      public void onResponse(Call<LoginResponse> call, Response<LoginResponse> response) {
        if (response.isSuccessful() && response.body() != null) {
          callback.onResult(response.body(), null);
        } else {
          String errorMsg = "Login failed. Please try again.";
          if (response.errorBody() != null) {
            try {
              String errorJson = response.errorBody().string();
              errorMsg = parseErrorMessage(errorJson);
            } catch (Exception e) {
              errorMsg = "Unable to parse error response.";
            }
          }
          callback.onResult(null, errorMsg);
        }
      }

      @Override
      public void onFailure(Call<LoginResponse> call, Throwable t) {
        // Silent failure, do not crash app
        callback.onResult(null, t.getMessage() != null ? t.getMessage() : "Unknown error");
      }
    });
  }

  public void refreshToken(String refreshToken, LoginCallback callback) {
    apiService.refreshToken(refreshToken).enqueue(new Callback<LoginResponse>() {
      @Override
      public void onResponse(Call<LoginResponse> call, Response<LoginResponse> response) {
        if (response.isSuccessful() && response.body() != null) {
          callback.onResult(response.body(), null);
        } else {
          String errorMsg = "Login failed. Please try again.";
          if (response.errorBody() != null) {
            try {
              String errorJson = response.errorBody().string();
              errorMsg = parseErrorMessage(errorJson);
            } catch (Exception e) {
              errorMsg = "Unable to parse error response.";
            }
          }
          callback.onResult(response.body(), errorMsg);
        }
      }

      @Override
      public void onFailure(Call<LoginResponse> call, Throwable t) {
        // Silent failure, do not crash app
        callback.onResult(null, t.getMessage() != null ? t.getMessage() : "Unknown error");
      }
    });
  }

  /**
   * Parses the error JSON and extracts a user-friendly error message.
   * This implementation expects a JSON object with a 'message' field.
   * If parsing fails or the field is missing, returns a generic message.
   */
  private String parseErrorMessage(String errorJson) {
    try {
      org.json.JSONObject jsonObject = new org.json.JSONObject(errorJson);
      if (jsonObject.has("message")) {
        return jsonObject.getString("message");
      }
    } catch (Exception e) {
      throw new RuntimeException("Malformed error response: unable to parse error JSON", e);
    }
    return "Login failed. Please try again.";
  }
}
