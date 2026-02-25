package com.accessvascularinc.hydroguide.mma.ui.admin;

import android.app.Dialog;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.CompoundButton;
import androidx.annotation.NonNull;
import androidx.databinding.DataBindingUtil;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.Navigation;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentManageFacilitiesBinding;
import com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase;
import com.accessvascularinc.hydroguide.mma.db.di.DatabaseModule;
import com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity;
import com.accessvascularinc.hydroguide.mma.db.repository.FacilityRepository;
import com.accessvascularinc.hydroguide.mma.ui.BaseHydroGuideFragment;
import com.accessvascularinc.hydroguide.mma.ui.ConfirmDialog;
import com.accessvascularinc.hydroguide.mma.ui.CustomToast;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;
import com.accessvascularinc.hydroguide.mma.utils.RecordsMode;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.ui.input.InputUtils;
import com.accessvascularinc.hydroguide.mma.ui.settings.BrightnessChangeFragment;
import com.accessvascularinc.hydroguide.mma.ui.settings.VolumeChangeFragment;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import com.google.gson.Gson;

import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;

import dagger.hilt.android.AndroidEntryPoint;
import javax.inject.Inject;

@AndroidEntryPoint
public class ManageFacilitiesFragment extends BaseHydroGuideFragment
    implements View.OnClickListener, CompoundButton.OnCheckedChangeListener {
  private FragmentManageFacilitiesBinding binding = null;
  private final Dialog closeKbBtn = null;
  private View root;
  private RecordsMode recordsMode;
  private FacilityEntity facility;
  private HydroGuideDatabase database;
  @Inject
  IStorageManager storageManager;

  public static ManageFacilitiesFragment newInstance() {
    ManageFacilitiesFragment fragment = new ManageFacilitiesFragment();
    Bundle args = new Bundle();
    fragment.setArguments(args);
    return fragment;
  }

  public View onCreateView(LayoutInflater inflater, ViewGroup container,
      Bundle savedInstanceState) {
    binding = DataBindingUtil.inflate(inflater, R.layout.fragment_manage_facilities, container,
        false);
    super.onCreateView(inflater, container, savedInstanceState);
    root = binding.getRoot();
    database = new DatabaseModule().provideDatabase(requireContext());
    facilityRepository = new FacilityRepository(database.facilityDao());
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
    root.getViewTreeObserver().addOnGlobalLayoutListener(
        InputUtils.getCloseKeyboardHandler(requireActivity(), root, closeKbBtn));
    renderWidgets();
    return root;
  }

  private void renderWidgets() {
    tvBattery.setVisibility(View.INVISIBLE);
    ivBattery.setVisibility(View.INVISIBLE);
    ivUsbIcon.setVisibility(View.INVISIBLE);

    binding.includeTopbar.controllerBatteryLabel.setVisibility(View.INVISIBLE);
    binding.includeTopbar.setTitle(getResources().getString(R.string.caption_facility_screen));
    binding.tvFacilityName.setText(
        getResources().getString(R.string.pref_facility_name_title) + ":");
    final Bundle bundl = getArguments();
    recordsMode = RecordsMode.valueOf(
        bundl.getString(getResources().getString(R.string.caption_record_type_mode)));
    switch (recordsMode) {
      case CREATE:
        binding.inclBottomBar.lnrHome.setVisibility(View.GONE);
        binding.inclBottomBar.lnrRemove.setVisibility(View.GONE);
        binding.inclBottomBar.lnrResetPassword.setVisibility(View.GONE);
        break;
      case EDIT:
        facility = new Gson().fromJson(
            bundl.getString(getResources().getString(R.string.caption_selected_record_data)),
            FacilityEntity.class);
        binding.edtFacilityName.setText(facility.getFacilityName());
        binding.chkPatientName.setChecked(facility.isShowPatientName());
        binding.chkInsertionVien.setChecked(facility.isShowInsertionVein());
        binding.chkPatientId.setChecked(facility.isShowPatientId());
        binding.chkPatientSide.setChecked(facility.isShowPatientSide());
        binding.chkPatientDob.setChecked(facility.isShowPatientDob());
        binding.chkArmCircumference.setChecked(facility.isShowArmCircumference());
        binding.chkVienSize.setChecked(facility.isShowVeinSize());
        binding.chkReason.setChecked(facility.isShowReasonForInsertion());
        binding.chkNoOfLumens.setChecked(facility.isShowNoOfLumens());
        binding.chkCatheterSize.setChecked(facility.isShowCatheterSize());
        binding.inclBottomBar.lnrResetPassword.setVisibility(View.GONE);
        binding.inclBottomBar.lnrRemove.setVisibility(View.GONE);
        break;
    }
    binding.inclBottomBar.lnrBack.setOnClickListener(this);
    binding.inclBottomBar.lnrVolume.setOnClickListener(this);
    binding.inclBottomBar.lnrBrightness.setOnClickListener(this);
    binding.inclBottomBar.lnrSave.setOnClickListener(this);
    binding.inclBottomBar.lnrRemove.setOnClickListener(this);
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
        createOrUpdateFacility();
        break;
      case R.id.lnrRemove:
        ConfirmDialog.show(requireContext(),
            getResources().getString(R.string.mesg_confirm_remove_facility), confirmed -> {
              if (confirmed) {
                removeFacility();
              }
            });
        break;
    }
  }

  @Override
  public void onCheckedChanged(@NonNull CompoundButton buttonView, boolean isChecked) {
    switch (buttonView.getId()) {
      case R.id.chkPatientName:
        binding.chkPatientName.setChecked(isChecked);
        break;
      case R.id.chkInsertionVien:
        binding.chkInsertionVien.setChecked(isChecked);
        break;
      case R.id.chkPatientId:
        binding.chkPatientId.setChecked(isChecked);
        break;
      case R.id.chkPatientSide:
        binding.chkPatientSide.setChecked(isChecked);
        break;
      case R.id.chkPatientDob:
        binding.chkPatientDob.setChecked(isChecked);
        break;
      case R.id.chkArmCircumference:
        binding.chkArmCircumference.setChecked(isChecked);
        break;
      case R.id.chkVienSize:
        binding.chkVienSize.setChecked(isChecked);
        break;
      case R.id.chkReason:
        binding.chkReason.setChecked(isChecked);
        break;
    }
  }

  private void createOrUpdateFacility() {
    String facilityName = binding.edtFacilityName.getText().toString().trim();
    String organizationId = com.accessvascularinc.hydroguide.mma.utils.PrefsManager.getOrganizationId(
        requireContext());
    com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse loginResponse = com.accessvascularinc.hydroguide.mma.utils.PrefsManager
        .getLoginResponse(requireContext());
    Integer createdBy = null;
    String createdByEmail = null;

    if (loginResponse != null && loginResponse.getData() != null) {
      String userIdStr = loginResponse.getData().getUserId();
      createdByEmail = loginResponse.getData().getEmail();
      if (userIdStr != null) {
        try {
          createdBy = Integer.parseInt(userIdStr);
        } catch (NumberFormatException ignored) {
          // If userId is not numeric (e.g., GUID from cloud), leave createdBy as null
        }
      }
    }

    final Integer finalCreatedBy = createdBy;
    final String finalCreatedByEmail = createdByEmail;
    if (!facilityName.isEmpty()) {
      Dialog dialog = ConfirmDialog.show(requireContext(),
          getResources().getString(R.string.mesg_confirm_save_facility), confirmed -> {
            if (confirmed) {
              if (recordsMode == RecordsMode.EDIT) {
                updateFacility(facilityName);
              } else if (recordsMode == RecordsMode.CREATE) {
                FacilityEntity facility = new FacilityEntity(
                    facilityName,
                    organizationId,
                    Utility.fetchDate(),
                    finalCreatedBy != null ? finalCreatedBy : 0,
                    Utility.fetchDate(),
                    true,
                    binding.chkPatientName.isChecked(),
                    binding.chkInsertionVien.isChecked(),
                    binding.chkPatientId.isChecked(),
                    binding.chkPatientSide.isChecked(),
                    binding.chkPatientDob.isChecked(),
                    binding.chkArmCircumference.isChecked(),
                    binding.chkVienSize.isChecked(),
                    binding.chkReason.isChecked(),
                    binding.chkNoOfLumens.isChecked(),
                    binding.chkCatheterSize.isChecked(),
                    finalCreatedByEmail != null ? finalCreatedByEmail : "");
                storageManager.createFacility(facility);
                MedDevLog.audit("Facility", "Facility created: " + facilityName);
                clearFields();
                CustomToast.show(requireContext(),
                    "Facility : " + facilityName + " created successfully",
                    CustomToast.Type.SUCCESS);
                if (syncManager != null) {
                  syncManager.syncFacility();
                }
              }
            }
          });
      dialog.dismiss();
      dialog.show();
      DisplayMetrics displayMetrics = new DisplayMetrics();
      dialog.getWindow().getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
      WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams();
      layoutParams.copyFrom(dialog.getWindow().getAttributes());
      layoutParams.width = (int) (displayMetrics.widthPixels * 0.7f);
      // layoutParams.height = (int) (displayMetrics.heightPixels * 0.3f);
      dialog.getWindow().setAttributes(layoutParams);
    } else {
      CustomToast.show(requireContext(), getResources().getString(R.string.validate_mesg_facility),
          CustomToast.Type.ERROR);
    }
  }

  private void updateFacility(String facilityName) {
    if (facility != null) {
      facility.setFacilityName(facilityName);
      facility.setShowPatientName(binding.chkPatientName.isChecked());
      facility.setShowInsertionVein(binding.chkInsertionVien.isChecked());
      facility.setShowPatientId(binding.chkPatientId.isChecked());
      facility.setShowPatientSide(binding.chkPatientSide.isChecked());
      facility.setShowPatientDob(binding.chkPatientDob.isChecked());
      facility.setShowArmCircumference(binding.chkArmCircumference.isChecked());
      facility.setShowVeinSize(binding.chkVienSize.isChecked());
      facility.setShowReasonForInsertion(binding.chkReason.isChecked());
      facility.setShowNoOfLumens(binding.chkNoOfLumens.isChecked());
      facility.setShowCatheterSize(binding.chkCatheterSize.isChecked());
      storageManager.updateFacility(facility);
      MedDevLog.audit("Facility", "Facility modified: " + facilityName);
      CustomToast.show(requireContext(),
          getResources().getString(R.string.validate_mesg_facility_updated),
          CustomToast.Type.SUCCESS);
    }
  }

  private void removeFacility() {
    if (facility != null) {
      storageManager.deleteFacility(facility);
      MedDevLog.audit("Facility", "Facility deactivated: " + facility.getFacilityName());
      CustomToast.show(requireContext(),
          getResources().getString(R.string.validate_mesg_facility_deleted),
          CustomToast.Type.SUCCESS);
      navigatePostTransaction();
    }
  }

  private void navigatePostTransaction() {
    new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
      @Override
      public void run() {
        Navigation.findNavController(requireView()).popBackStack();
      }
    }, 1000);
  }

  private void clearFields() {
    binding.edtFacilityName.setText("");
    //Patient Name,ID,DOB must be selected by default
    binding.chkPatientName.setChecked(true);
    binding.chkPatientId.setChecked(true);
    binding.chkPatientDob.setChecked(true);
    binding.chkInsertionVien.setChecked(false);
    binding.chkPatientSide.setChecked(false);
    binding.chkArmCircumference.setChecked(false);
    binding.chkVienSize.setChecked(false);
    binding.chkReason.setChecked(false);
    binding.chkNoOfLumens.setChecked(false);
    binding.chkCatheterSize.setChecked(false);
  }

  @Override
  public void onDestroyView() {
    super.onDestroyView();
    binding = null;
  }
}
