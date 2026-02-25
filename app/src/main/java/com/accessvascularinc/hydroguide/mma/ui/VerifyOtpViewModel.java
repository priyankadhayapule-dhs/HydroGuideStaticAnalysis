package com.accessvascularinc.hydroguide.mma.ui;

import android.app.Application;

import androidx.annotation.NonNull;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import com.accessvascularinc.hydroguide.mma.repository.VerifyOtpRepository;
import com.accessvascularinc.hydroguide.mma.network.VerifyOtpModel.VerifyOtpResponse;

public class VerifyOtpViewModel extends AndroidViewModel {
    private final MutableLiveData<VerificationState> verificationState = new MutableLiveData<>();
    private final VerifyOtpRepository repository;
    private VerifyOtpResponse lastVerifyOtpResponse;
    public VerifyOtpViewModel(@NonNull Application application) {
        super(application);
        repository = new VerifyOtpRepository(application.getApplicationContext());
    }

    public LiveData<VerificationState> getVerificationState() {
        return verificationState;
    }

    public void verifyOtp(String email, String otp) {
        verificationState.setValue(VerificationState.loading());
        if (email == null || email.isEmpty()) {
            verificationState.postValue(VerificationState.error("Email not set"));
            return;
        }
        repository.verifyOtp(email, otp, (response, errorMessage) -> {
            if (response != null && response.isSuccess()) {
                lastVerifyOtpResponse = response;
                verificationState.postValue(VerificationState.success());
            } else {
                String backendError = (response != null && response.getErrorMessage() != null
                        && !response.getErrorMessage().isEmpty())
                                ? response.getErrorMessage()
                                : null;
                String error = backendError != null ? backendError
                        : (errorMessage != null ? errorMessage : "OTP verification failed");
                verificationState.postValue(VerificationState.error(error));
            }
        });
    }

    public VerifyOtpResponse getLastVerifyOtpResponse() {
        return lastVerifyOtpResponse;
    }

    public static class VerificationState {
        public enum Status {
            LOADING, SUCCESS, ERROR
        }

        private final Status status;
        private final String errorMessage;

        private VerificationState(Status status, String errorMessage) {
            this.status = status;
            this.errorMessage = errorMessage;
        }

        public Status getStatus() {
            return status;
        }

        public String getErrorMessage() {
            return errorMessage;
        }

        public static VerificationState loading() {
            return new VerificationState(Status.LOADING, null);
        }

        public static VerificationState success() {
            return new VerificationState(Status.SUCCESS, null);
        }

        public static VerificationState error(String errorMessage) {
            return new VerificationState(Status.ERROR, errorMessage);
        }
    }
}
