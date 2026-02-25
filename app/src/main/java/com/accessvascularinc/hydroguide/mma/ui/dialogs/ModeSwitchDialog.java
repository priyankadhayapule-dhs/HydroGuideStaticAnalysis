package com.accessvascularinc.hydroguide.mma.ui.dialogs;

import android.app.AlertDialog;
import android.app.Dialog;
import android.os.Bundle;
import android.text.TextUtils;
import android.text.method.HideReturnsTransformationMethod;
import android.text.method.PasswordTransformationMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import androidx.lifecycle.Observer;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.db.entities.UsersEntity;
import com.accessvascularinc.hydroguide.mma.db.repository.UsersRepository;
import com.accessvascularinc.hydroguide.mma.service.ModeSwitchService;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.NetworkConnectivityLiveData;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;

import dagger.hilt.android.AndroidEntryPoint;

import javax.inject.Inject;

/**
 * Dialog for switching tablet from Online to Offline mode.
 * <p>
 * Workflow: 1. Validates network connectivity 2. Prompts admin to set temporary
 * clinician password
 * 3. Executes mode switch workflow via ModeSwitchService 4. Shows progress and
 * final result
 */
@AndroidEntryPoint
public class ModeSwitchDialog extends DialogFragment {
  private static final String TAG = "ModeSwitchDialog";

  @Inject
  ModeSwitchService modeSwitchService;

  @Inject
  UsersRepository usersRepository;

  private EditText passwordInput;
  private EditText confirmPasswordInput;
  private CheckBox termsCheckBox;
  private TextView statusMessage;
  private ProgressBar progressBar;
  private NetworkConnectivityLiveData networkConnectivityLiveData;
  private boolean isNetworkConnected = false;
  private MainViewModel viewModel;
  private boolean isPasswordVisible = false;
  private boolean isConfirmPasswordVisible = false;

  public static ModeSwitchDialog newInstance() {
    return new ModeSwitchDialog();
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setStyle(DialogFragment.STYLE_NORMAL, android.R.style.Theme_Material_Dialog);
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
    AlertDialog.Builder builder = new AlertDialog.Builder(requireActivity());
    LayoutInflater inflater = LayoutInflater.from(getActivity());
    View view = inflater.inflate(R.layout.dialog_mode_switch, null);

    initializeViews(view);
    setupNetworkListener();

    builder.setView(view)
        .setTitle("Switch to Offline Mode")
        .setPositiveButton("Switch", null) // Will be overridden below
        .setNegativeButton("Cancel", (dialog, which) -> {
          dismiss();
        });

    AlertDialog dialog = builder.create();

    // Override positive button to validate inputs before dismissing
    dialog.setOnShowListener(dialogInterface -> {
      dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v -> {
        if (validateInputs()) {
          executeModeSwitch(dialog);
        }
      });
    });

    // Make dialog non-cancellable during operation
    setCancelable(true);

    return dialog;
  }

  private void initializeViews(View view) {
    passwordInput = view.findViewById(R.id.clinician_password_input);
    confirmPasswordInput = view.findViewById(R.id.confirm_password_input);
    statusMessage = view.findViewById(R.id.status_message);
    progressBar = view.findViewById(R.id.progress_bar);
    termsCheckBox = view.findViewById(R.id.terms_checkbox);

    // Set password input type
    if (passwordInput != null) {
      passwordInput.setTransformationMethod(PasswordTransformationMethod.getInstance());
      // Set up password visibility toggle
      setupPasswordVisibilityToggle(passwordInput, false);
    }
    if (confirmPasswordInput != null) {
      confirmPasswordInput.setTransformationMethod(PasswordTransformationMethod.getInstance());
      // Set up confirm password visibility toggle
      setupPasswordVisibilityToggle(confirmPasswordInput, true);
    }

    progressBar.setVisibility(View.GONE);
  }

  private void setupPasswordVisibilityToggle(EditText passwordField, boolean isConfirmField) {
    passwordField.setOnTouchListener((v, event) -> {
      final int DRAWABLE_RIGHT = 2;
      if (event.getAction() == android.view.MotionEvent.ACTION_UP) {
        if (event.getRawX() >= passwordField.getRight()
            - passwordField.getCompoundDrawables()[DRAWABLE_RIGHT].getBounds().width()) {
          togglePasswordVisibility(passwordField, isConfirmField);
          return true;
        }
      }
      return false;
    });
  }

  private void togglePasswordVisibility(EditText passwordField, boolean isConfirmField) {
    if (isConfirmField) {
      isConfirmPasswordVisible = !isConfirmPasswordVisible;
      if (isConfirmPasswordVisible) {
        passwordField.setTransformationMethod(HideReturnsTransformationMethod.getInstance());
        passwordField.setCompoundDrawablesWithIntrinsicBounds(0, 0, R.drawable.ic_eye_open, 0);
      } else {
        passwordField.setTransformationMethod(PasswordTransformationMethod.getInstance());
        passwordField.setCompoundDrawablesWithIntrinsicBounds(0, 0, R.drawable.ic_eye_closed, 0);
      }
    } else {
      isPasswordVisible = !isPasswordVisible;
      if (isPasswordVisible) {
        passwordField.setTransformationMethod(HideReturnsTransformationMethod.getInstance());
        passwordField.setCompoundDrawablesWithIntrinsicBounds(0, 0, R.drawable.ic_eye_open, 0);
      } else {
        passwordField.setTransformationMethod(PasswordTransformationMethod.getInstance());
        passwordField.setCompoundDrawablesWithIntrinsicBounds(0, 0, R.drawable.ic_eye_closed, 0);
      }
    }
    // Keep cursor at end after transformation
    passwordField.setSelection(passwordField.getText().length());
  }

  private void setupNetworkListener() {
    networkConnectivityLiveData = new NetworkConnectivityLiveData(requireContext());
    Observer<Boolean> networkObserver = isConnected -> {
      isNetworkConnected = isConnected != null && isConnected;
      updateNetworkStatus();
    };
    networkConnectivityLiveData.observe(this, networkObserver);
  }

  private void updateNetworkStatus() {
    if (isNetworkConnected) {
      statusMessage.setText("✓ Internet connection available");
      statusMessage.setTextColor(getResources().getColor(android.R.color.holo_green_dark));
    } else {
      statusMessage.setText("✗ No internet connection. Switch requires internet access.");
      statusMessage.setTextColor(getResources().getColor(android.R.color.holo_red_dark));
    }
  }

  private boolean validateInputs() {
    // Check network connectivity first
    if (!isNetworkConnected) {
      Toast.makeText(requireContext(),
          "Internet connection required for mode switch",
          Toast.LENGTH_SHORT).show();
      return false;
    }

    // Check terms and conditions acceptance
    if (termsCheckBox != null && !termsCheckBox.isChecked()) {
      Toast.makeText(requireContext(),
          "Please accept the terms and conditions to proceed",
          Toast.LENGTH_SHORT).show();
      return false;
    }

    String password = passwordInput.getText().toString().trim();
    String confirmPassword = confirmPasswordInput.getText().toString().trim();

    // Validate password is not empty
    if (TextUtils.isEmpty(password)) {
      passwordInput.setError("Password cannot be empty");
      return false;
    }

    // Validate password length (minimum 6 characters)
    if (password.length() < 6) {
      passwordInput.setError("Password must be at least 6 characters");
      return false;
    }

    // Validate confirmation password matches
    if (!password.equals(confirmPassword)) {
      confirmPasswordInput.setError("Passwords do not match");
      return false;
    }

    return true;
  }

  private void executeModeSwitch(AlertDialog dialog) {
    String password = passwordInput.getText().toString().trim();
    String organizationId = PrefsManager.getOrganizationId(requireContext());

    if (TextUtils.isEmpty(organizationId)) {
      MedDevLog.error(TAG, "Organization ID not found");
      showError("Organization ID not found. Please login again.");
      return;
    }

    // Get current user from ViewModel
    viewModel = new androidx.lifecycle.ViewModelProvider(requireActivity())
        .get(MainViewModel.class);

    // Retrieve current user asynchronously to avoid main thread database access
    getCurrentUserAsync((currentUser) -> {
      if (currentUser == null) {
        MedDevLog.error(TAG, "Current user not found");
        showError("Current user not found. Please login again.");
        return;
      }

      // Disable inputs and show progress
      disableInputs();
      progressBar.setVisibility(View.VISIBLE);
      statusMessage.setText("Initializing mode switch...");

      // Execute mode switch with callback
      modeSwitchService.switchToOfflineMode(
          currentUser,
          password,
          organizationId,
          new ModeSwitchService.ModeSwitchCallback() {
            @Override
            public void onSuccess(String message) {
              android.util.Log.d(TAG, "Mode switch completed successfully");
              showSuccess(message, dialog);

              // Clear login response and navigate to login after 2 seconds
              dialog.getWindow().getDecorView().postDelayed(() -> {
                try {
                  // Clear the login response to effectively logout
                  PrefsManager.clearLoginResponse(requireContext());

                  // Dismiss dialog first
                  dismiss();

                  // Navigate to login using NavController with pop to root
                  androidx.navigation.NavController navController = androidx.navigation.Navigation
                      .findNavController(requireActivity(),
                          R.id.nav_host_fragment_activity_main);

                  // Create NavOptions to pop entire back stack and navigate to login
                  androidx.navigation.NavOptions navOptions = new androidx.navigation.NavOptions.Builder()
                      .setPopUpTo(R.id.nav_host_fragment_activity_main, false)
                      .build();

                  navController.navigate(R.id.navigation_login, null, navOptions);
                } catch (Exception e) {
                  MedDevLog.error(TAG, "Error during logout: " + e.getMessage(), e);
                }
              }, 2000);
            }

            @Override
            public void onFailure(String errorMessage) {
              showError(errorMessage);
              progressBar.setVisibility(View.GONE);
              enableInputs();
            }

            @Override
            public void onProgress(String progressMessage) {
              statusMessage.setText(progressMessage);
            }
          });
    });
  }

  @Nullable
  private UsersEntity getCurrentUser() {
    try {
      android.util.Log.d(TAG, "[MODE_SWITCH] getCurrentUser() called");

      // Get the current user email from PrefsManager
      String currentUserEmail = PrefsManager.getUserEmail(requireContext());
      android.util.Log.d(TAG,
          "[MODE_SWITCH] Retrieved email from PrefsManager: " + currentUserEmail);

      if (TextUtils.isEmpty(currentUserEmail)) {
        MedDevLog.error(TAG,
            "[MODE_SWITCH_ERROR] User email not found in preferences. User may not have logged in properly.");
        return null;
      }

      UsersEntity user = usersRepository.getUserByEmail(currentUserEmail);

      if (user == null) {
        MedDevLog.error(TAG, "User not found in database");
      }
      return user;
    } catch (Exception e) {
      MedDevLog.error(TAG, "Exception fetching current user: " + e.getMessage(), e);
      return null;
    }
  }

  private void getCurrentUserAsync(UserCallback callback) {
    try {
      String currentUserEmail = PrefsManager.getUserEmail(requireContext());

      if (TextUtils.isEmpty(currentUserEmail)) {
        MedDevLog.error(TAG, "User email not found in preferences");
        callback.onUserRetrieved(null);
        return;
      }

      // Fetch the user from database asynchronously
      usersRepository.fetchUserByEmailAsync(currentUserEmail, (user, error) -> {
        if (user == null) {
          MedDevLog.error(TAG, "User not found: " + (error != null ? error : "Unknown error"));
        }
        callback.onUserRetrieved(user);
      });
    } catch (Exception e) {
      MedDevLog.error(TAG, "Exception fetching current user: " + e.getMessage(), e);
      callback.onUserRetrieved(null);
    }
  }

  private interface UserCallback {
    void onUserRetrieved(UsersEntity user);
  }

  private void disableInputs() {
    if (passwordInput != null) {
      passwordInput.setEnabled(false);
    }
    if (confirmPasswordInput != null) {
      confirmPasswordInput.setEnabled(false);
    }
  }

  private void enableInputs() {
    if (passwordInput != null) {
      passwordInput.setEnabled(true);
    }
    if (confirmPasswordInput != null) {
      confirmPasswordInput.setEnabled(true);
    }
  }

  private void showSuccess(String message, AlertDialog dialog) {
    progressBar.setVisibility(View.GONE);
    statusMessage.setText("✓ " + message);
    statusMessage.setTextColor(getResources().getColor(android.R.color.holo_green_dark));

    // Auto-dismiss after 2 seconds
    statusMessage.postDelayed(() -> {
      dismiss();
      dialog.dismiss();
    }, 2000);
  }

  private void showError(String errorMessage) {
    statusMessage.setText("✗ " + errorMessage);
    statusMessage.setTextColor(getResources().getColor(android.R.color.holo_red_dark));
    Toast.makeText(requireContext(), errorMessage, Toast.LENGTH_LONG).show();
  }

  @Override
  public void onDestroyView() {
    super.onDestroyView();
    // Clean up network listener
    if (networkConnectivityLiveData != null) {
      networkConnectivityLiveData.removeObservers(this);
    }
  }
}
