package com.accessvascularinc.hydroguide.mma.ui;

import static com.accessvascularinc.hydroguide.mma.app.AppConstants.ONE;
import static com.accessvascularinc.hydroguide.mma.app.AppConstants.ORGAN_GLOBAL_ID_VASCULAR_LINEAR;
import static com.accessvascularinc.hydroguide.mma.app.AppConstants.ORGAN_ID_VASCULAR_LINEAR;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.databinding.DataBindingUtil;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentScanningUltrasoundBinding;

import androidx.lifecycle.ViewModelProvider;

import com.accessvascularinc.hydroguide.mma.model.ControllerState;
import com.accessvascularinc.hydroguide.mma.model.DataFiles;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;
import com.accessvascularinc.hydroguide.mma.ultrasound.GlobalUltrasoundManager;
import com.accessvascularinc.hydroguide.mma.ultrasound.UpsManager;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import com.echonous.hardware.ultrasound.DauException;
import com.echonous.hardware.ultrasound.ThorError;
import com.echonous.hardware.ultrasound.UltrasoundManager;
import com.echonous.hardware.ultrasound.UltrasoundViewer;

import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.Toast;

import androidx.core.view.GestureDetectorCompat;

import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Optional;

public class ScanningUltrasoundFragment extends BaseHydroGuideFragment {
  // Custom functional interface for animation with direction
  private interface DepthChangeAnimator {
    void run(int direction);
  }
  private static final String ACTION_USB_PERMISSION = "com.accessvascularinc.hydroguide.mma.USB_PERMISSION";
  private SurfaceView surfaceView;
  private UltrasoundViewer ultrasoundViewer;
  private RulerView rulerView;
  private CenterLineOverlay centerLineOverlay;
  private BroadcastReceiver usbPermissionReceiver;
  private BroadcastReceiver usbAttachmentReceiver;
  private android.widget.TextView depthLabel;
  private boolean isCheckingProbe = false;
  private String fromScreen = Utility.FROM_SCREEN_CONNECT_ULTRASOUND; // Track where user came from
  private boolean isVenousAccess = false;
  
  // Zoom functionality
  private ScaleGestureDetector mScaleDetector;
  private GestureDetectorCompat mGestureDetector;
  private boolean enableZoomAndPan = true;
  private float currentScale = 1.0f;
  
  // Store current gain settings
  private int currentNearGain = 100;
  private int currentFarGain = 100;
  
  // Gain calculation state
  private int lastOverallGain = 100;
  private int absNearGain = 100;
  private int absFarGain = 100;
  private boolean advanceGainInProgress = false;
  private enum Direction { LEFT, RIGHT, NONE }
  private Direction currentDirection = Direction.NONE;
  private static final int MIN_GAIN_VALUE = 0;
  private static final int MAX_GAIN_VALUE = 100;
  
  // Store current depth setting (0-based array index into getAvailableDepths())
  private int currentDepthIndex = 0; // Default to first available depth (20mm)
  
  // Dialog tracking for cleanup on probe detachment
  private android.app.AlertDialog depthDialog;
  private android.app.AlertDialog gainDialog;
  /**
   * Get available depth settings as [ID, mm] pairs
   * @return 2D array where each row is [depth_id, depth_in_mm]
   */
  private Integer[][] getAvailableDepths() {
    return new Integer[][]{
        new Integer[]{1, 20},
        new Integer[]{2, 30},
        new Integer[]{3, 40},
        new Integer[]{4, 60},
        new Integer[]{5, 80},
        new Integer[]{6, 100}
    };
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    
    // Load saved depth index from preferences
    currentDepthIndex = PrefsManager.getSelectedDepthIndex(requireContext());
    
    // Load saved gain values from preferences
    currentNearGain = PrefsManager.getNearGain(requireContext());
    currentFarGain = PrefsManager.getFarGain(requireContext());
    absNearGain = currentNearGain;
    absFarGain = currentFarGain;
    lastOverallGain = (currentNearGain + currentFarGain) / 2;
    Log.d("Ultrasound", "Loaded saved gain: near=" + currentNearGain + ", far=" + currentFarGain);
    
    // Update depth label with saved value
    if (depthLabel != null) {
      // Use depth array mapping to get actual cm value
      var depths = getAvailableDepths();
      if (currentDepthIndex >= 0 && currentDepthIndex < depths.length) {
        depthLabel.setText(depths[currentDepthIndex][1]/10 + "cm Depth");
      }
    }
    
    // Update gain label with saved value (average of near and far)
    android.widget.TextView gainLabel = view.findViewById(R.id.gain_value);
    if (gainLabel != null) {
      int avgGain = (currentNearGain + currentFarGain) / 2;
      gainLabel.setText(avgGain + "%");
      Log.d("Ultrasound", "Updated gain label to: " + avgGain + "%");
    }

    // Get SurfaceView reference
    surfaceView = view.findViewById(R.id.surfaceView);
    
    // Get RulerView reference
    rulerView = view.findViewById(R.id.ruler_view);
    
    // Initialize ruler with default depth (1cm = 10mm)
    updateRulerDepth();

    surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
      @Override
      public void surfaceCreated(@NonNull SurfaceHolder holder) {
        // Attach the surface to native rendering
        if (ultrasoundViewer != null) {
          ultrasoundViewer.setSurface(holder.getSurface());
          
          // If resuming from capture view, restart DAU now that surface is valid
          GlobalUltrasoundManager globalManager = GlobalUltrasoundManager.getInstance();
          if (globalManager != null && !globalManager.isDauStart()) {
            Log.d("Ultrasound", "Surface created - restarting DAU for live streaming");
            globalManager.attachCineIfRequired();
            new android.os.Handler(android.os.Looper.getMainLooper()).postDelayed(() -> {
              globalManager.startDau();
            }, 100); // Small delay for EGL context
          }
        }
      }

      @Override
      public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
        // Surface dimensions changed
        Log.d("Ultrasound", "Surface changed: " + width + "x" + height);
      }

      @Override
      public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
        // Detach surface
        if (ultrasoundViewer != null) {
          ultrasoundViewer.setSurface(null);
        }
      }
    });

    // TODO: Known Issue - Hot-plugging ultrasound probe behavior
    // The EchoNous SDK has a bug where if it's initialized before the probe is connected,
    // its internal DauReceiver crashes on USB_DEVICE_ATTACHED (NullPointerException in KLinkListener).
    // Workaround: MainActivity initializes SDK only when probe is attached (see MainActivity.mUsbReceiver).
    // This fragment delays getting UltrasoundViewer until probe is actually connected via connectProbe().
    // The 200ms delay ensures MainActivity's receiver initializes SDK before fragment tries to use it.
    
    // Register receiver for USB device attachment/detachment
    usbAttachmentReceiver = new BroadcastReceiver() {
      @Override
      public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
          UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
          if (device != null && device.getVendorId() == 0x1dbf) {
            Log.d("Ultrasound", "USB probe attached while on scanning screen");
            // Small delay to ensure MainActivity's receiver has initialized SDK
            new android.os.Handler(android.os.Looper.getMainLooper()).postDelayed(() -> {
              checkAndConnectProbe();
            }, 200);
          }
        } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
          UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
          if (device != null && device.getVendorId() == 0x1dbf) {
            Log.d("Ultrasound", "USB probe detached while on scanning screen");
            
            // Reset ultrasound used flag since probe was disconnected
            if (viewmodel != null && viewmodel.getEncounterData() != null) {
              viewmodel.getEncounterData().setIsUltrasoundUsed(false);
              Log.d("Ultrasound", "Reset isUltrasoundUsed flag due to probe disconnect");
              
              // Update procedure data file to reset ultrasound used flag
              updateProcedureFile();
            }
            
            // Clean up DAU immediately
            cleanupDau();
            
            // Dismiss any open dialogs before navigation
            new android.os.Handler(android.os.Looper.getMainLooper()).post(() -> {
              if (depthDialog != null && depthDialog.isShowing()) {
                depthDialog.dismiss();
                depthDialog = null;
              }
              if (gainDialog != null && gainDialog.isShowing()) {
                gainDialog.dismiss();
                gainDialog = null;
              }
            });
            
            // Navigate back based on where user came from
            new android.os.Handler(android.os.Looper.getMainLooper()).post(() -> {
              try {
                if (getView() != null && isAdded()) {
                  androidx.navigation.NavController navController = 
                      androidx.navigation.Navigation.findNavController(getView());
                  
                  // Navigate to appropriate screen based on origin
                  if (isVenousAccess) {
                    navController.navigate(R.id.action_scanning_ultrasound_to_setup_screen);
                    Log.d("Ultrasound", "Navigated back to setup screen");
                  } else {
                    Bundle bundle = new Bundle();
                    bundle.putString("fromScreen", fromScreen);
                    Log.d("Ultrasound", "onCreateView: fromScreen" + fromScreen);
                    // Probe connected, go to scanning screen
                    androidx.navigation.Navigation.findNavController(view)
                            .navigate(R.id.action_scanning_ultrasound_to_connect_ultrasound,bundle);
                    //navController.navigate(R.id.action_scanning_ultrasound_to_connect_ultrasound);
                    Log.d("Ultrasound", "Navigated back to patient input screen");
                  }
                }
              } catch (Exception e) {
                Log.e("Ultrasound", "Error navigating after probe disconnect", e);
              }
            });
          }
        }
      }
    };
    IntentFilter attachFilter = new IntentFilter();
    attachFilter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
    attachFilter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
    requireContext().registerReceiver(usbAttachmentReceiver, attachFilter, Context.RECEIVER_NOT_EXPORTED);

    // Check if probe is already connected
    checkAndConnectProbe();
    
    // Setup zoom gesture detectors
    setupGestureDetectors();
  }
  
  /**
   * Setup gesture detectors for pinch-to-zoom and pan functionality
   */
  private void setupGestureDetectors() {
    // Scale gesture detector for pinch-to-zoom
    mScaleDetector = new ScaleGestureDetector(requireContext(),
        new ScaleGestureDetector.OnScaleGestureListener() {
          @Override
          public boolean onScaleBegin(ScaleGestureDetector detector) {
            return enableZoomAndPan;
          }

          @Override
          public boolean onScale(ScaleGestureDetector detector) {
            if (enableZoomAndPan && ultrasoundViewer != null) {
              // Apply incremental scale
              float scaleFactor = detector.getScaleFactor();
              ultrasoundViewer.handleOnScale(scaleFactor);
              
              // Update current scale
              currentScale = ultrasoundViewer.getZoomScale();
              
              // Update ruler if needed
              if (rulerView != null) {
                //rulerView.checkZoomState();
              }
              
              android.util.Log.d("Zoom", "Scale: " + currentScale);
            }
            return true;
          }

          @Override
          public void onScaleEnd(ScaleGestureDetector detector) {
            // Final UI updates after zoom ends
            if (rulerView != null) {
              //rulerView.checkZoomState();
            }
          }
        });
    
    // Gesture detector for pan when zoomed
    mGestureDetector = new GestureDetectorCompat(requireContext(),
        new GestureDetector.SimpleOnGestureListener() {
          @Override
          public boolean onScroll(MotionEvent e1, MotionEvent e2,
                                  float distanceX, float distanceY) {
            if (isZoomed() && enableZoomAndPan && ultrasoundViewer != null) {
              // Calculate pan delta based on view dimensions
              if (surfaceView != null) {
                float deltaX = -distanceX / surfaceView.getWidth();
                float deltaY = -distanceY / surfaceView.getHeight();
                
                // Apply pan
                ultrasoundViewer.handleOnTouch(deltaX, deltaY, false);
                
                // Update ruler
                if (rulerView != null) {
                  //rulerView.checkZoomState();
                }
              }
              return true;
            }
            return false;
          }
        });
    
    // Attach gesture detectors to surface view
    if (surfaceView != null) {
      surfaceView.setOnTouchListener((v, event) -> {
        boolean scaleHandled = mScaleDetector.onTouchEvent(event);
        boolean gestureHandled = mGestureDetector.onTouchEvent(event);
        return scaleHandled || gestureHandled;
      });
    }
  }
  
  /**
   * Check if image is currently zoomed
   */
  private boolean isZoomed() {
    return currentScale > 1.0f;
  }
  
  /**
   * Reset zoom to 1.0x scale and center position
   */
  private void resetZoom() {
    if (ultrasoundViewer != null) {
      // Reset scale to 1.0
      GlobalUltrasoundManager.getInstance().setUltrasoundViewerScale(requireContext(), 1.0f);
      
      // Reset pan to origin
      ultrasoundViewer.setZoomPan(0.0f, 0.0f);
      
      // Update current scale
      currentScale = 1.0f;
      
      // Update UI
      if (rulerView != null) {
        //rulerView.checkZoomState();
      }
      
      // Redraw if needed
      ultrasoundViewer.handleOnScale(1.0f);
      
      Toast.makeText(getContext(), "Zoom reset", Toast.LENGTH_SHORT).show();
      android.util.Log.d("Zoom", "Zoom reset to 1.0x");
    }
  }

  private void checkAndConnectProbe() {
    if (isCheckingProbe) {
      Log.d("Ultrasound", "checkAndConnectProbe already in progress, skipping");
      return;
    }
    
    isCheckingProbe = true;
    Log.d("Ultrasound", "checkAndConnectProbe started");
    
    UsbManager usbManager = (UsbManager) requireContext().getSystemService(Context.USB_SERVICE);
    UsbDevice device = null;

    // Find the probe device
    HashMap<String, UsbDevice> deviceList = usbManager.getDeviceList();
    for (UsbDevice usbDevice : deviceList.values()) {
      // Check for EchoNous probe (vendor ID 7615 / 0x1dbf)
      if (usbDevice.getVendorId() == 0x1dbf) {
        device = usbDevice;
        break;
      }
    }

    if (device != null) {
      // Log device details for debugging
      Log.d("Ultrasound", "Found USB device - VendorID: 0x" + Integer.toHexString(device.getVendorId()) 
              + ", ProductID: 0x" + Integer.toHexString(device.getProductId()));
      
      // Check if we have permission
      if (!usbManager.hasPermission(device)) {
           // Unregister previous permission receiver if exists
        if (usbPermissionReceiver != null) {
          try {
            requireContext().unregisterReceiver(usbPermissionReceiver);
            Log.d("Ultrasound", "Unregistered previous USB permission receiver");
          } catch (Exception e) {
            MedDevLog.error("Ultrasound", "Error unregistering previous USB permission receiver", e);
          }
        }
        // Register broadcast receiver for permission result
        final UsbDevice finalDevice = device;
        final UsbManager finalUsbManager = usbManager;
        usbPermissionReceiver = new BroadcastReceiver() {
          @Override
          public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.d("Ultrasound", "BroadcastReceiver onReceive called with action: " + action);
            if (ACTION_USB_PERMISSION.equals(action)) {
              synchronized (this) {
                UsbDevice intentDevice = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                Log.d("Ultrasound", "Intent device: " + intentDevice);
                Log.d("Ultrasound", "Original device: " + finalDevice);
                
                // Check permission on the original device we requested for
                boolean hasPermission = finalUsbManager.hasPermission(finalDevice);
                Log.d("Ultrasound", "Permission check result: " + hasPermission);
                
                if (hasPermission) {
                  Log.d("Ultrasound", "USB permission granted, connecting probe");
                  // Permission granted, now connect the probe
                  connectProbe(finalDevice);
                } else {
                  MedDevLog.error("Ultrasound", "USB permission still not granted");
                }
              }
            }
          }
        };
        
        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        requireContext().registerReceiver(usbPermissionReceiver, filter, Context.RECEIVER_NOT_EXPORTED);
        
        // Request permission
        PendingIntent permissionIntent = PendingIntent.getBroadcast(
                requireContext(), 0,
                new Intent(ACTION_USB_PERMISSION),
                PendingIntent.FLAG_IMMUTABLE
        );
        usbManager.requestPermission(device, permissionIntent);
        Log.d("Ultrasound", "Requested USB permission");
      } else {
        // Permission already granted, connect the probe
        connectProbe(device);
      }
    } else {
      Log.d("Ultrasound", "No ultrasound probe found");
      isCheckingProbe = false;
    }
  }

  private void connectProbe(UsbDevice device) {
    // Clear the checking flag as we're now connecting
    isCheckingProbe = false;
    
    // Get UltrasoundViewer now that probe is connected
    // This ensures SDK is initialized only when probe is present
    if (ultrasoundViewer == null) {
      Log.d("Ultrasound", "Getting UltrasoundViewer instance");
      ultrasoundViewer = GlobalUltrasoundManager.getInstance()
              .getUltrasoundViewer(requireContext());

      // Set the surface if it's already created and valid
      if (surfaceView != null && surfaceView.getHolder() != null && surfaceView.getHolder().getSurface() != null) {
        android.view.Surface surface = surfaceView.getHolder().getSurface();
        if (surface.isValid()) {
          Log.d("Ultrasound", "Setting surface on UltrasoundViewer");
          ultrasoundViewer.setSurface(surface);
        } else {
          MedDevLog.warn("Ultrasound", "Surface exists but is not valid yet, will be set in surfaceCreated callback");
        }
      }
      else{
        Log.d("Ultrasound", "Surface not set. Surface is null : " + (surfaceView == null)+", surfaceView.getHolder() null : "+(surfaceView != null && surfaceView.getHolder() == null)+
                "surfaceView.getHolder().getSurface() is null : "+(surfaceView != null && surfaceView.getHolder() != null && surfaceView.getHolder().getSurface() == null));
      }
    }else{
      Log.d("Ultrasound", "ultrasoundViewer is not null");
    }
    
    // Set the active probe type based on product ID
    int productId = device.getProductId();
    if (productId == 0x0101) {
      GlobalUltrasoundManager.getInstance().setActiveProbe(UltrasoundManager.PROBE_TORSO_ONE);
      Log.d("Ultrasound", "Set active probe to TORSO_ONE");
    } else if (productId == 0x0102) {
      GlobalUltrasoundManager.getInstance().setActiveProbe(UltrasoundManager.PROBE_LEXSA);
      Log.d("Ultrasound", "Set active probe to LEXSA");
    } else {
      MedDevLog.error("Ultrasound", "Unknown product ID: 0x" + Integer.toHexString(productId));
    }
    
    // Now check if DAU is connected
    boolean isConnected = GlobalUltrasoundManager.getInstance()
            .isDauConnected(UltrasoundManager.DauCommMode.Usb, device);
    MedDevLog.error("Ultrassound", "isConnected: " + isConnected);

    if (isConnected) {
        // Set the database path for native code BEFORE opening DAU
        GlobalUltrasoundManager.getInstance().setUpsDatabaseProbeWise("thor.db");
        Log.d("Ultrasound", "Set UPS database path to thor.db");
        
        // Ensure UPS is initialized and database is ready
        try {
          GlobalUltrasoundManager.getInstance().initUps(requireContext());
          Log.d("Ultrasound", "UPS initialized successfully");
        } catch (Exception e) {
          MedDevLog.error("Ultrasound", "Error initializing UPS", e);
        }
        
        var defaultIndices = UpsManager.getInstance().getDefaultIndicesByOrgan(ORGAN_GLOBAL_ID_VASCULAR_LINEAR);
        if (defaultIndices == null) {
          MedDevLog.error("Ultrasound", "Failed to get default indices for organ ID: " + ORGAN_GLOBAL_ID_VASCULAR_LINEAR);
          return;
        }
        
        var defaultViewId = defaultIndices.getDefaultView();
        var depthId = defaultIndices.getDefaultDepthId();

        Log.d("Ultrasound", "Imaging parameters: organId=" + ORGAN_GLOBAL_ID_VASCULAR_LINEAR 
                + ", viewId=" + defaultViewId + ", depthId=" + depthId);

        // Now safe to open DAU
        GlobalUltrasoundManager.getInstance().openDau(UltrasoundManager.DauCommMode.Usb);
        
        // Get actual depth ID from array (not the array index)
        var depths = getAvailableDepths();
        int actualDepthId = depths[currentDepthIndex][0];
        
        // Use current depth index
        GlobalUltrasoundManager.getInstance().setImagingCase(
            ORGAN_ID_VASCULAR_LINEAR, 0, actualDepthId, 0);
        GlobalUltrasoundManager.getInstance().attachCine();
        Log.d("Ultrasound", "Initial imaging case set with depth ID: " + actualDepthId + " (array index: " + currentDepthIndex + ")");
        // After setSurface, wait for EGL context to initialize
        new android.os.Handler(android.os.Looper.getMainLooper()).postDelayed(() -> {
          GlobalUltrasoundManager.getInstance().startDau();
          // Set initial gain values after DAU starts
          GlobalUltrasoundManager.getInstance().setGain(currentNearGain, currentFarGain);
          Log.d("Ultrasound", "Initial gain set: near=" + currentNearGain + ", far=" + currentFarGain);
          
          // Mark ultrasound as used in the procedure
          if (viewmodel != null && viewmodel.getEncounterData() != null) {
            viewmodel.getEncounterData().setIsUltrasoundUsed(true);
            Log.d("Ultrasound", "Marked ultrasound as used in procedure");
            
            // Update procedure data file with ultrasound used flag
            updateProcedureFile();
          }
        }, 100); // 100ms delay
    }
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
      @Nullable Bundle savedInstanceState) {
    FragmentScanningUltrasoundBinding binding = DataBindingUtil.inflate(inflater,
        R.layout.fragment_scanning_ultrasound, container, false);

    // Get the fromScreen argument to determine title
    if (getArguments() != null) {
      fromScreen = getArguments().getString("fromScreen", Utility.FROM_SCREEN_CONNECT_ULTRASOUND);
      isVenousAccess = getArguments().getBoolean("isVenousAccess", false);
    } else {
      fromScreen = Utility.FROM_SCREEN_CONNECT_ULTRASOUND;
      isVenousAccess = false;
    }

    // Set title based on where user came from
    String title;
    if(isVenousAccess){
      title = getString(R.string.venous_access);
    }else{
      title = getString(R.string.scanning_ultrasound);
    }    
    binding.setTitle(title);

    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    ControllerState controllerState = viewmodel.getControllerState();
    binding.topBar.setControllerState(controllerState);
    binding.topBar.setTabletState(viewmodel.getTabletState());

    tvBattery = binding.topBar.controllerBatteryLevelPct;
    ivBattery = binding.topBar.remoteBatteryLevelIndicator;
    ivUsbIcon = binding.topBar.usbIcon;
    ivTabletBattery = binding.topBar.tabletBatteryIndicator.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.topBar.tabletBatteryIndicator.tabletBatteryLevelPct;
    hgLogo = binding.topBar.appLogo;
    lpTabletIcon = binding.topBar.tabletBatteryWarningIcon.batteryIcon;
    lpTabletSymbol = binding.topBar.tabletBatteryWarningIcon.batteryWarningImg;

    binding.topBar.remoteBatteryLevelIndicator.setVisibility(View.GONE);
    binding.topBar.controllerBatteryLabel.setVisibility(View.GONE);
    binding.topBar.controllerBatteryLevelPct.setVisibility(View.GONE);
    binding.topBar.usbIcon.setVisibility(View.GONE);

    View backBtn = binding.getRoot().findViewById(R.id.back_btn);
    backBtn.setOnClickListener(v -> {
      androidx.navigation.NavController navController = androidx.navigation.Navigation
              .findNavController(binding.getRoot());
      switch (fromScreen) {        
        case Utility.FROM_SCREEN_SETUP:
        case Utility.FROM_SCREEN_BLUETOOTH_PAIRING_PATTERN:
          navController.navigate(R.id.action_scanning_ultrasound_to_setup_screen);
          break;
        case Utility.FROM_SCREEN_HOME:
          navController.navigate(R.id.action_scanning_ultrasound_to_home);
          break;        
        case Utility.FROM_SCREEN_PATIENT_INPUT:
        case Utility.FROM_SCREEN_CONNECT_ULTRASOUND:        
        default:
          navController.navigate(R.id.action_scanning_ultrasound_to_patient_input);
          break;
      }
    });

    View captureBtn = binding.getRoot().findViewById(R.id.capture_btn);
    View proceedBtn = binding.getRoot().findViewById(R.id.proceed_btn);
    android.widget.TextView buttonLabel = binding.getRoot().findViewById(R.id.button_label);

    // Hide capture button if coming from bluetooth_pairing_pattern
    if (isVenousAccess) {
      captureBtn.setVisibility(View.GONE);
      proceedBtn.setVisibility(View.VISIBLE);
      if (buttonLabel != null) {
        buttonLabel.setText("Proceed");
      }

      // Add proceed button functionality
      proceedBtn.setOnClickListener(v -> {
        androidx.navigation.Navigation.findNavController(binding.getRoot())
                .navigate(R.id.action_scanning_ultrasound_to_callibration);
      });
    } else {
      // Show capture button for other screens
      captureBtn.setVisibility(View.VISIBLE);
      proceedBtn.setVisibility(View.GONE);
      if (buttonLabel != null) {
        buttonLabel.setText("Capture");
      }
    }

    if (captureBtn != null) {
      captureBtn.setOnLongClickListener(v -> false);
      captureBtn.setOnClickListener(v -> {
        try {
          // Generate unique filename with timestamp
          String fileName = "ultrasound_" + System.currentTimeMillis();
          
          Log.d("Ultrasound", "Capturing still image in .thor format");
          
          // Disable capture button during async operation
          captureBtn.setEnabled(false);
          
          // Ensure DAU is attached to CineBuffer for recording
          // recordStillImage requires DAU to be attached to capture frames
          GlobalUltrasoundManager.getInstance().attachCineIfRequired();
          
          // Register completion listener to wait for async recording to finish
          GlobalUltrasoundManager.getInstance().setCaptureCompletionListener(completionResult -> {
            // Re-enable capture button
            captureBtn.setEnabled(true);
            
            if (completionResult.isOK()) {
              Log.d("Ultrasound", "Image capture completed successfully: " + fileName + ".thor");
              
              // Navigate to capture fragment with the filename
              Bundle args = new Bundle();
              args.putString("capturedImageFile", fileName);
              androidx.navigation.Navigation.findNavController(binding.getRoot())
                  .navigate(R.id.action_scanning_ultrasound_to_ultrasound_capture, args);
            } else {
              MedDevLog.error("Ultrasound", "Capture completion failed: " + completionResult.getDescription());
              android.widget.Toast.makeText(getContext(), 
                  "Failed to save image: " + completionResult.getDescription(), 
                  android.widget.Toast.LENGTH_SHORT).show();
            }
          });
          
          // Start async capture (returns immediately)
          com.echonous.hardware.ultrasound.ThorError result = 
              GlobalUltrasoundManager.getInstance().saveImage(fileName);
          
          if (!result.isOK()) {
            // If capture didn't even start, re-enable button and show error
            captureBtn.setEnabled(true);
            MedDevLog.error("Ultrasound", "Failed to start capture: " + result.getDescription());
            android.widget.Toast.makeText(getContext(), 
                "Failed to capture image: " + result.getDescription(), 
                android.widget.Toast.LENGTH_SHORT).show();
          } else {
            Log.d("Ultrasound", "Capture started, waiting for completion...");
          }
        } catch (Exception e) {
          captureBtn.setEnabled(true);
          MedDevLog.error("Ultrasound", "Error capturing image", e);
          android.widget.Toast.makeText(getContext(), 
              "Error capturing image", android.widget.Toast.LENGTH_SHORT).show();
        }
      });
    }

    View depthControl = binding.getRoot().findViewById(R.id.depth_control);
    depthLabel = depthControl.findViewById(R.id.depth_label);
    View gainControl = binding.getRoot().findViewById(R.id.gain_control);
    View centerLineControl = binding.getRoot().findViewById(R.id.center_line_control);
    centerLineOverlay = binding.getRoot().findViewById(R.id.center_line_overlay_view);
    View zoomResetControl = binding.getRoot().findViewById(R.id.zoom_reset_control);
    
    // Load saved center line state
    if (centerLineOverlay != null) {
      boolean savedState = CenterLineOverlay.getCenterLineEnabled(requireContext());
      centerLineOverlay.setCenterLineEnabled(savedState);
      updateCenterLineControlUI(centerLineControl, savedState);
    }
    
    // Depth and gain labels will be set in onViewCreated() after loading from preferences

    depthControl.setOnClickListener(v -> {
      showSelectDepthDialog();
    });

    gainControl.setOnClickListener(v -> {
      Log.d("ScanningUltrasound", "Gain control clicked");
      showSelectGainDialog();
    });

    centerLineControl.setOnClickListener(v -> {
      Log.d("ScanningUltrasound", "Center Line control clicked");
      if (centerLineOverlay != null) {
        boolean newState = centerLineOverlay.toggleCenterLine();
        updateCenterLineControlUI(centerLineControl, newState);
        Toast.makeText(requireContext(), 
          newState ? "Center line enabled" : "Center line disabled", 
          Toast.LENGTH_SHORT).show();
      }
    });
    
    zoomResetControl.setOnClickListener(v -> {
      Log.d("ScanningUltrasound", "Zoom Reset control clicked");
      resetZoom();
    });

    return binding.getRoot();
  }

  private void showSelectDepthDialog() {
    //TODO : Replace with dynamic depths from UpsManager, but upsManager needs to get it from l38.db, currently using thor.db
    var depths = getAvailableDepths();
    for (Integer[] depth : depths) {
      Log.d("Depths", "showSelectDepthDialog: "+depth[0]+","+depth[1]);
    }
    android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(requireContext());
    LayoutInflater inflater = requireActivity().getLayoutInflater();
    View dialogView = inflater.inflate(R.layout.dialog_select_depth, null);

    final Integer minDepth = Arrays.stream(depths)
            .min(Comparator.comparingInt(row -> row[1]))
            .map(row -> row[0])
            .orElse(0);
    final Integer maxDepth = Arrays.stream(depths)
            .max(Comparator.comparingInt(row -> row[1]))
            .map(row -> row[0])
            .orElse(0);
    final int[] selectedIndex = { currentDepthIndex }; // Initialize with current depth

    android.widget.TextView depthSelected = dialogView.findViewById(R.id.depth_selected);
    android.widget.TextView depthAbove = dialogView.findViewById(R.id.depth_above);
    android.widget.TextView depthBelow = dialogView.findViewById(R.id.depth_below);
    android.widget.ImageView btnUp = dialogView.findViewById(R.id.arrow_up);
    android.widget.ImageView btnDown = dialogView.findViewById(R.id.arrow_down);
    android.widget.ImageButton btnDelete = dialogView.findViewById(R.id.btn_depth_delete);
    android.widget.ImageButton btnConfirm = dialogView.findViewById(R.id.btn_depth_confirm);
    android.widget.LinearLayout depthSelectionColumn = dialogView.findViewById(R.id.depth_selection_column);

    DepthChangeAnimator animateDepthChange = (direction) -> {
      // direction: 1 for up, -1 for down
      depthSelected.animate()
          .translationY(direction * -60)
          .alpha(0f)
          .setDuration(280)
          .setInterpolator(new android.view.animation.AccelerateDecelerateInterpolator())
          .withEndAction(() -> {
            depthSelected.setTranslationY(direction * 60);
            int idx = selectedIndex[0];
            depthSelected.setText(depths[idx][1]/10 + "cm");
            depthSelected.animate()
                .translationY(0)
                .alpha(1f)
                .setDuration(280)
                .setInterpolator(new android.view.animation.AccelerateDecelerateInterpolator())
                .start();
          })
          .start();
    };

    Runnable updateDepth = () -> {
      int idx = selectedIndex[0];
      depthSelected.setText(depths[idx][1]/10 + "cm");
      if (idx > 0) {
        depthAbove.setText((depths[idx - 1][1]/10 + "cm"));
        depthAbove.setAlpha(0.6f);
      } else {
        depthAbove.setText("");
        depthAbove.setAlpha(0f);
      }
      if (idx < (maxDepth - minDepth)) {
        depthBelow.setText((depths[idx + 1][1]/10 + "cm"));
        depthBelow.setAlpha(0.6f);
      } else {
        depthBelow.setText("");
        depthBelow.setAlpha(0f);
      }
    };
    updateDepth.run();

    btnUp.setOnClickListener(v -> {
      if (selectedIndex[0] > 0) {
        selectedIndex[0]--;
        animateDepthChange.run(-1);
        updateDepth.run();
      }
    });
    btnDown.setOnClickListener(v -> {
      if (selectedIndex[0] < (maxDepth - minDepth)) {
        selectedIndex[0]++;
        animateDepthChange.run(1);
        updateDepth.run();
      }
    });

    // Use GestureDetector for swipe detection
    final android.view.GestureDetector gestureDetector = new android.view.GestureDetector(dialogView.getContext(),
        new android.view.GestureDetector.SimpleOnGestureListener() {
          private float lastY = -1;

          @Override
          public boolean onFling(android.view.MotionEvent e1, android.view.MotionEvent e2, float velocityX,
              float velocityY) {
            float deltaY = e2.getY() - e1.getY();
            Log.d("DepthGesture", "onFling deltaY=" + deltaY + ", velocityY=" + velocityY);
            if (Math.abs(deltaY) > 20) {
              if (deltaY < 0 && selectedIndex[0] < (maxDepth - minDepth)) {
                selectedIndex[0]++;
                Log.d("DepthGesture", "Fling UP: new index=" + selectedIndex[0]);
                animateDepthChange.run(1);
                updateDepth.run();
                return true;
              } else if (deltaY > 0 && selectedIndex[0] > 0) {
                selectedIndex[0]--;
                Log.d("DepthGesture", "Fling DOWN: new index=" + selectedIndex[0]);
                animateDepthChange.run(-1);
                updateDepth.run();
                return true;
              }
            }
            return false;
          }

          @Override
          public boolean onScroll(android.view.MotionEvent e1, android.view.MotionEvent e2, float distanceX,
              float distanceY) {
            if (lastY == -1) {
              lastY = e1.getY();
            }
            float deltaY = e2.getY() - lastY;
            Log.d("DepthGesture", "onScroll deltaY=" + deltaY + ", distanceY=" + distanceY);
            if (Math.abs(deltaY) > 40) {
              if (deltaY < 0 && selectedIndex[0] < (maxDepth - minDepth)) {
                selectedIndex[0]++;
                Log.d("DepthGesture", "Scroll UP: new index=" + selectedIndex[0]);
                animateDepthChange.run(1);
                updateDepth.run();
                lastY = e2.getY();
                return true;
              } else if (deltaY > 0 && selectedIndex[0] > 0) {
                selectedIndex[0]--;
                Log.d("DepthGesture", "Scroll DOWN: new index=" + selectedIndex[0]);
                animateDepthChange.run(-1);
                updateDepth.run();
                lastY = e2.getY();
                return true;
              }
            }
            return false;
          }

          @Override
          public boolean onDown(android.view.MotionEvent e) {
            lastY = e.getY();
            Log.d("DepthGesture", "onDown y=" + lastY);
            return true;
          }
        });
    depthSelectionColumn.setClickable(true);
    depthSelectionColumn.setFocusable(true);
    depthSelectionColumn.setOnTouchListener((v, event) -> gestureDetector.onTouchEvent(event));

    btnDelete.setOnClickListener(v -> {
      // Reset to default depth (1cm = index 0)
      selectedIndex[0] = 0;
      updateDepth.run();
      applyDepthChange(0);
      if (depthDialog != null) {
        depthDialog.dismiss();
        depthDialog = null;
      }
    });
    btnConfirm.setOnClickListener(v -> {
      // Apply the selected depth change
      applyDepthChange(selectedIndex[0]);
      if (depthDialog != null) {
        depthDialog.dismiss();
        depthDialog = null;
      }
    });

    builder.setView(dialogView);
    depthDialog = builder.create();
    dialogView.setTag(depthDialog);
    depthDialog.show();
    int widthDp = 500;
    int heightDp = 500;
    int widthPx = (int) (widthDp * getResources().getDisplayMetrics().density);
    int heightPx = (int) (heightDp * getResources().getDisplayMetrics().density);
    if (depthDialog.getWindow() != null) {
      depthDialog.getWindow().setLayout(widthPx, heightPx);
      depthDialog.getWindow()
          .setBackgroundDrawable(new android.graphics.drawable.ColorDrawable(android.graphics.Color.TRANSPARENT));
      depthDialog.getWindow().setGravity(android.view.Gravity.CENTER);
    }
  }

  private void calculateNearFarGain(int overallGain, android.widget.SeekBar nearSeekBar, android.widget.SeekBar farSeekBar) {
    if (advanceGainInProgress) {
      lastOverallGain = overallGain;
      return;
    }

    if (lastOverallGain > overallGain) {
      // Decreasing gain (moving LEFT)
      currentDirection = Direction.LEFT;

      if (overallGain == MIN_GAIN_VALUE) {
        absNearGain = MIN_GAIN_VALUE;
        absFarGain = MIN_GAIN_VALUE;
      } else {
        int gainDif = lastOverallGain - overallGain;

        if (absNearGain > MIN_GAIN_VALUE && overallGain > 0 && absFarGain == MIN_GAIN_VALUE) {
          absNearGain = overallGain * 2;
        } else if (absNearGain > MIN_GAIN_VALUE) {
          absNearGain -= gainDif;
          if (absNearGain < MIN_GAIN_VALUE) {
            absNearGain = MIN_GAIN_VALUE;
          }
        }

        if (absFarGain > MIN_GAIN_VALUE && overallGain > MIN_GAIN_VALUE && absNearGain == MIN_GAIN_VALUE) {
          absFarGain = overallGain * 2;
        } else if (absFarGain > MIN_GAIN_VALUE) {
          absFarGain -= gainDif;
          if (absFarGain < MIN_GAIN_VALUE) {
            absFarGain = MIN_GAIN_VALUE;
          }
        }
      }
    } else if (lastOverallGain < overallGain) {
      // Increasing gain (moving RIGHT)
      currentDirection = Direction.RIGHT;

      if (overallGain == MAX_GAIN_VALUE) {
        absNearGain = MAX_GAIN_VALUE;
        absFarGain = MAX_GAIN_VALUE;
      } else {
        int overallRest = MAX_GAIN_VALUE - overallGain;
        int overallDif = overallGain - lastOverallGain;

        if (absNearGain < MAX_GAIN_VALUE && overallRest > 0 && absFarGain == MAX_GAIN_VALUE) {
          absNearGain = overallGain - overallRest;
        } else if (absNearGain < MAX_GAIN_VALUE) {
          absNearGain += overallDif;
          if (absNearGain > MAX_GAIN_VALUE) {
            absNearGain = MAX_GAIN_VALUE;
          }
        }

        if (absFarGain < MAX_GAIN_VALUE && overallRest > 0 && absNearGain == MAX_GAIN_VALUE) {
          absFarGain = overallGain - overallRest;
        } else if (absFarGain < MAX_GAIN_VALUE) {
          absFarGain += overallDif;
          if (absFarGain > MAX_GAIN_VALUE) {
            absFarGain = MAX_GAIN_VALUE;
          }
        }
      }
    } else {
      currentDirection = Direction.NONE;
    }

    // Update the seekbars
    nearSeekBar.setProgress(absNearGain);
    farSeekBar.setProgress(absFarGain);

    lastOverallGain = overallGain;

    // Apply to probe
    try {
      GlobalUltrasoundManager.getInstance().setGain(absNearGain, absFarGain);
      currentNearGain = absNearGain;
      currentFarGain = absFarGain;
      
      // Save to preferences
      PrefsManager.setNearGain(requireContext(), absNearGain);
      PrefsManager.setFarGain(requireContext(), absFarGain);
      
      Log.d("Ultrasound", "Applied gain from overall: near=" + absNearGain + ", far=" + absFarGain);
    } catch (Exception e) {
      MedDevLog.error("Ultrasound", "Error setting gain", e);
    }
  }

  private void applyDepthChange(int depthIndex) {
    try {
      // Store the current depth
      currentDepthIndex = depthIndex;
      
      // Save to preferences
      PrefsManager.setSelectedDepthIndex(requireContext(), depthIndex);
      
      // Stop DAU before changing imaging case
      GlobalUltrasoundManager.getInstance().stopDau();
      
      // Get actual depth ID from array (not the array index)
      var depths = getAvailableDepths();
      int actualDepthId = depths[depthIndex][0];
      
      // Apply to probe - setImagingCase needs organId, viewId, depthId, mode
      GlobalUltrasoundManager.getInstance().setImagingCase(
          ORGAN_ID_VASCULAR_LINEAR, // organId
          0, // viewId (using default)
          actualDepthId, // depthId (actual SDK depth ID: 1=20mm, 2=30mm, etc.)
          0  // mode
      );

      Log.d("Ultrasound", "applyDepthChange: array index=" + depthIndex + ", actual depth ID=" + actualDepthId);
      
      // Reapply current gain settings
      GlobalUltrasoundManager.getInstance().setGain(currentNearGain, currentFarGain);
      GlobalUltrasoundManager.getInstance().attachCine();
      // Restart DAU
      GlobalUltrasoundManager.getInstance().startDau();
      
      // Update ruler display immediately
      updateRulerDepth();
      
      // Update depth label with actual cm value from depths array
      if (depthLabel != null) {
        if (depthIndex >= 0 && depthIndex < depths.length) {
          depthLabel.setText(depths[depthIndex][1]/10 + "cm Depth");
        }
      }
      
      // Delay margin adjustment until imaging case change completes
      // SDK needs time to update display bounds after setImagingCase()
      new android.os.Handler(android.os.Looper.getMainLooper()).postDelayed(() -> {        
        View surfaceHolder = getView().findViewById(R.id.surface_holder);
        if (surfaceHolder == null) {
          return;
        }
        surfaceHolder.requestLayout();
      }, 200); // 200ms delay for SDK to stabilize
      
      Log.d("Ultrasound", "Applied depth change: array index=" + depthIndex + ", depth ID=" + actualDepthId + " (" + depths[depthIndex][1]/10 + "cm)");
    } catch (Exception e) {
      MedDevLog.error("Ultrasound", "Error applying depth change", e);
    }
  }

  private void showSelectGainDialog() {
    android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(requireContext());
    LayoutInflater inflater = requireActivity().getLayoutInflater();
    View dialogView = inflater.inflate(R.layout.dialog_select_gain, null);

    android.widget.SeekBar seekbarNear = dialogView.findViewById(R.id.seekbar_near_gain);
    android.widget.SeekBar seekbarGain = dialogView.findViewById(R.id.seekbar_gain);
    android.widget.SeekBar seekbarFar = dialogView.findViewById(R.id.seekbar_far_gain);
    android.widget.TextView nearValue = dialogView.findViewById(R.id.near_gain_value);
    android.widget.TextView gainValue = dialogView.findViewById(R.id.gain_value);
    android.widget.TextView farValue = dialogView.findViewById(R.id.far_gain_value);
    android.widget.ImageButton btnDelete = dialogView.findViewById(R.id.btn_delete);
    android.widget.ImageButton btnConfirm = dialogView.findViewById(R.id.btn_confirm);

    // Store original gain values to restore if dialog is cancelled
    final int originalNearGain = currentNearGain;
    final int originalFarGain = currentFarGain;
    
    // Initialize abs gain values from current settings
    absNearGain = currentNearGain;
    absFarGain = currentFarGain;
    int overallGain = (currentNearGain + currentFarGain) / 2;
    lastOverallGain = overallGain;
    advanceGainInProgress = false;
    
    // Set initial values from current gain settings
    seekbarNear.setProgress(currentNearGain);
    seekbarGain.setProgress(overallGain); // Average for display
    seekbarFar.setProgress(currentFarGain);
    
    // Update text values
    nearValue.setText(currentNearGain + "%");
    gainValue.setText(overallGain + "%");
    farValue.setText(currentFarGain + "%");

    android.widget.SeekBar.OnSeekBarChangeListener listener = new android.widget.SeekBar.OnSeekBarChangeListener() {
      @Override
      public void onProgressChanged(android.widget.SeekBar seekBar, int progress,
          boolean fromUser) {
        if (seekBar == seekbarNear) {
          nearValue.setText(progress + "%");
        } else if (seekBar == seekbarGain) {
          gainValue.setText(progress + "%");
        } else if (seekBar == seekbarFar) {
          farValue.setText(progress + "%");
        }
        
        // Apply gain changes to probe in real-time if changed by user
        if (fromUser) {
          if (seekBar == seekbarGain) {
            // Overall gain changed - use sophisticated calculation
            calculateNearFarGain(progress, seekbarNear, seekbarFar);
            // Update text values after calculation
            nearValue.setText(absNearGain + "%");
            farValue.setText(absFarGain + "%");
          } else {
            // Individual near/far gain changed - direct update
            advanceGainInProgress = true;
            try {
              int nearGain = seekbarNear.getProgress();
              int farGain = seekbarFar.getProgress();
              
              absNearGain = nearGain;
              absFarGain = farGain;
              
              GlobalUltrasoundManager.getInstance().setGain(nearGain, farGain);
              
              // Update stored values
              currentNearGain = nearGain;
              currentFarGain = farGain;
              
              // Save to preferences
              PrefsManager.setNearGain(requireContext(), nearGain);
              PrefsManager.setFarGain(requireContext(), farGain);
              
              // Update middle gain display to show average
              int avgGain = (nearGain + farGain) / 2;
              seekbarGain.setProgress(avgGain);
              lastOverallGain = avgGain;
              
              Log.d("Ultrasound", "Applied gain (direct): near=" + nearGain + ", far=" + farGain);
            } catch (Exception e) {
              MedDevLog.error("Ultrasound", "Error setting gain", e);
            } finally {
              advanceGainInProgress = false;
            }
          }
        }
      }

      @Override
      public void onStartTrackingTouch(android.widget.SeekBar seekBar) {
      }

      @Override
      public void onStopTrackingTouch(android.widget.SeekBar seekBar) {
      }
    };
    seekbarNear.setOnSeekBarChangeListener(listener);
    seekbarGain.setOnSeekBarChangeListener(listener);
    seekbarFar.setOnSeekBarChangeListener(listener);

    builder.setView(dialogView);
    gainDialog = builder.create();

    // Flag to track if dialog was explicitly confirmed or reset
    final boolean[] dialogConfirmed = {false};

    btnDelete.setOnClickListener(v -> {
      dialogConfirmed[0] = true;
      seekbarNear.setProgress(100);
      seekbarGain.setProgress(100);
      seekbarFar.setProgress(100);
      
      // Reset probe gain and all stored values
      try {
        GlobalUltrasoundManager.getInstance().setGain(100, 100);
        currentNearGain = 100;
        currentFarGain = 100;
        absNearGain = 100;
        absFarGain = 100;
        lastOverallGain = 100;
        
        // Save to preferences
        PrefsManager.setNearGain(requireContext(), 100);
        PrefsManager.setFarGain(requireContext(), 100);
        
        // Update gain label on main screen
        android.widget.TextView gainLabel = getView().findViewById(R.id.gain_value);
        if (gainLabel != null) {
          gainLabel.setText("100%");
        }
        
        Log.d("Ultrasound", "Reset gain to defaults: near=100, far=100");
      } catch (Exception e) {
        MedDevLog.error("Ultrasound", "Error resetting gain", e);
      }
      
      if (gainDialog != null) {
        gainDialog.dismiss();
        gainDialog = null;
      }
    });
    btnConfirm.setOnClickListener(v -> {
      dialogConfirmed[0] = true;
      android.widget.TextView gainLabel = getView().findViewById(R.id.gain_value);
      if (gainLabel != null) {
        gainLabel.setText(seekbarGain.getProgress() + "%");
      }
      if (gainDialog != null) {
        gainDialog.dismiss();
        gainDialog = null;
      }
    });

    // Handle dialog dismissal (clicked outside) - restore original values
    gainDialog.setOnDismissListener(dialogInterface -> {
      // Only restore if dialog was not explicitly confirmed
      if (!dialogConfirmed[0]) {
        // Restore original gain values if dialog was cancelled (not confirmed)
        try {
          GlobalUltrasoundManager.getInstance().setGain(originalNearGain, originalFarGain);
          currentNearGain = originalNearGain;
          currentFarGain = originalFarGain;
          absNearGain = originalNearGain;
          absFarGain = originalFarGain;
          lastOverallGain = (originalNearGain + originalFarGain) / 2;
          
          // Save to preferences
          PrefsManager.setNearGain(requireContext(), originalNearGain);
          PrefsManager.setFarGain(requireContext(), originalFarGain);
          
          Log.d("Ultrasound", "Dialog dismissed - restored original gain: near=" + originalNearGain + ", far=" + originalFarGain);
        } catch (Exception e) {
          MedDevLog.error("Ultrasound", "Error restoring gain on dialog dismiss", e);
        }
      }
    });

    if (gainDialog != null) {
      gainDialog.show();
    }
  }

  private void cleanupDau() {
    Log.d("Ultrasound", "cleanupDau: Stopping DAU");
    
    // Only stop DAU, don't close it
    // Closing DAU tries to access USB device which may already be disconnected
    // causing LIBUSB_ERROR_NO_DEVICE and mutex errors
    try {
      GlobalUltrasoundManager globalManager = GlobalUltrasoundManager.getInstance();
      if (globalManager != null) {
        globalManager.stopDau();
        globalManager.closeDau();
        //globalManager.clean();
        Log.d("Ultrasound", "DAU stopped successfully");
      }
    } catch (Exception e) {
      MedDevLog.error("Ultrasound", "Error stopping DAU", e);
    }
    
    // Reset ultrasound viewer reference so it can be recreated when probe reconnects
    ultrasoundViewer = null;
  }

  @Override
  public void onDestroyView() {
    super.onDestroyView();
    Log.d("Ultrasound", "onDestroyView: Cleaning up ultrasound resources");
    
    // Unregister USB permission receiver
    if (usbPermissionReceiver != null) {
      try {
        requireContext().unregisterReceiver(usbPermissionReceiver);
        usbPermissionReceiver = null;
        Log.d("Ultrasound", "USB permission receiver unregistered");
      } catch (Exception e) {
        MedDevLog.error("Ultrasound", "Error unregistering USB permission receiver", e);
      }
    }
    
    // Unregister USB attachment receiver
    if (usbAttachmentReceiver != null) {
      try {
        requireContext().unregisterReceiver(usbAttachmentReceiver);
        usbAttachmentReceiver = null;
        Log.d("Ultrasound", "USB attachment receiver unregistered");
      } catch (Exception e) {
        MedDevLog.error("Ultrasound", "Error unregistering USB attachment receiver", e);
      }
    }
    
    // Clean up DAU
    cleanupDau();
    
    // Clean up surface view
    if (surfaceView != null && surfaceView.getHolder() != null) {
      surfaceView.getHolder().removeCallback(null);
    }
    
    // Reset references
    surfaceView = null;
  }

  @Override
  public void onPause() {
    super.onPause();
    Log.d("Ultrasound", "onPause: Pausing ultrasound");
    
    // Reset checking flag so onResume can retry if permission was granted
    if (isCheckingProbe) {
      Log.d("Ultrasound", "Resetting isCheckingProbe flag in onPause");
      isCheckingProbe = false;
    }
    
    // Remove keep screen on flag
    if (getActivity() != null && getActivity().getWindow() != null) {
      getActivity().getWindow().clearFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
      Log.d("Ultrasound", "Screen keep awake disabled");
    }
    
    // Optionally freeze imaging when paused
    try {
      GlobalUltrasoundManager globalManager = GlobalUltrasoundManager.getInstance();
      if (globalManager != null) {
        globalManager.freeze();
      }
    } catch (Exception e) {
      MedDevLog.error("Ultrasound", "Error freezing DAU", e);
    }
  }

  @Override
  public void onResume() {
    super.onResume();
    Log.d("Ultrasound", "onResume: Resuming ultrasound");
    
    // Keep screen on during ultrasound scanning
    if (getActivity() != null && getActivity().getWindow() != null) {
      getActivity().getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
      Log.d("Ultrasound", "Screen keep awake enabled");
    }
    
    // If viewer not initialized yet, check for probe and request connection
    // Permission dialog may not trigger onPause, so reset flag here if needed
    if (ultrasoundViewer == null) {
      Log.d("Ultrasound", "Viewer not initialized, checking for probe connection");
      // Reset flag to allow retry after permission dialog
      if (isCheckingProbe) {
        Log.d("Ultrasound", "Resetting isCheckingProbe flag to allow permission retry");
        isCheckingProbe = false;
      }
      checkAndConnectProbe();
      return;
    }
    
    //checkAndConnectProbe();
    try {
      GlobalUltrasoundManager globalManager = GlobalUltrasoundManager.getInstance();
      if (globalManager != null) {
        // Close player file if open (navigating back from capture)
        if (globalManager.getUltraSoundPlayer() != null && 
            globalManager.getUltraSoundPlayer().isRawFileOpen()) {
          Log.d("Ultrasound", "Closing player file from previous capture view");
          globalManager.getUltraSoundPlayer().closeRawFile();
        }
        
        // Detach player if attached
        if (globalManager.getUltraSoundPlayer() != null && 
            globalManager.getUltraSoundPlayer().isCineAttached()) {
          Log.d("Ultrasound", "Detaching player from CineBuffer");
          globalManager.getUltraSoundPlayer().detachCine();
        }
        
        // Check if surface is valid before restarting DAU
        boolean isSurfaceValid = surfaceView != null && 
                                 surfaceView.getHolder() != null && 
                                 surfaceView.getHolder().getSurface() != null &&
                                 surfaceView.getHolder().getSurface().isValid();
        
        if (isSurfaceValid) {
          // Reattach DAU to CineBuffer and restart
          if (!globalManager.isDauStart()) {
            Log.d("Ultrasound", "Reattaching DAU and restarting live streaming");
            globalManager.startDau();
            surfaceView.requestLayout();
          } else {
            // DAU is already started but might be frozen
            Log.d("Ultrasound", "Unfreezing DAU");
            globalManager.unfreeze();
          }
        } else {
            Log.d("Ultrasound", "Surface not valid yet, will restart in surfaceCreated callback");
        }
      }
    } catch (Exception e) {
      MedDevLog.error("Ultrasound", "Error resuming ultrasound", e);
    }
  }
  
  /**
   * Update ruler view with current depth settings
   * Gets actual mm value from depth array
   */
  private void updateRulerDepth() {
    if (rulerView != null) {
      // Get actual depth in mm from array
      var depths = getAvailableDepths();
      if (currentDepthIndex >= 0 && currentDepthIndex < depths.length) {
        float currentDepthMM = depths[currentDepthIndex][1];
        rulerView.setMinMaxMMValues(0f, currentDepthMM);
        Log.d("Ultrasound_Margin", "Updated ruler: 0-" + currentDepthMM + "mm (depth index: " + currentDepthIndex + ", depth ID: " + depths[currentDepthIndex][0] + ")");
      }
    }
  }
  
  /**
   * Update center line control UI to reflect current state
   */
  private void updateCenterLineControlUI(View centerLineControl, boolean enabled) {
    if (centerLineControl == null) return;
    
    ImageView icon = centerLineControl.findViewById(R.id.center_line_icon);
    if (icon != null) {
      icon.setImageResource(enabled ? R.drawable.ic_center_line_on : R.drawable.ic_center_line_off);
      icon.setAlpha(enabled ? 1.0f : 0.6f);
    }
    
    // Optional: Update background to show selected state
    centerLineControl.setSelected(enabled);
  }
}
