package com.accessvascularinc.hydroguide.mma.ui;

import android.app.Application;

import androidx.annotation.NonNull;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import com.accessvascularinc.hydroguide.mma.db.repository.UsersRepository;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;
import com.accessvascularinc.hydroguide.mma.utils.Utility;

import javax.inject.Inject;

import dagger.hilt.android.lifecycle.HiltViewModel;

@HiltViewModel
public class OfflineAdminSetupViewModel extends AndroidViewModel {

  public static class SetupState {
    public enum Status {
      IDLE, LOADING, SUCCESS, ERROR
    }

    private final Status status;
    private final String message;

    private SetupState(Status status, String message) {
      this.status = status;
      this.message = message;
    }

    public Status getStatus() {
      return status;
    }

    public String getMessage() {
      return message;
    }

    public static SetupState idle() {
      return new SetupState(Status.IDLE, null);
    }

    public static SetupState loading() {
      return new SetupState(Status.LOADING, null);
    }

    public static SetupState success(String message) {
      return new SetupState(Status.SUCCESS, message);
    }

    public static SetupState error(String message) {
      return new SetupState(Status.ERROR, message);
    }
  }

  private final UsersRepository usersRepository;
  private final MutableLiveData<SetupState> setupState = new MutableLiveData<>(SetupState.idle());

  @Inject
  public OfflineAdminSetupViewModel(@NonNull Application application,
      UsersRepository usersRepository) {
    super(application);
    this.usersRepository = usersRepository;
  }

  public LiveData<SetupState> getSetupState() {
    return setupState;
  }

  /**
   * Generates a random 8-character password with mixed character types. Reuses
   * the same logic style
   * as ManageCliniciansFragment.
   */
  public String generateRandomPassword() {
    return Utility.generateRandomPassword();
  }

  /**
   * Validates the username (name field) according to business rules.
   */
  public Utility.ValidationResult validateUsername(String name) {
    return Utility.validateUsername(name);
  }

  /**
   * Creates the first offline admin user using UsersRepository.
   */
  public void createOfflineAdmin(String name, String email, String password) {
    if (name == null || name.trim().isEmpty() ||
        email == null || email.trim().isEmpty() ||
        password == null || password.trim().isEmpty()) {
      setupState.setValue(SetupState.error("All fields are required"));
      return;
    }

    // Validate username format
    Utility.ValidationResult usernameValidation = validateUsername(name);
    if (!usernameValidation.isValid()) {
      setupState.setValue(SetupState.error(usernameValidation.getErrorMessage()));
      return;
    }

    setupState.setValue(SetupState.loading());

    // Determine organizationId from prefs; fall back to default if missing.
    String organizationId = PrefsManager.getOrganizationId(getApplication());
    if (organizationId == null || organizationId.trim().isEmpty()) {
      organizationId = Utility.DEFAULT_ORGANIZATION_ID;
    }

    usersRepository.createLocalTabletUser(name, email, password,
        IStorageManager.UserType.Organization_Admin.toString(),
        organizationId,
        (success, message) -> {
          if (success) {
            setupState.setValue(SetupState.success(message));
          } else {
            setupState.setValue(SetupState.error(message));
          }
        });
  }
}
