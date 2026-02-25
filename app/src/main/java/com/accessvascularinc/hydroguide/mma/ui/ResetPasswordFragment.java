
package com.accessvascularinc.hydroguide.mma.ui;

import android.text.InputType;

import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.ViewModelProvider;

import com.accessvascularinc.hydroguide.mma.databinding.FragmentResetPasswordBinding;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.network.ResetPasswordModel.ResetPasswordModel.ResetPasswordResponse;
import com.accessvascularinc.hydroguide.mma.security.EncryptionManager;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import javax.inject.Inject;

import dagger.hilt.android.AndroidEntryPoint;

@AndroidEntryPoint
public class ResetPasswordFragment extends Fragment {
  private static final String TAG = "ResetPasswordFragment";
  
  private FragmentResetPasswordBinding binding;
  private ResetPasswordViewModel resetPasswordViewModel;
  
  @Inject
  IStorageManager storageManager;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
      @Nullable Bundle savedInstanceState) {
    binding = FragmentResetPasswordBinding.inflate(inflater, container, false);
    View view = binding.getRoot();
    resetPasswordViewModel = new ViewModelProvider(this,
            new ViewModelProvider.AndroidViewModelFactory(requireActivity().getApplication()))
            .get(ResetPasswordViewModel.class);

    binding.usernameInput.setVisibility(View.GONE);
    binding.usernameLabel.setVisibility(View.GONE);
    binding.pinSetupSection.setVisibility(View.GONE);

    final boolean[] passwordVisible = { false };
    binding.passwordEye.setOnClickListener(v -> {
      passwordVisible[0] = !passwordVisible[0];
      if (passwordVisible[0]) {
        binding.passwordInput
            .setInputType(
                InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
        binding.passwordEye.setImageResource(R.drawable.ic_eye_open);
      } else {
        binding.passwordInput.setInputType(
            InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
        binding.passwordEye.setImageResource(R.drawable.ic_eye_closed);
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
        binding.confirmPasswordEye.setImageResource(R.drawable.ic_eye_open);
      } else {
        binding.confirmPasswordInput
            .setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
        binding.confirmPasswordEye.setImageResource(R.drawable.ic_eye_closed);
      }
      binding.confirmPasswordInput.setSelection(binding.confirmPasswordInput.getText().length());
    });

    binding.backButton.setOnClickListener(v -> {
      ConfirmDialog.show(requireContext(), "Are you sure you want to go back?", confirmed -> {
        if (confirmed) {
          androidx.navigation.NavController navController = androidx.navigation.Navigation
              .findNavController(requireView());
          navController.navigate(R.id.navigation_login);
        }
      });
    });

    // Get arguments from bundle (userId, email, token)
    Bundle args = getArguments();
    String userId = args != null ? args.getString("userId", "") : "";
    String email = args != null ? args.getString("email", "") : "";
    String token = args != null ? args.getString("token", "") : "";
    String username = args != null ? args.getString("username", "") : "";

    binding.resetPasswordButton.setOnClickListener(v -> {
      String password = binding.passwordInput.getText().toString();
      String confirmPassword = binding.confirmPasswordInput.getText().toString();
      if (password.isEmpty() || confirmPassword.isEmpty()) {
        CustomToast.show(requireContext(), "Please enter both fields", CustomToast.Type.ERROR);
        return;
      }
      if (!password.equals(confirmPassword)) {
        CustomToast.show(requireContext(), "Passwords do not match", CustomToast.Type.ERROR);
        return;
      }

      MedDevLog.info("Password", "Resetting password for userId:" + userId + ", email:" + email);
      resetPasswordViewModel.resetPassword(userId, email, password, password, token, username,
              username);
    });

    resetPasswordViewModel.getResetPasswordState().observe(getViewLifecycleOwner(), state -> {
      if (state == null) {
        return;
      }
      switch (state.getStatus()) {
        case LOADING:
          // Optionally show loading indicator
          break;
        case SUCCESS:
          // Verify the response is actually successful
          if (state.getResponse() != null && state.getResponse().isSuccess()) {
            CustomToast.show(requireContext(), "Password reset successfully!",
                CustomToast.Type.SUCCESS);
            
            androidx.navigation.NavController navController = androidx.navigation.Navigation
                .findNavController(requireView());
            navController.navigate(R.id.navigation_login);
          } else {
            String errorMsg = state.getResponse() != null && state.getResponse().getErrorMessage() != null
                ? state.getResponse().getErrorMessage()
                : "Password reset failed";
            CustomToast.show(requireContext(), errorMsg, CustomToast.Type.ERROR);
          }
          break;
        case ERROR:
          String errorMsg = state.getErrorMessage() != null ? state.getErrorMessage()
              : "Password reset failed";
          CustomToast.show(requireContext(), errorMsg, CustomToast.Type.ERROR);
          break;
      }
    });

    return view;
  }

  @Override
  public void onDestroyView() {
    super.onDestroyView();
    binding = null;
  }
}
