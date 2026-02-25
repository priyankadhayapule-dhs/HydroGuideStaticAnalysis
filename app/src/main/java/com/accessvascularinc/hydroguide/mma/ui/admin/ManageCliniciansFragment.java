package com.accessvascularinc.hydroguide.mma.ui.admin;

import android.app.Dialog;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import java.util.regex.Pattern;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import androidx.databinding.DataBindingUtil;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.NavController;
import androidx.navigation.Navigation;
import androidx.navigation.fragment.NavHostFragment;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentManageCliniciansBinding;
import com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase;
import com.accessvascularinc.hydroguide.mma.db.di.DatabaseModule;
import com.accessvascularinc.hydroguide.mma.ui.BaseHydroGuideFragment;
import com.accessvascularinc.hydroguide.mma.ui.ConfirmDialog;
import com.accessvascularinc.hydroguide.mma.ui.CustomToast;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.RecordsMode;
import com.accessvascularinc.hydroguide.mma.ui.input.InputUtils;
import com.accessvascularinc.hydroguide.mma.ui.settings.BrightnessChangeFragment;
import com.accessvascularinc.hydroguide.mma.ui.settings.VolumeChangeFragment;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import com.google.gson.Gson;

import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.CreateClinicianRequest;

import dagger.hilt.android.AndroidEntryPoint;

import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.utils.NetworkConnectivityLiveData;

import javax.inject.Inject;

@AndroidEntryPoint
public class ManageCliniciansFragment extends BaseHydroGuideFragment
    implements View.OnClickListener {
  @Inject
  IStorageManager storageManager;

  private static final String TAG = "ManageClinicians";
  private FragmentManageCliniciansBinding binding = null;
  private final Dialog closeKbBtn = null;
  private View root;
  private RecordsMode recordsMode;
  private HydroGuideDatabase database;
  private com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel clinician;
  private NetworkConnectivityLiveData networkConnectivityLiveData;
  private boolean isNetworkConnected = false;
  private String generatedPassword = "";
  private static final Pattern EMAIL_PATTERN = Pattern.compile(
      "^[A-Za-z0-9+_.-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$");

  /**
   * Validates email format
   */
  private boolean isValidEmail(String email) {
    return EMAIL_PATTERN.matcher(email).matches();
  }

  public static ManageCliniciansFragment newInstance() {
    ManageCliniciansFragment fragment = new ManageCliniciansFragment();
    Bundle args = new Bundle();
    fragment.setArguments(args);
    return fragment;
  }

  public View onCreateView(LayoutInflater inflater, ViewGroup container,
      Bundle savedInstanceState) {
    binding = DataBindingUtil.inflate(inflater, R.layout.fragment_manage_clinicians, container,
        false);
    super.onCreateView(inflater, container, savedInstanceState);
    // When called from credentials manager
    refreshClinicianCredentials();
    root = binding.getRoot();
    database = new DatabaseModule().provideDatabase(requireContext());
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    tvBattery = binding.includeTopbar.controllerBatteryLevelPct;
    ivBattery = binding.includeTopbar.remoteBatteryLevelIndicator;
    ivUsbIcon = binding.includeTopbar.usbIcon;
    ivTabletBattery = binding.includeTopbar.tabletBatteryIndicator.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.includeTopbar.tabletBatteryIndicator.tabletBatteryLevelPct;
    binding.includeTopbar.includeSearch.searchField.setQuery("", false);
    // final ProfileState profileState = viewmodel.getProfileState();
    // binding.setProfileState(profileState);
    binding.includeTopbar.setControllerState(viewmodel.getControllerState());
    binding.includeTopbar.setTabletState(viewmodel.getTabletState());
    root.getViewTreeObserver()
        .addOnGlobalLayoutListener(
            InputUtils.getCloseKeyboardHandler(requireActivity(), root, closeKbBtn));
    renderWidgets();
    android.widget.AutoCompleteTextView userTypeDropdown = root.findViewById(R.id.userTypeDropdown);
    if (userTypeDropdown != null) {
      String[] userTypes = new String[] {
          IStorageManager.UserType.Organization_Admin.name(),
          IStorageManager.UserType.Clinician.name()
      };
      android.widget.ArrayAdapter<String> userTypeAdapter = new android.widget.ArrayAdapter<>(
          requireContext(), R.layout.dropdown_list_item, userTypes);
      userTypeDropdown.setAdapter(userTypeAdapter);
      userTypeDropdown.setDropDownBackgroundResource(android.R.color.white);
      userTypeDropdown.setTextColor(getResources().getColor(R.color.black, null));
      userTypeDropdown.setOnClickListener(v -> userTypeDropdown.showDropDown());
    }

    // Add TextWatcher to convert email input to lowercase and validate
    binding.edtEmail.addTextChangedListener(new TextWatcher() {
      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after) {
      }

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count) {
      }

      @Override
      public void afterTextChanged(Editable s) {
        String email = s.toString();
        String lowercase = email.toLowerCase();
        
        // Convert to lowercase if needed
        if (!email.equals(lowercase)) {
          binding.edtEmail.setText(lowercase);
          binding.edtEmail.setSelection(lowercase.length());
        }
        
        // Validate email format and provide visual feedback
        if (!email.isEmpty() && !isValidEmail(email)) {
          binding.edtEmail.setError(getString(R.string.clinician_email_invalid_format));
        } else {
          binding.edtEmail.setError(null);
        }
      }
    });

    // Setup network connectivity monitoring
    networkConnectivityLiveData = new NetworkConnectivityLiveData(requireContext());
    networkConnectivityLiveData.observe(getViewLifecycleOwner(), isConnected -> {
      isNetworkConnected = isConnected != null && isConnected;
      updateScreenStateForClinicianManagement();
    });
    updateScreenStateForClinicianManagement();

    // fetchCliniciansList();
    // If in EDIT mode and clinician_id is present, fetch details from ViewModel
    // (MVVM)
    Bundle bundl = getArguments();
    if (bundl != null && RecordsMode
        .valueOf(
            bundl.getString(getResources().getString(R.string.caption_record_type_mode))) == RecordsMode.EDIT) {
      String clinicianId = bundl.getString("clinician_id");
      if (clinicianId != null) {
        Log.d("Records", "onCreateView: " + clinicianId);
        fetchClinicianById(clinicianId);
      }
    }
    return root;
  }

  private void updateScreenStateForClinicianManagement() {
    boolean isAccessible = isClinicianManagementAccessible(isNetworkConnected); // Use the base class method
    if (isAccessible) {
      binding.relClinicianInputs.setVisibility(View.VISIBLE);
      binding.inclBottomBar.lnrRemove.setVisibility(View.VISIBLE);
      if (recordsMode == RecordsMode.EDIT &&
          storageManager.getStorageMode() == IStorageManager.StorageMode.ONLINE) {
        binding.inclBottomBar.lnrResetPassword.setVisibility(View.VISIBLE);
      } else {
        binding.inclBottomBar.lnrResetPassword.setVisibility(View.GONE);
      }
      binding.tvNoInternetMessage.setVisibility(View.GONE);
    } else {
      binding.relClinicianInputs.setVisibility(View.GONE);
      binding.inclBottomBar.lnrRemove.setVisibility(View.GONE);
      binding.inclBottomBar.lnrResetPassword.setVisibility(View.GONE);
      binding.tvNoInternetMessage.setVisibility(View.VISIBLE);
    }
  }

  private void renderWidgets() {
    tvBattery.setVisibility(View.INVISIBLE);
    ivBattery.setVisibility(View.INVISIBLE);
    ivUsbIcon.setVisibility(View.INVISIBLE);
    binding.includeTopbar.controllerBatteryLabel.setVisibility(View.INVISIBLE);
    binding.includeTopbar.setTitle(getResources().getString(R.string.caption_clinician_screen));
    Bundle bundl = getArguments();
    recordsMode = RecordsMode.valueOf(
        bundl.getString(getResources().getString(R.string.caption_record_type_mode)));
    switch (recordsMode) {
      case CREATE:
        binding.inclBottomBar.lnrHome.setVisibility(View.GONE);
        binding.inclBottomBar.lnrRemove.setVisibility(View.GONE);
        binding.inclBottomBar.lnrResetPassword.setVisibility(View.GONE);
        binding.edtClinicianName.setEnabled(true);
        binding.edtEmail.setEnabled(true);
        break;
      case EDIT:
        // Fields will be set and disabled after API fetch in onCreateView
        binding.inclBottomBar.lnrHome.setVisibility(View.GONE);
        binding.inclBottomBar.lnrRemove.setVisibility(View.VISIBLE);
        IStorageManager.StorageMode storageMode = storageManager.getStorageMode();
        if (storageMode == IStorageManager.StorageMode.ONLINE) {
          binding.inclBottomBar.lnrResetPassword.setVisibility(View.VISIBLE);
        } else {
          binding.inclBottomBar.lnrResetPassword.setVisibility(View.GONE);
        }
        binding.inclBottomBar.lnrSave.setVisibility(View.GONE);
        binding.edtClinicianName.setEnabled(false);
        binding.edtEmail.setEnabled(false);
        binding.userTypeDropdown.setEnabled(false);
        this.clinician = clinician;
        break;
    }
    binding.inclBottomBar.lnrBack.setOnClickListener(this);
    binding.inclBottomBar.lnrVolume.setOnClickListener(this);
    binding.inclBottomBar.lnrBrightness.setOnClickListener(this);
    binding.inclBottomBar.lnrResetPassword.setOnClickListener(this);
    binding.inclBottomBar.lnrSave.setOnClickListener(this);
    binding.inclBottomBar.lnrRemove.setOnClickListener(this);

    // Set up password field visibility and listeners only for Offline mode
    if (storageManager.getStorageMode() == IStorageManager.StorageMode.OFFLINE) {
      binding.lnrPassword.setVisibility(View.VISIBLE);
      binding.btnRefreshPassword.setOnClickListener(v -> onRefreshPasswordClicked());
      binding.btnCopyPassword.setOnClickListener(v -> copyPasswordToClipboard());
    } else {
      binding.lnrPassword.setVisibility(View.GONE);
    }
  }

  @Override
  public void onClick(View viewId) {
    switch (viewId.getId()) {
      case R.id.lnrBack:
        ConfirmDialog.show(requireContext(), getResources().getString(R.string.mesg_screen_back),
            confirmed -> {
              if (confirmed) {
                Navigation.findNavController(requireView()).popBackStack();
              }
            });
        break;
      case R.id.lnrVolume:
        VolumeChangeFragment volumeChange = new VolumeChangeFragment();
        volumeChange.show(getChildFragmentManager(), volumeChange.getClass().getSimpleName());
        break;
      case R.id.lnrBrightness:
        BrightnessChangeFragment brightnessChange = new BrightnessChangeFragment();
        brightnessChange.show(getChildFragmentManager(),
            brightnessChange.getClass().getSimpleName());
        break;
      case R.id.lnrSave:
        if (storageManager.getStorageMode() == IStorageManager.StorageMode.ONLINE && !isNetworkConnected) {
          CustomToast.show(requireContext(), "No internet connection. Please connect to save.", CustomToast.Type.WARNING);
          return;
        }
        saveClinician();
        break;
      case R.id.lnrRemove:
        ConfirmDialog.show(requireContext(),
            getResources().getString(R.string.mesg_confirm_remove_clinician),
            confirmed -> {
              if (confirmed) {
                removeClinician();
              }
            });
        break;
      case R.id.lnrResetPassword:
        ConfirmDialog.show(requireContext(), "Are you sure you want to reset the password?",
            confirmed -> {
              if (confirmed) {
                resetPassword();
              }
            });
        break;
    }
  }

  private void saveClinician() {
    String clinicianName = binding.edtClinicianName.getText().toString().trim();
    String clinicianEmail = binding.edtEmail.getText().toString().trim();
    String clinicianPassword = binding.edtPassword.getText().toString().trim();
    String userType = "";
    if (binding.userTypeDropdown != null && binding.userTypeDropdown.getText() != null) {
      userType = binding.userTypeDropdown.getText().toString().trim();
    }

    if (!clinicianEmail.isEmpty() && !clinicianName.isEmpty() && !userType.isEmpty()) {
      // Validate email format
      if (!isValidEmail(clinicianEmail)) {
        CustomToast.show(requireContext(), getString(R.string.clinician_email_invalid), CustomToast.Type.ERROR);
        return;
      }
      
      // Validate username format
      Utility.ValidationResult usernameValidation = Utility.validateUsername(clinicianName);
      if (!usernameValidation.isValid()) {
        CustomToast.show(requireContext(), usernameValidation.getErrorMessage(), CustomToast.Type.ERROR);
        return;
      }

      IStorageManager.StorageMode storageMode = storageManager.getStorageMode();
      if (storageMode == IStorageManager.StorageMode.OFFLINE && clinicianPassword.isEmpty()) {
        CustomToast.show(requireContext(), getString(R.string.clinician_password_required), CustomToast.Type.WARNING);
        return;
      }
      String organizationId = PrefsManager.getOrganizationId(requireContext());
      String password = clinicianPassword;
      String roleName = userType;
      String userName = "";

      CreateClinicianRequest request = new CreateClinicianRequest(
          clinicianEmail, clinicianName, password, roleName, organizationId, userName);

      storageManager.createClinician(organizationId, request, (response, errorMessage) -> {
        if (response != null && response.isSuccess()) {
          binding.edtClinicianName.setText("");
          binding.edtEmail.setText("");
          binding.edtPassword.setText("");
          generatedPassword = "";
          MedDevLog.audit("User Management", "User created with role: " + roleName + ", email: " + clinicianEmail);
          CustomToast.show(requireContext(), getString(R.string.clinician_created_success),
              CustomToast.Type.SUCCESS);
          Navigation.findNavController(requireView()).popBackStack();
        } else {
          String errMsg = errorMessage;
          if (errMsg == null && response != null) {
            errMsg = response.getErrorMessage();
          }
          CustomToast.show(requireContext(),
              errMsg != null ? errMsg : getString(R.string.clinician_error_generic),
              CustomToast.Type.ERROR);
        }
      });
    } else {
      CustomToast.show(requireContext(), getString(R.string.clinician_all_fields_required), CustomToast.Type.WARNING);
    }
  }

  private void updateClinician(String name, String email) {
    // No update logic required; only display clinicians from API. Local DB logic
    // removed.
  }

  private void removeClinician() {
    if (clinician != null) {
      String organizationId = PrefsManager.getOrganizationId(requireContext());
      storageManager.removeClinician(organizationId, clinician.getUserId(),
          (success, errorMessage) -> {
            if (success) {
              CustomToast.show(requireContext(), getString(R.string.clinician_removed_success),
                  CustomToast.Type.SUCCESS);
              Navigation.findNavController(requireView()).popBackStack();
            } else {
              CustomToast.show(requireContext(), errorMessage != null ? errorMessage : getString(R.string.clinician_error_generic),
                  CustomToast.Type.ERROR);
            }
          });
    } else {
      CustomToast.show(requireContext(), getString(R.string.clinician_none_selected), CustomToast.Type.WARNING);
    }
  }

  private void resetPassword() {
    if (clinician != null) {
      String organizationId = PrefsManager.getOrganizationId(requireContext());
      // true indicates this is an admin-initiated reset
      storageManager.resetPassword(organizationId, clinician.getUserId(), "", null,null,
          true, (success, errorMessage) -> {
            if (success) {
              MedDevLog.audit("User Management", "Password reset initiated for user: " + clinician.getUserEmail());
              CustomToast.show(requireContext(), getString(R.string.clinician_password_reset_success),
                  CustomToast.Type.SUCCESS);
            } else {
              CustomToast.show(requireContext(), errorMessage != null ? errorMessage : getString(R.string.clinician_error_generic),
                  CustomToast.Type.ERROR);
            }
          });
    } else {
      CustomToast.show(requireContext(), getString(R.string.clinician_none_selected), CustomToast.Type.WARNING);
    }
  }

  private void navigateToScreen(int layoutIdentifier, Bundle bundl) {
    Navigation.findNavController(requireView()).navigate(layoutIdentifier, bundl);
  }

  private void navigatePostTransaction() {
    new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
      @Override
      public void run() {
        Navigation.findNavController(requireView()).popBackStack();
        // binding.inclBottomBar.lnrBack.performClick();
      }
    }, 1000);
  }

  private void refreshClinicianCredentials() {
    NavController navController = NavHostFragment.findNavController(this);
    LiveData<String> liveData = navController.getCurrentBackStackEntry().getSavedStateHandle()
        .getLiveData(getResources().getString(R.string.stored_credentials));
    liveData.observe(getViewLifecycleOwner(), data -> {
      clinician = new Gson().fromJson(data,
          com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel.class);
    });
  }

  private void fetchClinicianById(String userId) {
    String organizationId = PrefsManager.getOrganizationId(requireContext());
    storageManager.fetchUsersById(organizationId, userId, (clinician, errorMessage) -> {
      if (clinician != null) {
        binding.edtClinicianName.setText(clinician.getUserName());
        binding.edtEmail.setText(clinician.getUserEmail());
        binding.userTypeDropdown.setText(clinician.getRoles().get(0).getRole());
        binding.edtClinicianName.setEnabled(false);
        binding.edtEmail.setEnabled(false);
        binding.userTypeDropdown.setEnabled(false);
        binding.userTypeDropdown.setTextColor(getResources().getColor(R.color.av_gray, null));
        this.clinician = clinician;
      } else {
        CustomToast.show(requireContext(),
            errorMessage != null ? errorMessage : getString(R.string.clinician_error_fetching),
            CustomToast.Type.ERROR);
      }
    });
  }

  @Override
  public void onResume() {
    super.onResume();
  }

  @Override
  public void onDestroyView() {
    super.onDestroyView();
    if (networkConnectivityLiveData != null) {
      networkConnectivityLiveData.removeObservers(getViewLifecycleOwner());
    }
    binding = null;
  }

  /**
   * Generates a random 8-character password with mixed uppercase, lowercase,
   * digits, and special
   * characters
   */
  private String generateRandomPassword() {
    return Utility.generateRandomPassword();
  }

  /**
   * Generates and displays password in the password field
   */
  private void generateAndDisplayPassword() {
    generatedPassword = generateRandomPassword();
    binding.edtPassword.setText(generatedPassword);
    if (recordsMode == RecordsMode.EDIT) {
      String organizationId = PrefsManager.getOrganizationId(requireContext());
      // true indicates this is an admin-initiated reset
      storageManager.resetPassword(organizationId, clinician.getUserId(), generatedPassword,null,null,
          true, (success, errorMessage) -> {
            if (success) {
              CustomToast.show(requireContext(), getString(R.string.clinician_password_reset_success),
                  CustomToast.Type.SUCCESS);
            } else {
              CustomToast.show(requireContext(), errorMessage != null ? errorMessage : getString(R.string.clinician_error_resetting_password),
                  CustomToast.Type.ERROR);
            }
          });
    }
  }

  /**
   * Handles the refresh password button click. In CREATE mode: generates password
   * immediately In
   * EDIT mode: shows confirmation dialog before generating
   */
  private void onRefreshPasswordClicked() {
    if (recordsMode == RecordsMode.EDIT) {
      // In EDIT mode, show confirmation dialog
      ConfirmDialog.show(requireContext(), getString(R.string.clinician_password_reset_confirm),
          confirmed -> {
            if (confirmed) {
              generateAndDisplayPassword();
            }
          });
    } else {
      // In CREATE mode, generate password directly
      generateAndDisplayPassword();
    }
  }

  /**
   * Copies the generated password to clipboard when user clicks the copy button
   */
  private void copyPasswordToClipboard() {
    if (generatedPassword.isEmpty()) {
      CustomToast.show(requireContext(), getString(R.string.clinician_generate_password_first),
          CustomToast.Type.WARNING);
      return;
    }

    ClipboardManager clipboard = (ClipboardManager) requireContext().getSystemService(Context.CLIPBOARD_SERVICE);
    if (clipboard != null) {
      ClipData clip = ClipData.newPlainText("password", generatedPassword);
      clipboard.setPrimaryClip(clip);
      CustomToast.show(requireContext(), getResources().getString(R.string.password_copied),
          CustomToast.Type.SUCCESS);
    }
  }
}
