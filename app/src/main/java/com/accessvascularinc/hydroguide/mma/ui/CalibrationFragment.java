package com.accessvascularinc.hydroguide.mma.ui;
import android.os.Bundle;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.Navigation;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.LinearLayout;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentCalibrationBinding;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
//import com.accessvascularinc.hydroguide.mma.viewmodel.MainViewModel;

/**
 * A simple {@link Fragment} subclass. Use the {@link CalibrationFragment#newInstance} factory
 * method to create an instance of this fragment.
 */
public class CalibrationFragment extends BaseHydroGuideFragment implements View.OnClickListener {

  private static final String ARG_PARAM1 = "param1";
  private static final String ARG_PARAM2 = "param2";
  private FragmentCalibrationBinding binding = null;
  private MainViewModel viewmodel;
  private String mParam1;
  private String mParam2;

  // Button references
  private Button step1OkBtn;
  private Button step2OkBtn;
  private Button step3OkBtn;
  private ImageButton resetBtn;
  private ImageButton backBtn;
  private ImageButton proceedBtn;
  private LinearLayout proceedLayout;

  // Layout references
  private LinearLayout step1Container;
  private LinearLayout step2Container;
  private LinearLayout step3Container;

  // State tracking
  private boolean step1Confirmed = false;
  private boolean step2Confirmed = false;
  private boolean step3Confirmed = false;

  public CalibrationFragment() {
    // Required empty public constructor
  }

  /**
   * Use this factory method to create a new instance of this fragment using the provided
   * parameters.
   *
   * @param param1 Parameter 1.
   * @param param2 Parameter 2.
   *
   * @return A new instance of fragment CalibrationFragment.
   */
  public static CalibrationFragment newInstance(String param1, String param2) {
    CalibrationFragment fragment = new CalibrationFragment();
    Bundle args = new Bundle();
    args.putString(ARG_PARAM1, param1);
    args.putString(ARG_PARAM2, param2);
    fragment.setArguments(args);
    return fragment;
  }

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    if (getArguments() != null) {
      mParam1 = getArguments().getString(ARG_PARAM1);
      mParam2 = getArguments().getString(ARG_PARAM2);
    }
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container,
                           Bundle savedInstanceState) {
    super.onCreateView(inflater, container, savedInstanceState);

    View view = inflater.inflate(R.layout.fragment_calibration, container, false);
    // Use data binding`
    binding = FragmentCalibrationBinding.inflate(inflater, container, false);

    tvBattery = binding.topBar.controllerBatteryLevelPct;
    ivBattery = binding.topBar.remoteBatteryLevelIndicator;
    ivUsbIcon = binding.topBar.usbIcon;
    ivTabletBattery = binding.topBar.tabletBatteryIndicator.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.topBar.tabletBatteryIndicator.tabletBatteryLevelPct;
    hgLogo = binding.topBar.appLogo;
    lpTabletIcon = binding.topBar.tabletBatteryWarningIcon.batteryIcon;
    binding.step1OkBtn.setBackgroundResource(R.drawable.av_btn);
    binding.step1OkBtn.setBackgroundTintList(null);
    binding.step1OkBtn.setPadding(30, 3, 30, 3);
    binding.step1OkBtn.setTextSize(16);
    binding.step1OkBtn.setAllCaps(false);

    binding.step2OkBtn.setBackgroundResource(R.drawable.av_btn);
    binding.step2OkBtn.setBackgroundTintList(null);
    binding.step1OkBtn.setPadding(30, 3, 30, 3);
    binding.step2OkBtn.setTextSize(16);
    binding.step2OkBtn.setAllCaps(false);

    binding.step3OkBtn.setBackgroundResource(R.drawable.av_btn);
    binding.step3OkBtn.setBackgroundTintList(null);
    binding.step1OkBtn.setPadding(30, 3, 30, 3);
    binding.step3OkBtn.setTextSize(16);
    binding.step3OkBtn.setAllCaps(false);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);

    binding.topBar.setEncounterData(viewmodel.getEncounterData());
    binding.topBar.setMonitorData(viewmodel.getMonitorData());
    binding.topBar.setControllerState(viewmodel.getControllerState());
    binding.topBar.setTabletState(viewmodel.getTabletState());
    view = binding.getRoot();
    initializeViews(view);
    return view;
  }

  /**
   * Initialize and setup button click listeners
   */
  private void initializeViews(View view) {
    step1OkBtn = view.findViewById(R.id.step1_ok_btn);
    step2OkBtn = view.findViewById(R.id.step2_ok_btn);
    step3OkBtn = view.findViewById(R.id.step3_ok_btn);
    resetBtn = view.findViewById(R.id.reset_btn);
    backBtn = view.findViewById(R.id.back_btn);
    proceedBtn = view.findViewById(R.id.proceed_btn);
    proceedLayout = view.findViewById(R.id.proceed_layout);

    step1Container = view.findViewById(R.id.step1_container);
    step2Container = view.findViewById(R.id.step2_container);
    step3Container = view.findViewById(R.id.step3_container);

    step1OkBtn.setOnClickListener(this);
    step2OkBtn.setOnClickListener(this);
    step3OkBtn.setOnClickListener(this);
    resetBtn.setOnClickListener(this);
    backBtn.setOnClickListener(this);
    proceedBtn.setOnClickListener(this);
  }

  @Override
  public void onClick(View v) {
    if (v.getId() == R.id.step1_ok_btn) {
      onStep1Confirmed();
    } else if (v.getId() == R.id.step2_ok_btn) {
      onStep2Confirmed();
    } else if (v.getId() == R.id.step3_ok_btn) {
      onStep3Confirmed();
    } else if (v.getId() == R.id.reset_btn) {
      showResetConfirmationDialog();
    } else if (v.getId() == R.id.back_btn) {     
      TryNavigateToUltrasound(Utility.FROM_SCREEN_SETUP,true,R.id.action_navigation_patient_input_to_setup_screen);
    } else if (v.getId() == R.id.proceed_btn) {
      Navigation.findNavController(v).navigate(R.id.action_navigation_calibration_to_procedure);
    }
  }

  /**
   * Check if all steps are confirmed and show proceed button
   */
  private void checkAllStepsConfirmed() {
    if (step1Confirmed && step2Confirmed && step3Confirmed) {
      proceedBtn.setVisibility(View.VISIBLE);
      // Also show the proceed text label
      LinearLayout proceedLayout = (LinearLayout) proceedBtn.getParent();
      for (int i = 0; i < proceedLayout.getChildCount(); i++) {
        View child = proceedLayout.getChildAt(i);
        if (child instanceof android.widget.TextView) {
          child.setVisibility(View.VISIBLE);
        }
      }
    }
  }
  private void showResetConfirmationDialog() {
    ConfirmDialog.show(requireContext(), "Are you sure you want to reset calibration?", confirmed -> {
      if (confirmed) {
        resetCalibration();
      }
    });
  }

  /**
   * Reset all calibration steps
   */
  private void resetCalibration() {
    step1OkBtn.setText("OK");
    step2OkBtn.setText("OK");
    step3OkBtn.setText("OK");

    step1OkBtn.setBackgroundResource(R.drawable.av_btn);
    step1OkBtn.setBackgroundTintList(null);
    step1OkBtn.setPadding(48, 8, 48, 8);
    step2OkBtn.setBackgroundResource(R.drawable.av_btn);
    step2OkBtn.setBackgroundTintList(null);
    step2OkBtn.setPadding(48, 8, 48, 8);
    step3OkBtn.setBackgroundResource(R.drawable.av_btn);
    step3OkBtn.setBackgroundTintList(null);
    step3OkBtn.setPadding(48, 8, 48, 8);

    step1OkBtn.setTextColor(getResources().getColor(android.R.color.white));
    step2OkBtn.setTextColor((getResources().getColor(android.R.color.white)));
    step3OkBtn.setTextColor((getResources().getColor(android.R.color.white)));
    step1OkBtn.setEnabled(true);
    step2OkBtn.setEnabled(true);
    step3OkBtn.setEnabled(true);

    step1Confirmed = false;
    step2Confirmed = false;
    step3Confirmed = false;

    step2Container.setVisibility(LinearLayout.GONE);
    step3Container.setVisibility(LinearLayout.GONE);
    
    // Hide proceed button elements
    proceedBtn.setVisibility(View.INVISIBLE);
    LinearLayout proceedLayout = (LinearLayout) proceedBtn.getParent();
    for (int i = 0; i < proceedLayout.getChildCount(); i++) {
      View child = proceedLayout.getChildAt(i);
      if (child instanceof android.widget.TextView) {
        child.setVisibility(View.INVISIBLE);
      }
    }
  }

  /**
   * Handle Step 1 confirmation
   */
  private void onStep1Confirmed() {
    if (!step1Confirmed) {
      step1Confirmed = true;
      step1OkBtn.setText("✓ Confirmed");
      step1OkBtn.setBackgroundColor(getResources().getColor(android.R.color.holo_green_light));
      step1OkBtn.setTextColor(getResources().getColor(android.R.color.black));
      step1OkBtn.setEnabled(false);
      step2Container.setVisibility(LinearLayout.VISIBLE);
    }
  }

  /**
   * Handle Step 2 confirmation
   */
  private void onStep2Confirmed() {
    if (!step2Confirmed) {
      step2Confirmed = true;
      step2OkBtn.setText("✓ Confirmed");
      step2OkBtn.setBackgroundColor(getResources().getColor(android.R.color.holo_green_light));
      step2OkBtn.setTextColor(getResources().getColor(android.R.color.black));
      step2OkBtn.setEnabled(false);
      step3Container.setVisibility(LinearLayout.VISIBLE);
    }
  }

  /**
   * Handle Step 3 confirmation
   */
  private void onStep3Confirmed() {
    if (!step3Confirmed) {
      step3Confirmed = true;
      step3OkBtn.setText("✓ Confirmed");
      step3OkBtn.setBackgroundColor(getResources().getColor(android.R.color.holo_green_light));
      step3OkBtn.setTextColor(getResources().getColor(android.R.color.black));
      step3OkBtn.setEnabled(false);
      checkAllStepsConfirmed();
    }
  }
}