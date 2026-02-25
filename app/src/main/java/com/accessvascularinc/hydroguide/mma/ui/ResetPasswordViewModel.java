package com.accessvascularinc.hydroguide.mma.ui;

import android.app.Application;

import androidx.annotation.NonNull;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import com.accessvascularinc.hydroguide.mma.network.ResetPasswordModel.ResetPasswordModel.ResetPasswordResponse;
import com.accessvascularinc.hydroguide.mma.repository.ResetPasswordRepository;

public class ResetPasswordViewModel extends AndroidViewModel {
  private final ResetPasswordRepository repository;
  private final MutableLiveData<ResetPasswordState> resetPasswordState = new MutableLiveData<>();

  public ResetPasswordViewModel(@NonNull Application application) {
    super(application);
    repository = new ResetPasswordRepository(application.getApplicationContext());
  }

  public LiveData<ResetPasswordState> getResetPasswordState() {
    return resetPasswordState;
  }

  public void sendResetPasswordCode(String email) {
    resetPasswordState.setValue(ResetPasswordState.loading());
    repository.sendResetPasswordCode(email, (response, errorMessage) -> {
      if (response != null && response.isSuccess()) {
        resetPasswordState.postValue(ResetPasswordState.success(response));
      } else {
        String backendError = (response != null && response.getErrorMessage() != null
            && !response.getErrorMessage().isEmpty())
                ? response.getErrorMessage()
                : null;
        String error = backendError != null ? backendError
            : (errorMessage != null ? errorMessage : "Reset password failed");
        resetPasswordState.postValue(ResetPasswordState.error(error));
      }
    });
  }

  public void resetPassword(String userId, String email, String newPassword, String currentPassword, String token,
                            String oldUserName, String newUserName) {
    resetPasswordState.setValue(ResetPasswordState.loading());
    if (newPassword.length() < 8) {
      resetPasswordState.postValue(ResetPasswordState.error("Password must be at least 8 characters"));
      return;
    }
    repository.updatePassword(userId, email, newPassword, currentPassword, token, oldUserName,
            newUserName, (response, errorMessage) -> {
      if (response != null && response.isSuccess()) {
        resetPasswordState.postValue(ResetPasswordState.success(response));
      } else {
        String backendError = (response != null && response.getErrorMessage() != null
            && !response.getErrorMessage().isEmpty())
                ? response.getErrorMessage()
                : null;
        String error = backendError != null ? backendError
            : (errorMessage != null ? errorMessage : "Password reset failed");
        resetPasswordState.postValue(ResetPasswordState.error(error));
      }
    });
  }

  /**
   * Resets the password reset state to prevent cached state from being replayed
   * when the fragment is recreated. This is called when ChangePasswordFragment
   * is created to ensure a fresh state for each password change operation.
   */
  public void resetState() {
    resetPasswordState.setValue(null);
  }

  public static class ResetPasswordState {
    public enum Status {
      LOADING, SUCCESS, ERROR
    }

    private final Status status;
    private final ResetPasswordResponse response;
    private final String errorMessage;

    private ResetPasswordState(Status status, ResetPasswordResponse response, String errorMessage) {
      this.status = status;
      this.response = response;
      this.errorMessage = errorMessage;
    }

    public Status getStatus() {
      return status;
    }

    public ResetPasswordResponse getResponse() {
      return response;
    }

    public String getErrorMessage() {
      return errorMessage;
    }

    public static ResetPasswordState loading() {
      return new ResetPasswordState(Status.LOADING, null, null);
    }

    public static ResetPasswordState success(ResetPasswordResponse response) {
      return new ResetPasswordState(Status.SUCCESS, response, null);
    }

    public static ResetPasswordState error(String errorMessage) {
      return new ResetPasswordState(Status.ERROR, null, errorMessage);
    }
  }
}
