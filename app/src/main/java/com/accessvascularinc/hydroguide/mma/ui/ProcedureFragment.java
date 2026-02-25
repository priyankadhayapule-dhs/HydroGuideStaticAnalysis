package com.accessvascularinc.hydroguide.mma.ui;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.drawable.Drawable;
import android.media.MediaScannerConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.LinearInterpolator;
import android.view.inputmethod.BaseInputConnection;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.databinding.Observable;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.Navigation;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.accessvascularinc.hydroguide.mma.BR;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.AlertContentBinding;
import com.accessvascularinc.hydroguide.mma.databinding.CaptureLayoutBinding;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentProcedureBinding;
import com.accessvascularinc.hydroguide.mma.databinding.MultiAlertContentBinding;
import com.accessvascularinc.hydroguide.mma.databinding.ProcedureBottomBarBinding;
import com.accessvascularinc.hydroguide.mma.model.AlarmEvent;
import com.accessvascularinc.hydroguide.mma.model.Capture;
import com.accessvascularinc.hydroguide.mma.model.ChartConfig;
import com.accessvascularinc.hydroguide.mma.model.DataFiles;
import com.accessvascularinc.hydroguide.mma.model.EncounterData;
import com.accessvascularinc.hydroguide.mma.model.EncounterState;
import com.accessvascularinc.hydroguide.mma.model.Facility;
import com.accessvascularinc.hydroguide.mma.model.MockDataState;
import com.accessvascularinc.hydroguide.mma.model.MonitorData;
import com.accessvascularinc.hydroguide.mma.model.PeakLabel;
import com.accessvascularinc.hydroguide.mma.model.ProfileState;
import com.accessvascularinc.hydroguide.mma.model.TabletState;
import com.accessvascularinc.hydroguide.mma.model.UserPreferences;
import com.accessvascularinc.hydroguide.mma.model.UserProfile;
import com.accessvascularinc.hydroguide.mma.serial.Button;
import com.accessvascularinc.hydroguide.mma.serial.ButtonState;
import com.accessvascularinc.hydroguide.mma.serial.ControllerStatus;
import com.accessvascularinc.hydroguide.mma.serial.EcgSample;
import com.accessvascularinc.hydroguide.mma.serial.Heartbeat;
import com.accessvascularinc.hydroguide.mma.serial.HeartbeatEventStates;
import com.accessvascularinc.hydroguide.mma.serial.ImpedanceSample;
import com.accessvascularinc.hydroguide.mma.serial.flags.EcgAnalysisStatusFlags;
import com.accessvascularinc.hydroguide.mma.ui.captures.CaptureDialogFragment;
import com.accessvascularinc.hydroguide.mma.ui.captures.IntravCaptureListAdapter;
import com.accessvascularinc.hydroguide.mma.ui.input.InputUtils;
import com.accessvascularinc.hydroguide.mma.ui.plot.EcgDataSeries;
import com.accessvascularinc.hydroguide.mma.ui.plot.FadeRenderer;
import com.accessvascularinc.hydroguide.mma.ui.plot.ImpedanceDataSeries;
import com.accessvascularinc.hydroguide.mma.ui.plot.PlotStyle;
import com.accessvascularinc.hydroguide.mma.ui.plot.PlotUtils;
import com.accessvascularinc.hydroguide.mma.ui.settings.BrightnessChangeFragment;
import com.accessvascularinc.hydroguide.mma.ui.settings.TimeScaleModeChangeFragment;
import com.accessvascularinc.hydroguide.mma.ui.settings.VerticalOffsetValueChangeFragment;
import com.accessvascularinc.hydroguide.mma.ui.settings.VerticalScaleValueChangeFragment;
import com.accessvascularinc.hydroguide.mma.ui.settings.VolumeChangeFragment;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.androidplot.Plot;
import com.androidplot.util.PixelUtils;
import com.androidplot.util.Redrawer;
import com.androidplot.xy.BarFormatter;
import com.androidplot.xy.BoundaryMode;
import com.androidplot.xy.RectRegion;
import com.androidplot.xy.SimpleXYSeries;
import com.androidplot.xy.StepMode;
import com.androidplot.xy.XYPlot;
import com.accessvascularinc.hydroguide.mma.ui.plot.FadeFormatter;

import org.json.JSONException;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.Objects;
import java.util.PriorityQueue;

/**
 * These comments are from the ECG demo but are helpful for understanding the
 * data plotting setup. -
 * An example of a real-time plot displaying an asynchronously updated model of
 * ECG data. There are
 * three key items to pay attention to here: 1 - The model data is updated
 * independently of all
 * other data via a background thread. This is typical of most signal inputs.
 * <p>
 * 2 - The main render loop is controlled by a separate thread governed by an
 * instance of
 * {@link Redrawer}. The alternative is to try synchronously invoking
 * {@link Plot#redraw()} within
 * whatever system is updating the model, which would severely degrade
 * performance.
 * <p>
 * 3 - The plot is set to render using a background thread via config attr in
 * R.layout.ecg_example.xml. This ensures that the rest of the app will remain
 * responsive during
 * rendering.
 */

public class ProcedureFragment extends BaseHydroGuideFragment implements PlotUtils.SwipeListener {
  public static final int BPM_DEFAULT = 70;
  public static final int BPM_MIN = 30;
  public static final int BPM_MAX = 160;
  private boolean drawCatheterDots = true;
  private boolean wrongTurn = false;

  private FragmentProcedureBinding binding = null;
  private Boolean wasExternalCapturePresentOnFirstClick = null;
  protected MockDataState mockDataState = MockDataState.None;
  private boolean plotMockData = false;
  private TextView plotTitle = null, txtCaptureInstruction = null, tvMockPhase = null;
  private ImpedanceDataSeries impedanceData = null;
  private EcgDataSeries externalData = null, intravascularData = null, extCapSeries = null;
  private SimpleXYSeries gExternal = null, gCurrent = null, gLastCap = null, gMax = null;
  // Active graph pointer (mvGraph) plus explicit external & intravascular graphs
  private XYPlot mvGraph = null; // points to currently selected graph for capture context
  private XYPlot extGraph = null, intraGraph = null, extCapture = null;
  private RecyclerView intravCapsListView = null;
  private IntravCaptureListAdapter intravCapsListAdapter = null;
  private ImageButton graphToggleBtn = null;
  private LinearLayout llCaptureInstruction = null;
  private Double[] mockExternalEcgData = null, mockIntravEcgData = null;
  private PeakLabel[] mockExternalPeaksData = null, mockIntravPeaksData = null;
  private int mockIdx = 0;
  private BaseInputConnection inputConnection = null;
  /**
   * Uses a separate thread to modulate redraw frequency.
   */
  // Separate redrawers for each live plot
  private Redrawer externalGraphRedrawer = null, intravascularGraphRedrawer = null;
  private Thread mockDataThread = null;
  private CaptureDialogFragment captureDialog = null;
  private boolean showIntravascular = false;
  private boolean keepRunningMock = false;
  private boolean timeScaleAutoContEnabled = false;
  private boolean freezeCaptures = false;
  private boolean multiContentEnabled = false;
  private boolean leadsConnected = false;
  private Number gCaptureStore = 0.0, gMaxStore = 0.0;
  private boolean gCurrentActive = false, gExternalActive = false;
  private boolean useExposedLength = false;
  // Tracks whether ECG streaming has been requested (started) for this fragment's
  // lifecycle.
  private boolean ecgStreamingActive = false;
  // Tracks whether we've already shown the reconnection prompt to avoid showing it multiple times
  private boolean hasShownReconnectionPrompt = false;
  // Tracks whether we're waiting for user decision on resume/terminate to prevent auto-plotting
  private boolean waitingForUserDecision = false;

  private final View.OnClickListener graphToggleBtn_OnClickListener = v -> toggleGraph();
  private final View.OnClickListener demoCaptureBtn_OnClickListener = v -> toggleCapture();
  private final View.OnClickListener cycleMockDataBtn_OnClickListener = v -> cycleMockData();
  private final View.OnClickListener navToPatientSummary_OnClickListener = v -> Navigation
      .findNavController(requireView())
      .navigate(R.id.action_navigation_procedure_to_summary);
  private final View.OnClickListener volumeBtn_OnClickListener = v -> {
    VolumeChangeFragment bottomSheet = new VolumeChangeFragment();
    bottomSheet.show(getChildFragmentManager(), "TEST");
  };
  private final View.OnClickListener brightnessBtn_OnClickListener = v -> {
    BrightnessChangeFragment bottomSheet = new BrightnessChangeFragment();
    bottomSheet.show(getChildFragmentManager(), "TEST");
  };
  private final View.OnClickListener vertOffsetBtn_OnClickListener = v -> {
    VerticalOffsetValueChangeFragment bottomSheet = new VerticalOffsetValueChangeFragment();
    bottomSheet.show(getChildFragmentManager(), "TEST");
  };
  private final View.OnClickListener vertScaleBtn_OnClickListener = v -> {
    VerticalScaleValueChangeFragment bottomSheet = new VerticalScaleValueChangeFragment();
    bottomSheet.show(getChildFragmentManager(), "TEST");
  };
  private final View.OnClickListener timeScaleBtn_OnClickListener = v -> {
    TimeScaleModeChangeFragment bottomSheet = new TimeScaleModeChangeFragment();
    bottomSheet.show(getChildFragmentManager(), "TEST");
  };
  private final View.OnClickListener navToInput_OnClickListener = v -> {
    Navigation.findNavController(requireView())
        .navigate(R.id.action_navigation_procedure_to_patient_input);
    // cancelProcedure(); // We dont want to delete procedure data.
  };

  public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container,
      final Bundle savedInstanceState) {
    super.onCreateView(inflater, container, savedInstanceState);
    setupAlarmArrays();
    MedDevLog.info("Navigation", "Procedure screen entered");
    binding = FragmentProcedureBinding.inflate(inflater, container, false);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    requireActivity().getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    impedanceData = new ImpedanceDataSeries();

    updateLatestCaptureFromAllCaptures();
    setupVolumeAndBrightnessListeners();
    Log.d("ABC", "3: ");
    binding.procedureTopBar.setEncounterData(viewmodel.getEncounterData());
    binding.procedureTopBar.setMonitorData(viewmodel.getMonitorData());
    binding.procedureTopBar.setControllerState(viewmodel.getControllerState());
    binding.procedureTopBar.setTabletState(viewmodel.getTabletState());
    Log.d("ABC", "5: ");
    binding.procedureContent.setEncounterData(viewmodel.getEncounterData());
    binding.procedureContent.setMonitorData(viewmodel.getMonitorData());
    binding.procedureContent.setChartConfig(viewmodel.getChartConfig());

    binding.procedureBottomBar.setMonitorData(viewmodel.getMonitorData());
    binding.procedureBottomBar.setChartConfig(viewmodel.getChartConfig());
    binding.procedureBottomBar.setTabletState(viewmodel.getTabletState());

    tvTestUsbText = binding.procedureContent.pwaveGraphs.testUsbText;
    tvBattery = binding.procedureTopBar.controllerBatteryLevelPct;
    ivBattery = binding.procedureTopBar.remoteBatteryLevelIndicator;
    ivUsbIcon = binding.procedureTopBar.usbIcon;
    ivTabletBattery = binding.procedureTopBar.tabletBatteryIndicator.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.procedureTopBar.tabletBatteryIndicator.tabletBatteryLevelPct;
    lpTabletIcon = binding.procedureTopBar.tabletBatteryWarningIcon.batteryIcon;
    hgLogo = binding.procedureTopBar.appLogo;
    lpTabletSymbol = binding.procedureTopBar.tabletBatteryWarningIcon.batteryWarningImg;

    // Setting mute button
    updateMuteBtnDrawable();

    viewmodel.getChartConfig().addOnPropertyChangedCallback(chartConfigChanged_Listener);
    viewmodel.getEncounterData().addOnPropertyChangedCallback(encounterDataChanged_Listener);
    viewmodel.getMonitorData().addOnPropertyChangedCallback(monitorDataChanged_Listener);
    viewmodel.getTabletState().addOnPropertyChangedCallback(tabletStateChanged_Listener);
    initBottomButtons();
    initLivePlots();
    initCaptures();
    updateMuteBtnDrawable();
    updateAllRangeBoundaries();

    gCaptureStore = viewmodel.getMonitorData().getCapturedPWaveAmplitudes().intravascular_mV;
    gMaxStore = viewmodel.getMonitorData().getMaxPWaveAmplitudes().intravascular_mV;

    //TODO: What is this? It has been here since 5/19/2025, Sparks? (Jack Cochran). Should this Mock fn call be removed? Is it doing anything today (i.e. giving us mock data that we think is real???) 20260204 MG
    // Uncomment to use mock data generator from original ecg demo
    initMockData();
    writeErrorDisplay(false);

    leadsConnected = false;
    if (viewmodel.getControllerCommunicationManager().checkConnection() || keepRunningMock) {
      startProcedure();
    }

    if (viewmodel.getEncounterData().isLive()) {
      checkStatePermissions();
      updateErrorDisplay();
      if (!keepRunningMock) {
       updateTabletChargingProcedureAlert();
      }
    }

    if (binding != null && binding.procedureContent != null) {
      if (binding.procedureContent.latestCapture != null) {
        View latestCaptureCardView = binding.procedureContent.latestCapture.getRoot();
        latestCaptureCardView.setOnClickListener(v -> onLatestCaptureClicked());
      }
    }
    return binding.getRoot();
  }

  private void onLatestCaptureClicked() {
    // Show all captures in a fullscreen dialog
    Capture[] allCaptures = viewmodel.getEncounterData().getIntravCaptures();
    // Optionally, add the external capture as the first item if needed:
    Capture externalCapture = viewmodel.getEncounterData().getExternalCapture();
    if (externalCapture != null) {
      Capture[] withExternal = new Capture[allCaptures.length + 1];
      withExternal[0] = externalCapture;
      System.arraycopy(allCaptures, 0, withExternal, 1, allCaptures.length);
      allCaptures = withExternal;
    }
    // Log all captures before sending to dialog
    android.util.Log.d("ProcedureFragment",
        "onLatestCaptureClicked: allCaptures.length=" +
            (allCaptures != null ? allCaptures.length : -1));
    if (allCaptures != null) {
      for (int i = 0; i < allCaptures.length; i++) {
        Capture c = allCaptures[i];
        android.util.Log
            .d("ProcedureFragment",
                "onLatestCaptureClicked: capture[" + i + "]="
                    + (c != null ? ("id=" + c.getCaptureId() + ", isIntravascular=" +
                        c.getIsIntravascular()
                        + ", insertedLength=" + c.getInsertedLengthCm() +
                        ", exposedLength=" + c.getExposedLengthCm())
                        : "null"));
      }
    }

    // Get currently selected capture if it exists
    Capture currentlySelected = viewmodel.getEncounterData().getSelectedCapture();
    android.util.Log.d("ProcedureFragment",
        "onLatestCaptureClicked: currentlySelected="
            + (currentlySelected != null ? currentlySelected.getCaptureId() : "null"));

    com.accessvascularinc.hydroguide.mma.ui.captures.AllCapturesDialogFragment.newInstance(
        allCaptures, currentlySelected)
        .show(getChildFragmentManager(), "AllCapturesDialog");
  }

  @Override
  public void onViewCreated(@NonNull final View view, @Nullable final Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    View latestCapture = view.findViewById(R.id.latest_capture);
    View procedureImage = view.findViewById(R.id.procedure_internal_route_image);
    if (procedureImage instanceof ImageView) {
      ImageView imageView = (ImageView) procedureImage;
      final Handler handler = new Handler(Looper.getMainLooper());

      final Drawable baseDrawable = imageView.getDrawable();

      if (impedanceData != null && drawCatheterDots) {
        Runnable drawDotRunnable = new Runnable() {
          @Override
          public void run() {
            // Log.d("ProcedureFragment", "Running catheter position rendering with value
            // count: "
            // + catheterPositionValues.length);

            // Update bioz average in UI TextView for pre-comp testing 2/4/2026
            if(viewmodel != null && viewmodel.getEncounterData() != null
              && viewmodel.getEncounterData().getImpedanceData() != null
              && binding != null
              && binding.procedureContent != null
              && binding.procedureContent.biozdataAverageDebug != null
            ) {
              long avgRA = viewmodel.getEncounterData().getImpedanceData().getAverageRA();
              long avgLA = viewmodel.getEncounterData().getImpedanceData().getAverageLA();
              long avgLL = viewmodel.getEncounterData().getImpedanceData().getAverageLL();

              binding.procedureContent.biozdataAverageDebug.setText(
                      String.format(Locale.US, "BioZ: %d | %d | %d",
                              avgRA,
                              avgLA,
                              avgLL)
              );

//            binding.procedureContent.biozdataAverageDebug.setText(
//              String.format(Locale.US, "BioZ: %s | %s | %s",
//                      Long.toUnsignedString(avgRA),
//                      Long.toUnsignedString(avgLA),
//                      Long.toUnsignedString(avgLL))
//            );
            }

            Bitmap baseBitmap = Bitmap.createBitmap(
                baseDrawable.getIntrinsicWidth(),
                baseDrawable.getIntrinsicHeight(),
                Bitmap.Config.ARGB_8888);
            Canvas baseCanvas = new Canvas(baseBitmap);

            int imageXMin = 0;
            int imageXMax = baseCanvas.getWidth();
            int imageYMin = 0;
            int imageYMax = baseCanvas.getHeight();

            // TODO: These are empirically determined values based on our initial realistic
            // test data from Todd. Need a more universal approach. 20260107 MG
            // Focus bounds to vessel area, assuming left arm
            // final int imageXMinFinal = imageXMin + 95;
            // final int imageXMaxFinal = imageXMax - 200;
            // final int imageYMinFinal = imageYMin + 100;
            // final int imageYMaxFinal = imageYMax - 250;
            final int imageXMinFinal = imageXMin + 300;
            final int imageXMaxFinal = imageXMax - 0;
            final int imageYMinFinal = imageYMin + 120;
            final int imageYMaxFinal = imageYMax - 250;

            // TODO: Data boundaries shouldn't be hardcoded, but how do we get these? Should
            // come from calibration work which is yet to be defined. 20251217 MG
            // Initial realistic dummy test data copied directly from Todd's C code header
            final int algorithmXMin = -15124;
            final int algorithmXMax = 9101;
            final int algorithmYMin = -19651;
            final int algorithmYMax = -10566;

            baseDrawable.setBounds(imageXMin, imageYMin, imageXMax, imageYMax);
            baseDrawable.draw(baseCanvas);

            Bitmap overlayBitmap = baseBitmap.copy(Bitmap.Config.ARGB_8888, true);
            Canvas canvas = new Canvas(overlayBitmap);
            Paint paint = new Paint();
            paint.setColor(wrongTurn ? Color.RED : Color.GREEN);
            paint.setAntiAlias(true);

            // Keep track of mins/maxs for mapping
            // TODO: Need a more universal approach
            int inputXMin = 1000;
            int inputXMax = -1000;
            int inputYMin = 1000;
            int inputYMax = -1000;

            int x, y;

            // Draw all points
            for (int i = 0; i < impedanceData.size(); i++) {

              x = impedanceData.getX(i).intValue();
              y = impedanceData.getY(i).intValue();

              if (inputXMin > x) {
                inputXMin = x;
              }
              if (inputXMax < x) {
                inputXMax = x;
              }
              if (inputYMin > y) {
                inputYMin = y;
              }
              if (inputYMax < y) {
                inputYMax = y;
              }

              // Map to image coordinates
              x = imageXMinFinal
                  + (x - algorithmXMin) * (imageXMaxFinal - imageXMinFinal) /
                      (algorithmXMax - algorithmXMin);
              y = imageYMaxFinal
                  - (y - algorithmYMin) * (imageYMaxFinal - imageYMinFinal) /
                      (algorithmYMax - algorithmYMin);

              canvas.drawCircle(x, y, 5, paint);
            }

            imageView.setImageBitmap(overlayBitmap);
            handler.postDelayed(this, 300);
          }
        };
        imageView.post(drawDotRunnable);
      }
    }

    if (latestCapture != null &&
        latestCapture.getLayoutParams() instanceof RelativeLayout.LayoutParams) {
      RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) latestCapture.getLayoutParams();
      // Remove any conflicting rules
      params.removeRule(RelativeLayout.BELOW);
      // Force to bottom of parent
      params.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
      latestCapture.setLayoutParams(params);
    }
    // Force external_capture to be hidden
    View externalCapture = view.findViewById(R.id.external_capture);
    if (externalCapture != null) {
      externalCapture.setVisibility(View.GONE);
      externalCapture.setEnabled(false);
      externalCapture.setAlpha(0f);
    }
    View noCapturePlaceholder = view.findViewById(R.id.no_capture_placeholder);
    EncounterData encounterData = null;
    if (viewmodel != null) {
      encounterData = viewmodel.getEncounterData();
    }
    boolean hasCapture = (encounterData != null && encounterData.getLatestCapture() != null);
    if (hasCapture) {
      RelativeLayout parent = (RelativeLayout) binding.procedureContent.getRoot();
      View latestCaptureView = parent.findViewById(R.id.latest_capture);
      if (latestCapture != null) {
        latestCapture.setVisibility(View.VISIBLE);
      }
      if (noCapturePlaceholder != null) {
        noCapturePlaceholder.setVisibility(View.GONE);
        noCapturePlaceholder.setEnabled(false);
        noCapturePlaceholder.setAlpha(0f);
      }
      if (latestCaptureView != null) {
        latestCaptureView.setOnClickListener(v -> onLatestCaptureClicked());
      }
    } else {
      if (latestCapture != null) {
        latestCapture.setVisibility(View.GONE);
      }
      if (noCapturePlaceholder != null) {
        noCapturePlaceholder.setVisibility(View.VISIBLE);
        noCapturePlaceholder.setEnabled(true);
        noCapturePlaceholder.setAlpha(1f);
      }
    }
    inputConnection = new BaseInputConnection(view, true);
    checkStatePermissions();
  }

  @Override
  public void onResume() {
    super.onResume();
    // Don't auto-start plotting if we're waiting for user to choose Resume/Terminate
    if (!waitingForUserDecision) {
      wipeDataPlotting();
      resumeDataPlotting();
    }
    // Always update latest capture and rebuild card/graph after resume
    updateLatestCaptureFromAllCaptures();
    initCaptures();
    Log.d("ProcedureFragment", "onResume called");
    // Restore pink and blue lines if present, without duplication
    if (intraGraph != null && intravascularData != null && !viewmodel.getMockCycle()) {
      final var intraFormatter = (FadeFormatter) intraGraph.getRenderer(FadeRenderer.class)
          .getFormatter(intravascularData);
      if (intraFormatter != null) {
        // Restore pink line (last capture)
        final EncounterData encounter = viewmodel.getEncounterData();
        final Capture[] intravCaptures = encounter.getIntravCaptures();
        if (intravCaptures.length > 0) {
          final Capture lastCapture = intravCaptures[intravCaptures.length - 1];
          final int lastPMarkIdx = lastCapture.getCaptureData().getLatestMarkedPointIndex();
          final Number lastCapPMarkVal = lastCapture.getCaptureData().getMarker(lastPMarkIdx);
          if (lastCapPMarkVal != null) {
            intraFormatter.setCapturedPWaveAmplitude(lastCapPMarkVal);
            intraFormatter.setShowHorizontalLines(true);
          } else {
            intraFormatter.setCapturedPWaveAmplitude(null);
          }
        } else {
          intraFormatter.setCapturedPWaveAmplitude(null);
        }
        // Restore blue line (max amplitude)
        final MonitorData monitorData = viewmodel.getMonitorData();
        final var latestMax = monitorData.getMaxPWaveAmplitudes();
        if (latestMax != null && latestMax.intravascular_mV != 0.0) {
          intraFormatter.setMaxPWaveAmplitude((float) latestMax.intravascular_mV);
          intraFormatter.setShowHorizontalLines(true);
        } else {
          intraFormatter.setMaxPWaveAmplitude(null);
        }
      }
    }
    // Start ECG streaming upon becoming visible/resumed if connected (but not if waiting for user decision)
    if (!waitingForUserDecision && viewmodel != null && viewmodel.getControllerCommunicationManager() != null &&
        viewmodel.getControllerCommunicationManager().checkConnection() &&
        !ecgStreamingActive) {
      viewmodel.getControllerCommunicationManager().sendECGCommand(true);
      ecgStreamingActive = true;
    }
    if (syncManager != null) {
      syncManager.setBlockSync(true);
    }
  }

  /**
   * Sets the latest capture in EncounterData by combining external and
   * intravascular captures. If
   * no captures exist, sets latestCapture to null (shows no_capture_placeholder).
   * Call this after
   * resuming from HomeScreen.
   */
  private void updateLatestCaptureFromAllCaptures() {
    if (viewmodel == null || viewmodel.getEncounterData() == null) {
      return;
    }
    EncounterData encounterData = viewmodel.getEncounterData();
    List<Capture> allCaptures = new ArrayList<>();
    Capture externalCapture = encounterData.getExternalCapture();
    Capture[] intravCaptures = encounterData.getIntravCaptures();
    if (externalCapture != null && externalCapture.getCaptureData() != null &&
        externalCapture.getCaptureData().getData() != null &&
        externalCapture.getCaptureData().getData().length > 0) {
      allCaptures.add(externalCapture);
    }
    if (intravCaptures != null && intravCaptures.length > 0) {
      for (Capture c : intravCaptures) {
        if (c != null && c.getCaptureData() != null &&
            c.getCaptureData().getData() != null &&
            c.getCaptureData().getData().length > 0) {
          allCaptures.add(c);
        }
      }
    }
    Capture latestCapture = null;
    if (!allCaptures.isEmpty()) {
      latestCapture = allCaptures.get(allCaptures.size() - 1);
    }
    encounterData.setLatestCapture(latestCapture);
    android.util.Log.d("ProcedureFragment", "updateLatestCaptureFromAllCaptures: latestCapture="
        + (latestCapture != null ? latestCapture.toString() : "null"));
  }

  /**
   * Sets the latest capture in EncounterData by combining external and
   * intravascular captures. If
   * no captures exist, sets latestCapture to null (shows no_capture_placeholder).
   * Call this after
   * resuming from HomeScreen.
   */

  @Override
  public void onPause() {
    if (syncManager != null) {
      syncManager.setBlockSync(false);
    }
    // Stop ECG streaming when navigating away from this fragment
    if (viewmodel != null && viewmodel.getControllerCommunicationManager() != null &&
        viewmodel.getControllerCommunicationManager().checkConnection()) {
      viewmodel.getControllerCommunicationManager().sendECGCommand(false);
    }
    ecgStreamingActive = false;
    pauseDataPlotting();
    // Clear blue and pink overlays (horizontal lines) if present
    if (externalGraphRedrawer != null && intravascularGraphRedrawer != null) {
      final var intraFormatter = (com.accessvascularinc.hydroguide.mma.ui.plot.FadeFormatter) intraGraph
          .getRenderer(com.accessvascularinc.hydroguide.mma.ui.plot.FadeRenderer.class)
          .getFormatter(intravascularData);
      if (intraFormatter != null) {
        intraFormatter.setCapturedPWaveAmplitude(null); // pink
        intraFormatter.setMaxPWaveAmplitude(null); // blue
        intraFormatter.setShowHorizontalLines(false);
        intraGraph.redraw();
      }
    }
    super.onPause();
  }

  @Override
  public void onDestroyView() {
    if (externalGraphRedrawer != null) {
      externalGraphRedrawer.finish();
    }
    if (intravascularGraphRedrawer != null) {
      intravascularGraphRedrawer.finish();
    }
    viewmodel.getChartConfig().removeOnPropertyChangedCallback(chartConfigChanged_Listener);
    viewmodel.getEncounterData().removeOnPropertyChangedCallback(encounterDataChanged_Listener);
    viewmodel.getMonitorData().removeOnPropertyChangedCallback(monitorDataChanged_Listener);
    viewmodel.getTabletState().removeOnPropertyChangedCallback(tabletStateChanged_Listener);
    super.onDestroyView();
    binding = null;
  }

  private TextView intraPlotTitle = null; // second plot title

  private void initLivePlots() {
    MedDevLog.info("Background", "Initialized live plots");
    plotTitle = binding.procedureContent.pwaveGraphs.liveGraphTitle; // External title
    intraPlotTitle = binding.procedureContent.pwaveGraphs.intraLiveGraphTitle; // Intravascular title
    plotTitle.setText(getString(R.string.external_label));
    intraPlotTitle.setText(getString(R.string.intravascular_label));

    // Allow choosing active graph by tapping title
    plotTitle.setOnClickListener(v -> {
      showIntravascular = false;
      selectActiveGraph();
    });
    intraPlotTitle.setOnClickListener(v -> {
      showIntravascular = true;
      selectActiveGraph();
    });

    tvMockPhase = binding.procedureContent.pwaveGraphs.mockPhaseText;
    tvMockPhase.setText(String.format("Mock Case: %s", mockDataState.toString()));

    // initialize our XYPlot references:
    extGraph = binding.procedureContent.pwaveGraphs.liveGraph;
    intraGraph = binding.procedureContent.pwaveGraphs.intraLiveGraph;
    mvGraph = extGraph; // active defaults to external

    // Each point should be 5ms apart. Buffer of 10 every 50 ms.
    final int dataSize = Math.round(viewmodel.getChartConfig().getTimeScale() * PlotUtils.POINTS_PER_SECOND);

    externalData = new EcgDataSeries(dataSize);
    intravascularData = new EcgDataSeries(dataSize);

    gExternal = new SimpleXYSeries(SimpleXYSeries.ArrayFormat.Y_VALS_ONLY, "External", 0);
    gCurrent = new SimpleXYSeries(SimpleXYSeries.ArrayFormat.Y_VALS_ONLY, "Current", 0);
    gLastCap = new SimpleXYSeries(SimpleXYSeries.ArrayFormat.Y_VALS_ONLY, "Last Capture", 0);
    gMax = new SimpleXYSeries(SimpleXYSeries.ArrayFormat.Y_VALS_ONLY, "Maximum", 0);

    gCurrentActive = true;
    gExternalActive = true;

    // External graph setup
    extGraph.addSeries(externalData,
        PlotUtils.getFormatter(requireContext(), PlotStyle.External, dataSize));
    extGraph.setRangeBoundaries(PlotUtils.GRAPH_LOWER_BOUND_DEFAULT,
        PlotUtils.GRAPH_UPPER_BOUND_DEFAULT, BoundaryMode.FIXED);
    extGraph.setRangeStep(StepMode.INCREMENT_BY_VAL, PlotUtils.GRAPH_RANGE_INCREMENT_DEFAULT);
    extGraph.setDomainBoundaries(0, dataSize, BoundaryMode.FIXED);
    PlotUtils.stylePlot(extGraph);

    // Intravascular graph setup
    intraGraph.addSeries(intravascularData,
        PlotUtils.getFormatter(requireContext(), PlotStyle.Intravascular, dataSize));
    intraGraph.setRangeBoundaries(PlotUtils.GRAPH_LOWER_BOUND_DEFAULT,
        PlotUtils.GRAPH_UPPER_BOUND_DEFAULT, BoundaryMode.FIXED);
    intraGraph.setRangeStep(StepMode.INCREMENT_BY_VAL, PlotUtils.GRAPH_RANGE_INCREMENT_DEFAULT);
    intraGraph.setDomainBoundaries(0, dataSize, BoundaryMode.FIXED);
    PlotUtils.stylePlot(intraGraph);

    // Ensure intravascular horizontal (blue max) line is visible from start
    final var intraFormatterInit = (FadeFormatter) intraGraph.getRenderer(FadeRenderer.class)
        .getFormatter(intravascularData);
    if (intraFormatterInit != null) {
      intraFormatterInit.setShowHorizontalLines(true); // show blue line immediately
    }

    externalData.setSwipeListener(this);
    intravascularData.setSwipeListener(this);

    // Create Redrawers but don't auto-start if waiting for user decision on reconnect
    boolean shouldAutoStart = !waitingForUserDecision;
    externalGraphRedrawer = new Redrawer(extGraph, 60, shouldAutoStart);
    intravascularGraphRedrawer = new Redrawer(intraGraph, 60, shouldAutoStart);

    if (viewmodel.getEncounterData().getState() == EncounterState.Suspended) {
      if (binding != null) {
        hideGauges(true, true);
      }
      pauseDataPlotting();
    }
  }

  private void initCaptures() {
    MedDevLog.info("Background", "Initialized captures");
    android.util.Log.d("ProcedureFragment", "initCaptures: called");
    llCaptureInstruction = binding.procedureContent.intravCapturesContainer.intravInstruction;
    txtCaptureInstruction = binding.procedureContent.intravCapturesContainer.intravInstructionText;

    // Only update EncounterData.latestCapture for data binding; UI will update
    // automatically
    final Capture[] intravCaptures = viewmodel.getEncounterData().getIntravCaptures();
    final Capture externalCapture = viewmodel.getEncounterData().getExternalCapture();
    android.util.Log.d("ProcedureFragment",
        "initCaptures: externalCapture=" +
            (externalCapture != null ? externalCapture.toString() : "null"));
    android.util.Log.d("ProcedureFragment",
        "initCaptures: intravCaptures.length=" +
            (intravCaptures != null ? intravCaptures.length : -1));
    Capture latestCapture = null;
    java.util.List<Capture> allCaptures = new java.util.ArrayList<>();
    // Only add externalCapture if it has real data (not just non-null)
    if (externalCapture != null && externalCapture.getCaptureData() != null &&
        externalCapture.getCaptureData().getData() != null &&
        externalCapture.getCaptureData().getData().length > 0) {
      allCaptures.add(externalCapture);
    }
    if (intravCaptures != null && intravCaptures.length > 0) {
      java.util.Collections.addAll(allCaptures, intravCaptures);
    }
    if (!allCaptures.isEmpty()) {
      latestCapture = viewmodel.getEncounterData()
          .getLatestCapture();// allCaptures.get(allCaptures.size() - 1);
      if (latestCapture != null) {
        android.util.Log.d("ProcedureFragment", "initCaptures: Latest capture insertedLength=" +
            latestCapture.getInsertedLengthCm() + " isIntravascular=" +
            latestCapture.getIsIntravascular() + " captureId="
            + latestCapture.getCaptureId());
      } else {
        MedDevLog.warn("ProcedureFragment", "initCaptures: latestCapture is null after resume!");
      }
    }
    android.util.Log.d("ProcedureFragment",
        "Latest capture for card: " +
            (latestCapture != null ? latestCapture.toString() : "null"));
    viewmodel.getEncounterData().setLatestCapture(latestCapture);

    // Determine which capture to display: selected capture (if any) or latest
    // capture
    Capture captureToDisplay = null;
    Capture selectedCapture = viewmodel.getEncounterData().getSelectedCapture();
    android.util.Log.d("ProcedureFragment",
        "initCaptures: selectedCapture=" +
            (selectedCapture != null ? selectedCapture.getCaptureId() : "null") +
            ", latestCapture=" +
            (latestCapture != null ? latestCapture.getCaptureId() : "null"));
    if (selectedCapture != null) {
      captureToDisplay = selectedCapture;
      android.util.Log.d("ProcedureFragment",
          "[DISPLAY DECISION] Using SELECTED capture: captureId=" +
              captureToDisplay.getCaptureId() +
              ", isIntravascular=" + captureToDisplay.getIsIntravascular());
    } else {
      captureToDisplay = latestCapture;
      android.util.Log.d("ProcedureFragment",
          "[DISPLAY DECISION] No selection, using LATEST capture: captureId=" +
              (latestCapture != null ? latestCapture.getCaptureId() : "null") +
              ", isIntravascular=" +
              (latestCapture != null ? latestCapture.getIsIntravascular() : "N/A"));
    }

    // Bind latest_capture (capture_layout.xml) fields and plot in code
    if (binding != null && binding.procedureContent != null) {
      RelativeLayout parent = (RelativeLayout) binding.procedureContent.getRoot();
      View oldLatestCapture = parent.findViewById(R.id.latest_capture);
      int index = (oldLatestCapture != null) ? parent.indexOfChild(oldLatestCapture) : 0;
      if (oldLatestCapture != null) {
        // Remove any plot series from the old plot to avoid overlap
        XYPlot oldPlot = oldLatestCapture.findViewById(R.id.capture_plot_graph);
        if (oldPlot != null) {
          oldPlot.clear();
        }
        parent.removeView(oldLatestCapture);
      }
      // Always add new latest_capture if there is a valid captureToDisplay (external
      // or
      // intravascular)
      if (captureToDisplay != null && captureToDisplay.getCaptureData() != null &&
          captureToDisplay.getCaptureData().getData() != null &&
          captureToDisplay.getCaptureData().getData().length > 0) {
        LayoutInflater inflater = LayoutInflater.from(parent.getContext());
        View newLatestCapture = inflater.inflate(R.layout.capture_layout, parent, false);
        newLatestCapture.setId(R.id.latest_capture);
        parent.addView(newLatestCapture, index);
        // Bind the data to the new view
        ImageView captureIcon = newLatestCapture.findViewById(R.id.capture_icon);
        TextView captureId = newLatestCapture.findViewById(R.id.capture_id);
        TextView captureHeaderLabel = newLatestCapture.findViewById(R.id.capture_header_label);
        TextView captureInsertedLength = newLatestCapture.findViewById(R.id.capture_inserted_length);
        TextView captureFooterLabel = newLatestCapture.findViewById(R.id.capture_footer_label);
        TextView captureExposedLength = newLatestCapture.findViewById(R.id.capture_exposed_length);
        com.androidplot.xy.XYPlot capturePlotGraph = newLatestCapture.findViewById(R.id.capture_plot_graph);
        if (capturePlotGraph != null) {
          capturePlotGraph.clear(); // Ensure no old series remain
        }
        if (captureToDisplay.getIsIntravascular()) {
          captureIcon.setImageResource(R.drawable.logo_capture_internal);
          captureId.setVisibility(View.VISIBLE);
          captureId.setText(String.valueOf(captureToDisplay.getCaptureId()));
          captureHeaderLabel.setVisibility(View.VISIBLE);
          captureInsertedLength.setVisibility(View.VISIBLE);
          captureFooterLabel.setVisibility(View.VISIBLE);
          captureExposedLength.setVisibility(View.VISIBLE);
          captureInsertedLength.setText(
              String.format("%.1f cm", captureToDisplay.getInsertedLengthCm()));
          captureExposedLength.setText(
              String.format("%.1f cm", captureToDisplay.getExposedLengthCm()));
        } else {
          captureIcon.setImageResource(R.drawable.logo_external);
          captureId.setVisibility(View.INVISIBLE);
          captureHeaderLabel.setVisibility(View.VISIBLE);
          captureInsertedLength.setVisibility(View.VISIBLE);
          captureFooterLabel.setVisibility(View.INVISIBLE);
          captureExposedLength.setVisibility(View.INVISIBLE);
          captureInsertedLength.setText(
              String.format("%.1f cm", captureToDisplay.getInsertedLengthCm()));
        }
        // Defensive null checks before drawing capture graph
        android.util.Log.d("ProcedureFragment", "initCaptures: About to draw capture graph");
        android.util.Log.d("ProcedureFragment",
            "initCaptures: capturePlotGraph=" + (capturePlotGraph != null));
        android.util.Log.d("ProcedureFragment",
            "initCaptures: captureToDisplay.getCaptureData()=" +
                (captureToDisplay.getCaptureData() != null));
        android.util.Log.d("ProcedureFragment",
            "initCaptures: captureToDisplay.getFormatter()=" +
                (captureToDisplay.getFormatter() != null));
        if (capturePlotGraph != null && captureToDisplay.getCaptureData() != null) {
          if (captureToDisplay.getFormatter() == null) {
            PlotStyle style = captureToDisplay.getIsIntravascular() ? PlotStyle.Intravascular : PlotStyle.External;
            captureToDisplay.setFormatter(PlotUtils.getFormatter(requireContext(), style, 0));
            android.util.Log.d("ProcedureFragment",
                "initCaptures: Formatter was null, set new formatter");
          }
          android.util.Log.d("ProcedureFragment", "initCaptures: Drawing capture graph now");
          com.accessvascularinc.hydroguide.mma.ui.plot.PlotUtils.drawCaptureGraph(capturePlotGraph,
              captureToDisplay);
          PlotUtils.stylePlot(capturePlotGraph);
        } else {
          MedDevLog.error("ProcedureFragment",
              "Cannot draw capture graph: plot or data is null. capturePlotGraph=" +
                  (capturePlotGraph != null)
                  + ", captureData=" + (captureToDisplay.getCaptureData() != null) +
                  ", formatter="
                  + (captureToDisplay.getFormatter() != null));
        }
      }
    }

    if (binding != null && binding.procedureContent != null) {
      RelativeLayout parent = (RelativeLayout) binding.procedureContent.getRoot();
      View latestCaptureView = parent.findViewById(R.id.latest_capture);
      View noCapturePlaceholderView = parent.findViewById(R.id.no_capture_placeholder);
      boolean hasLatestCapture = (latestCaptureView != null && captureToDisplay != null);

      if (hasLatestCapture) {
        latestCaptureView.setVisibility(View.VISIBLE);
        if (noCapturePlaceholderView != null) {
          noCapturePlaceholderView.setVisibility(View.GONE);
          noCapturePlaceholderView.setEnabled(false);
          noCapturePlaceholderView.setAlpha(0f);
        }
        latestCaptureView.setOnClickListener(v -> onLatestCaptureClicked());
      } else {
        if (latestCaptureView != null) {
          latestCaptureView.setVisibility(View.GONE);
        }
        if (noCapturePlaceholderView != null) {
          noCapturePlaceholderView.setVisibility(View.VISIBLE);
          noCapturePlaceholderView.setEnabled(true);
          noCapturePlaceholderView.setAlpha(1f);
        }
      }
      if (latestCaptureView != null &&
          latestCaptureView.getLayoutParams() instanceof RelativeLayout.LayoutParams) {
        RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) latestCaptureView.getLayoutParams();
        params.removeRule(RelativeLayout.BELOW);
        params.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        // params.addRule();
        params.setMargins(0, 0, 0, 10);
        params.setMarginStart(90);
        latestCaptureView.setLayoutParams(params);
      }
    }

    // --- (Keep original external capture logic for other uses, but hidden in UI)
    // ---
    final CaptureLayoutBinding extCapLayout = binding.procedureContent.externalCapture;
    final UserProfile profile = viewmodel.getProfileState().getSelectedProfile();
    useExposedLength = profile.getUserPreferences()
        .getWaveformCaptureLabel() == UserPreferences.WaveformCaptureLabel.ExposedLength;
    android.util.Log.d("ProcedureFragment",
        "setupExternalCapture: externalCapture=" +
            (externalCapture != null ? externalCapture.toString() : "null"));
    // PlotUtils.setupExternalCapture(extCapLayout, externalCapture,
    // useExposedLength);
    if (externalCapture != null) {
      // extCapLayout.captureLayout.setVisibility(View.VISIBLE);
      final EcgDataSeries extEcgData = externalCapture.getCaptureData();
      android.util.Log.d("ProcedureFragment", "ExternalCapture data points: "
          + (extEcgData != null && extEcgData.getData() != null ? extEcgData.getData().length : -1));
      extCapSeries = new EcgDataSeries(extEcgData.getData(), extEcgData.getMarkedPoints());
      final Number lastCapPMarkVal = extEcgData.getY(extEcgData.getLatestMarkedPointIndex());
      if (lastCapPMarkVal.doubleValue() != 0.0) {
        gLastCap.setY(lastCapPMarkVal, 0);
        gCaptureStore = lastCapPMarkVal;
      }
    } else {
      extCapLayout.captureLayout.setVisibility(View.INVISIBLE);
      extCapSeries = new EcgDataSeries(new Number[] {}, new Number[] {});
    }
    this.extCapture = extCapLayout.capturePlotGraph;
    android.util.Log.d("ProcedureFragment", "Adding extCapSeries to extCapture plot");
    this.extCapture.clear();
    this.extCapture.addSeries(extCapSeries,
        PlotUtils.getFormatter(requireContext(), PlotStyle.External, 0));
    android.util.Log.d("ProcedureFragment", "After addSeries for extCapture plot");

    intravCapsListView = binding.procedureContent.intravCapturesContainer.capturesList;
    intravCapsListView.setLayoutManager(
        new LinearLayoutManager(requireContext(), LinearLayoutManager.HORIZONTAL, false));
    intravCapsListAdapter = new IntravCaptureListAdapter();
    android.util.Log.d("ProcedureFragment", "initCaptures: externalCapture=" +
        (externalCapture != null ? externalCapture.toString() : "null"));
    android.util.Log.d("ProcedureFragment", "initCaptures: intravCaptures.length=" +
        (intravCaptures != null ? intravCaptures.length : -1));
    // Removed duplicate declaration of latestCapture
    // Removed duplicate declaration of allCaptures
    if (externalCapture != null) {
      allCaptures.add(externalCapture);
    }
    if (intravCaptures != null && intravCaptures.length > 0) {
      java.util.Collections.addAll(allCaptures, intravCaptures);
    }
    if (!allCaptures.isEmpty()) {
      latestCapture = allCaptures.get(allCaptures.size() - 1);
    }
    android.util.Log.d("ProcedureFragment", "Latest capture for card: " +
        (latestCapture != null ? latestCapture.toString() : "null"));
    viewmodel.getEncounterData().setLatestCapture(latestCapture);

    // Log details for each intravascular capture
    if (intravCaptures != null && intravCaptures.length > 0) {
      for (int i = 0; i < intravCaptures.length; i++) {
        Capture c = intravCaptures[i];
        android.util.Log.d("ProcedureFragment",
            "IntravCapture[" + i + "]: id=" + c.getCaptureId() + ", insertedLength=" +
                c.getInsertedLengthCm()
                + ", exposedLength=" + c.getExposedLengthCm() + ", dataPoints="
                + (c.getCaptureData() != null && c.getCaptureData().getData() != null
                    ? c.getCaptureData().getData().length
                    : -1));
      }
    }

    intravCapsListView.setAdapter(intravCapsListAdapter);
  }

  private BarFormatter getBarFormatter(final PlotUtils.PWaveChange changeType) {
    // Can make max bar be behind the other bars with margins and adding series
    // earlier
    int barColor = Color.TRANSPARENT;
    float leftMargin = 0;
    float rightMargin = 0;
    switch (changeType) {
      case External -> {
        barColor = getColor(R.color.white);
        leftMargin = PixelUtils.dpToPix(15);
        rightMargin = PixelUtils.dpToPix(95);
      }
      case LastCapture -> {
        barColor = getColor(R.color.av_pink);
        leftMargin = PixelUtils.dpToPix(55);
        rightMargin = PixelUtils.dpToPix(55);
      }
      case Current -> {
        barColor = getColor(R.color.av_yellow);
        leftMargin = PixelUtils.dpToPix(95);
        rightMargin = PixelUtils.dpToPix(15);
      }
      case Maximum -> {
        barColor = getColor(R.color.av_light_blue);
        leftMargin = PixelUtils.dpToPix(50);
        rightMargin = PixelUtils.dpToPix(10);
      }
    }

    final BarFormatter formatter = new BarFormatter(barColor, Color.TRANSPARENT);
    formatter.setMarginLeft(leftMargin);
    formatter.setMarginRight(rightMargin);
    return formatter;
  }

  protected void initMockData() {
    final int mockDelay = 50;

    populateMockDataSource();
    keepRunningMock = true;

    mockDataThread = new Thread(() -> {
      try {
        Looper.prepare();
        while (keepRunningMock) {
          // in a real procedure, 10 data points will come ever 50 ms
          if ((mockIdx + 10 >= mockExternalEcgData.length) ||
              (mockIdx + 10 >= mockIntravEcgData.length)) {
            mockIdx = 0;
          }

          if (plotMockData) {
            for (int i = 0; i < 10; i++) {
              final int mockTimestamp = (mockIdx * 5) + (5 * i);
              updateEcgDataPoints(mockTimestamp, mockExternalEcgData[mockIdx + i],
                  mockIntravEcgData[mockIdx + i]);

              if (mockExternalPeaksData[mockIdx + i] == PeakLabel.P ||
                  mockIntravPeaksData[mockIdx + i] == PeakLabel.P) {
                updatePWavePoints(mockTimestamp, mockExternalEcgData[mockIdx + i].floatValue(),
                    mockIntravEcgData[mockIdx + i].floatValue());
              }
            }
          }

          Thread.sleep(mockDelay);
          mockIdx = mockIdx + 10;
        }
      } catch (final InterruptedException e) {
        keepRunningMock = false;
        Thread.currentThread().interrupt();
        throw new RuntimeException(e);
      }
    });
    mockDataThread.start();
  }

  private void updateErrorDisplay() {
    MedDevLog.info("Alarm", "Error display updated");
    final TabletState tabletState = viewmodel.getTabletState();
    final boolean enabled = !tabletState.getActiveAlarms().isEmpty();
    if (enabled) {
      AlarmEvent target = tabletState.getActiveAlarms().peek();
      if (viewmodel.getEncounterData().getState() == EncounterState.DualSuspended) {
        boolean inIntra = false; // if the top is an internal suspend
        for (final AlarmEvent alarm : internalSuspendAlarms) {
          if (target == alarm) {
            inIntra = true;
            break;
          }
        }
        if (showIntravascular && !inIntra) {
          final PriorityQueue<AlarmEvent> pqueue = new PriorityQueue<>();
          Collections.addAll(pqueue, internalSuspendAlarms);
          for (int i = 0; i < pqueue.size(); i++) {
            if (tabletState.getActiveAlarms().contains(pqueue.peek())) {
              target = pqueue.peek();
              break;
            }
            pqueue.poll();
          }
        }
        boolean inExtern = false;
        for (final AlarmEvent alarm : externalSuspendAlarms) {
          if (target == alarm) {
            inExtern = true;
            break;
          }
        }
        if (!showIntravascular && !inExtern) {
          final PriorityQueue<AlarmEvent> pqueue = new PriorityQueue<>();
          Collections.addAll(pqueue, externalSuspendAlarms);
          for (int i = 0; i < pqueue.size(); i++) {
            if (tabletState.getActiveAlarms().contains(pqueue.peek())) {
              target = pqueue.peek();
              break;
            }
            pqueue.poll();
          }
        }
      }
      final List<AlarmEvent> eventList = new ArrayList<>();
      Collections.addAll(eventList,
          viewmodel.getTabletState().getActiveAlarms().toArray(new AlarmEvent[0]));
      String additionalAlarms = "";
      for (final AlarmEvent ae : eventList) {
        if (ae.getAlarmType() == target.getAlarmType()) {
          continue;
        }
        switch (ae.getAlarmType()) {
          case BatterySubsystemFault -> {
            additionalAlarms += ContextCompat.getString(requireContext(), R.string.battery_subs_unique_id) +
                ", ";
          }
          case EcgDataMissing -> {
            additionalAlarms += ContextCompat.getString(requireContext(), R.string.ecg_data_unique_id) + ", ";
          }
          case ControllerStatusTimeout -> {
            additionalAlarms += ContextCompat.getString(requireContext(), R.string.ctrl_status_unique_id) +
                ", ";
          }
          case InternalLeadOff -> {
            additionalAlarms += ContextCompat.getString(requireContext(), R.string.lead_off_intrav_unique_id) +
                ", ";
          }
          case ExternalLeadOff -> {
            additionalAlarms += ContextCompat.getString(requireContext(), R.string.lead_off_extern_unique_id) +
                ", ";
          }
          case FuelGaugeStatusFault -> {
            additionalAlarms += ContextCompat.getString(requireContext(), R.string.fuel_gauge_unique_id) + ", ";
          }
          case SupplyRangeFault -> {
            additionalAlarms += ContextCompat.getString(requireContext(), R.string.hardware_fault_unique_id) +
                ", ";
          }
        }
      }
      if (additionalAlarms.length() > 0) {
        additionalAlarms = additionalAlarms.substring(0, additionalAlarms.length() - 2);
      }

      multiContentEnabled = (additionalAlarms.length() > 0);

      switch (target.getAlarmType()) {
        case BatterySubsystemFault -> {
          writeErrorDisplay(R.string.battery_subs_title, R.string.battery_subs_description,
              R.string.battery_subs_instructions, R.string.battery_subs_note_text,
              R.string.battery_subs_unique_id, target.getCompositeAlerts(), additionalAlarms);
        }
        case EcgDataMissing -> {
          writeErrorDisplay(R.string.ecg_data_title, R.string.ecg_data_description,
              R.string.ecg_data_instructions, R.string.ecg_data_note_text,
              R.string.ecg_data_unique_id, target.getCompositeAlerts(), additionalAlarms);
        }
        case ControllerStatusTimeout -> {
          writeErrorDisplay(R.string.ctrl_status_title, R.string.ctrl_status_description,
              R.string.ctrl_status_instructions, R.string.ctrl_status_note_text,
              R.string.ctrl_status_unique_id, target.getCompositeAlerts(), additionalAlarms);
        }
        case InternalLeadOff -> {
          writeErrorDisplay(R.string.lead_off_intrav_title, R.string.lead_off_intrav_description,
              R.string.lead_off_intrav_instructions, R.string.lead_off_intrav_note_text,
              R.string.lead_off_intrav_unique_id, target.getCompositeAlerts(),
              additionalAlarms);
        }
        case ExternalLeadOff -> {
          writeErrorDisplay(R.string.lead_off_extern_title, R.string.lead_off_extern_description,
              R.string.lead_off_extern_instructions, R.string.lead_off_extern_note_text,
              R.string.lead_off_extern_unique_id, target.getCompositeAlerts(),
              additionalAlarms);
        }
        case FuelGaugeStatusFault -> {
          writeErrorDisplay(R.string.fuel_gauge_title, R.string.fuel_gauge_description,
              R.string.fuel_gauge_instructions, R.string.fuel_gauge_note_text,
              R.string.fuel_gauge_unique_id, target.getCompositeAlerts(), additionalAlarms);
        }
        case SupplyRangeFault -> {
          writeErrorDisplay(R.string.hardware_fault_title, R.string.hardware_fault_description,
              R.string.hardware_fault_instructions, R.string.hardware_fault_note_text,
              R.string.hardware_fault_unique_id, target.getCompositeAlerts(), additionalAlarms);
        }
      }
    }
  }

  private void writeErrorDisplay(final int title, final int description, final int instructions,
      final int note,
      final int alertId, final String composites,
      final String additionalAlarms) {
    MedDevLog.info("Alarm", "Rewrote error display text("+title+")");

    if (multiContentEnabled) {
      final MultiAlertContentBinding alertLayout = binding.procedureContent.multiAlertContent;
      alertLayout.alertWidgetTitle.setText(title);
      alertLayout.alertWidgetDescription.setText(description);
      alertLayout.alertWidgetBullets.setText(instructions);
      alertLayout.alertWidgetNoteLine.setText(note);
      alertLayout.alertIdText.setText(alertId);
      alertLayout.alertPriorityIndicator.setText(
          String.valueOf(viewmodel.getTabletState().getActiveAlarms().peek().getPriority()));
      alertLayout.additionalAlertsCounter.setText(
          String.valueOf(viewmodel.getTabletState().getActiveAlarms().size() - 1));
      alertLayout.compositeAlertsIndicator.setText(composites);
      alertLayout.additionalAlertsList.setText(additionalAlarms);
      alertLayout.compositeAlertsLabel.setVisibility((composites == "") ? View.GONE : View.VISIBLE);
    } else {
      final AlertContentBinding alertLayout = binding.procedureContent.alertContent;
      alertLayout.alertWidgetTitle.setText(title);
      alertLayout.alertWidgetDescription.setText(description);
      alertLayout.alertWidgetBullets.setText(instructions);
      alertLayout.alertWidgetNoteLine.setText(note);
      alertLayout.alertIdText.setText(alertId);
      alertLayout.alertPriorityIndicator.setText(
          String.valueOf(viewmodel.getTabletState().getActiveAlarms().peek().getPriority()));
      alertLayout.compositeAlertsIndicator.setText(composites);
      alertLayout.compositeAlertsLabel.setVisibility((composites == "") ? View.GONE : View.VISIBLE);
    }
  }

  private void toggleBlinkingAnimation(final RelativeLayout icon, final boolean active) {
    if (active) {
      final Animation blinkAnimation = new AlphaAnimation(1.0f, 0.2f);
      blinkAnimation.setDuration(500); // 500ms = 1/2 second
      blinkAnimation.setInterpolator(new LinearInterpolator());
      blinkAnimation.setRepeatMode(Animation.REVERSE);
      blinkAnimation.setRepeatCount(Animation.INFINITE);
      icon.startAnimation(blinkAnimation);
    } else {
      icon.clearAnimation();
    }
  }

  private void writeErrorDisplay(final boolean enabled) {
    MedDevLog.info("Alarm", (enabled) ? "Error display enabled" : "Error display disabled");
    if (!enabled) {
      binding.procedureContent.alertContent.alertContentLayout.setVisibility(View.GONE);
      binding.procedureContent.multiAlertContent.alertContentLayout.setVisibility(View.GONE);
    }
    if (multiContentEnabled) {
      final boolean freed = false;
      final MultiAlertContentBinding macBinding = binding.procedureContent.multiAlertContent;
      if (!viewmodel.getEncounterData().getProcedureStarted() && !leadsConnected) {
        macBinding.alertWidgetRedBorder.setBackground(
            ContextCompat.getDrawable(requireContext(), R.color.white));
        macBinding.alertWidget2RedBorder.setBackground(
            ContextCompat.getDrawable(requireContext(), R.color.white));
        // macBinding.alertHighlightSymbol.setVisibility(View.GONE);
        // OR
        macBinding.alertHighlightSymbol.setImageDrawable(
            ContextCompat.getDrawable(requireContext(), R.drawable.remote_battery_warning));
      } else {
        if (!leadsConnected) {
          toggleBlinkingAnimation(macBinding.alertWidgetRedBorder, enabled);
        }
        leadsConnected = true;
        macBinding.alertWidgetRedBorder.setBackground(
            ContextCompat.getDrawable(requireContext(), R.color.battery_red));
        macBinding.alertWidget2RedBorder.setBackground(
            ContextCompat.getDrawable(requireContext(), R.color.battery_red));
        // macBinding.alertHighlightSymbol.setVisibility(View.VISIBLE);
        // OR
        macBinding.alertHighlightSymbol.setImageDrawable(
            ContextCompat.getDrawable(requireContext(), R.drawable.warning_icon));

      }
      if (macBinding.alertContentLayout.getVisibility() != ((enabled) ? View.VISIBLE : View.GONE) ||
          freed) {
        macBinding.alertContentLayout.setVisibility(
            (enabled) ? View.VISIBLE : View.GONE);
        binding.procedureContent.alertContent.alertContentLayout.setVisibility(
            (!enabled) ? View.VISIBLE : View.GONE);
        if (leadsConnected) {
          toggleBlinkingAnimation(macBinding.alertWidgetRedBorder,
              enabled);
        }
      }
      binding.procedureContent.alertContent.alertContentLayout.setVisibility(View.GONE);
    } else {
      final boolean freed = false;
      final AlertContentBinding acBinding = binding.procedureContent.alertContent;
      if (!viewmodel.getEncounterData().getProcedureStarted() && !leadsConnected) {
        acBinding.alertWidgetRedBorder.setBackground(
            ContextCompat.getDrawable(requireContext(), R.color.white));
        // acBinding.alertHighlightSymbol.setVisibility(View.GONE);
        // OR
        acBinding.alertHighlightSymbol.setImageDrawable(
            ContextCompat.getDrawable(requireContext(), R.drawable.remote_battery_warning));
      } else {
        if (!leadsConnected) {
          toggleBlinkingAnimation(acBinding.alertWidgetRedBorder, enabled);
        }
        leadsConnected = true;
        acBinding.alertWidgetRedBorder.setBackground(
            ContextCompat.getDrawable(requireContext(), R.color.battery_red));
        // acBinding.alertHighlightSymbol.setVisibility(View.VISIBLE);
        // OR
        acBinding.alertHighlightSymbol.setImageDrawable(
            ContextCompat.getDrawable(requireContext(), R.drawable.warning_icon));
      }
      if (acBinding.alertContentLayout.getVisibility() != ((enabled) ? View.VISIBLE : View.GONE) ||
          freed) {
        acBinding.alertContentLayout.setVisibility(
            (enabled) ? View.VISIBLE : View.GONE);
        binding.procedureContent.multiAlertContent.alertContentLayout.setVisibility(
            (!enabled) ? View.VISIBLE : View.GONE);
        if (leadsConnected) {
          toggleBlinkingAnimation(acBinding.alertWidgetRedBorder, enabled);
        }
      }
      binding.procedureContent.multiAlertContent.alertContentLayout.setVisibility(View.GONE);
    }
  }

  private void updateImpedanceDataPoints(final long timestamp,
      final ImpedanceSample newImpedanceSample) {
    // Log.d("ProcedureFragment", "Updating impedance data at timestamp: " +
    // timestamp +
    // " with sample: " + newImpedanceSample.toString());
    viewmodel.getEncounterData().getImpedanceData().addDataPoint(timestamp, newImpedanceSample);
  }

  private void updateEcgDataPoints(final long timestamp, final Number newExtEcgPoint,
      final Number newIntravEcgPoint) {
    externalData.addDataPoint(timestamp, newExtEcgPoint);
    intravascularData.addDataPoint(timestamp, newIntravEcgPoint);
  }

  private void updatePWavePoints(final long timestamp, final float newExtPeak,
      final float newIntravPeak) {
    externalData.addPMarkPoint(timestamp, newExtPeak, true);
    intravascularData.addPMarkPoint(timestamp, newIntravPeak, true);

    /* if (extIsPWave || intIsPWave) */
    {
      viewmodel.getMonitorData()
          .setCurrentPWaveAmplitudes(new MonitorData.TrackedPWaves(newExtPeak, newIntravPeak));

      final var priorMax = viewmodel.getMonitorData().getMaxPWaveAmplitudes();
      var intravMax = priorMax.intravascular_mV;
      var extMax = priorMax.external_mV;
      boolean maxChanged = false;

      if (newIntravPeak > intravMax) {
        intravMax = newIntravPeak;
        maxChanged = true;
      }
      if (newExtPeak > extMax) {
        extMax = newExtPeak;
        maxChanged = true;
      }

      if (maxChanged) {
        if (intravMax < 5.0) {
          viewmodel.getMonitorData()
              .setMaxPWaveAmplitudes(new MonitorData.TrackedPWaves(extMax, intravMax));
        }
      }
    } /*
       * else {
       * final String alertMsg =
       * String.format("No p-wave for either waveform. Time: %s, ExtPeak: %s, " +
       * "IntPeak: %s", timestamp, newExtPeak, newIntravPeak);
       * MedDevLog.warn("Update P-Wave Points", alertMsg);
       * }
       */
  }

  private void populateMockDataSource() {
    mockExternalEcgData = getEcgFromFile(mockDataState.getExternalEcg());
    mockExternalPeaksData = getPeaksFromFile(mockDataState.getExternalPeaks());
    mockIntravEcgData = getEcgFromFile(mockDataState.getIntravascularEcg());
    mockIntravPeaksData = getPeaksFromFile(mockDataState.getIntravascularPeaks());
  }

  private Double[] getEcgFromFile(final String filename) {
    try {
      final List<Double> ecgVals = new ArrayList<>();

      final BufferedReader ecgBf = new BufferedReader(
          new InputStreamReader(requireContext().getAssets().open(filename),
              StandardCharsets.UTF_8));

      String ecgLine = ecgBf.readLine();
      while (ecgLine != null) {
        ecgVals.add(Double.parseDouble(ecgLine));
        ecgLine = ecgBf.readLine();
      }
      ecgBf.close();
      return ecgVals.toArray(new Double[0]);
    } catch (final IOException e) {
      throw new RuntimeException(e);
    }
  }

  private PeakLabel[] getPeaksFromFile(final String filename) {
    try {
      final List<PeakLabel> peakVals = new ArrayList<>();
      final BufferedReader peakBf = new BufferedReader(
          new InputStreamReader(requireContext().getAssets().open(filename),
              StandardCharsets.UTF_8));

      String peakLine = peakBf.readLine();
      while (peakLine != null) {
        peakVals.add(PeakLabel.valueOfInt((int) Double.valueOf(peakLine).longValue()));
        peakLine = peakBf.readLine();
      }
      peakBf.close();
      return peakVals.toArray(new PeakLabel[0]);
    } catch (final IOException e) {
      throw new RuntimeException(e);
    }
  }

  private void updateAllRangeBoundaries() {
    final ChartConfig config = viewmodel.getChartConfig();
    // Offset and scale need to be calculated together since both affect the range
    // boundaries
    // adjust bounds so data is visually higher/lower
    final float offsetAdjustment = config.getVerticalOffset();
    // Adjust graph bounds so data visually takes up more/less vertical space.
    final float scaleAdjustment = config.getVerticalScale() / 2;
    final Number newUpper = (PlotUtils.GRAPH_UPPER_BOUND_DEFAULT - scaleAdjustment) - offsetAdjustment;
    final Number newLower = (PlotUtils.GRAPH_LOWER_BOUND_DEFAULT + scaleAdjustment) - offsetAdjustment;

    final Capture extCapture = viewmodel.getEncounterData().getExternalCapture();
    // Update both live waveform graphs and all capture graphs.
    if (extGraph != null) {
      extGraph.setRangeBoundaries(newLower, newUpper, BoundaryMode.FIXED);
    }
    if (intraGraph != null) {
      intraGraph.setRangeBoundaries(newLower, newUpper, BoundaryMode.FIXED);
    }

    if (extCapture != null) {
      extCapture.setLowerBound(newLower);
      extCapture.setUpperBound(newUpper);

      this.extCapture.setRangeBoundaries(newLower, newUpper, BoundaryMode.FIXED);
      this.extCapture.redraw();
    }

    if (viewmodel.getEncounterData().getIntravCaptures().length > 0) {
      intravCapsListAdapter.updateGraphBounds(newLower, newUpper);
    }
    /*
     * TODO: Uncomment after adding input to toggle whether three range increment
     * lines are always
     * shown or not.
     * intravGraph.setRangeStep(StepMode.INCREMENT_BY_VAL,
     * graph_range_increment_default - (config.getVerticalScaleIntravascular()/4));
     */
  }

  @Override
  public void onSwipeEnd() {
    // Auto scale the time/speed once the sweep ends (line reaches the right edge)
    requireActivity().runOnUiThread(() -> {
      if (timeScaleAutoContEnabled) {
        final double bpm = viewmodel.getMonitorData().getHeartRateBpm();
        if (bpm >= 0) {
          viewmodel.getChartConfig().calcTimeScaleFromBpm(bpm);
        }
      }
    });
  }

  private final Observable.OnPropertyChangedCallback chartConfigChanged_Listener = new Observable.OnPropertyChangedCallback() {
    @Override
    public void onPropertyChanged(final Observable sender, final int propertyId) {
      final ChartConfig config = (ChartConfig) sender;
      if (propertyId == BR.timeScale) {
        final int newDataSize = Math.round(config.getTimeScale() * PlotUtils.POINTS_PER_SECOND);

        if ((externalData.getData().length != newDataSize) ||
            (intravascularData.getData().length != newDataSize)) {
          externalData.resize(newDataSize);
          intravascularData.resize(newDataSize);
          extGraph.setDomainBoundaries(0, newDataSize, BoundaryMode.FIXED);
          intraGraph.setDomainBoundaries(0, newDataSize, BoundaryMode.FIXED);
          extGraph.getRenderer(FadeRenderer.class).getFormatter(externalData)
              .setTrailSize(newDataSize);
          intraGraph.getRenderer(FadeRenderer.class).getFormatter(intravascularData)
              .setTrailSize(newDataSize);
        }
      } else if ((propertyId == BR.verticalOffset) || (propertyId == BR.verticalScale)) {
        updateAllRangeBoundaries();
      } else if (propertyId == BR.timeScaleMode) {
        final ChartConfig.AutoscaleAction action = config.getTimeScaleMode();
        // Auto modes calculate with BPM how many "beats" should be displayed
        final double bpm = viewmodel.getMonitorData().getHeartRateBpm();

        switch (action) {
          case Manual:
            viewmodel.getChartConfig().setPlotSpeed(1);
            timeScaleAutoContEnabled = false;
            break;
          case AutoOnce:
            if (bpm > 0) {
              viewmodel.getChartConfig().calcTimeScaleFromBpm(bpm);
            }
            timeScaleAutoContEnabled = false;
            break;
          case AutoContinuous:
            if (bpm > 0) {
              viewmodel.getChartConfig().calcTimeScaleFromBpm(bpm);
            }
            timeScaleAutoContEnabled = true;
            break;
          default:
            timeScaleAutoContEnabled = false;
            break;
        }

        if (bpm <= 0) {
          MedDevLog.error("Time Scale Mode",
              getString(R.string.speed_auto_scale_invalid_bpm_message));
        }
      } else {
        MedDevLog.error("OnChartConfigChange",
            String.format("Unknown Property ID: %s.", propertyId));
      }
    }
  };

  private final Observable.OnPropertyChangedCallback tabletStateChanged_Listener = new Observable.OnPropertyChangedCallback() {
    @Override
    public void onPropertyChanged(final Observable sender, final int propertyId) {
      if (propertyId == BR.activeAlarms) {
        checkStatePermissions();
      }
    }
  };

  private final Observable.OnPropertyChangedCallback monitorDataChanged_Listener = new Observable.OnPropertyChangedCallback() {
    @Override
    public void onPropertyChanged(final Observable sender, final int propertyId) {
      final MonitorData config = (MonitorData) sender;
      final var latestCurrent = config.getCurrentPWaveAmplitudes();
      final var latestMax = config.getMaxPWaveAmplitudes();
      if (propertyId == BR.currentPWaveAmplitudes) {
        if ((gCurrent != null) && (gExternal != null)) {
          Log.i("Debugging", "Current: " + latestCurrent.intravascular_mV + " External: " +
              latestCurrent.external_mV + " Max: " + latestMax.intravascular_mV +
              " MaxExt: " + latestMax.external_mV);
          gCurrent.setY(latestCurrent.intravascular_mV, 0);
          gExternal.setY(latestCurrent.external_mV, 0);
        }

      } else if (propertyId == BR.maxPWaveAmplitudes) {
        final float newMax = (float) latestMax.intravascular_mV;
        if (newMax != 0.0) {
          Log.i("Debugging", "Max: " + newMax);
          gMaxStore = newMax;
          gMax.setY(newMax, 0);
          // Update formatter for both graphs with max P-wave amplitude
          final var intraFormatter = (FadeFormatter) intraGraph.getRenderer(FadeRenderer.class)
              .getFormatter(intravascularData);
          // Only apply max amplitude line logic to intravascular graph
          if (intraFormatter != null) {
            intraFormatter.setMaxPWaveAmplitude(newMax);
          }
        }

      } else if (propertyId == BR.capturedPWaveAmplitudes) {
        if (config.getCapturedPWaveAmplitudes().intravascular_mV != 0.0) {
          gCaptureStore = config.getCapturedPWaveAmplitudes().intravascular_mV;
          Log.i("Debugging", "Capture: " + gCaptureStore);
          gLastCap.setY(gCaptureStore, 0);
        }
      } else if (propertyId == BR.heartRateBpm) {
        if (config.getHeartRateBpm() == 0.0) {
          MedDevLog.info("Notable Heartrate",
              "Abnormal heartrate detected: " + config.getHeartRateBpm());
        }
        final EncounterState procState = viewmodel.getEncounterData().getState();
        if ((binding != null) && (procState == EncounterState.Active ||
            procState == EncounterState.IntraSuspended)) {
          binding.procedureTopBar.heartbeatValue.setText(config.getDisplayHeartRate());
        }
      } else {
        MedDevLog.error("OnMonitorDataChange",
            String.format("Unknown Property ID: %s.", propertyId));
      }
    }
  };

  private final Observable.OnPropertyChangedCallback encounterDataChanged_Listener = new Observable.OnPropertyChangedCallback() {
    @Override
    public void onPropertyChanged(final Observable sender, final int propertyId) {
      final EncounterData procedure = (EncounterData) sender;
      final MonitorData monitorData = viewmodel.getMonitorData();
      if (propertyId == BR.externalCapture) {
        final CaptureLayoutBinding extCapLayout = binding.procedureContent.externalCapture;
        if (useExposedLength) {
          final String newExpText = String.format(Locale.US, "%.1f",
              procedure.getExternalCatheterCm().orElse(-1));
          extCapLayout.captureExposedLength.setText(String.format("%s cm", newExpText));
        } else {
          final String newInsText = String.format(Locale.US, "%.1f",
              procedure.getInsertedCatheterCm().orElse(-1));
          extCapLayout.captureInsertedLength.setText(String.format("%s cm", newInsText));
        }

        final Capture extCap = procedure.getExternalCapture();
        if (extCap != null) {
          txtCaptureInstruction.setText(getString(
              (showIntravascular) ? R.string.intrav_captures_instruction_intra
                  : R.string.intrav_captures_alt_instruction));
          // extCapLayout.captureLayout.setVisibility(View.VISIBLE);
          extCapSeries.setData(extCap.getCaptureData().getData(),
              extCap.getCaptureData().getMarkedPoints());
          extCapture.setRangeBoundaries(extCap.getLowerBound(), extCap.getUpperBound(),
              BoundaryMode.FIXED);
          extCapture.redraw();

          // extCap.getCaptureData().findLatestPMarkIdx();

          final int lastPMarkIdx = extCap.getCaptureData().getLatestMarkedPointIndex();
          final Number lastCapPMarkVal = extCap.getCaptureData().getMarker(lastPMarkIdx);
          monitorData.setCapturedPWaveAmplitudes(
              new MonitorData.TrackedPWaves(
                  (lastCapPMarkVal != null) ? lastCapPMarkVal.doubleValue() : 0,
                  monitorData.getCapturedPWaveAmplitudes().intravascular_mV));
        }
      } else if (propertyId == BR.intravCaptures) {
        // For now, the only change should be when adding one capture to the array
        if (procedure.getIntravCaptures().length > 0) {
          llCaptureInstruction.setVisibility(View.GONE);
          intravCapsListView.setVisibility(View.VISIBLE);

          final Capture newCapture = procedure.getIntravCaptures()[procedure.getIntravCaptures().length - 1];
          intravCapsListAdapter.addCapture(newCapture);
          intravCapsListView.smoothScrollToPosition(
              intravCapsListAdapter.getItemCount() - 1);

          final int lastPMarkIdx = newCapture.getCaptureData().getLatestMarkedPointIndex();
          final Number lastCapPMarkVal = newCapture.getCaptureData().getMarker(lastPMarkIdx);
          monitorData.setCapturedPWaveAmplitudes(new MonitorData.TrackedPWaves(
              monitorData.getCapturedPWaveAmplitudes().external_mV,
              (lastCapPMarkVal != null) ? lastCapPMarkVal.doubleValue() : 0));

          // Ensure intravascular graph shows captured & max amplitude horizontal lines
          final var intraFormatter = (FadeFormatter) intraGraph.getRenderer(FadeRenderer.class)
              .getFormatter(intravascularData);
          if (intraFormatter != null) {
            intraFormatter.setCapturedPWaveAmplitude(lastCapPMarkVal);
            intraFormatter.setShowHorizontalLines(
                true); // show lines (pink/blue) on intravascular graph
          }
          // External graph intentionally left without horizontal lines
        }
      } else if (propertyId == BR.selectedCapture) {
        Capture selectedCapture = viewmodel.getEncounterData().getSelectedCapture();
        android.util.Log.d("ProcedureFragment",
            "[OBSERVER] selectedCapture property changed to: " +
                (selectedCapture != null ? selectedCapture.getCaptureId() : "null") +
                " (isIntravascular=" +
                (selectedCapture != null ? selectedCapture.getIsIntravascular() : "N/A") + ")");
        android.util.Log.d("ProcedureFragment",
            "[OBSERVER] Calling initCaptures() to update display with new selection");
        initCaptures();
        Log.d("ProcedureFragment",
            "[OBSERVER] initCaptures() completed, display updated");
      } else if (propertyId == BR.state) {
        checkStatePermissions();
      } else {
        MedDevLog.error("OnEncounterChange",
            String.format("Unknown Property ID: %s.", propertyId));
      }
    }
  };

  private void checkStatePermissions() {
    MedDevLog.info("State Change", "Applying state changes to procedure screen...");
    final EncounterData encounter = viewmodel.getEncounterData();
    if (binding == null) {
      return;
    }
    final boolean isSuspended = (encounter.getState() == EncounterState.Suspended) ||
        (encounter.getState() == EncounterState.DualSuspended);

    // Handle Bottom bar
    final ProcedureBottomBarBinding bottomBarBinding = binding.procedureBottomBar;
    final ImageView btnExtSelExtErr = bottomBarBinding.gtbtnExternalSelectedExternalError;
    final ImageView btnIntravSelExtErr = bottomBarBinding.gtbtnIntravascularSelectedExternalError;

    if (isSuspended || (encounter.getState() == EncounterState.ExternalSuspended)) {
      // External alert
      btnExtSelExtErr.setVisibility(showIntravascular ? View.GONE : View.VISIBLE);
      btnIntravSelExtErr.setVisibility(showIntravascular ? View.VISIBLE : View.GONE);
    } else {
      btnExtSelExtErr.setVisibility(View.GONE);
      btnIntravSelExtErr.setVisibility(View.GONE);
    }

    final ImageView btnExtSelIntravErr = bottomBarBinding.gtbtnExternalSelectedIntravascularError;
    final ImageView btnIntravSelIntravErr = bottomBarBinding.gtbtnIntravascularSelectedIntravascularError;
    if (isSuspended || (encounter.getState() == EncounterState.IntraSuspended)) {
      // Internal alert
      btnExtSelIntravErr.setVisibility(showIntravascular ? View.GONE : View.VISIBLE);
      btnIntravSelIntravErr.setVisibility(showIntravascular ? View.VISIBLE : View.GONE);
    } else {
      btnExtSelIntravErr.setVisibility(View.GONE);
      btnIntravSelIntravErr.setVisibility(View.GONE);
    }

    // Handle gauges
    if (isSuspended || (encounter.getState() == EncounterState.ExternalSuspended)) {
      hideGauges(true, true);
      binding.procedureTopBar.heartbeatValue.setText("???");
    } else if (encounter.getState() == EncounterState.IntraSuspended) {
      hideGauges(true, false);
    } else if (encounter.isLive()) {
      hideGauges(false, false);
    }

    // Handle view
    if (isSuspended ||
            (encounter.getState() == EncounterState.IntraSuspended) ||
            (encounter.getState() == EncounterState.ExternalSuspended)) {
      // if (binding != null) {
      // binding.procedureContent.pwaveGraphs.liveGraph.setVisibility(View.GONE);
      // }
      updateErrorDisplay();
      writeErrorDisplay(true);
      if (isSuspended) {
        viewmodel.getMonitorData().clearMonitorData(); // source of disapearance
        pauseDataPlotting();
        wipeDataPlotting();
      }
    } else if (encounter.isLive()) {
      // if (binding != null) {
      // binding.procedureContent.pwaveGraphs.liveGraph.setVisibility(View.VISIBLE);
      // }
      writeErrorDisplay(false);
      // Don't auto-resume plotting if we're waiting for user to choose Resume/Terminate
      if (!waitingForUserDecision) {
        resumeDataPlotting();
      }
    }
  }

  private void hideGauges(final boolean hideCurrent, final boolean hideExternal) {
    if (hideCurrent) {
      gCurrentActive = false;
    } else if (!gCurrentActive) {
      gCurrentActive = true;
    }
    if (hideExternal) {
      gExternalActive = false;
    } else if (!gExternalActive) {
      gExternalActive = true;
    }
  }

  private void initBottomButtons() {
    final ProcedureBottomBarBinding bottomBarBinding = binding.procedureBottomBar;
    bottomBarBinding.navToPatientInputBtn.setOnClickListener(navToInput_OnClickListener);
    bottomBarBinding.navToPatientSummary.setOnClickListener(navToPatientSummary_OnClickListener);
    bottomBarBinding.volumeBtn.setOnClickListener(volumeBtn_OnClickListener);
    bottomBarBinding.brightnessBtn.setOnClickListener(brightnessBtn_OnClickListener);
    bottomBarBinding.muteBtn.setOnClickListener(muteBtn_OnClickListener);
    bottomBarBinding.cancelBtn.setOnClickListener(cancelBtn_OnClickListener);

    graphToggleBtn = bottomBarBinding.graphToggleBtn;
    graphToggleBtn.setOnClickListener(graphToggleBtn_OnClickListener);

    bottomBarBinding.verticalOffsetBtn.setOnClickListener(vertOffsetBtn_OnClickListener);
    bottomBarBinding.verticalScaleBtn.setOnClickListener(vertScaleBtn_OnClickListener);
    bottomBarBinding.timeScaleBtn.setOnClickListener(timeScaleBtn_OnClickListener);
    bottomBarBinding.cycleMockDataBtn.setOnClickListener(cycleMockDataBtn_OnClickListener);
    bottomBarBinding.demoCaptureBtn.setOnClickListener(demoCaptureBtn_OnClickListener);
  }

  private final View.OnClickListener cancelBtn_OnClickListener = v -> {
    ConfirmDialog.show(requireContext(), "Are you sure you want to cancel Procedure?",
        confirmed -> {
          if (confirmed) {
            onCancelClicked();
          }
        });
  };

  private void onCancelClicked() {
    deleteProcedureFile(viewmodel.getEncounterData());
    MedDevLog.audit("Procedure Completion", "Procedure Cancelled");
    Navigation.findNavController(requireView())
        .navigate(R.id.action_navigation_procedure_to_home);

  }

  private void toggleCapture() {
    Log.d("Procedure", "toggleCapture: received");
    final EncounterState currentState = viewmodel.getEncounterData().getState();
    if (!viewmodel.getEncounterData().isLive() || currentState == EncounterState.Suspended ||
        currentState == EncounterState.DualSuspended || freezeCaptures) {
      return;
    }
    final RectRegion graphBounds = mvGraph.getBounds();
    final EncounterData encounter = viewmodel.getEncounterData();
    double bpm = viewmodel.getMonitorData().getHeartRateBpm();
    if (bpm <= 0) {
      bpm = BPM_DEFAULT;
    } else if (bpm <= BPM_MIN) {
      bpm = BPM_MIN;
    } else if (bpm >= BPM_MAX) {
      bpm = BPM_MAX;
    }
    final int captureSize = (int) (PlotUtils.POINTS_PER_SECOND * 60 / bpm) * 3;

    // Create both internal and external captures
    final Capture internalCapture = new Capture(
        (encounter.getIntravCaptures().length + 1),
        true,
        encounter.getInsertedCatheterCm().orElse(-1),
        encounter.getExternalCatheterCm().orElse(-1),
        intravascularData.getDataSnapshot(captureSize),
        graphBounds.getMaxY(), graphBounds.getMinY(), mvGraph.getRangeStepValue(),
        PlotUtils.getFormatter(requireContext(), PlotStyle.Intravascular, 0));

    final Capture externalCapture = new Capture(
        0,
        false,
        encounter.getInsertedCatheterCm().orElse(-1),
        encounter.getExternalCatheterCm().orElse(-1),
        externalData.getDataSnapshot(captureSize),
        graphBounds.getMaxY(), graphBounds.getMinY(), mvGraph.getRangeStepValue(),
        PlotUtils.getFormatter(requireContext(), PlotStyle.External, 0));

    wasExternalCapturePresentOnFirstClick = (viewmodel.getEncounterData().getExternalCapture() != null);

    // Pass both captures to the dialog with callback to refresh UI and tab
    // preference
    captureDialog = CaptureDialogFragment.newInstance(
        internalCapture, externalCapture, this::initCaptures,
        wasExternalCapturePresentOnFirstClick != null && wasExternalCapturePresentOnFirstClick);
    captureDialog.show(getChildFragmentManager(), "TEST");
  }

  private void cycleMockData() {
    if (mockDataState.next() == MockDataState.Normal) {
      mockIdx = 0;
      wipeDataPlotting();
    }
    mockDataState = mockDataState.next();
    tvMockPhase.setText(String.format("Mock Case: %s", mockDataState.toString()));
    plotMockData = mockDataState != MockDataState.None;
    populateMockDataSource();
    viewmodel.setMockCycle(true);
  }

  private void toggleGraph() {
    showIntravascular = !showIntravascular;
    MedDevLog.info("Navigation",
        showIntravascular ? "Active graph switched to intravascular" : "Active graph switched to external");
    mvGraph = showIntravascular ? intraGraph : extGraph;
    selectActiveGraph();
  }

  private void selectActiveGraph() {
    // Highlight active title, dim inactive
    if (plotTitle != null && intraPlotTitle != null) {
      plotTitle.setAlpha(showIntravascular ? 0.6f : 1f);
      intraPlotTitle.setAlpha(showIntravascular ? 1f : 0.6f);
    }
    if (graphToggleBtn != null) {
      graphToggleBtn.setImageResource(showIntravascular ? R.drawable.button_intravascular_selected
          : R.drawable.button_external_selected);
    }
    final String external_msg = getString(
        (viewmodel.getEncounterData().getExternalCapture() != null) ? R.string.intrav_captures_alt_instruction
            : R.string.intrav_captures_instruction);
    txtCaptureInstruction.setText(
        showIntravascular ? getString(R.string.intrav_captures_instruction_intra) : external_msg);
    checkStatePermissions();
    updateErrorDisplay();
  }

  private int getColor(final int colorId) {
    return ContextCompat.getColor(requireContext(), colorId);
  }

  private void startProcedure() {
    updateErrorDisplay();
    final EncounterData procedure = viewmodel.getEncounterData();
    if (procedure.getStartTime() != null) {
      return;
    }

    MedDevLog.audit("Procedure", "Procedure started");
    MedDevLog.info("State Change", "State transitioned to Active");

    if (!procedure.isLive()) {
      procedure.setState(EncounterState.Active);
    }
    procedure.setProcedureStarted(false);

    final File proceduresDir = requireContext().getExternalFilesDir(DataFiles.PROCEDURES_DIR);

    if ((proceduresDir != null) && !proceduresDir.exists()) {
      final boolean dirCreated = proceduresDir.mkdir();
      if (dirCreated) {
        Toast.makeText(requireContext(), "Procedures directory created successfully.",
            Toast.LENGTH_SHORT).show();
      } else {
        Toast.makeText(requireContext(), "Procedures directory creation failed. File not saved.",
            Toast.LENGTH_SHORT).show();
        return;
      }
    }

    procedure.setAppSoftwareVersion(TabletState.getAppVersion(requireContext()));
    procedure.setControllerFirmwareVersion(viewmodel.getControllerState().getFirmwareVersion());

    // Create new directory named after procedure start time to store data files
    final ProfileState profileState = viewmodel.getProfileState();
    procedure.setStartTime(new Date());
    procedure.getPatient().setPatientInserter(profileState.getSelectedProfile().getProfileName());
    final Facility procFacility = procedure.getPatient().getPatientFacility();
    final String startTimeString = procedure.getStartTimeText();
    procFacility.setDateLastUsed(startTimeString);
    profileState.updateFacilityRecency(procFacility, requireContext());
    final File procedureDir = new File(proceduresDir, startTimeString);

    final String alertMsg;
    // Check if directory and file already exist (created from ultrasound capture
    // screen)
    final File procedureDataFile = new File(procedureDir, DataFiles.PROCEDURE_DATA);
    if (procedureDir.exists() && procedureDataFile.exists()) {
      // Directory and file already exist, just use them
      Log.d("ProcedureFragment", "Procedure directory already exists at: " + startTimeString);
      procedure.setDataDirPath(procedureDir.getAbsolutePath());
      alertMsg = "Using existing procedure directory";
      Log.d("ProcedureFragment", alertMsg);
    } else if (procedureDir.exists() || procedureDir.mkdir()) {
      // Directory exists or was created successfully
      Log.d("ProcedureFragment", "Directory created successfully at: " + startTimeString);
      procedure.setDataDirPath(procedureDir.getAbsolutePath());

      // Create or update procedure data file
      if (!procedureDataFile.exists()) {
        try (final FileWriter writer = new FileWriter(procedureDataFile, StandardCharsets.UTF_8)) {
          final String[] data = procedure.getProcedureDataText();

          for (final String str : data) {
            Log.d("ABC", "startProcedure: " + str);
            writer.write(str);
            Log.d("ABC", "lineSeparator: " + System.lineSeparator());
            writer.write(System.lineSeparator());
            writer.flush();
          }

          alertMsg = DataFiles.PROCEDURE_DATA + " was saved successfully";
          MediaScannerConnection.scanFile(requireContext(),
              new String[] { procedureDataFile.getAbsolutePath() },
              new String[] {}, null);
        } catch (final IOException | JSONException e) {
          MedDevLog.error("Procedure Fragment", "Error saving procedure file", e);
          Toast.makeText(requireContext(), e.getMessage(), Toast.LENGTH_SHORT).show();
          throw new RuntimeException(e);
        }
      } else {
        alertMsg = "Procedure data file already exists";
        Log.d("ProcedureFragment", alertMsg);
      }
    } else {
      alertMsg = "Failed to create directory for this procedure.";
    }

    Toast.makeText(requireContext(), alertMsg, Toast.LENGTH_SHORT).show();
  }

  private void cancelProcedure() {
    // reset procedure data : NOT CURRENTLY IN USE
    MedDevLog.audit("Procedure", "Procedure cancelled");
    mockIdx = 0;
    plotMockData = false;
    mockDataThread = null;
    viewmodel.getEncounterData().clearProcedureData();
    viewmodel.getMonitorData().clearMonitorData();
  }

  private void pauseDataPlotting() {
    MedDevLog.info("Background", "Plotting paused");
    binding.procedureTopBar.heartbeatValue.setText("???");
    gCaptureStore = gLastCap.getY(0);
    gMaxStore = gMax.getY(0);
    if (externalGraphRedrawer != null) {
      externalGraphRedrawer.pause();
    }
    if (intravascularGraphRedrawer != null) {
      intravascularGraphRedrawer.pause();
    }
  }

  private void wipeDataPlotting() {
    MedDevLog.info("Background", "Plotting wiped");
    externalData.clearData();
    intravascularData.clearData();
  }

  private void resumeDataPlotting() {
    MedDevLog.info("Background", "Plotting resumed");
    if (externalGraphRedrawer != null) {
      externalGraphRedrawer.start();
    }
    if (intravascularGraphRedrawer != null) {
      intravascularGraphRedrawer.start();
    }
    gLastCap.setY(gCaptureStore, 0);
    gMax.setY(gMaxStore, 0);
    // paused = false;
  }

  /**
   * Prompts the user to either resume or terminate a suspended procedure after reconnection.
   * This method is called when the controller reconnects during an active/suspended procedure.
   */
  private void promptContinueProcedure() {
    final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireActivity());
    builder.setMessage("Controller reconnected. Do you want to resume the suspended procedure or terminate it?");
    builder.setNegativeButton("Terminate", (dialog, id) -> {
      MedDevLog.audit("Procedure", "User chose to terminate procedure after reconnection");
      waitingForUserDecision = false;
      // Delete the procedure data
      deleteProcedureFile(viewmodel.getEncounterData());
      // Clear the encounter data
      viewmodel.getEncounterData().clearEncounterData();
      viewmodel.getMonitorData().clearMonitorData();
      // Navigate back to home
      Navigation.findNavController(requireView())
          .navigate(R.id.action_navigation_procedure_to_home);
      dialog.dismiss();
    });
    builder.setPositiveButton("Resume", (dialog, id) -> {
      MedDevLog.audit("Procedure", "User chose to resume procedure after reconnection");
      waitingForUserDecision = false;
      // Resume the procedure - wipe plots and restart data collection (same as normal connection flow)
      wipeDataPlotting();
      resumeDataPlotting();
      startProcedure();
      // Start ECG streaming if fragment is resumed
      if (getLifecycle().getCurrentState().isAtLeast(Lifecycle.State.RESUMED) &&
          viewmodel != null && viewmodel.getControllerCommunicationManager() != null &&
          viewmodel.getControllerCommunicationManager().checkConnection() &&
          !ecgStreamingActive) {
        viewmodel.getControllerCommunicationManager().sendECGCommand(true);
        ecgStreamingActive = true;
      }
      dialog.dismiss();
    });
    builder.setCancelable(false);
    builder.show();
  }

  @Override
  public void onConnectionStatusChange(final boolean isConnected) {
    Log.d("Procedure", "onConnectionStatusChange: " + isConnected);

    if (isConnected) {
      // Check if this is a reconnection during an active/suspended procedure
      boolean isReconnectionDuringProcedure = viewmodel != null &&
          viewmodel.getEncounterData() != null &&
          viewmodel.getEncounterData().isLive() &&
          viewmodel.getEncounterData().getStartTime() != null &&
          !hasShownReconnectionPrompt;

      if (isReconnectionDuringProcedure) {
        // CRITICAL: Set flags and pause redrawers BEFORE calling super to prevent plotting
        hasShownReconnectionPrompt = true;
        waitingForUserDecision = true;
        // Immediately pause any active Redrawers to stop plotting
        if (externalGraphRedrawer != null) {
          externalGraphRedrawer.pause();
        }
        if (intravascularGraphRedrawer != null) {
          intravascularGraphRedrawer.pause();
        }
      }

      // Now safe to call super - Redrawers are paused if needed
      super.onConnectionStatusChange(isConnected);

      if (isReconnectionDuringProcedure) {
        // Show prompt to user
        promptContinueProcedure();
      } else if (!waitingForUserDecision) {
        // Normal connection - start procedure (only if not waiting for user decision)
        wipeDataPlotting();
        resumeDataPlotting();
        startProcedure();
        // If the fragment is currently resumed and streaming not yet started, start it now
        if (getLifecycle().getCurrentState().isAtLeast(Lifecycle.State.RESUMED) &&
            viewmodel != null && viewmodel.getControllerCommunicationManager() != null &&
            viewmodel.getControllerCommunicationManager().checkConnection() &&
            !ecgStreamingActive) {
          viewmodel.getControllerCommunicationManager().sendECGCommand(true);
          ecgStreamingActive = true;
        }
      }
    } else {
      super.onConnectionStatusChange(isConnected);

      // Connection lost - reset the prompt flag so it can be shown again on reconnect
      hasShownReconnectionPrompt = false;
      waitingForUserDecision = false;
      pauseDataPlotting();
      // Ensure streaming flag reset and send stop if needed
      if (ecgStreamingActive && viewmodel != null &&
          viewmodel.getControllerCommunicationManager() != null) {
        if (viewmodel.getControllerCommunicationManager().checkConnection()) {
          viewmodel.getControllerCommunicationManager().sendECGCommand(false);
        }
        ecgStreamingActive = false;
      }
    }
  }

  @Override
  public void onCatheterImpedance(ImpedanceSample impedanceSample, String impedanceLogMsg) {
    // Log.d("BiozDataSample", "onCatheterImpedance: " + impedanceLogMsg);
    super.onCatheterImpedance(impedanceSample, impedanceLogMsg);
    updateImpedanceDataPoints(impedanceSample.timestamp_ms, impedanceSample);
  }

  @Override
  public void onFilteredSamples(final EcgSample sample, final String sampleLogMsg) {
    super.onFilteredSamples(sample, sampleLogMsg);
    if (!plotMockData) {
      for (int i = 0; i < 10; i++) {
        updateEcgDataPoints(
            sample.startTime_ms + (5 * i),
            sample.samples[0][i] * 1000,
            sample.samples[1][i] * 1000);
      }
    }
  }

  @Override
  public void onPatientHeartbeat(final Heartbeat heartbeat, final String heartbeatLogMsg) {
    super.onPatientHeartbeat(heartbeat, heartbeatLogMsg);
    viewmodel.getMonitorData()
        .setInvalidHeartRate(heartbeat.eventStatus != HeartbeatEventStates.Good);
    if (!plotMockData && heartbeat.pWavePeakTime_ms > 0) {
      // will be 0 if no p-wave detected
      updatePWavePoints(
          heartbeat.pWavePeakTime_ms,
          heartbeat.pWaveExternalAmplitude_V * 1000,
          heartbeat.pWaveInternalAmplitude_V * 1000);
      Log.i("Debugging", "Reported Hearbeat: " + heartbeatLogMsg);
    }
  }

  @Override
  public void onButtonsStateChange(final Button newButton, final String buttonLogMsg) {
    Log.d("Procedure", "onButtonsStateChange: " + buttonLogMsg + "," + newButton);
    super.onButtonsStateChange(newButton, buttonLogMsg);

    for (int idx = 0; idx < Button.ButtonIndex.NumButtons.getIntValue(); idx++) {
      final Button.ButtonIndex buttonIdx = Button.ButtonIndex.valueOfInt(idx);
      final ButtonState buttonState = newButton.buttons.getButtonStateAtKey(
          Objects.requireNonNull(buttonIdx));

      if (buttonState == ButtonState.ButtonIdle) {
        continue;
      }

      if (buttonIdx == Button.ButtonIndex.Capture && buttonState == ButtonState.PressTransition) {
        if ((captureDialog == null) || (!captureDialog.isAdded())) {
          toggleCapture();
        } else if ((captureDialog != null) && (captureDialog.isAdded())) {
          captureDialog.requestToggle();
        }
      } else {
        InputUtils.simulateKeyInput(buttonIdx, buttonState, inputConnection);
      }
    }
  }

  @Override
  public void onControllerStatus(final ControllerStatus ctrlStatus, final String logMsg) {
    super.onControllerStatus(ctrlStatus, logMsg);
    boolean noPWaveDetected = false;
    if (EcgAnalysisStatusFlags.PWavesNotDetected.isFlaggedIn(ctrlStatus.ecgAnalysisStatus)) {
      MedDevLog.info("Alert", "No P-Wave detected alert recieved");
      noPWaveDetected = true;
    }
    if (binding == null) {
      return;
    }

    final EncounterState procState = viewmodel.getEncounterData().getState();
    if (!freezeCaptures && noPWaveDetected) {
      freezeCaptures = true;
      hideGauges(true, true);
    } else if (!noPWaveDetected && freezeCaptures) {
      freezeCaptures = false;
      if ((procState != EncounterState.Suspended) && (procState != EncounterState.DualSuspended)) {
        if (procState != EncounterState.ExternalSuspended) {
          hideGauges(false, true);
        }
        if (procState != EncounterState.IntraSuspended) {
          hideGauges(false, false);
        }
      }
    }
    // TODO: Reach to controller status messages.
  }

  private void updateMuteBtnDrawable() {
    binding.procedureBottomBar.muteBtn.setImageDrawable(ContextCompat.getDrawable(requireContext(),
        (viewmodel.getTabletState().getIsTabletMuted()) ? R.drawable.button_active_mute
            : R.drawable.button_neutral_mute));
  }

  private final View.OnClickListener muteBtn_OnClickListener = v -> {
    viewmodel.getTabletState().setTabletMuted(!viewmodel.getTabletState().getIsTabletMuted());
    updateMuteBtnDrawable();
  };
}