package com.accessvascularinc.hydroguide.mma.ui;

import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.databinding.DataBindingUtil;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.Navigation;

import com.accessvascularinc.hydroguide.mma.databinding.SystemUpdateBinding;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.ui.input.OnItemClickListener;

import java.util.List;
import com.accessvascularinc.hydroguide.mma.model.SoftwareUpdateSource;
import com.accessvascularinc.hydroguide.mma.utils.Utility;


public class SoftwareUpdateFragment extends BaseHydroGuideFragment implements OnItemClickListener
{

  private SystemUpdateBinding binding = null;
  @Override
  public void onItemClickListener(int position) {}

  @Override
  public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container, final Bundle savedInstanceState)
  {
    binding = DataBindingUtil.inflate(inflater, R.layout.system_update, container, false);
    super.onCreateView(inflater, container, savedInstanceState);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    // Initialize ivUsbIcon to avoid NullPointerException in BaseHydroGuideFragment
    binding.includeTopbar.setTabletState(viewmodel.getTabletState());
    ivUsbIcon = binding.includeTopbar.usbIcon;
    tvBattery = binding.includeTopbar.controllerBatteryLevelPct;
    ivBattery = binding.includeTopbar.remoteBatteryLevelIndicator;
    ivTabletBattery = binding.includeTopbar.tabletBatteryIndicator.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.includeTopbar.tabletBatteryIndicator.tabletBatteryLevelPct;

    binding.includeTopbar.setTitle(getString(R.string.system_update));
    binding.includeTopbar.controllerBatteryLabel.setVisibility(View.INVISIBLE);
    binding.includeTopbar.controllerBatteryLevelPct.setVisibility(View.INVISIBLE);
    binding.includeTopbar.remoteBatteryLevelIndicator.setVisibility(View.INVISIBLE);
    binding.includeTopbar.usbIcon.setVisibility(View.INVISIBLE);

    binding.inclBottomBar.lnrBack.setVisibility(View.GONE);
    binding.inclBottomBar.lnrVolume.setVisibility(View.GONE);
    binding.inclBottomBar.lnrBrightness.setVisibility(View.GONE);
    binding.inclBottomBar.lnrResetPassword.setVisibility(View.GONE);
    binding.inclBottomBar.lnrRemove.setVisibility(View.GONE);
    binding.inclBottomBar.lnrSave.setVisibility(View.GONE);

    binding.inclSysUpdateOnline.imgSysUpdateModeIcon.setImageResource(R.drawable.ic_search);
    binding.inclSysUpdateOnline.tvSearchSource.setText("" + getString(R.string.sys_update_tile_online_source));
    binding.inclSysUpdateOnline.tvSourceDesc.setText("" + getString(R.string.sys_update_tile_online_source_desc));

    binding.inclSysUpdateLocalStorage.imgSysUpdateModeIcon.setImageResource(R.drawable.icon_local_storage);
    binding.inclSysUpdateLocalStorage.tvSearchSource.setText("" + getString(R.string.sys_update_tile_local_storage_source));
    binding.inclSysUpdateLocalStorage.tvSourceDesc.setText("" + getString(R.string.sys_update_tile_local_storage_source_desc));
    binding.inclSysUpdateLocalStorage.lnrSysUpdateModeTile.setOnClickListener(search_OnClickListener);

    binding.inclSysUpdateUsb.imgSysUpdateModeIcon.setImageResource(R.drawable.ic_usb);
    binding.inclSysUpdateUsb.tvSearchSource.setText("" + getString(R.string.sys_update_tile_usb_source));
    binding.inclSysUpdateUsb.tvSourceDesc.setText("" + getString(R.string.sys_update_tile_usb_source_desc));
    binding.inclSysUpdateUsb.lnrSysUpdateModeTile.setOnClickListener(search_OnClickListener);

    boolean isLocalPackageAvailable = Utility.getLocalPackageBuilds(requireContext()).size() > 0;
    binding.inclSysUpdateLocalStorage.lnrSysUpdateModeTile.setVisibility(isLocalPackageAvailable ? View.VISIBLE : View.GONE);

    binding.inclBottomBar.lnrHome.setOnClickListener(lnrHomeClick);
    binding.inclSysUpdateOnline.lnrSysUpdateModeTile.setOnClickListener(search_OnClickListener);
    binding.inclSysUpdateUsb.lnrSysUpdateModeTile.setOnClickListener(search_OnClickListener);

    if(storageManager.getStorageMode() == IStorageManager.StorageMode.OFFLINE)
    {
      binding.inclSysUpdateOnline.lnrSysUpdateModeTile.setVisibility(View.GONE);
    }
    return binding.getRoot();
  }

  private final View.OnClickListener search_OnClickListener = v -> {

    /*for online mode check
    onProcedureExitAttempt(() -> {
      SoftwareUpdateSource source = SoftwareUpdateSource.ONLINE;
      if (v.getId() == binding.btnSearchUsb.getId()) {
        source = SoftwareUpdateSource.USB;
      }
      Bundle bundle = new Bundle();
      bundle.putSerializable("updateSource", source);
      Navigation.findNavController(requireView())
              .navigate(R.id.action_navigation_software_update_to_search, bundle);
    });*/
    Bundle bundle = new Bundle();
    SoftwareUpdateSource source;
    switch (v.getId())
    {
      case R.id.inclSysUpdateOnline:
        break;
      case R.id.inclSysUpdateUsb:
        source = SoftwareUpdateSource.USB;
        bundle.putSerializable(getResources().getString(R.string.map_keys_update_src), source);
        Navigation.findNavController(requireView()).navigate(R.id.action_navigation_software_update_to_search, bundle);
        break;
      case R.id.inclSysUpdateLocalStorage:
        source = SoftwareUpdateSource.LOCAL_STORAGE;
        bundle.putSerializable(getResources().getString(R.string.map_keys_update_src), source);
        Navigation.findNavController(requireView()).navigate(R.id.action_navigation_software_update_to_search, bundle);
        break;
    }
  };

  private final View.OnClickListener lnrHomeClick = new View.OnClickListener() {
    @Override
    public void onClick(View v)
    {
      ConfirmDialog.show(requireContext(), getResources().getString(R.string.mesg_screen_back), confirmed -> {
        if (confirmed)
        {
          // online mode conition
          /*onProcedureExitAttempt(() -> {
            SoftwareUpdateSource source = SoftwareUpdateSource.ONLINE;
            if (v.getId() == binding.btnSearchUsb.getId()) {
              source = SoftwareUpdateSource.USB;
            }
            Bundle bundle = new Bundle();
            bundle.putSerializable("updateSource", source);
            Navigation.findNavController(requireView())
                    .navigate(R.id.action_navigation_software_update_to_search, bundle);
          });*/
          Navigation.findNavController(requireView()).popBackStack();
        }});
    }
  };
}
