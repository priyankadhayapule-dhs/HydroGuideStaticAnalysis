package com.accessvascularinc.hydroguide.mma.ui;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Patterns;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.Navigation;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;
import com.accessvascularinc.hydroguide.mma.utils.Utility;

import dagger.hilt.android.AndroidEntryPoint;

@AndroidEntryPoint
public class OfflineAdminSetupFragment extends Fragment {

  private OfflineAdminSetupViewModel viewModel;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
      @Nullable Bundle savedInstanceState) {
    return inflater.inflate(R.layout.fragment_offline_admin_setup, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    viewModel = new ViewModelProvider(this).get(OfflineAdminSetupViewModel.class);

    // Set top bar title similar to ConnectUltrasoundFragment
    View topBarInclude = view.findViewById(R.id.includeTopbar);
    if (topBarInclude != null) {
      TextView topBarTitle = topBarInclude.findViewById(R.id.top_bar_title);
      if (topBarTitle != null) {
        topBarTitle.setText(getString(R.string.offline_admin_setup_title));
        topBarTitle.setVisibility(View.VISIBLE);
      }
    }

    // Hide unused top bar indicators for this screen
    View controllerBatteryLabel = view.findViewById(R.id.controller_battery_label);
    View controllerBatteryIcon = view.findViewById(R.id.remote_battery_level_indicator);
    View controllerBatteryPct = view.findViewById(R.id.controller_battery_level_pct);
    View usbIcon = view.findViewById(R.id.usb_icon);
    View syncIcon = view.findViewById(R.id.sync_status_icon);
    if (controllerBatteryLabel != null) {
      controllerBatteryLabel.setVisibility(View.GONE);
    }
    if (controllerBatteryIcon != null) {
      controllerBatteryIcon.setVisibility(View.GONE);
    }
    if (controllerBatteryPct != null) {
      controllerBatteryPct.setVisibility(View.GONE);
    }
    if (usbIcon != null) {
      usbIcon.setVisibility(View.GONE);
    }
    if (syncIcon != null) {
      syncIcon.setVisibility(View.GONE);
    }

    // Hide unused bottom bar actions for this screen
    View homeButton = view.findViewById(R.id.lnrHome);
    View volumeButton = view.findViewById(R.id.lnrVolume);
    View brightnessButton = view.findViewById(R.id.lnrBrightness);
    View resetPasswordButton = view.findViewById(R.id.lnrResetPassword);
    View removeButton = view.findViewById(R.id.lnrRemove);
    if (homeButton != null) {
      homeButton.setVisibility(View.GONE);
    }
    if (volumeButton != null) {
      volumeButton.setVisibility(View.GONE);
    }
    if (brightnessButton != null) {
      brightnessButton.setVisibility(View.GONE);
    }
    if (resetPasswordButton != null) {
      resetPasswordButton.setVisibility(View.GONE);
    }
    if (removeButton != null) {
      removeButton.setVisibility(View.GONE);
    }

    // Back button: navigate explicitly to ModeSetup screen
    View backButton = view.findViewById(R.id.lnrBack);
    if (backButton != null) {
      backButton.setOnClickListener(v -> Navigation.findNavController(v)
          .navigate(R.id.action_offline_admin_setup_to_mode_setup));
    }

    EditText nameEditText = view.findViewById(R.id.edit_offline_admin_name);
    EditText emailEditText = view.findViewById(R.id.edit_offline_admin_email);
    EditText passwordEditText = view.findViewById(R.id.edit_offline_admin_password);
    ImageButton refreshPasswordButton = view.findViewById(R.id.btnRefreshOfflineAdminPassword);
    ImageButton copyPasswordButton = view.findViewById(R.id.btnCopyOfflineAdminPassword);
    View saveButtonContainer = view.findViewById(R.id.lnrSave);
    ProgressBar progressBar = view.findViewById(R.id.progress_offline_admin_setup);

    final String[] generatedPassword = { "" };

    refreshPasswordButton.setOnClickListener(v -> {
      String password = viewModel.generateRandomPassword();
      generatedPassword[0] = password;
      passwordEditText.setText(password);
    });

    copyPasswordButton.setOnClickListener(v -> {
      if (TextUtils.isEmpty(generatedPassword[0])) {
        Toast.makeText(requireContext(), R.string.offline_admin_setup_generate_password_first,
            Toast.LENGTH_LONG).show();
        return;
      }

      ClipboardManager clipboard = (ClipboardManager) requireContext()
          .getSystemService(Context.CLIPBOARD_SERVICE);
      if (clipboard != null) {
        ClipData clip = ClipData.newPlainText("password", generatedPassword[0]);
        clipboard.setPrimaryClip(clip);
        Toast.makeText(requireContext(), R.string.password_copied, Toast.LENGTH_LONG).show();
      }
    });

    saveButtonContainer.setOnClickListener(v -> {
      String name = nameEditText.getText().toString().trim();
      String email = emailEditText.getText().toString().trim();
      String password = passwordEditText.getText().toString().trim();

      if (TextUtils.isEmpty(name) || TextUtils.isEmpty(email) || TextUtils.isEmpty(password)) {
        Toast.makeText(requireContext(), R.string.offline_admin_setup_error_required,
            Toast.LENGTH_LONG).show();
        return;
      }

      if (!android.util.Patterns.EMAIL_ADDRESS.matcher(email).matches()) {
        Toast.makeText(requireContext(), R.string.offline_admin_setup_error_email_invalid,
            Toast.LENGTH_LONG).show();
        return;
      }

      // Validate username format
      Utility.ValidationResult usernameValidation = viewModel.validateUsername(name);
      if (!usernameValidation.isValid()) {
        Toast.makeText(requireContext(), usernameValidation.getErrorMessage(),
            Toast.LENGTH_LONG).show();
        return;
      }

      viewModel.createOfflineAdmin(name, email, password);
    });

    viewModel.getSetupState().observe(getViewLifecycleOwner(), state -> {
      if (state == null) {
        return;
      }

      switch (state.getStatus()) {
        case LOADING:
          progressBar.setVisibility(View.VISIBLE);
          saveButtonContainer.setEnabled(false);
          break;
        case SUCCESS:
          progressBar.setVisibility(View.GONE);
          saveButtonContainer.setEnabled(true);
          if (getContext() != null) {
            // Mark tablet as configured for offline mode now that admin exists
            PrefsManager.setTabletConfigurationStateIsOffline(getContext(), true);
            Toast.makeText(getContext(), state.getMessage() != null ? state.getMessage()
                : getString(R.string.offline_admin_setup_success), Toast.LENGTH_LONG).show();
          }
          // Navigate to normal login screen so admin can sign in
          Navigation.findNavController(view).navigate(R.id.action_offline_admin_setup_to_login);
          break;
        case ERROR:
          progressBar.setVisibility(View.GONE);
          saveButtonContainer.setEnabled(true);
          if (getContext() != null) {
            Toast.makeText(getContext(),
                state.getMessage() != null ? state.getMessage()
                    : getString(R.string.offline_admin_setup_generic_error),
                Toast.LENGTH_LONG)
                .show();
          }
          break;
        case IDLE:
        default:
          progressBar.setVisibility(View.GONE);
          saveButtonContainer.setEnabled(true);
          break;
      }
    });
  }
}
