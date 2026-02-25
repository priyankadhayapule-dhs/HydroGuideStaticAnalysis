package com.accessvascularinc.hydroguide.mma.ui;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.databinding.DataBindingUtil;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.Navigation;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.InstallUpdateBinding;
import com.accessvascularinc.hydroguide.mma.model.SoftwareUpdateSource;
import com.accessvascularinc.hydroguide.mma.ui.input.OnItemClickListener;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import java.util.List;

public class SoftwareUpdateInstall extends BaseHydroGuideFragment implements OnItemClickListener {

  private InstallUpdateBinding binding = null;
  @Override
  public void onItemClickListener(int position) {

  }

  @Override
  public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container,
                           final Bundle savedInstanceState) {
    binding = DataBindingUtil.inflate(inflater, R.layout.install_update, container, false);
    super.onCreateView(inflater, container, savedInstanceState);
    // Read SoftwareUpdateSource from arguments
    SoftwareUpdateSource updateSource = null;
    final Bundle args = getArguments();
    if (args != null) {
      final Object srcObj = args.getSerializable(getResources().getString(R.string.map_keys_update_src));
      if (srcObj instanceof SoftwareUpdateSource) {
        updateSource = (SoftwareUpdateSource) srcObj;
      }
    }
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    // Initialize ivUsbIcon to avoid NullPointerException in BaseHydroGuideFragment
    binding.homeTopBar.setTabletState(viewmodel.getTabletState());
    ivUsbIcon = binding.homeTopBar.usbIcon;
    tvBattery = binding.homeTopBar.controllerBatteryLevelPct;
    ivBattery = binding.homeTopBar.remoteBatteryLevelIndicator;
    ivTabletBattery = binding.homeTopBar.tabletBatteryIndicator.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.homeTopBar.tabletBatteryIndicator.tabletBatteryLevelPct;
    assert updateSource != null;
    if (updateSource.equals(SoftwareUpdateSource.ONLINE)) {
      binding.setTitle(getString(R.string.install_online));
    } else
      binding.setTitle(getString(R.string.searching_usb));

    binding.homeTopBar.controllerBatteryLabel.setVisibility(View.INVISIBLE);
    binding.homeTopBar.controllerBatteryLevelPct.setVisibility(View.INVISIBLE);
    binding.homeTopBar.remoteBatteryLevelIndicator.setVisibility(View.INVISIBLE);
    binding.homeTopBar.usbIcon.setVisibility(View.INVISIBLE);
    binding.backBtn.setOnClickListener(backButton_OnClickListener);

    binding.retryBtn.setOnClickListener(v -> {
      // Reset to loading state
      binding.retrySection.setVisibility(View.GONE);
      // Trigger your search again
      //performUpdateSearch();
    });

    binding.btnExtract.setOnClickListener(v -> {
      // Hide package view and show progress UI
      binding.updateFoundContainer.setVisibility(View.GONE);
      binding.extractingContainer.setVisibility(View.VISIBLE);

      // Simulate extraction progress
      new Thread(() -> {
        for (int i = 0; i <= 100; i += 2) {
          int progress = i;
          requireActivity().runOnUiThread(() -> binding.progressBar.setProgress(progress));
          try {
            Thread.sleep(100);
          } catch (InterruptedException ignored) {
          }
        }

        requireActivity().runOnUiThread(() -> {
          // binding.extractMessage.setText("Extraction complete!");
          showInstallPackages();
        });
      }).start();
    });

    binding.btnInstallApp.setOnClickListener(v -> startInstallation("App"));
    binding.btnInstallController.setOnClickListener(v -> startInstallation("Controller"));

    final View root = binding.getRoot();
    return root;
  }

  private final View.OnClickListener backButton_OnClickListener = v -> {
    onProcedureExitAttempt(() -> {
      Navigation.findNavController(requireView())
              .navigate(R.id.action_navigation_software_search_to_update);
    });
  };

  public void showNoUpdatesFound() {
    binding.retrySection.setVisibility(View.VISIBLE);
  }

  private void showInstallPackages() {
    binding.extractingContainer.setVisibility(View.GONE);
    binding.installPackagesContainer.setVisibility(View.VISIBLE);
  }

  private void startInstallation(String type) {
    // Hide list view and show install progress
    binding.installPackagesContainer.setVisibility(View.GONE);
    binding.installProgressContainer.setVisibility(View.VISIBLE);
    binding.installMessage.setText("Installing " + type + " updates...");

    new Thread(() -> {
      for (int i = 0; i <= 100; i += 2) {
        int progress = i;
        requireActivity().runOnUiThread(() -> binding.installProgressBar.setProgress(progress));
        try { Thread.sleep(80); } catch (InterruptedException ignored) {}
      }

      requireActivity().runOnUiThread(() -> {
        binding.installMessage.setText(type + " installation complete!");
        MedDevLog.audit("Software Update", type + " software update installed successfully");
        new Handler(Looper.getMainLooper()).postDelayed(() -> {
          // TODO: transition to next screen or show confirmation dialog
          binding.installProgressContainer.setVisibility(View.GONE);
          binding.installSuccessContainer.setVisibility(View.VISIBLE);
        }, 1000);
      });
    }).start();
  }

}
