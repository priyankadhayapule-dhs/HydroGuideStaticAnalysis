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
import com.accessvascularinc.hydroguide.mma.service.OfflineToOnlineSyncService;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.NetworkConnectivityLiveData;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;

import dagger.hilt.android.AndroidEntryPoint;

import javax.inject.Inject;

/**
 * Dialog for switching tablet from Offline to Online mode.
 *
 * Workflow:
 * 1. Validates network connectivity
 * 2. Prompts admin to enter cloud credentials (email + password)
 * 3. Shows terms and conditions
 * 4. Executes sync workflow via OfflineToOnlineSyncService
 * 5. Shows progress and final result
 */
@AndroidEntryPoint
public class OfflineToOnlineModeDialog extends DialogFragment {
    private static final String TAG = "OfflineToOnlineModeDialog";

    @Inject
    OfflineToOnlineSyncService syncService;

    @Inject
    UsersRepository usersRepository;

    private EditText cloudEmailInput;
    private EditText cloudPasswordInput;
    private CheckBox termsCheckBox;
    private TextView statusMessage;
    private ProgressBar progressBar;
    private NetworkConnectivityLiveData networkConnectivityLiveData;
    private boolean isNetworkConnected = false;
    private MainViewModel viewModel;
    private boolean isPasswordVisible = false;

    public static OfflineToOnlineModeDialog newInstance() {
        return new OfflineToOnlineModeDialog();
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
        View view = inflater.inflate(R.layout.dialog_offline_to_online_mode_switch, null);

        initializeViews(view);
        setupNetworkListener();

        builder.setView(view)
                .setTitle(R.string.offline_to_online_dialog_title)
                .setPositiveButton(R.string.offline_to_online_switch_button, null)
                .setNegativeButton(R.string.offline_to_online_cancel_button, (dialog, which) -> {
                    dismiss();
                });

        AlertDialog dialog = builder.create();

        dialog.setOnShowListener(dialogInterface -> {
            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v -> {
                if (validateInputs()) {
                    executeOfflineToOnlineSync(dialog);
                }
            });
        });

        setCancelable(true);

        return dialog;
    }

    private void initializeViews(View view) {
        cloudEmailInput = view.findViewById(R.id.cloud_email_input);
        cloudPasswordInput = view.findViewById(R.id.cloud_password_input);
        statusMessage = view.findViewById(R.id.status_message);
        progressBar = view.findViewById(R.id.progress_bar);
        termsCheckBox = view.findViewById(R.id.terms_checkbox);

        // Set password input type
        if (cloudPasswordInput != null) {
            cloudPasswordInput.setTransformationMethod(PasswordTransformationMethod.getInstance());
            setupPasswordVisibilityToggle(cloudPasswordInput);
        }

        progressBar.setVisibility(View.GONE);
    }

    private void setupPasswordVisibilityToggle(EditText passwordField) {
        passwordField.setOnTouchListener((v, event) -> {
            final int DRAWABLE_RIGHT = 2;
            if (event.getAction() == android.view.MotionEvent.ACTION_UP) {
                if (event.getRawX() >= passwordField.getRight()
                        - passwordField.getCompoundDrawables()[DRAWABLE_RIGHT].getBounds().width()) {
                    togglePasswordVisibility(passwordField);
                    return true;
                }
            }
            return false;
        });
    }

    private void togglePasswordVisibility(EditText passwordField) {
        isPasswordVisible = !isPasswordVisible;
        if (isPasswordVisible) {
            passwordField.setTransformationMethod(HideReturnsTransformationMethod.getInstance());
            passwordField.setCompoundDrawablesWithIntrinsicBounds(0, 0, R.drawable.ic_eye_open, 0);
        } else {
            passwordField.setTransformationMethod(PasswordTransformationMethod.getInstance());
            passwordField.setCompoundDrawablesWithIntrinsicBounds(0, 0, R.drawable.ic_eye_closed, 0);
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
            statusMessage.setText(R.string.offline_to_online_network_connected);
            statusMessage.setTextColor(getResources().getColor(android.R.color.holo_green_dark));
        } else {
            statusMessage.setText(R.string.offline_to_online_network_disconnected);
            statusMessage.setTextColor(getResources().getColor(android.R.color.holo_red_dark));
        }
    }

    private boolean validateInputs() {
        if (!isNetworkConnected) {
            Toast.makeText(requireContext(),
                    R.string.offline_to_online_error_no_internet,
                    Toast.LENGTH_SHORT).show();
            return false;
        }

        if (termsCheckBox != null && !termsCheckBox.isChecked()) {
            Toast.makeText(requireContext(),
                    R.string.offline_to_online_error_terms_not_accepted,
                    Toast.LENGTH_SHORT).show();
            return false;
        }

        String email = cloudEmailInput.getText().toString().trim();
        String password = cloudPasswordInput.getText().toString().trim();

        if (TextUtils.isEmpty(email)) {
            cloudEmailInput.setError(getString(R.string.offline_to_online_error_email_empty));
            return false;
        }

        if (!android.util.Patterns.EMAIL_ADDRESS.matcher(email).matches()) {
            cloudEmailInput.setError(getString(R.string.offline_to_online_error_email_invalid));
            return false;
        }

        if (TextUtils.isEmpty(password)) {
            cloudPasswordInput.setError(getString(R.string.offline_to_online_error_password_empty));
            return false;
        }

        if (password.length() < 6) {
            cloudPasswordInput.setError(getString(R.string.offline_to_online_error_password_short));
            return false;
        }

        return true;
    }

    private void executeOfflineToOnlineSync(AlertDialog dialog) {
        String cloudEmail = cloudEmailInput.getText().toString().trim();
        String cloudPassword = cloudPasswordInput.getText().toString().trim();

        viewModel = new androidx.lifecycle.ViewModelProvider(requireActivity())
                .get(MainViewModel.class);

        getCurrentUserAsync((currentUser) -> {
            if (currentUser == null) {
                MedDevLog.error("OfflineToOnlineModeDialog", "Current user not found");
                showError(getString(R.string.offline_to_online_error_user_not_found));
                return;
            }

            disableInputs();
            progressBar.setVisibility(View.VISIBLE);
            statusMessage.setText(R.string.offline_to_online_progress_initializing);

            syncService.switchToOnlineMode(
                    currentUser,
                    cloudEmail,
                    cloudPassword,
                    new OfflineToOnlineSyncService.OfflineToOnlineSyncCallback() {
                        @Override
                        public void onSuccess(String message) {
                            MedDevLog.audit("OfflineToOnlineModeDialog", "Offline→Online sync completed successfully");
                            showSuccess(message, dialog);

                            dialog.getWindow().getDecorView().postDelayed(() -> {
                                try {
                                    PrefsManager.clearLoginResponse(requireContext());
                                    dismiss();

                                    androidx.navigation.NavController navController = androidx.navigation.Navigation
                                            .findNavController(requireActivity(),
                                                    R.id.nav_host_fragment_activity_main);

                                    androidx.navigation.NavOptions navOptions = new androidx.navigation.NavOptions.Builder()
                                            .setPopUpTo(R.id.nav_host_fragment_activity_main, false)
                                            .build();

                                    navController.navigate(R.id.navigation_login, null, navOptions);
                                } catch (Exception e) {
                                    MedDevLog.error("OfflineToOnlineModeDialog",
                                            "Error during navigation: " + e.getMessage(), e);
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

    private void getCurrentUserAsync(UserCallback callback) {
        try {
            String currentUserEmail = PrefsManager.getUserEmail(requireContext());

            if (TextUtils.isEmpty(currentUserEmail)) {
                MedDevLog.warn("OfflineToOnlineModeDialog", "User email not found in preferences");
                callback.onUserRetrieved(null);
                return;
            }

            usersRepository.fetchUserByEmailAsync(currentUserEmail, (user, error) -> {
                if (user == null) {
                    MedDevLog.warn("OfflineToOnlineModeDialog",
                            getString(R.string.offline_to_online_error_user_fetch) + ": " +
                                    (error != null ? error : "Unknown"));
                }
                callback.onUserRetrieved(user);
            });
        } catch (Exception e) {
            MedDevLog.error("OfflineToOnlineModeDialog", "Exception fetching current user: " + e.getMessage(), e);
            callback.onUserRetrieved(null);
        }
    }

    private interface UserCallback {
        void onUserRetrieved(UsersEntity user);
    }

    private void disableInputs() {
        if (cloudEmailInput != null) {
            cloudEmailInput.setEnabled(false);
        }
        if (cloudPasswordInput != null) {
            cloudPasswordInput.setEnabled(false);
        }
    }

    private void enableInputs() {
        if (cloudEmailInput != null) {
            cloudEmailInput.setEnabled(true);
        }
        if (cloudPasswordInput != null) {
            cloudPasswordInput.setEnabled(true);
        }
    }

    private void showSuccess(String message, AlertDialog dialog) {
        progressBar.setVisibility(View.GONE);
        statusMessage.setText(getString(R.string.offline_to_online_success_prefix) + message);
        statusMessage.setTextColor(getResources().getColor(android.R.color.holo_green_dark));

        statusMessage.postDelayed(() -> {
            dismiss();
            dialog.dismiss();
        }, 2000);
    }

    private void showError(String errorMessage) {
        statusMessage.setText(getString(R.string.offline_to_online_error_prefix) + errorMessage);
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
