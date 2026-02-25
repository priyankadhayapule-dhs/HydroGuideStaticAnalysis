package com.accessvascularinc.hydroguide.mma.ui;

import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.ViewModelProvider;
import com.accessvascularinc.hydroguide.mma.network.VerifyOtpModel.VerifyOtpResponse;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentVerifyOtpBinding;

public class VerifyOtpFragment extends Fragment {
  private FragmentVerifyOtpBinding binding;
  private VerifyOtpViewModel verifyOtpViewModel;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
      @Nullable Bundle savedInstanceState) {
    binding = FragmentVerifyOtpBinding.inflate(inflater, container, false);
    View view = binding.getRoot();
    verifyOtpViewModel = new ViewModelProvider(this,
        new ViewModelProvider.AndroidViewModelFactory(requireActivity().getApplication()))
        .get(VerifyOtpViewModel.class);

    EditText[] otpBoxes = { binding.otpBox1, binding.otpBox2, binding.otpBox3, binding.otpBox4, binding.otpBox5,
        binding.otpBox6 };
    for (int i = 0; i < otpBoxes.length; i++) {
      final int index = i;
      otpBoxes[i].addTextChangedListener(new TextWatcher() {
        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {
        }

        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
          if (s.length() == 1 && index < otpBoxes.length - 1) {
            otpBoxes[index + 1].requestFocus();
          }
        }

        @Override
        public void afterTextChanged(Editable s) {
        }
      });
    }

    final String email;
    if (getArguments() != null && getArguments().containsKey("email")) {
      email = getArguments().getString("email");
    } else {
      email = "";
    }

    binding.verifyOtpButton.setOnClickListener(v -> {
      StringBuilder otp = new StringBuilder();
      for (EditText box : otpBoxes) {
        otp.append(box.getText().toString());
      }
      verifyOtpViewModel.verifyOtp(email, otp.toString());
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

    verifyOtpViewModel.getVerificationState().observe(getViewLifecycleOwner(), state -> {
      if (state == null)
        return;
      switch (state.getStatus()) {
        case LOADING:
          break;
        case SUCCESS:
          CustomToast.show(requireContext(), "OTP verified!", CustomToast.Type.SUCCESS);
          // Get userId, email, and token from lastVerifyOtpResponse
          String userId = "";
          String emailArg = "";
          String token = "";
          String username = "";
          VerifyOtpResponse lastResponse = verifyOtpViewModel.getLastVerifyOtpResponse();
          if (lastResponse != null && lastResponse.getData() != null) {
            userId = lastResponse.getData().getUserId();
            emailArg = lastResponse.getData().getEmail();
            token = lastResponse.getData().getToken();
            username = lastResponse.getData().getUserName();
          }
          Bundle args = new Bundle();
          args.putString("userId", userId);
          args.putString("email", emailArg);
          args.putString("token", token);
          args.putString("username", username);

          androidx.navigation.NavController navController = androidx.navigation.Navigation
              .findNavController(requireView());
          navController.navigate(R.id.action_navigation_verify_otp_to_reset_password, args);
          break;
        case ERROR:
          String errorMsg = state.getErrorMessage() != null ? state.getErrorMessage() : "OTP verification failed";
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
