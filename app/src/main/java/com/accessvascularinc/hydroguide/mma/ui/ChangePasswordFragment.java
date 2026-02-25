package com.accessvascularinc.hydroguide.mma.ui;

import android.os.Bundle;
import android.text.InputType;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.Navigation;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentResetPasswordBinding;
import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse;
import com.accessvascularinc.hydroguide.mma.security.EncryptionManager;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.Utility;

import javax.inject.Inject;

import dagger.hilt.android.AndroidEntryPoint;

@AndroidEntryPoint
public class ChangePasswordFragment extends Fragment {
  private static final String TAG = "ChangePasswordFragment";
  private static final String CHANGE_TYPE_FIRST_TIME = "FIRST_TIME_RESET";
  private static final String CHANGE_TYPE_REGULAR = "REGULAR_CHANGE";

  private FragmentResetPasswordBinding binding;
  private ResetPasswordViewModel resetPasswordViewModel;
  @Inject
  IStorageManager storageManager;

  private String changeType = CHANGE_TYPE_REGULAR;
  private String usernameRead = "";


  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
      @Nullable Bundle savedInstanceState) {
    binding = FragmentResetPasswordBinding.inflate(inflater, container, false);

    usernameRead = "";
    if (getArguments() != null) {
      changeType = getArguments().getString("changeType", CHANGE_TYPE_REGULAR);
      usernameRead = getArguments().getString("username");
    }

    resetPasswordViewModel = new ViewModelProvider(requireActivity()).get(ResetPasswordViewModel.class);
    resetPasswordViewModel.resetState();

    if (changeType != null && CHANGE_TYPE_FIRST_TIME.equals(changeType)) {
      binding.usernameInput.setVisibility(View.VISIBLE);
      binding.usernameLabel.setVisibility(View.VISIBLE);
    } else {
      binding.usernameInput.setVisibility(View.GONE);
      binding.usernameLabel.setVisibility(View.GONE);
    }
    
    // Show PIN setup section for both first-time and regular password changes, but not in offline mode
    if (storageManager.getStorageMode() != IStorageManager.StorageMode.OFFLINE) {
      binding.pinSetupSection.setVisibility(View.VISIBLE);
    } else {
      binding.pinSetupSection.setVisibility(View.GONE);
    }
    
    binding.usernameInput.setText(usernameRead);
    binding.resetPasswordTitle.setText("Change Password");

    // PIN checkbox listener
    binding.enablePinCheckbox.setOnCheckedChangeListener((buttonView, isChecked) -> {
      binding.pinFieldsLayout.setVisibility(isChecked ? View.VISIBLE : View.GONE);
    });

    final boolean[] passwordVisible = { false };
    binding.passwordEye.setOnClickListener(v -> {
      passwordVisible[0] = !passwordVisible[0];
      if (passwordVisible[0]) {
        binding.passwordInput
            .setInputType(
                InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
        binding.passwordEye.setImageResource(R.drawable.ic_eye_closed);
      } else {
        binding.passwordInput.setInputType(
            InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
        binding.passwordEye.setImageResource(R.drawable.ic_eye_open);
      }
      binding.passwordInput.setSelection(binding.passwordInput.getText().length());
    });

    final boolean[] confirmPasswordVisible = { false };
    binding.confirmPasswordEye.setOnClickListener(v -> {
      confirmPasswordVisible[0] = !confirmPasswordVisible[0];
      if (confirmPasswordVisible[0]) {
        binding.confirmPasswordInput
            .setInputType(
                InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
        binding.confirmPasswordEye.setImageResource(R.drawable.ic_eye_closed);
      } else {
        binding.confirmPasswordInput
            .setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
        binding.confirmPasswordEye.setImageResource(R.drawable.ic_eye_open);
      }
      binding.confirmPasswordInput.setSelection(binding.confirmPasswordInput.getText().length());
    });

    final boolean[] pinVisible = { false };
    binding.pinEye.setOnClickListener(v -> {
      pinVisible[0] = !pinVisible[0];
      if (pinVisible[0]) {
        binding.pinInput
            .setInputType(
                InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_VARIATION_NORMAL);
        binding.pinEye.setImageResource(R.drawable.ic_eye_closed);
      } else {
        binding.pinInput.setInputType(
            InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_VARIATION_PASSWORD);
        binding.pinEye.setImageResource(R.drawable.ic_eye_open);
      }
      binding.pinInput.setSelection(binding.pinInput.getText().length());
    });

    final boolean[] confirmPinVisible = { false };
    binding.confirmPinEye.setOnClickListener(v -> {
      confirmPinVisible[0] = !confirmPinVisible[0];
      if (confirmPinVisible[0]) {
        binding.confirmPinInput
            .setInputType(
                InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_VARIATION_NORMAL);
        binding.confirmPinEye.setImageResource(R.drawable.ic_eye_closed);
      } else {
        binding.confirmPinInput.setInputType(
            InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_VARIATION_PASSWORD);
        binding.confirmPinEye.setImageResource(R.drawable.ic_eye_open);
      }
      binding.confirmPinInput.setSelection(binding.confirmPinInput.getText().length());
    });

    binding.resetPasswordButton.setOnClickListener(v -> {
      String newpassword = binding.passwordInput.getText().toString().trim();
      String confirmpassword = binding.confirmPasswordInput.getText().toString().trim();
      boolean changingPassword = !newpassword.isEmpty() || !confirmpassword.isEmpty();
      boolean changingPin = binding.enablePinCheckbox.isChecked();

      // Must change at least one thing
      if (!changingPassword && !changingPin) {
        CustomToast.show(requireContext(), getString(R.string.please_enter_new_password_or_enable_pin),
            CustomToast.Type.ERROR);
        return;
      }

      // Validate password if changing it
      if (changingPassword) {
        if (newpassword.isEmpty()) {
          CustomToast.show(requireContext(), getString(R.string.new_password_cannot_be_empty), CustomToast.Type.ERROR);
          return;
        }
        if (confirmpassword.isEmpty()) {
          CustomToast.show(requireContext(), getString(R.string.confirm_password_cannot_be_empty),
              CustomToast.Type.ERROR);
          return;
        }
        if (!newpassword.equals(confirmpassword)) {
          CustomToast.show(requireContext(),
              getString(R.string.passwords_do_not_match),
              CustomToast.Type.ERROR);
          return;
        }
      }

      // Validate PIN if changing it
      String encryptedPin = null;
      if (changingPin) {
        String pin = binding.pinInput.getText().toString().trim();
        String confirmPin = binding.confirmPinInput.getText().toString().trim();
        
        if (pin.isEmpty()) {
          CustomToast.show(requireContext(), getString(R.string.please_enter_a_pin), CustomToast.Type.ERROR);
          return;
        }
        
        if (!Utility.isValidPin(pin)) {
          CustomToast.show(requireContext(), getString(R.string.pin_must_be_4_6_digits), CustomToast.Type.ERROR);
          return;
        }
        
        if (!pin.equals(confirmPin)) {
          CustomToast.show(requireContext(), getString(R.string.pins_do_not_match), CustomToast.Type.ERROR);
          return;
        }
        
        // Encrypt PIN
        try {
          EncryptionManager encryptionManager = new EncryptionManager(requireContext());
          encryptedPin = encryptionManager.encryptPassword(pin);
        } catch (Exception e) {
          CustomToast.show(requireContext(), getString(R.string.failed_to_secure_pin), CustomToast.Type.ERROR);
          return;
        }
      }

      // Handle PIN-only change
      if (changingPin && !changingPassword) {
        handlePinOnlyChange(encryptedPin);
        return;
      }

      // Store encrypted PIN if changing both (will be saved after password change succeeds)
      if (changingPin && changingPassword && getArguments() != null) {
        getArguments().putString("encryptedPin", encryptedPin);
        getArguments().putBoolean("pinEnabled", true);
      }
      
      // Continue with password change
      String userId = null;
      String email = null;
      String token = null;
      String oldUsername = null;
      String newUserName = null;

      IStorageManager.StorageMode mode = storageManager != null
              ? storageManager.getStorageMode()
              : IStorageManager.StorageMode.UNKNOWN;

      String usernameInput = binding.usernameInput.getText().toString().trim();
      // Determine where to get user credentials based on flow type and storage mode
      if (CHANGE_TYPE_FIRST_TIME.equals(changeType)) {
        // First-time reset: Get from arguments (passed from LoginFragment)
        if (getArguments() != null) {
          // Check if username has been changed
          userId = getArguments().getString("userId");
          email = getArguments().getString("email");
          token = getArguments().getString("token");
          oldUsername = usernameRead;
          newUserName = usernameInput;
        }
      } else {
        LoginResponse loginResponse = PrefsManager.getLoginResponse(requireContext());
        if (loginResponse != null && loginResponse.getData() != null) {
          userId = loginResponse.getData().getUserId();
          email = loginResponse.getData().getEmail();
          token = loginResponse.getData().getToken();
          oldUsername = loginResponse.getData().getUserName();
          newUserName = loginResponse.getData().getUserName();
        }
      }

      if (mode == IStorageManager.StorageMode.OFFLINE) {
        storageManager.resetPassword("", userId, confirmpassword, oldUsername, newUserName,
                (success, errorMessage) -> {
                  if (success) {
                    CustomToast.show(requireContext(), getString(R.string.password_changed_successfully),
                            CustomToast.Type.SUCCESS);
                    handlePasswordChangeSuccess();
                  } else {
                    String msg = errorMessage != null ? errorMessage : getString(R.string.change_password_failed);
                    CustomToast.show(requireContext(), msg, CustomToast.Type.ERROR);
                  }
                });
      } else {
        resetPasswordViewModel.resetPassword(userId, email, confirmpassword, confirmpassword,
                token,oldUsername,newUserName);
      }
    });

    binding.backButton.setOnClickListener(v -> handleBackButtonClick());

    resetPasswordViewModel.getResetPasswordState().observe(getViewLifecycleOwner(),
        new Observer<ResetPasswordViewModel.ResetPasswordState>() {
          @Override
          public void onChanged(ResetPasswordViewModel.ResetPasswordState state) {
            if (state == null) {
              return;
            }
            switch (state.getStatus()) {
              case LOADING:
                break;
              case SUCCESS:
                if (state.getResponse() != null && state.getResponse().isSuccess()) {
                  handlePasswordChangeSuccess();
                }
                break;
              case ERROR:
                String errorMsg = state.getErrorMessage() != null ? state.getErrorMessage() : getString(R.string.change_password_failed);
                CustomToast.show(requireContext(), errorMsg, CustomToast.Type.ERROR);
                break;
            }
          }
        });

    return binding.getRoot();
  }

  private String getUsernameForPinChange() {
    if (CHANGE_TYPE_FIRST_TIME.equals(changeType)) {
      return usernameRead;
    } else {
      LoginResponse loginResponse = PrefsManager.getLoginResponse(requireContext());
      if (loginResponse != null && loginResponse.getData() != null) {
        return loginResponse.getData().getUserName();
      }
    }
    return null;
  }

  private void savePinToDatabase(String username, String encryptedPin, Runnable onSuccess, Runnable onError) {
    if (storageManager != null && encryptedPin != null && username != null && !username.isEmpty()) {
      new Thread(() -> {
        try {
          storageManager.updateUserPin(username, encryptedPin, true);
          requireActivity().runOnUiThread(() -> {
            if (onSuccess != null) onSuccess.run();
          });
        } catch (Exception e) {
          Log.e(TAG, "Error saving PIN", e);
          requireActivity().runOnUiThread(() -> {
            if (onError != null) onError.run();
          });
        }
      }).start();
    }
  }

  private void handlePinOnlyChange(String encryptedPin) {
    String username = getUsernameForPinChange();
    
    if (username == null || username.isEmpty()) {
      CustomToast.show(requireContext(), getString(R.string.unable_to_identify_user), CustomToast.Type.ERROR);
      return;
    }
    
    savePinToDatabase(username, encryptedPin, 
      () -> {
        CustomToast.show(requireContext(), getString(R.string.pin_changed_successfully),
            CustomToast.Type.SUCCESS);
        androidx.navigation.NavController navController = androidx.navigation.Navigation
            .findNavController(requireView());
        navController.popBackStack();
      },
      () -> {
        CustomToast.show(requireContext(), getString(R.string.failed_to_change_pin),
            CustomToast.Type.ERROR);
      }
    );
  }

  private void handlePasswordChangeSuccess() {
    MedDevLog.audit("User Management", "Password changed successfully");
    // Save PIN if it was set during password change (both first-time and regular)
    if (getArguments() != null) {
      String encryptedPin = getArguments().getString("encryptedPin");
      boolean pinEnabled = getArguments().getBoolean("pinEnabled", false);
      
      if (pinEnabled && encryptedPin != null) {
        String username = getUsernameForPinChange();
        savePinToDatabase(username, encryptedPin, 
          () -> Log.d(TAG, "PIN saved successfully for user: " + username),
          null
        );
      }
    }
    
    androidx.navigation.NavController navController = androidx.navigation.Navigation
        .findNavController(requireView());

    if (CHANGE_TYPE_FIRST_TIME.equals(changeType)) {
      navController.navigate(R.id.navigation_login, null, new androidx.navigation.NavOptions.Builder()
          .setPopUpTo(R.id.navigation_login, true)
          .build());
    } else {
      PrefsManager.clearLoginResponse(requireContext());
      navController.navigate(R.id.navigation_login, null, new androidx.navigation.NavOptions.Builder()
          .setPopUpTo(R.id.navigation_login, true)
          .build());
    }
  }

  private void handleBackButtonClick() {
    androidx.navigation.NavController navController = androidx.navigation.Navigation
        .findNavController(requireView());

    if (CHANGE_TYPE_FIRST_TIME.equals(changeType)) {
      navController.navigate(R.id.navigation_login, null, new androidx.navigation.NavOptions.Builder()
          .setPopUpTo(R.id.navigation_login, true)
          .build());
    } else {
      navController.popBackStack();
    }
  }

  @Override
  public void onDestroyView() {
    super.onDestroyView();
    binding = null;
  }
}
