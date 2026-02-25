package com.accessvascularinc.hydroguide.mma.ui;

import android.Manifest;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.text.InputType;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.GridView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.core.content.FileProvider;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.ViewModelProvider;
import dagger.hilt.android.AndroidEntryPoint;
import android.widget.Toast;

import com.accessvascularinc.hydroguide.mma.databinding.FragmentLoginBinding;

import androidx.lifecycle.Observer;

import com.accessvascularinc.hydroguide.mma.model.UserProfile;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.ui.LoginViewModel.LoginState;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.adapters.EmailGridAdapter;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import com.accessvascularinc.hydroguide.mma.security.EncryptionManager;
import com.accessvascularinc.hydroguide.mma.db.entities.UsersEntity;
import com.accessvascularinc.hydroguide.mma.utils.JwtHelper;
import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;

import android.widget.GridView;

import java.io.File;
import java.lang.reflect.Field;
import java.util.Collections;

import javax.inject.Inject;

@AndroidEntryPoint
public class LoginFragment extends Fragment {

  private void saveProfileToStorage(final String profileName, UserProfile userProfile) {
    final File profilesDir = requireContext()
        .getExternalFilesDir(com.accessvascularinc.hydroguide.mma.model.DataFiles.PROFILES_DIR);
    if (profilesDir != null && !profilesDir.exists()) {
      profilesDir.mkdir();
    }
    // Delete all files in the directory before saving new profile
    if (profilesDir != null && profilesDir.isDirectory()) {
      File[] files = profilesDir.listFiles();
      if (files != null) {
        for (File f : files) {
          if (f.isFile()) {
            f.delete();
          }
        }
      }
    }
    final String fileName = profileName + ".txt";
    userProfile.setFileName(fileName);
    final File file = new File(profilesDir, fileName);
    try {
      final String[] writeData = userProfile.getProfileDataText();
      if (file.exists() && file.isFile()) {
        com.accessvascularinc.hydroguide.mma.model.DataFiles.writeArrayToFile(file, writeData,
            requireContext());
      } else {
        final boolean fileCreated = file.createNewFile();
        if (fileCreated) {
          com.accessvascularinc.hydroguide.mma.model.DataFiles.writeArrayToFile(file, writeData,
              requireContext());
        }
      }
    } catch (final Exception e) {
      MedDevLog.error("BaseHydroGuideFragment", "Error writing profile file", e);
    }
  }

  private LoginViewModel loginViewModel;
  private ResetPasswordViewModel resetPasswordViewModel;
  private FragmentLoginBinding binding;

  @Inject
  IStorageManager storageManager;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
      @Nullable Bundle savedInstanceState) {
    binding = FragmentLoginBinding.inflate(inflater, container, false);
    View view = binding.getRoot();

    // Setup GridView for email selection with radio buttons
    GridView emailGridView = binding.emailGridView;
    loginViewModel = new ViewModelProvider(this).get(LoginViewModel.class);
    loginViewModel.getActiveUserEmails().observe(getViewLifecycleOwner(), emails -> {
      if (emails != null && !emails.isEmpty()) {
        EmailGridAdapter adapter = new EmailGridAdapter(requireContext(), emails);
        emailGridView.setAdapter(adapter);

        // Handle email selection callback
        adapter.setOnEmailSelectedListener((email, position) -> {
          binding.emailEditText.setText(email);
          // Move focus to password field
          binding.passwordEditText.requestFocus();
        });
      }
    });
    if (storageManager.getStorageMode() == IStorageManager.StorageMode.OFFLINE) {
      binding.resetPasswordContainer.setVisibility(View.GONE);
    }

    // Check if user has PIN enabled when email/username is entered or selected
    Runnable checkPinEnabled = () -> {
      String email = binding.emailEditText.getText().toString().trim();
      if (!email.isEmpty()) {
        new Thread(() -> {
          UsersEntity user = storageManager.getUserByUsernameOrEmail(email);
          requireActivity().runOnUiThread(() -> {
            if (binding == null)
              return; // View destroyed, exit early
            if (user != null && user.getPinEnabled() != null && user.getPinEnabled() &&
                user.getEncryptedPin() != null && !user.getEncryptedPin().isEmpty()) {
              binding.usePinCheckbox.setVisibility(View.VISIBLE);
            } else {
              binding.usePinCheckbox.setVisibility(View.GONE);
              binding.usePinCheckbox.setChecked(false);
            }
          });
        }).start();
      }
    };

    // Check PIN on focus change
    binding.emailEditText.setOnFocusChangeListener((v, hasFocus) -> {
      if (!hasFocus) {
        checkPinEnabled.run();
      }
    });

    // Check PIN on text change (after user stops typing for a moment)
    binding.emailEditText.addTextChangedListener(new android.text.TextWatcher() {
      private android.os.Handler handler = new android.os.Handler(android.os.Looper.getMainLooper());
      private Runnable workRunnable;

      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after) {
      }

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count) {
      }

      @Override
      public void afterTextChanged(android.text.Editable s) {
        if (workRunnable != null) {
          handler.removeCallbacks(workRunnable);
        }
        workRunnable = checkPinEnabled;
        handler.postDelayed(workRunnable, 500); // Check after 500ms of no typing
      }
    });

    // PIN/Password toggle
    binding.usePinCheckbox.setOnCheckedChangeListener((buttonView, isChecked) -> {
      if (isChecked) {
        binding.passwordEditText.setVisibility(View.GONE);
        binding.pinEditText.setVisibility(View.VISIBLE);
        binding.pinEditText.requestFocus();
        binding.passwordLabel.setText(R.string.pin);
        // Disable autofill for PIN mode
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
          binding.passwordEditText.setImportantForAutofill(View.IMPORTANT_FOR_AUTOFILL_NO);
        }
      } else {
        binding.passwordEditText.setVisibility(View.VISIBLE);
        binding.pinEditText.setVisibility(View.GONE);
        binding.passwordEditText.requestFocus();
        binding.passwordLabel.setText(R.string.password_label);
        // Re-enable autofill for password mode
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
          binding.passwordEditText.setImportantForAutofill(View.IMPORTANT_FOR_AUTOFILL_AUTO);
        }
      }
    });

    // Password/PIN visibility toggle logic
    final EditText passwordEditText = binding.passwordEditText;
    final EditText pinEditText = binding.pinEditText;
    final ImageView passwordToggle = binding.passwordToggle;
    final int eyeOnRes = R.drawable.ic_eye_open;
    final int eyeOffRes = R.drawable.ic_eye_closed;
    passwordToggle.setOnClickListener(new View.OnClickListener() {
      private boolean isPasswordVisible = false;

      @Override
      public void onClick(View v) {
        // Check which field is currently visible
        boolean isPinMode = pinEditText.getVisibility() == View.VISIBLE;
        EditText activeField = isPinMode ? pinEditText : passwordEditText;

        if (isPasswordVisible) {
          if (isPinMode) {
            activeField.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_VARIATION_PASSWORD);
          } else {
            activeField.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
          }
          passwordToggle.setImageResource(eyeOffRes);
        } else {
          if (isPinMode) {
            activeField.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_VARIATION_NORMAL);
          } else {
            activeField.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
          }
          passwordToggle.setImageResource(eyeOnRes);
        }
        // Move cursor to end
        activeField.setSelection(activeField.getText().length());
        isPasswordVisible = !isPasswordVisible;
      }
    });
    resetPasswordViewModel = new ViewModelProvider(this).get(ResetPasswordViewModel.class);
    binding.loginImage.setOnClickListener(v -> {
      String email = binding.emailEditText.getText().toString().trim();

      if (email.isEmpty()) {
        CustomToast.show(requireContext(), getString(R.string.username_email_cannot_be_empty), CustomToast.Type.ERROR);
        return;
      }

      // Check if using PIN
      if (binding.usePinCheckbox.isChecked()) {
        String pin = binding.pinEditText.getText().toString().trim();

        if (pin.isEmpty()) {
          CustomToast.show(requireContext(), getString(R.string.pin_cannot_be_empty), CustomToast.Type.ERROR);
          return;
        }

        // Validate PIN format
        if (!Utility.isValidPin(pin)) {
          CustomToast.show(requireContext(), getString(R.string.pin_must_be_4_6_digits), CustomToast.Type.ERROR);
          return;
        }

        // Verify PIN against stored encrypted PIN
        new Thread(() -> {
          UsersEntity user = storageManager.getUserByUsernameOrEmail(email);
          if (user == null || user.getEncryptedPin() == null) {
            requireActivity().runOnUiThread(() -> {
              CustomToast.show(requireContext(), getString(R.string.pin_not_configured_for_this_user),
                  CustomToast.Type.ERROR);
            });
            return;
          }

          // Decrypt and verify PIN using EncryptionManager
          boolean pinMatches = false;
          try {
            EncryptionManager encryptionManager = new EncryptionManager(requireContext());
            pinMatches = encryptionManager.verifyPassword(pin, user.getEncryptedPin());
          } catch (Exception e) {
            // Decryption failed
          }

          if (pinMatches) {
            // PIN is correct, check if we have stored tokens
            if (user.getEncryptedToken() != null && user.getEncryptedRefreshToken() != null) {
              try {
                EncryptionManager encryptionManager = new EncryptionManager(requireContext());
                String token = encryptionManager.decryptPassword(user.getEncryptedToken());
                String refreshToken = encryptionManager.decryptPassword(user.getEncryptedRefreshToken());

                // Check if token is still valid
                boolean tokenExpired = user.getTokenExpiry() != null
                    ? JwtHelper.isTokenExpiryPassed(user.getTokenExpiry())
                    : JwtHelper.isTokenExpired(token);

                if (!tokenExpired) {
                  // Token is valid, proceed with cached data WITHOUT calling refresh token API
                  requireActivity().runOnUiThread(() -> {
                    // Load cached user data
                    String userName = user.getUserName();
                    String userEmail = user.getEmail();
                    String userRole = user.getRole();
                    String userId = user.getCloudUserId(); // Use cloud user ID (GUID)
                    String organizationId = user.getCloudOrganizationId();

                    // Create a mock login response from cached data for navigation
                    LoginResponse cachedResponse = new LoginResponse();
                    LoginResponse.Data loginData = new LoginResponse.Data();

                    // Use reflection to set private fields since there are no setters
                    try {
                      Field userIdField = LoginResponse.Data.class.getDeclaredField("userId");
                      userIdField.setAccessible(true);
                      userIdField.set(loginData, userId);

                      Field emailField = LoginResponse.Data.class.getDeclaredField("email");
                      emailField.setAccessible(true);
                      emailField.set(loginData, userEmail);

                      Field userNameField = LoginResponse.Data.class.getDeclaredField("userName");
                      userNameField.setAccessible(true);
                      userNameField.set(loginData, userName);

                      Field isSystemGenField = LoginResponse.Data.class.getDeclaredField("isSystemGeneratedPassword");
                      isSystemGenField.setAccessible(true);
                      isSystemGenField.set(loginData, false);

                      Field tokenField = LoginResponse.Data.class.getDeclaredField("token");
                      tokenField.setAccessible(true);
                      tokenField.set(loginData, token);

                      Field refreshTokenField = LoginResponse.Data.class.getDeclaredField("refreshToken");
                      refreshTokenField.setAccessible(true);
                      refreshTokenField.set(loginData, refreshToken);

                      if (userRole != null && !userRole.isEmpty()) {
                        LoginResponse.Role role = new LoginResponse.Role();
                        Field roleField = LoginResponse.Role.class.getDeclaredField("role");
                        roleField.setAccessible(true);
                        roleField.set(role, userRole);

                        // Set organization_id in the role
                        if (organizationId != null) {
                          Field orgIdField = LoginResponse.Role.class.getDeclaredField("organization_id");
                          orgIdField.setAccessible(true);
                          orgIdField.set(role, organizationId);
                        }

                        Field rolesField = LoginResponse.Data.class.getDeclaredField("roles");
                        rolesField.setAccessible(true);
                        rolesField.set(loginData, Collections.singletonList(role));
                      }

                      Field dataField = LoginResponse.class.getDeclaredField("data");
                      dataField.setAccessible(true);
                      dataField.set(cachedResponse, loginData);

                      Field successField = LoginResponse.class.getDeclaredField("success");
                      successField.setAccessible(true);
                      successField.set(cachedResponse, true);

                      // Save to prefs for the session
                      PrefsManager.saveLoginResponse(requireContext(), cachedResponse);
                      if (userId != null) {
                        PrefsManager.setUserId(requireContext(), userId);
                      }
                      PrefsManager.setUserEmail(requireContext(), userEmail);
                      if (userId != null) {
                        MedDevLog.setLoggedInUserId(userId);
                      }

                      handleSuccessfulPinLogin(cachedResponse);
                      CustomToast.show(requireContext(), getString(R.string.logged_in_with_pin),
                          CustomToast.Type.SUCCESS);
                    } catch (Exception e) {
                      Log.e("Login", "Error creating cached response", e);
                      CustomToast.show(requireContext(), getString(R.string.error_loading_cached_data),
                          CustomToast.Type.ERROR);
                    }
                  });
                } else {
                  // Token expired, ask user to login with password
                  requireActivity().runOnUiThread(() -> {
                    CustomToast.show(requireContext(), getString(R.string.session_expired_please_login_with_password),
                        CustomToast.Type.ERROR);
                    // Clear stored tokens
                    storageManager.updateUserTokens(user.getUserName(), null, null, null);
                    binding.usePinCheckbox.setChecked(false);
                  });
                }
              } catch (Exception e) {
                requireActivity().runOnUiThread(() -> {
                  CustomToast.show(requireContext(),
                      getString(R.string.error_decrypting_tokens_please_login_with_password), CustomToast.Type.ERROR);
                });
              }
            } else {
              requireActivity().runOnUiThread(() -> {
                CustomToast.show(requireContext(),
                    getString(R.string.no_stored_session_found_please_login_with_password_first),
                    CustomToast.Type.ERROR);
              });
            }
          } else {
            requireActivity().runOnUiThread(() -> {
              CustomToast.show(requireContext(), getString(R.string.incorrect_pin), CustomToast.Type.ERROR);
            });
          }
        }).start();
      } else {
        // Use password
        String password = binding.passwordEditText.getText().toString().trim();

        if (password.isEmpty()) {
          CustomToast.show(requireContext(), getString(R.string.password_cannot_be_empty), CustomToast.Type.ERROR);
          return;
        }

        loginViewModel.login(email, password);
      }
    });

    binding.resetPasswordImage.setOnClickListener(v -> {
      triggerResetPassword();
    });

    loginViewModel.getLoginState().observe(getViewLifecycleOwner(), new Observer<LoginState>() {
      @Override
      public void onChanged(LoginState state) {
        if (state == null) {
          return;
        }
        switch (state.getStatus()) {
          case LOADING:
            break;
          case SUCCESS:
            // if (binding.progressBar != null)
            // binding.progressBar.setVisibility(View.GONE);
            if (state.getResponse() != null && state.getResponse().isSuccess()) {
              // Check if user has PIN enabled on this device
              String loginEmail = binding.emailEditText.getText().toString().trim();
              final LoginResponse finalResponse = state.getResponse();

              if (!loginEmail.isEmpty()) {
                new Thread(() -> {
                  UsersEntity user = storageManager.getUserByUsernameOrEmail(loginEmail);
                  if (user != null) {
                    boolean hasPinOnDevice = user.getPinEnabled() != null && user.getPinEnabled();

                    requireActivity().runOnUiThread(() -> {
                      if (hasPinOnDevice) {
                        // Update stored tokens
                        new Thread(() -> updateStoredTokens(user.getUserName(), finalResponse)).start();
                        // Continue with normal login
                        proceedWithLogin(finalResponse);
                      } else {
                        // Offer PIN setup only if NOT first-time login AND NOT offline mode
                        if (!finalResponse.getData().getFirstTimeLogin() &&
                            storageManager.getStorageMode() != IStorageManager.StorageMode.OFFLINE) {
                          offerPinSetup(loginEmail, finalResponse);
                        } else {
                          // First-time login or offline mode, proceed without PIN setup
                          proceedWithLogin(finalResponse);
                        }
                      }
                    });
                  } else {
                    // No user found, just proceed
                    requireActivity().runOnUiThread(() -> proceedWithLogin(finalResponse));
                  }
                }).start();
              } else {
                proceedWithLogin(finalResponse);
              }
            }
            break;
          case OTP_REQUIRED:
            // Navigate to OTP verification screen
            Bundle otpArgs = new Bundle();
            otpArgs.putString("email", binding.emailEditText.getText().toString().trim());
            androidx.navigation.NavController navControllerOtp = androidx.navigation.Navigation
                .findNavController(requireView());
            navControllerOtp.navigate(R.id.action_navigation_login_to_verify_otp, otpArgs);
            break;
          case ERROR:
            String errorMsg = state.getErrorMessage() != null ? state.getErrorMessage() : "Login failed";
            CustomToast.show(requireContext(), errorMsg, CustomToast.Type.ERROR);
            break;
        }
      }
    });

    resetPasswordViewModel.getResetPasswordState().observe(getViewLifecycleOwner(),
        new Observer<ResetPasswordViewModel.ResetPasswordState>() {
          @Override
          public void onChanged(ResetPasswordViewModel.ResetPasswordState state) {
            if (state == null)
              return;
            switch (state.getStatus()) {
              case LOADING:
                // Optionally show loading indicator
                break;
              case SUCCESS:
                if (state.getResponse() != null && state.getResponse().isSuccess()) {
                  CustomToast.show(requireContext(), "Verification code sent successfully",
                      CustomToast.Type.SUCCESS);
                  // Pass email to VerifyOtpFragment via navigation arguments
                  Bundle args = new Bundle();
                  args.putString("email", binding.emailEditText.getText().toString().trim());
                  androidx.navigation.NavController navController = androidx.navigation.Navigation
                      .findNavController(requireView());
                  navController.navigate(R.id.action_navigation_login_to_verify_otp, args);
                }
                break;
              case ERROR:
                String errorMsg = state.getErrorMessage() != null ? state.getErrorMessage()
                    : "Failed to send verification code";
                CustomToast.show(requireContext(), errorMsg, CustomToast.Type.ERROR);
                break;
            }
          }
        });
    return view;
  }

  /**
   * Update stored encrypted tokens for PIN-based login
   */
  private void updateStoredTokens(String username, LoginResponse response) {
    if (response == null || response.getData() == null) {
      return;
    }

    try {
      EncryptionManager encryptionManager = new EncryptionManager(requireContext());
      String token = response.getData().getToken();
      String refreshToken = response.getData().getRefereshToken();

      if (token != null && refreshToken != null) {
        String encryptedToken = encryptionManager.encryptPassword(token);
        String encryptedRefreshToken = encryptionManager.encryptPassword(refreshToken);

        // Extract token expiry or set to 24 hours from now as default
        Long tokenExpiry = JwtHelper.extractTokenExpiry(token);
        if (tokenExpiry == null) {
          tokenExpiry = System.currentTimeMillis() + (24 * 60 * 60 * 1000); // 24 hours
        }

        storageManager.updateUserTokens(username, encryptedToken, encryptedRefreshToken, tokenExpiry);
      }
    } catch (Exception e) {
      MedDevLog.error("LoginFragment", "Error encrypting tokens", e);
    }
  }

  /**
   * Create a LoginResponse from stored user data
   */
  private LoginResponse createLoginResponseFromUser(UsersEntity user, String token, String refreshToken) {
    // Note: This creates a mock LoginResponse for navigation purposes
    // The actual response structure may need adjustment based on your needs
    LoginResponse response = new LoginResponse();
    // Since LoginResponse fields are private and we can't set them directly,
    // we'll need to use the existing data from the user entity
    // For now, we'll just return null and handle navigation directly
    return null;
  }

  /**
   * Handle successful PIN login and navigate to appropriate screen
   */
  private void handleSuccessfulPinLogin(LoginResponse response) {
    if (response == null || response.getData() == null) {
      CustomToast.show(requireContext(), "Error loading user data", CustomToast.Type.ERROR);
      return;
    }

    String email = binding.emailEditText.getText().toString().trim();

    // Create and save profile
    String loginUserName = response.getData().getEmail() != null ? response.getData().getEmail() : "User";
    UserProfile userProfile = new UserProfile();
    userProfile.setProfileName(loginUserName);
    String profileName = com.accessvascularinc.hydroguide.mma.model.DataFiles
        .cleanFileNameString(userProfile.getProfileName());
    saveProfileToStorage(profileName, userProfile);

    // Navigate based on role
    androidx.navigation.NavController navController = androidx.navigation.Navigation
        .findNavController(requireView());

    String userRole = null;
    if (response.getData().getRoles() != null && !response.getData().getRoles().isEmpty()) {
      userRole = response.getData().getRoles().get(0).getRole();
    }

    if (userRole != null && IStorageManager.UserType.Organization_Admin.toString().equals(userRole)) {
      navController.navigate(R.id.navigation_dashboard);
    } else {
      navController.navigate(R.id.navigation_home);
    }
  }

  /**
   * Offer PIN setup to user after successful login on a new device
   */
  private void offerPinSetup(String email, LoginResponse loginResponse) {
    android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(requireContext());
    builder.setTitle("Setup PIN");
    builder.setMessage("Would you like to set up a PIN for quick login on this device?");

    builder.setPositiveButton("Setup PIN", (dialog, which) -> {
      showPinSetupDialog(email, loginResponse);
    });

    builder.setNegativeButton("Skip", (dialog, which) -> {
      // Continue with normal login flow
      proceedWithLogin(loginResponse);
    });

    builder.setCancelable(false);
    builder.show();
  }

  /**
   * Show dialog to setup PIN
   */
  private void showPinSetupDialog(String email, LoginResponse loginResponse) {
    android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(requireContext());
    builder.setTitle("Create PIN");

    android.view.LayoutInflater inflater = requireActivity().getLayoutInflater();
    android.view.View dialogView = inflater.inflate(R.layout.dialog_pin_setup, null);
    builder.setView(dialogView);

    android.widget.EditText pinInput = dialogView.findViewById(R.id.pinInput);
    android.widget.EditText confirmPinInput = dialogView.findViewById(R.id.confirmPinInput);
    android.widget.ImageView pinEye = dialogView.findViewById(R.id.pinEye);
    android.widget.ImageView confirmPinEye = dialogView.findViewById(R.id.confirmPinEye);

    // PIN visibility toggle
    final boolean[] pinVisible = { false };
    pinEye.setOnClickListener(v -> {
      pinVisible[0] = !pinVisible[0];
      if (pinVisible[0]) {
        pinInput.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_VARIATION_NORMAL);
        pinEye.setImageResource(R.drawable.ic_eye_open);
      } else {
        pinInput.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_VARIATION_PASSWORD);
        pinEye.setImageResource(R.drawable.ic_eye_closed);
      }
      pinInput.setSelection(pinInput.getText().length());
    });

    // Confirm PIN visibility toggle
    final boolean[] confirmPinVisible = { false };
    confirmPinEye.setOnClickListener(v -> {
      confirmPinVisible[0] = !confirmPinVisible[0];
      if (confirmPinVisible[0]) {
        confirmPinInput.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_VARIATION_NORMAL);
        confirmPinEye.setImageResource(R.drawable.ic_eye_open);
      } else {
        confirmPinInput.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_VARIATION_PASSWORD);
        confirmPinEye.setImageResource(R.drawable.ic_eye_closed);
      }
      confirmPinInput.setSelection(confirmPinInput.getText().length());
    });

    builder.setPositiveButton("Save", null); // Set to null to override later
    builder.setNegativeButton("Cancel", (dialog, which) -> {
      proceedWithLogin(loginResponse);
    });

    android.app.AlertDialog dialog = builder.create();
    dialog.setCancelable(false);
    dialog.show();

    // Override positive button to prevent auto-dismiss on validation errors
    dialog.getButton(android.app.AlertDialog.BUTTON_POSITIVE).setOnClickListener(v -> {
      String pin = pinInput.getText().toString().trim();
      String confirmPin = confirmPinInput.getText().toString().trim();

      if (pin.isEmpty()) {
        CustomToast.show(requireContext(), "Please enter a PIN", CustomToast.Type.ERROR);
        return;
      }

      if (!Utility.isValidPin(pin)) {
        CustomToast.show(requireContext(), "PIN must be 4-6 digits", CustomToast.Type.ERROR);
        return;
      }

      if (!pin.equals(confirmPin)) {
        CustomToast.show(requireContext(), "PINs do not match", CustomToast.Type.ERROR);
        return;
      }

      // Save PIN
      new Thread(() -> {
        try {
          EncryptionManager encryptionManager = new EncryptionManager(requireContext());
          String encryptedPin = encryptionManager.encryptPassword(pin);

          UsersEntity user = storageManager.getUserByUsernameOrEmail(email);
          if (user != null) {
            storageManager.updateUserPin(user.getUserName(), encryptedPin, true);
            updateStoredTokens(user.getUserName(), loginResponse);

            requireActivity().runOnUiThread(() -> {
              CustomToast.show(requireContext(), "PIN setup successful", CustomToast.Type.SUCCESS);
              dialog.dismiss();
              proceedWithLogin(loginResponse);
            });
          }
        } catch (Exception e) {
          requireActivity().runOnUiThread(() -> {
            CustomToast.show(requireContext(), "Failed to setup PIN", CustomToast.Type.ERROR);
          });
        }
      }).start();
    });
  }

  /**
   * Proceed with login flow after PIN setup (or skip)
   */
  private void proceedWithLogin(LoginResponse response) {
    if (response.getData().getFirstTimeLogin()) {
      Bundle args = new Bundle();
      args.putString("userId", response.getData().getUserId());
      args.putString("email", response.getData().getEmail());
      args.putString("token", response.getData().getToken());
      args.putString("changeType", "FIRST_TIME_RESET");
      args.putString("username", response.getData().getUserName());
      androidx.navigation.NavController navController = androidx.navigation.Navigation
          .findNavController(requireView());
      navController.navigate(R.id.navigation_changepassword, args);
    } else {
      String loginUserName = response.getData().getEmail();
      if (loginUserName == null) {
        loginUserName = "User";
      }

      UserProfile userProfile = new UserProfile();
      userProfile.setProfileName(loginUserName);
      String profileName = com.accessvascularinc.hydroguide.mma.model.DataFiles
          .cleanFileNameString(userProfile.getProfileName());
      saveProfileToStorage(profileName, userProfile);

      String role = response.getData().getRoles().get(0).getRole();
      androidx.navigation.NavController navController = androidx.navigation.Navigation
          .findNavController(requireView());

      if (IStorageManager.UserType.Organization_Admin.toString().equals(role)) {
        navController.navigate(R.id.navigation_dashboard);
      } else {
        navController.navigate(R.id.navigation_home);
      }
    }
  }

  @Override
  public void onDestroyView() {
    super.onDestroyView();
    binding = null;
  }

  private void triggerResetPassword() {
    String email = binding.emailEditText.getText().toString().trim();
    if (email.isEmpty()) {
      CustomToast.show(requireContext(), "Email is required to Reset Password", CustomToast.Type.ERROR);
      return;
    }
    resetPasswordViewModel.sendResetPasswordCode(email);
  }

}
