package com.accessvascularinc.hydroguide.mma.ui;

import android.app.Application;

import androidx.annotation.NonNull;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import javax.inject.Inject;
import dagger.hilt.android.lifecycle.HiltViewModel;
import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;

import com.accessvascularinc.hydroguide.mma.db.repository.UsersRepository;
import java.util.ArrayList;
import java.util.List;

import android.app.Application;
import android.util.Log;

@HiltViewModel
public class LoginViewModel extends AndroidViewModel {
    private final MutableLiveData<LoginState> loginState = new MutableLiveData<>();
    private final IStorageManager storageManager;

    private final MutableLiveData<List<String>> activeUserEmails = new MutableLiveData<>();
    private final UsersRepository usersRepository;

    @Inject
    public LoginViewModel(@NonNull Application application, IStorageManager storageManager,
            UsersRepository usersRepository) {
        super(application);
        this.storageManager = storageManager;
        this.usersRepository = usersRepository;
        loadActiveUserEmails();
    }

    private void loadActiveUserEmails() {
        new Thread(() -> {
            List<String> emails = new ArrayList<>();
            if (usersRepository != null) {
                List<com.accessvascularinc.hydroguide.mma.db.entities.UsersEntity> activeUsers = usersRepository
                        .fetchAllActiveUsers();
                if (activeUsers != null) {
                    for (com.accessvascularinc.hydroguide.mma.db.entities.UsersEntity user : activeUsers) {
                        if (user.getEmail() != null) {
                            emails.add(user.getEmail());
                        }
                    }
                }
            }
            activeUserEmails.postValue(emails);
        }).start();
    }

    public LiveData<List<String>> getActiveUserEmails() {
        return activeUserEmails;
    }

    public LiveData<LoginState> getLoginState() {
        return loginState;
    }

    public void login(String email, String password) {
        MedDevLog.info("Login", "Attempting VM login");
        Log.d("LoginViewModel", "[LOGIN_START] Email: " + email);
        loginState.setValue(LoginState.loading());
        if (storageManager == null) {
            MedDevLog.error("LoginViewModel", "[ERROR] StorageManager not initialized");
            loginState.postValue(LoginState.error("StorageManager not initialized"));
            return;
        }
        storageManager.login(email, password, (LoginResponse response, String errorMessage) -> {
            if (response != null && response.isSuccess() && response.getData().getFirstTimeLogin()) {
                Log.d("LoginViewModel", "First time login success");
                // Save userId and email for future reference
                PrefsManager.setUserId(getApplication(), response.getData().getUserId());
                PrefsManager.setUserEmail(getApplication(), response.getData().getEmail());

                MedDevLog.audit("Login", "Logged in (first time) " + response.getData().getUserId());
                MedDevLog.setLoggedInUserId(response.getData().getUserId());

                loginState.postValue(LoginState.success(response));
            } else if (response != null && response.isSuccess()) {
                Log.d("LoginViewModel", "Login success");
                PrefsManager.saveLoginResponse(getApplication(), response);
                // Save userId and email for future reference
                PrefsManager.setUserId(getApplication(), response.getData().getUserId());
                PrefsManager.setUserEmail(getApplication(), response.getData().getEmail());

                MedDevLog.audit("Login", "Logged in " + response.getData().getUserId());
                MedDevLog.setLoggedInUserId(response.getData().getUserId());

                // Set token_saved_time for expiry logic
                android.content.SharedPreferences prefs = getApplication().getSharedPreferences("avi_prefs",
                        android.content.Context.MODE_PRIVATE);
                prefs.edit().putLong("token_saved_time", System.currentTimeMillis()).apply();
                Log.d("RefreshToken", "login: " + System.currentTimeMillis());
                loginState.postValue(LoginState.success(response));
            } else {
                String backendError = (response != null && response.getErrorMessage() != null
                        && !response.getErrorMessage().isEmpty())
                                ? response.getErrorMessage()
                                : null;

                // Check if OTP verification is required
                if ("OTP_VERIFICATION_REQUIRED".equals(backendError)) {
                    Log.d("LoginViewModel", "OTP verification required");
                    MedDevLog.info("Login", "OTP verification required for user");
                    loginState.postValue(LoginState.otpRequired());
                    return;
                }

                String error = backendError != null ? backendError
                        : (errorMessage != null ? errorMessage : "Login failed");
                MedDevLog.warn("Login", "Login failed: " + error);
                loginState.postValue(LoginState.error(error));
            }
        });
    }

    public static class LoginState {
        public enum Status {
            LOADING, SUCCESS, ERROR, OTP_REQUIRED
        }

        private final Status status;
        private final LoginResponse response;
        private final String errorMessage;

        private LoginState(Status status, LoginResponse response, String errorMessage) {
            this.status = status;
            this.response = response;
            this.errorMessage = errorMessage;
        }

        public Status getStatus() {
            return status;
        }

        public LoginResponse getResponse() {
            return response;
        }

        public String getErrorMessage() {
            return errorMessage;
        }

        public static LoginState loading() {
            return new LoginState(Status.LOADING, null, null);
        }

        public static LoginState success(LoginResponse response) {
            return new LoginState(Status.SUCCESS, response, null);
        }

        public static LoginState error(String errorMessage) {
            return new LoginState(Status.ERROR, null, errorMessage);
        }

        public static LoginState otpRequired() {
            return new LoginState(Status.OTP_REQUIRED, null, null);
        }
    }
}
