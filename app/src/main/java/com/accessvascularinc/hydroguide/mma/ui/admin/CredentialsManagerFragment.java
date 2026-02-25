package com.accessvascularinc.hydroguide.mma.ui.admin;

import android.app.Dialog;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.databinding.DataBindingUtil;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.NavController;
import androidx.navigation.Navigation;
import androidx.navigation.fragment.NavHostFragment;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentCredentialsManagerBinding;
import com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase;
import com.accessvascularinc.hydroguide.mma.db.di.DatabaseModule;
import com.accessvascularinc.hydroguide.mma.ui.BaseHydroGuideFragment;
import com.accessvascularinc.hydroguide.mma.ui.ConfirmDialog;
import com.accessvascularinc.hydroguide.mma.ui.CustomToast;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;
import com.accessvascularinc.hydroguide.mma.ui.input.InputUtils;
import com.accessvascularinc.hydroguide.mma.utils.DbHelper;
import com.google.gson.Gson;

public class CredentialsManagerFragment extends BaseHydroGuideFragment implements View.OnClickListener
{
  private FragmentCredentialsManagerBinding binding = null;
  private final Dialog closeKbBtn = null;
  private View root;
  private HydroGuideDatabase database;
  public static CredentialsManagerFragment newInstance()
  {
    CredentialsManagerFragment fragment = new CredentialsManagerFragment();
    Bundle args = new Bundle();
    fragment.setArguments(args);
    return fragment;
  }

  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    binding = DataBindingUtil.inflate(inflater, R.layout.fragment_credentials_manager, container, false);
    super.onCreateView(inflater, container, savedInstanceState);
    root = binding.getRoot();
    database = new DatabaseModule().provideDatabase(requireContext());
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    tvBattery = binding.includeTopbar.controllerBatteryLevelPct;
    ivBattery = binding.includeTopbar.remoteBatteryLevelIndicator;
    ivUsbIcon = binding.includeTopbar.usbIcon;
    ivTabletBattery = binding.includeTopbar.tabletBatteryIndicator.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.includeTopbar.tabletBatteryIndicator.tabletBatteryLevelPct;
    binding.includeTopbar.includeSearch.searchField.setQuery("",false);
    //final ProfileState profileState = viewmodel.getProfileState();
    //binding.setProfileState(profileState);
    binding.includeTopbar.setControllerState(viewmodel.getControllerState());
    binding.includeTopbar.setTabletState(viewmodel.getTabletState());
    root.getViewTreeObserver().addOnGlobalLayoutListener(InputUtils.getCloseKeyboardHandler(requireActivity(), root, closeKbBtn));
    renderWidgets();
    return root;
  }

  private void renderWidgets()
  {
    tvBattery.setVisibility(View.INVISIBLE);
    ivBattery.setVisibility(View.INVISIBLE);
    ivUsbIcon.setVisibility(View.INVISIBLE);
    binding.includeTopbar.controllerBatteryLabel.setVisibility(View.INVISIBLE);
    binding.includeTopbar.setTitle(getResources().getString(R.string.caption_clinician_screen));
    final Bundle bundl = getArguments();
    binding.inclBottomBar.lnrHome.setVisibility(View.GONE);
    binding.inclBottomBar.lnrRemove.setVisibility(View.GONE);
    binding.inclBottomBar.lnrResetPassword.setVisibility(View.GONE);
    binding.inclBottomBar.lnrVolume.setVisibility(View.GONE);
    binding.inclBottomBar.lnrBrightness.setVisibility(View.GONE);
    binding.inclBottomBar.tvRecordsAction.setText(getResources().getString(R.string.caption_reset_credentials_action));
    binding.inclBottomBar.lnrBack.setOnClickListener(this);
    binding.inclBottomBar.lnrSave.setOnClickListener(this);

  }

  @Override
  public void onClick(View viewId)
  {
    switch (viewId.getId())
    {
      case R.id.lnrBack:
        ConfirmDialog.show(requireContext(), getResources().getString(R.string.mesg_screen_back), confirmed -> {
          if (confirmed) {
            Navigation.findNavController(requireView()).popBackStack();
          }});
        break;

      case R.id.lnrSave:
        resetCredentials();
        break;
    }
  }

  private void resetCredentials()
  {
    String password = binding.edtPassword.getText().toString().trim();
    String confirmPassword = binding.edtConfirmPassword.getText().toString().trim();
    if(!password.isEmpty() & !confirmPassword.isEmpty())
    {
      if(!password.equalsIgnoreCase(confirmPassword))
      {
        CustomToast.show(requireContext(),getResources().getString(R.string.validate_mesg_password_mismatch), CustomToast.Type.ERROR);
      }
    }
    else
    {
      CustomToast.show(requireContext(),getResources().getString(R.string.validate_mesg_credentials), CustomToast.Type.ERROR);
    }
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    binding = null;
  }
}
