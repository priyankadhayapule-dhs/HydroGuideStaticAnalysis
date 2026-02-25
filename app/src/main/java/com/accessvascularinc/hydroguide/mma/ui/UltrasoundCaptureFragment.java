package com.accessvascularinc.hydroguide.mma.ui;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.app.Dialog;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.PixelCopy;
import android.os.Handler;
import android.os.Looper;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

import android.widget.CheckBox;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.GestureDetector;

import androidx.core.view.GestureDetectorCompat;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.databinding.DataBindingUtil;
import androidx.lifecycle.ViewModelProvider;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentUltrasoundCaptureBinding;
import com.accessvascularinc.hydroguide.mma.model.ControllerState;
import com.accessvascularinc.hydroguide.mma.model.DataFiles;
import com.accessvascularinc.hydroguide.mma.model.EncounterData;
import com.accessvascularinc.hydroguide.mma.model.EncounterState;
import com.accessvascularinc.hydroguide.mma.model.Facility;
import com.accessvascularinc.hydroguide.mma.model.MeasurementModel;
import com.accessvascularinc.hydroguide.mma.model.PatientData;
import com.accessvascularinc.hydroguide.mma.model.ProfileState;
import com.accessvascularinc.hydroguide.mma.model.TabletState;
import com.accessvascularinc.hydroguide.mma.ultrasound.GlobalUltrasoundManager;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.echonous.hardware.ultrasound.UltrasoundEncoder;
import com.echonous.hardware.ultrasound.UltrasoundPlayer;
import com.echonous.hardware.ultrasound.UltrasoundViewer;
import com.echonous.hardware.ultrasound.ThorError;

public class UltrasoundCaptureFragment extends BaseHydroGuideFragment {
    private static final String TAG = "UltrasoundCapture";

    private MainViewModel viewmodel;
    private String selectedCatheterSize = null;
    private UltrasoundPlayer player;
    private UltrasoundViewer viewer;
    private SurfaceView surfaceView;

    // Measurement components
    private MeasurementOverlay measurementOverlay;
    private CenterLineOverlay centerLineOverlay;
    private boolean isMeasurementMode = false;
    private float[] transformationMatrix;

    // Magnifier components
    private MagnifierView magnifierView;
    private Handler magnifierUpdateHandler;
    private Runnable magnifierUpdateRunnable;
    private volatile Bitmap currentMergedBitmap;

    // Zoom functionality
    private ScaleGestureDetector mScaleDetector;
    private GestureDetectorCompat mGestureDetector;
    private boolean enableZoomAndPan = true;
    private float currentScale = 1.0f;

    private String NumberOfLumen = null;
    private BroadcastReceiver usbDetachmentReceiver;
    
    // Dialog tracking for cleanup on probe detachment
    private Dialog catheterSizeDialog;

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        FragmentUltrasoundCaptureBinding binding = DataBindingUtil.inflate(inflater,
                R.layout.fragment_ultrasound_capture, container, false);

        String title = getString(R.string.ultrasound_capture_title);
        if (getArguments() != null && getArguments().containsKey("title")) {
            String argTitle = getArguments().getString("title");
            if (argTitle != null && !argTitle.isEmpty()) {
                title = argTitle;
            }
        }

        binding.setTitle(title);

        viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
        if (viewmodel != null && viewmodel.getControllerState() != null) {
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
        } else {
            MedDevLog.error("UltrasoundCaptureFragment", "MainViewModel or ControllerState is null");
            android.widget.Toast.makeText(getContext(),
                    "Controller not available. Please restart the app.",
                    android.widget.Toast.LENGTH_LONG).show();
        }

        // Ensure ViewModel is initialized
        View backBtn = binding.getRoot().findViewById(R.id.back_btn);
        backBtn.setOnClickListener(
                v -> androidx.navigation.Navigation.findNavController(binding.getRoot())
                        .navigate(R.id.action_ultrasound_capture_to_scanning_ultrasound));

        // Register USB detachment receiver
        usbDetachmentReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
                    UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                    if (device != null && device.getVendorId() == 0x1dbf) {
                        MedDevLog.debug(TAG, "USB ultrasound probe detached - navigating back");
                        
                        // Dismiss any open dialogs before navigation
                        new Handler(Looper.getMainLooper()).post(() -> {
                            if (catheterSizeDialog != null && catheterSizeDialog.isShowing()) {
                                catheterSizeDialog.dismiss();
                                catheterSizeDialog = null;
                            }
                        });
                        
                        // Navigate back to connect ultrasound screen
                        new Handler(Looper.getMainLooper()).post(() -> {
                            try {
                                if (getView() != null && isAdded()) {
                                    androidx.navigation.NavController navController = 
                                        androidx.navigation.Navigation.findNavController(getView());
                                    navController.navigate(R.id.action_ultrasound_capture_to_connect_ultrasound);
                                    MedDevLog.debug(TAG, "Navigated to connect ultrasound screen");
                                }
                            } catch (Exception e) {
                                MedDevLog.error(TAG, "Error navigating after USB detachment", e);
                            }
                        });
                    }
                }
            }
        };
        IntentFilter usbFilter = new IntentFilter();
        usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        requireContext().registerReceiver(usbDetachmentReceiver, usbFilter, Context.RECEIVER_NOT_EXPORTED);

        View proceedBtn = binding.getRoot().findViewById(R.id.proceed_btn);
        proceedBtn.setOnClickListener(v -> {
            // Check if any ultrasound captures have been saved
            EncounterData procedure = viewmodel.getEncounterData();
            
            // Save catheter size and number of lumens to patient data
            if (selectedCatheterSize != null && !selectedCatheterSize.isEmpty()) {
                // Extract numeric value from "X French" format for database storage
                String numericCatheterSize = selectedCatheterSize;
                if (numericCatheterSize.contains("French")) {
                    numericCatheterSize = numericCatheterSize.replace("French", "").trim();
                }
                procedure.getPatient().setPatientCatheterSize(numericCatheterSize);
                MedDevLog.debug(TAG, "Saved catheter size to patient data: " + numericCatheterSize);
            }
            if (NumberOfLumen != null && !NumberOfLumen.isEmpty()) {
                procedure.getPatient().setPatientNoOfLumens(NumberOfLumen);
                MedDevLog.debug(TAG, "Saved number of lumens to patient data: " + NumberOfLumen);
            }
            
            if (procedure.getUltrasoundCaptureCount() == 0) {
                // Warn user that no captures have been saved
                showNoCaptureWarningDialog(() -> {
                    // User confirmed - proceed without saving
                    androidx.navigation.Navigation.findNavController(binding.getRoot())
                            .navigate(R.id.action_ultrasound_capture_to_setup_screen);
                });
            } else {
                androidx.navigation.Navigation.findNavController(binding.getRoot())
                        .navigate(R.id.action_ultrasound_capture_to_setup_screen);
            }
        });

        // Catheter size selection dialog logic
        View catheterSizeOption = binding.getRoot().findViewById(R.id.catheter_size_option);
        View selectedCatheterContainer = binding.getRoot().findViewById(R.id.selected_catheter_size_container);
        android.widget.TextView selectedCatheterText = binding.getRoot().findViewById(R.id.selected_catheter_size_text);
        android.widget.TextView lumnenNumber = binding.getRoot().findViewById(R.id.lumes_text);

        // Show/hide selected catheter size if already selected
        updateSelectedCatheterSizeUI(selectedCatheterContainer, selectedCatheterText, lumnenNumber);

        catheterSizeOption.setOnClickListener(
                v -> showCatheterSizeDialog(selectedCatheterContainer, selectedCatheterText,
                        lumnenNumber));

        // --- Measurement Tool Setup ---
        measurementOverlay = binding.getRoot().findViewById(R.id.measurement_overlay);
        centerLineOverlay = binding.getRoot().findViewById(R.id.center_line_overlay);
        
        // Log center line state
        if (centerLineOverlay != null) {
            android.util.Log.d(TAG, "Found centerLineOverlay, enabled=" + centerLineOverlay.isCenterLineEnabled());
        } else {
            android.util.Log.e(TAG, "centerLineOverlay is NULL after findViewById!");
        }
        View measurementToolOption = binding.getRoot().findViewById(R.id.measurement_tool_option);
        ImageView measurementToolIcon = binding.getRoot().findViewById(R.id.measurement_tool_icon);

        // Set up measurement tool click listener
        measurementToolOption.setOnClickListener(v -> toggleMeasurementMode(measurementToolIcon));

        // Set up clear measurements button
        View clearMeasurementsOption = binding.getRoot().findViewById(R.id.clear_measurements_option);
        clearMeasurementsOption.setOnClickListener(v -> clearAllMeasurements());

        // Set up save capture button
        View saveCaptureOption = binding.getRoot().findViewById(R.id.save_capture_option);
        TextView saveCaptureText = binding.getRoot().findViewById(R.id.save_capture_text);
        ImageView saveCaptureIcon = binding.getRoot().findViewById(R.id.save_capture_icon);
        updateSaveCaptureButtonState(saveCaptureText, saveCaptureIcon);
        saveCaptureOption.setOnClickListener(
                v -> saveUltrasoundCapture(saveCaptureText, saveCaptureIcon));

        // Set up zoom reset button
        View zoomResetOption = binding.getRoot().findViewById(R.id.zoom_reset_option);
        zoomResetOption.setOnClickListener(v -> resetZoom());

        // Set up measurement completion listener
        measurementOverlay.setOnMeasurementCompleteListener(measurement -> {
            android.util.Log.d("UltrasoundCapture",
                    "Measurement completed: " + measurement.getFormattedDistance());
            Toast.makeText(getContext(),
                    "Measured: " + measurement.getFormattedDistance(),
                    Toast.LENGTH_SHORT).show();
            // Update magnifier bitmap after measurement completes
            updateMagnifierBitmap();
            // Toggle measurement mode off after a new measurement is completed
            // (only if mode is currently on)
            if (isMeasurementMode) {
                toggleMeasurementMode(measurementToolIcon);
            }
        });

        // Set up touch event listener for magnifier display during measurements
        measurementOverlay.setOnTouchEventListener((x, y, action) -> {
            if (magnifierView != null) {
                switch (action) {
                    case MotionEvent.ACTION_DOWN:
                        updateMagnifierBitmap();
                        magnifierView.setVisibility(View.VISIBLE);
                        magnifierView.bringToFront();
                        magnifierView.showMagnifier(x, y);
                        android.util.Log.d(TAG, "Magnifier shown from overlay at (" + x + ", " + y + ")");
                        break;
                    case MotionEvent.ACTION_MOVE:
                        if (magnifierView.getVisibility() == View.VISIBLE) {
                            magnifierView.showMagnifier(x, y);
                        }
                        break;
                    case MotionEvent.ACTION_UP:
                    case MotionEvent.ACTION_CANCEL:
                        magnifierView.hideMagnifier();
                        magnifierView.setVisibility(View.GONE);
                        android.util.Log.d(TAG, "Magnifier hidden from overlay");
                        break;
                }
            }
        });

        // --- Magnifier logic ---
        magnifierView = binding.getRoot().findViewById(R.id.magnifier_view);
        if (magnifierView != null) {
            magnifierUpdateHandler = new Handler(Looper.getMainLooper());
            android.util.Log.d(TAG, "MagnifierView initialized successfully");
            // Initialize magnifier bitmap update on first layout
            magnifierView.post(() -> updateMagnifierBitmap());
        } else {
            MedDevLog.error(TAG, "MagnifierView not found in layout!");
        }

        // Get SurfaceView reference
        surfaceView = binding.getRoot().findViewById(R.id.ultrasound_surface_view);

        // Initialize player and viewer
        player = GlobalUltrasoundManager.getInstance().getUltraSoundPlayer();
        viewer = GlobalUltrasoundManager.getInstance().getUltrasoundViewer(requireContext());

        // Setup SurfaceView callback
        if (surfaceView != null) {
            surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
                @Override
                public void surfaceCreated(@NonNull SurfaceHolder holder) {
                    // Connect viewer to surface
                    viewer.setSurface(holder.getSurface());

                    // Set source view dimensions for magnifier
                    if (magnifierView != null) {
                        magnifierView.setSourceViewDimensions(
                                surfaceView.getWidth(),
                                surfaceView.getHeight());
                    }

                    // Load captured image if provided
                    if (getArguments() != null && getArguments().containsKey("capturedImageFile")) {
                        String fileName = getArguments().getString("capturedImageFile");
                        loadCapturedThorImage(fileName);
                    }
                }

                @Override
                public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width,
                        int height) {
                    // Surface changed - no action needed
                }

                @Override
                public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
                    viewer.setSurface(null);
                }
            });
        } else {
            MedDevLog.error("UltrasoundCapture", "SurfaceView not found in layout");
        }

        // Setup zoom gesture detectors
        setupGestureDetectors();

        // Set bitmap to magnifier after layout is drawn
        /*
         * ultrasoundImage.post(() -> {
         * Bitmap bitmap = getBitmapFromImageView(ultrasoundImage);
         * if (bitmap != null) {
         * magnifierView.setImageBitmap(bitmap);
         * }
         * });
         */

        // Handle touch events for magnifier, measurements, and zoom
        surfaceView.setOnTouchListener((v, event) -> {
            // Pass to zoom detectors first
            boolean scaleHandled = mScaleDetector != null && mScaleDetector.onTouchEvent(event);
            boolean gestureHandled = mGestureDetector != null && mGestureDetector.onTouchEvent(event);

            // Update matrix when touch ends after zoom/pan operations
            if (event.getAction() == MotionEvent.ACTION_UP && isZoomed() &&
                    (scaleHandled || gestureHandled)) {
                if (measurementOverlay != null) {
                    updateTransformationMatrix();
                }
            }

            // If zoom handled, don't process further
            if (scaleHandled || gestureHandled) {
                return true;
            }

            // Always show magnifier regardless of measurement mode
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    if (magnifierView != null) {
                        // Update magnifier bitmap when first touched
                        updateMagnifierBitmap();
                        magnifierView.setVisibility(View.VISIBLE);
                        magnifierView.bringToFront();
                        magnifierView.showMagnifier(event.getX(), event.getY());
                        android.util.Log.d(TAG,
                                "Magnifier shown at (" + event.getX() + ", " + event.getY() + ")");
                    } else {
                        MedDevLog.error(TAG, "MagnifierView is null on ACTION_DOWN");
                    }
                    break;
                case MotionEvent.ACTION_MOVE:
                    if (magnifierView != null && magnifierView.getVisibility() == View.VISIBLE) {
                        magnifierView.showMagnifier(event.getX(), event.getY());
                    }
                    break;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    if (magnifierView != null) {
                        magnifierView.hideMagnifier();
                        magnifierView.setVisibility(View.GONE);
                        android.util.Log.d(TAG, "Magnifier hidden");
                    }
                    break;
            }
            
            // If measurement mode is active, let the overlay also handle touches for drawing
            if (isMeasurementMode) {
                return false; // Let the overlay handle it too
            }
            
            return true;
        });

        return binding.getRoot();

    }

    private void loadCapturedThorImage(String fileName) {
        try {
            // Get the clips directory path from external files
            java.io.File clipsDir = new java.io.File(requireContext().getExternalFilesDir(null), "clips");
            String imagePath = new java.io.File(clipsDir, fileName + ".thor").getAbsolutePath();
            android.util.Log.d("UltrasoundCapture", "Loading .thor image from: " + imagePath);

            // Check if file exists
            java.io.File thorFile = new java.io.File(imagePath);
            if (!thorFile.exists()) {
                MedDevLog.error("UltrasoundCapture", "Thor file not found: " + imagePath);
                Toast.makeText(getContext(), "Image file not found", Toast.LENGTH_SHORT).show();
                return;
            }

            // Use GlobalUltrasoundManager.openFile() which handles:
            // 1. Detaching DAU from CineBuffer if attached
            // 2. Attaching player to CineBuffer
            // 3. Opening the file
            // Note: openFile expects filename WITH .thor extension
            GlobalUltrasoundManager.getInstance().openFile(fileName + ".thor");

            // Verify file was loaded successfully
            if (player.isRawFileOpen()) {
                int frameCount = player.getFrameCount();
                int duration = player.getDuration();
                android.util.Log.d("UltrasoundCapture", "Thor image loaded successfully: " +
                        fileName + ", Frames: " + frameCount + ", Duration: " + duration + "ms");

                // Seek to first frame to display the image
                player.seekTo(0);
                android.util.Log.d("UltrasoundCapture", "Positioned to frame 0 for display");

                // Get transformation matrix for measurements
                // CRITICAL: Matrix must be captured from the loaded image
                initializeTransformationMatrix();
            } else {
                MedDevLog.error("UltrasoundCapture", "Failed to load .thor file");
                Toast.makeText(getContext(), "Failed to load image", Toast.LENGTH_SHORT).show();
            }
        } catch (Exception e) {
            MedDevLog.error("UltrasoundCapture", "Error loading .thor image", e);
            Toast.makeText(getContext(), "Error loading image: " + e.getMessage(), 
                Toast.LENGTH_SHORT).show();
        }
    }

    /**
     * Update the transformation matrix from the viewer This should be called after
     * zoom/pan
     * operations to keep measurements accurate
     */
    private void updateTransformationMatrix() {
        try {
            if (measurementOverlay != null && viewer != null) {
                float[] newMatrix = GlobalUltrasoundManager.getInstance()
                        .getTransformationMatrix(requireContext());
                if (newMatrix != null && newMatrix.length == 9) {
                    transformationMatrix = newMatrix;
                    measurementOverlay.setTransformationMatrix(transformationMatrix);
                    android.util.Log.d("UltrasoundCapture", "Transformation matrix updated");
                }
            }
        } catch (Exception e) {
            MedDevLog.error("UltrasoundCapture", "Error updating transformation matrix", e);
        }
    }

    /**
     * Initialize transformation matrix from the loaded ultrasound image This matrix
     * is essential for
     * accurate coordinate conversion in measurements
     */
    private void initializeTransformationMatrix() {
        try {
            // Get transformation matrix from GlobalUltrasoundManager
            // This retrieves the matrix from the native layer for the currently displayed
            // image
            transformationMatrix = GlobalUltrasoundManager.getInstance()
                    .getTransformationMatrix(requireContext());

            if (transformationMatrix != null && transformationMatrix.length == 9) {
                // Set matrix in measurement overlay
                measurementOverlay.setTransformationMatrix(transformationMatrix);

                android.util.Log.d("UltrasoundCapture",
                        "Transformation matrix initialized successfully");
                android.util.Log.d("UltrasoundCapture", String.format(
                        "Matrix: [%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f]",
                        transformationMatrix[0], transformationMatrix[1], transformationMatrix[2],
                        transformationMatrix[3], transformationMatrix[4], transformationMatrix[5],
                        transformationMatrix[6], transformationMatrix[7], transformationMatrix[8]));
            } else {
                MedDevLog.error("UltrasoundCapture",
                    "Failed to get valid transformation matrix");
                Toast.makeText(getContext(), 
                    "Warning: Measurements may not be accurate", 
                    Toast.LENGTH_SHORT).show();
            }
        } catch (Exception e) {
            MedDevLog.error("UltrasoundCapture",
                "Error initializing transformation matrix", e);
        }
    }

    /**
     * Toggle measurement mode on/off When enabled, user can draw distance
     * measurements on the
     * overlay
     */
    private void toggleMeasurementMode(ImageView icon) {
        isMeasurementMode = !isMeasurementMode;

        if (isMeasurementMode) {
            // Check if matrix is available
            if (transformationMatrix == null) {
                initializeTransformationMatrix();

                if (transformationMatrix == null) {
                    Toast.makeText(getContext(),
                            "Cannot enable measurements: No image loaded",
                            Toast.LENGTH_SHORT).show();
                    isMeasurementMode = false;
                    return;
                }
            }

            // Enable measurement mode
            measurementOverlay.setMeasurementMode(true);
            icon.setColorFilter(Color.YELLOW); // Highlight icon when active
            Toast.makeText(getContext(),
                    "Measurement mode: ON\nDrag to measure distance",
                    Toast.LENGTH_SHORT).show();
            android.util.Log.d("UltrasoundCapture", "Measurement mode enabled");
        } else {
            // Disable measurement mode
            measurementOverlay.setMeasurementMode(false);
            icon.setColorFilter(Color.WHITE); // Normal icon color
            Toast.makeText(getContext(),
                    "Measurement mode: OFF",
                    Toast.LENGTH_SHORT).show();
            android.util.Log.d("UltrasoundCapture", "Measurement mode disabled");
        }
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
                        if (enableZoomAndPan && viewer != null) {
                            // Apply incremental scale
                            float scaleFactor = detector.getScaleFactor();
                            viewer.handleOnScale(scaleFactor);

                            // Update current scale
                            currentScale = viewer.getZoomScale();

                            // Update matrix during zoom so measurements move in real-time
                            if (measurementOverlay != null) {
                                updateTransformationMatrix();
                            }

                            android.util.Log.d("Zoom", "Scale: " + currentScale);
                        }
                        return true;
                    }

                    @Override
                    public void onScaleEnd(ScaleGestureDetector detector) {
                        // Zoom gesture ended, update matrix once at the end
                        if (measurementOverlay != null) {
                            updateTransformationMatrix();
                        }
                        android.util.Log.d("Zoom", "Zoom gesture ended at scale: " + currentScale);
                    }
                });

        // Gesture detector for pan when zoomed
        mGestureDetector = new GestureDetectorCompat(requireContext(),
                new GestureDetector.SimpleOnGestureListener() {
                    @Override
                    public boolean onScroll(MotionEvent e1, MotionEvent e2,
                            float distanceX, float distanceY) {
                        if (isZoomed() && enableZoomAndPan && viewer != null) {
                            // Calculate pan delta based on view dimensions
                            if (surfaceView != null) {
                                float deltaX = -distanceX / surfaceView.getWidth();
                                float deltaY = -distanceY / surfaceView.getHeight();

                                // Apply pan
                                viewer.handleOnTouch(deltaX, deltaY, false);
                            }
                            return true;
                        }
                        return false;
                    }

                    @Override
                    public boolean onSingleTapConfirmed(MotionEvent e) {
                        // Update matrix after any touch interaction completes
                        if (isZoomed() && measurementOverlay != null) {
                            updateTransformationMatrix();
                        }
                        return super.onSingleTapConfirmed(e);
                    }
                });
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
        if (viewer != null) {
            // Reset scale to 1.0
            GlobalUltrasoundManager.getInstance().setUltrasoundViewerScale(requireContext(), 1.0f);

            // Reset pan to origin
            viewer.setZoomPan(0.0f, 0.0f);

            // Update current scale
            currentScale = 1.0f;

            // Update measurement overlay matrix after reset
            if (measurementOverlay != null) {
                updateTransformationMatrix();
            }

            // Redraw if needed
            viewer.handleOnScale(1.0f);

            // Redraw current frame
            if (player != null && !player.isRunning()) {
                player.seekToFrame(player.getCurrentFrameNo(), false);
            }

            Toast.makeText(getContext(), "Zoom reset", Toast.LENGTH_SHORT).show();
            android.util.Log.d("Zoom", "Zoom reset to 1.0x");
        }
    }

    /**
     * Clear all measurements from the overlay
     */
    private void clearAllMeasurements() {
        if (measurementOverlay != null) {
            measurementOverlay.clearMeasurements();
            // Update magnifier bitmap after clearing measurements
            updateMagnifierBitmap();
            Toast.makeText(getContext(),
                    "All measurements cleared",
                    Toast.LENGTH_SHORT).show();
        }
    }

    /**
     * Get all measurements for saving/export
     */
    private java.util.List<MeasurementModel> getAllMeasurements() {
        if (measurementOverlay != null) {
            return measurementOverlay.getMeasurements();
        }
        return new java.util.ArrayList<>();
    }

    /**
     * Update magnifier view with current merged bitmap of surface and overlay
     */
    private void updateMagnifierBitmap() {
        if (surfaceView == null) {
            MedDevLog.error(TAG, "updateMagnifierBitmap: surfaceView is null");
            return;
        }
        if (measurementOverlay == null) {
            MedDevLog.error(TAG, "updateMagnifierBitmap: measurementOverlay is null");
            return;
        }
        if (magnifierView == null) {
            MedDevLog.error(TAG, "updateMagnifierBitmap: magnifierView is null");
            return;
        }

        android.util.Log.d(TAG, "Starting magnifier bitmap update...");

        // Capture merged bitmap asynchronously
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N) {
            int width = surfaceView.getWidth();
            int height = surfaceView.getHeight();

            if (width <= 0 || height <= 0) {
                MedDevLog.error(TAG, "Invalid surface dimensions: " + width + "x" + height);
                return;
            }

            android.util.Log.d(TAG, "Creating bitmap: " + width + "x" + height);
            Bitmap surfaceBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);

            try {
                PixelCopy.request(
                        surfaceView.getHolder().getSurface(),
                        surfaceBitmap,
                        copyResult -> {
                            if (copyResult == PixelCopy.SUCCESS) {
                                // Merge with overlay
                                Bitmap merged = mergeBitmaps(surfaceBitmap, measurementOverlay);
                                if (merged != null) {
                                    // Recycle old bitmap if exists
                                    if (currentMergedBitmap != null && !currentMergedBitmap.isRecycled()) {
                                        currentMergedBitmap.recycle();
                                    }
                                    currentMergedBitmap = merged;
                                    magnifierView.setImageBitmap(currentMergedBitmap);
                                    android.util.Log.d(TAG, "Magnifier bitmap updated with merged image");
                                }
                                surfaceBitmap.recycle();
                            } else {
                                MedDevLog.error(TAG,
                                        "Failed to capture surface for magnifier: " + copyResult);
                                surfaceBitmap.recycle();
                            }
                        },
                        magnifierUpdateHandler);
            } catch (Exception e) {
                MedDevLog.error(TAG, "Error capturing surface for magnifier", e);
                surfaceBitmap.recycle();
            }
        } else {
            // Fallback for older Android versions - just use overlay
            Bitmap overlayBitmap = captureViewAsBitmap(measurementOverlay);
            if (overlayBitmap != null) {
                if (currentMergedBitmap != null && !currentMergedBitmap.isRecycled()) {
                    currentMergedBitmap.recycle();
                }
                currentMergedBitmap = overlayBitmap;
                magnifierView.setImageBitmap(currentMergedBitmap);
            }
        }
    }

    /**
     * Merge ultrasound bitmap with measurement overlay and center line
     * @param surfaceBitmap The captured ultrasound surface bitmap
     * @param overlay       The measurement overlay view
     *
     * @return Merged bitmap
     */
    private Bitmap mergeBitmaps(Bitmap surfaceBitmap, View overlay) {
        if (surfaceBitmap == null) {
            MedDevLog.error(TAG, "Surface bitmap is null");
            return null;
        }

        // Create result bitmap with same dimensions as surface
        Bitmap mergedBitmap = Bitmap.createBitmap(
                surfaceBitmap.getWidth(),
                surfaceBitmap.getHeight(),
                Bitmap.Config.ARGB_8888);

        Canvas canvas = new Canvas(mergedBitmap);

        // Draw ultrasound image first
        canvas.drawBitmap(surfaceBitmap, 0, 0, null);
        
        // Draw center line if enabled (before measurement overlay so measurements appear on top)
        if (centerLineOverlay != null) {
            android.util.Log.d(TAG, "centerLineOverlay is not null, enabled=" + centerLineOverlay.isCenterLineEnabled());
            if (centerLineOverlay.isCenterLineEnabled()) {
                // Detect ultrasound image bounds to constrain center line
                android.util.Log.d(TAG, "Starting ultrasound bounds detection...");
                float[] imageBounds = detectUltrasoundImageBounds(surfaceBitmap);
                if (imageBounds != null && imageBounds.length == 4) {
                    // Draw center line only within ultrasound image region
                    centerLineOverlay.drawCenterLineOnCanvas(canvas, 
                        surfaceBitmap.getWidth(), 
                        surfaceBitmap.getHeight(),
                        imageBounds[1], // top
                        imageBounds[3]  // bottom
                    );
                    android.util.Log.d(TAG, String.format(
                        "Added bounded center line to merged bitmap: top=%.1f, bottom=%.1f", 
                        imageBounds[1], imageBounds[3]));
                } else {
                    // Fallback to full height if bounds detection fails
                    centerLineOverlay.drawCenterLineOnCanvas(canvas, 
                        surfaceBitmap.getWidth(), 
                        surfaceBitmap.getHeight());
                    android.util.Log.d(TAG, "Added full-height center line (bounds detection failed)");
                }
            }
        } else {
            android.util.Log.w(TAG, "centerLineOverlay is null, cannot draw center line");
        }
        
        // Draw measurement overlay on top
        if (overlay != null) {
            overlay.draw(canvas);
            android.util.Log.d(TAG, "Merged surface bitmap with measurement overlay");
        }

        return mergedBitmap;
    }
    
    /**
     * Detect the bounds of the actual ultrasound image within the bitmap.
     * Scans the bitmap to find non-black regions that contain ultrasound data.
     * 
     * @param bitmap The ultrasound surface bitmap to analyze
     * @return float array [left, top, right, bottom] representing the ultrasound image region,
     *         or null if detection fails
     */
    private float[] detectUltrasoundImageBounds(Bitmap bitmap) {
        if (bitmap == null) {
            return null;
        }
        
        int width = bitmap.getWidth();
        int height = bitmap.getHeight();
        
        // Threshold for considering a pixel as "non-black" (ultrasound data vs background)
        // Using a low threshold to catch even faint ultrasound signals
        final int threshold = 20; // RGB value > 20 for any channel
        
        int top = -1;
        int bottom = -1;
        int left = -1;
        int right = -1;
        
        // Find top boundary - scan from top down
        outerTop:
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int pixel = bitmap.getPixel(x, y);
                int r = (pixel >> 16) & 0xff;
                int g = (pixel >> 8) & 0xff;
                int b = pixel & 0xff;
                if (r > threshold || g > threshold || b > threshold) {
                    top = y;
                    break outerTop;
                }
            }
        }
        
        // Find bottom boundary - scan from bottom up
        outerBottom:
        for (int y = height - 1; y >= 0; y--) {
            for (int x = 0; x < width; x++) {
                int pixel = bitmap.getPixel(x, y);
                int r = (pixel >> 16) & 0xff;
                int g = (pixel >> 8) & 0xff;
                int b = pixel & 0xff;
                if (r > threshold || g > threshold || b > threshold) {
                    bottom = y;
                    break outerBottom;
                }
            }
        }
        
        // Find left boundary - scan from left to right
        outerLeft:
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                int pixel = bitmap.getPixel(x, y);
                int r = (pixel >> 16) & 0xff;
                int g = (pixel >> 8) & 0xff;
                int b = pixel & 0xff;
                if (r > threshold || g > threshold || b > threshold) {
                    left = x;
                    break outerLeft;
                }
            }
        }
        
        // Find right boundary - scan from right to left
        outerRight:
        for (int x = width - 1; x >= 0; x--) {
            for (int y = 0; y < height; y++) {
                int pixel = bitmap.getPixel(x, y);
                int r = (pixel >> 16) & 0xff;
                int g = (pixel >> 8) & 0xff;
                int b = pixel & 0xff;
                if (r > threshold || g > threshold || b > threshold) {
                    right = x;
                    break outerRight;
                }
            }
        }
        
        // Validate bounds
        if (top == -1 || bottom == -1 || left == -1 || right == -1) {
            android.util.Log.e(TAG, "Failed to detect ultrasound image bounds");
            return null;
        }
        
        if (top >= bottom || left >= right) {
            android.util.Log.e(TAG, "Invalid bounds detected: top=" + top + ", bottom=" + bottom + 
                ", left=" + left + ", right=" + right);
            return null;
        }
        
        android.util.Log.d(TAG, String.format(
            "Detected ultrasound image bounds: left=%d, top=%d, right=%d, bottom=%d (%.1f%% of height)",
            left, top, right, bottom, ((bottom - top) * 100.0f / height)));
        
        return new float[]{left, top, right, bottom};
    }
    
    /**
     * Capture a view as a bitmap
     *
     * @param view The view to capture
     *
     * @return Bitmap of the view
     */
    private Bitmap captureViewAsBitmap(View view) {
        if (view == null) {
            return null;
        }

        Bitmap bitmap = Bitmap.createBitmap(
                view.getWidth(),
                view.getHeight(),
                Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        view.draw(canvas);
        return bitmap;
    }

    /**
     * Save bitmap to local storage
     *
     * @param bitmap The bitmap to save
     *
     * @return Path to saved file, or null if failed
     */
    private String saveBitmapToFile(Bitmap bitmap) {
        if (bitmap == null) {
            MedDevLog.error(TAG, "Cannot save null bitmap");
            return null;
        }

        try {
            // Create images directory in external files
            File imagesDir = new File(requireContext().getExternalFilesDir(null), "images");
            if (!imagesDir.exists() && !imagesDir.mkdirs()) {
                MedDevLog.error(TAG, "Failed to create images directory");
                return null;
            }

            // Generate filename with timestamp
            SimpleDateFormat dateFormat = new SimpleDateFormat(
                "yyyyMMdd_HHmmss", 
                Locale.US
            );
            String timestamp = dateFormat.format(new Date());
            String filename = "ultrasound_" + timestamp + ".png";
            
            File imageFile = new File(imagesDir, filename);
            
            // Save bitmap to file
            FileOutputStream out = new FileOutputStream(imageFile);
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, out);
            out.flush();
            out.close();
            
            android.util.Log.i(TAG, "Bitmap saved successfully to: " + imageFile.getAbsolutePath());
            return imageFile.getAbsolutePath();
            
        } catch (IOException e) {
            MedDevLog.error(TAG, "Error saving bitmap to file", e);
            return null;
        }
    }

    private Bitmap getBitmapFromImageView(ImageView imageView) {
        imageView.setDrawingCacheEnabled(true);
        imageView.buildDrawingCache();
        Bitmap bmp = null;
        if (imageView.getDrawable() != null) {
            bmp = Bitmap.createBitmap(imageView.getDrawable().getIntrinsicWidth(), imageView.getDrawable().getIntrinsicHeight(), Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bmp);
            imageView.getDrawable().draw(canvas);
        }
        imageView.setDrawingCacheEnabled(false);
        return bmp;
    }

    private void updateSelectedCatheterSizeUI(View container, android.widget.TextView textView, android.widget.TextView textViewLumen) {
        if (selectedCatheterSize != null) {
            container.setVisibility(View.VISIBLE);
            textView.setText(selectedCatheterSize);
            textViewLumen.setText(NumberOfLumen);

        } else {
            container.setVisibility(View.GONE);
        }
    }

    private void showCatheterSizeDialog(View selectedCatheterContainer, android.widget.TextView selectedCatheterText, android.widget.TextView lumenText) {
        catheterSizeDialog = new Dialog(requireContext());
        catheterSizeDialog.setContentView(R.layout.dialog_select_catheter_size);
        catheterSizeDialog.getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        TextView numberOfLumexText = catheterSizeDialog.findViewById(R.id.number_lumen_input);
        TextView veinCatheterRatioText = catheterSizeDialog.findViewById(R.id.vein_catheter_ratio_text);
        
        // Get last measurement as vein size
        Float veinSizeMm = null;
        if (measurementOverlay != null) {
            java.util.List<MeasurementModel> measurements = measurementOverlay.getMeasurements();
            if (measurements != null && !measurements.isEmpty()) {
                MeasurementModel lastMeasurement = measurements.get(measurements.size() - 1);
                veinSizeMm = lastMeasurement.calculateDistanceMm();
                
                // Display vein size at top
                veinCatheterRatioText.setVisibility(View.VISIBLE);
                veinCatheterRatioText.setText(String.format("Measured Vein Size: %.2f mm", veinSizeMm));
            }
        }
        
        // Store vein size for use in checkbox text
        final Float finalVeinSizeMm = veinSizeMm;
        
        // Only allow one checkbox to be selected at a time
        CheckBox[] checkBoxes = new CheckBox[] {
                catheterSizeDialog.findViewById(R.id.size_1_9),
                catheterSizeDialog.findViewById(R.id.size_2_6),
                catheterSizeDialog.findViewById(R.id.size_3),
                catheterSizeDialog.findViewById(R.id.size_4),
                catheterSizeDialog.findViewById(R.id.size_5),
                catheterSizeDialog.findViewById(R.id.size_6)
        };
        
        // Update checkbox text to include ratio for each size
        if (finalVeinSizeMm != null) {
            for (CheckBox cb : checkBoxes) {
                String originalText = cb.getText().toString();
                // Extract numeric value from "X French" format (1 French = 0.33mm)
                try {
                    String numStr = originalText.replace("French", "").trim();
                    float frenchSize = Float.parseFloat(numStr);
                    float catheterSizeMm = frenchSize / 3f;
                    float ratio = catheterSizeMm / finalVeinSizeMm;
                    
                    // Update checkbox text to include CVR (catheter-to-vein ratio)
                    cb.setText(String.format("%s (CVR: %.2f)", originalText, ratio));
                } catch (NumberFormatException e) {
                    MedDevLog.error(TAG, "Error parsing catheter size", e);
                }
            }
        }
        
        for (CheckBox cb : checkBoxes) {
            cb.setOnCheckedChangeListener((buttonView, isChecked) -> {
                if (isChecked) {
                    for (CheckBox other : checkBoxes) {
                        if (other != buttonView)
                            other.setChecked(false);
                    }
                }
            });
        }

        // Pre-select if already selected
        if (selectedCatheterSize != null) {
            for (CheckBox cb : checkBoxes) {
                String cbText = cb.getText().toString();
                // Compare against original catheter size (before ratio was appended)
                if (cbText.startsWith(selectedCatheterSize) || selectedCatheterSize.equals(cbText)) {
                    cb.setChecked(true);
                    break;
                }
            }
        }
        numberOfLumexText.setText(NumberOfLumen);

        ImageButton btnDelete = catheterSizeDialog.findViewById(R.id.btn_delete);
        ImageButton btnConfirm = catheterSizeDialog.findViewById(R.id.btn_confirm);

        btnDelete.setOnClickListener(v -> {
            if (catheterSizeDialog != null) {
                catheterSizeDialog.dismiss();
                catheterSizeDialog = null;
            }
        });

        btnConfirm.setOnClickListener(v -> {
            String selected = null;
            for (CheckBox cb : checkBoxes) {
                if (cb.isChecked()) {
                    String fullText = cb.getText().toString();
                    // Extract original catheter size (before CVR text)
                    // Format is "X French (CVR: X.XX)"
                    if (fullText.contains(" (CVR:")) {
                        selected = fullText.substring(0, fullText.indexOf(" (CVR:"));
                    } else {
                        selected = fullText;
                    }
                    break;
                }
            }
            NumberOfLumen =  numberOfLumexText.getText().toString();
            if (NumberOfLumen.isEmpty())
                Toast.makeText(requireContext(), "Please enter a number of Lumens", Toast.LENGTH_SHORT).show();

            if (selected != null && !NumberOfLumen.isEmpty()) {
                selectedCatheterSize = selected;

                updateSelectedCatheterSizeUI(selectedCatheterContainer, selectedCatheterText,lumenText);
                if (catheterSizeDialog != null) {
                    catheterSizeDialog.dismiss();
                    catheterSizeDialog = null;
                }
            } else {
                Toast.makeText(requireContext(), "Please select a catheter size", Toast.LENGTH_SHORT).show();
            }
        });

        if (catheterSizeDialog != null) {
            catheterSizeDialog.show();
        }
    }
    
    /**
     * Show warning dialog when user tries to proceed without saving any captures
     * @param onConfirm Callback to execute if user confirms they want to proceed without saving
     */
    private void showNoCaptureWarningDialog(Runnable onConfirm) {
        String message = getString(R.string.you_haven_t_saved_any_ultrasound_captures);
        ConfirmDialog.show(requireContext(), message, confirmed -> {
            if (confirmed && onConfirm != null) {
                onConfirm.run();
            }
        });
    }
    
    /**
     * Update the save capture button state (text and highlight)
     * @param textView The TextView to update
     * @param iconView The ImageView icon to highlight if capture exists
     */
    private void updateSaveCaptureButtonState(TextView textView, ImageView iconView) {
        if (textView == null || viewmodel == null) return;
        
        EncounterData procedure = viewmodel.getEncounterData();
        boolean hasCapture = procedure != null && procedure.getUltrasoundCaptureCount() > 0;
        
        textView.setText("Save");
        
        // Highlight icon if capture exists
        if (iconView != null) {
            if (hasCapture) {
                iconView.setColorFilter(getResources().getColor(R.color.av_yellow, null));
            } else {
                iconView.clearColorFilter();
            }
        }
    }
    
    /**
     * Save the ultrasound capture with measurement overlay to procedure directory
     * @param textView The save button text to update after saving
     * @param iconView The save button icon to update after saving
     */
    private void saveUltrasoundCapture(TextView textView, ImageView iconView) {
        if (viewmodel == null) {
            Toast.makeText(requireContext(), "ViewModel not available", Toast.LENGTH_SHORT).show();
            return;
        }
        
        EncounterData procedure = viewmodel.getEncounterData();
        if (procedure == null) {
            Toast.makeText(requireContext(), "No active procedure", Toast.LENGTH_SHORT).show();
            return;
        }
        
        // Check if capture already exists and show confirmation dialog
        if (procedure.getUltrasoundCaptureCount() > 0) {
            ConfirmDialog.show(requireContext(), 
                "Replace existing ultrasound capture?", 
                confirmed -> {
                    if (confirmed) {
                        performSaveUltrasoundCapture(textView, iconView, procedure);
                    }
                });
            return;
        }
        
        // No existing capture, proceed with save
        performSaveUltrasoundCapture(textView, iconView, procedure);
    }
    
    /**
     * Perform the actual save operation
     * @param textView The save button text to update after saving
     * @param iconView The save button icon to update after saving
     * @param procedure The encounter data to save to
     */
    private void performSaveUltrasoundCapture(TextView textView, ImageView iconView, EncounterData procedure) {
        // Ensure procedure has been started and has a data directory
        if (procedure.getDataDirPath() == null || procedure.getDataDirPath().isEmpty()) {
            // Create procedure directory if not exists
            File proceduresDir = requireContext().getExternalFilesDir(DataFiles.PROCEDURES_DIR);
            if (proceduresDir == null) {
                Toast.makeText(requireContext(), "Failed to access storage", Toast.LENGTH_SHORT).show();
                return;
            }
            
            String startTimeString = procedure.getStartTimeText();
            if (startTimeString == null || startTimeString.isEmpty()) {
                // Initialize start time if not set
                procedure.setStartTime(new Date());
                startTimeString = procedure.getStartTimeText();
            }
            
            // Set procedure state to Active if not already live
            if (!procedure.isLive()) {
                procedure.setState(EncounterState.Active);
            }
            
            // Set app and controller versions
            procedure.setAppSoftwareVersion(TabletState.getAppVersion(requireContext()));
            if (viewmodel.getControllerState() != null) {
                procedure.setControllerFirmwareVersion(viewmodel.getControllerState().getFirmwareVersion());
            }
            
            // Set patient inserter and facility information
            ProfileState profileState = viewmodel.getProfileState();
            if (profileState != null && profileState.getSelectedProfile() != null) {
                procedure.getPatient().setPatientInserter(profileState.getSelectedProfile().getProfileName());
            }
            
            Facility procFacility = procedure.getPatient().getPatientFacility();
            if (procFacility != null) {
                procFacility.setDateLastUsed(startTimeString);
                if (profileState != null) {
                    profileState.updateFacilityRecency(procFacility, requireContext());
                }
            }
            
            File procedureDir = new File(proceduresDir, startTimeString);
            if (!procedureDir.exists() && !procedureDir.mkdirs()) {
                Toast.makeText(requireContext(), "Failed to create procedure directory", Toast.LENGTH_SHORT).show();
                return;
            }
            
            procedure.setDataDirPath(procedureDir.getAbsolutePath());
            procedure.setProcedureStarted(true);
            android.util.Log.d(TAG, "Created procedure directory: " + procedureDir.getAbsolutePath());
            
            // Create proceduredata.txt if it doesn't exist
            File procedureDataFile = new File(procedureDir, DataFiles.PROCEDURE_DATA);
            if (!procedureDataFile.exists()) {
                updateProcedureFile();
            }
        }
        
        // Save the merged image with measurement overlay
        saveMergedImageToProcedure(() -> {
            // Update button state with highlight
            updateSaveCaptureButtonState(textView, iconView);
            
            Toast.makeText(requireContext(), 
                "Ultrasound capture saved", 
                Toast.LENGTH_SHORT).show();
        });
    }
    
    /**
     * Save merged ultrasound image with measurement overlay to procedure directory
     * @param onComplete Callback to run after save completes
     */
    private void saveMergedImageToProcedure(Runnable onComplete) {
        if (surfaceView == null || measurementOverlay == null) {
            MedDevLog.error(TAG, "surfaceView or measurementOverlay is null");
            if (onComplete != null) {
                onComplete.run();
            }
            return;
        }
        
        // Use PixelCopy to capture the surface (Android O and above)
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            Bitmap surfaceBitmap = Bitmap.createBitmap(
                surfaceView.getWidth(),
                surfaceView.getHeight(),
                Bitmap.Config.ARGB_8888
            );
            
            PixelCopy.request(
                    surfaceView,
                    surfaceBitmap,
                    copyResult -> {
                        if (copyResult == PixelCopy.SUCCESS) {
                            android.util.Log.d(TAG, "PixelCopy successful");

                            // Merge with measurement overlay
                            Bitmap mergedBitmap = mergeBitmaps(surfaceBitmap, measurementOverlay);

                            if (mergedBitmap != null) {
                                String savedPath = saveBitmapToProcedure(mergedBitmap);
                                if (savedPath != null) {
                                    android.util.Log.d(TAG, "Saved to procedure: " + savedPath);
                                }
                                mergedBitmap.recycle();
                            }

                            surfaceBitmap.recycle();
                        } else {
                            MedDevLog.error(TAG, "PixelCopy failed with result: " + copyResult);
                        }

                        if (onComplete != null) {
                            onComplete.run();
                        }
                    },
                    new Handler(Looper.getMainLooper()));
        } else {
            // For older Android versions, just capture the overlay
            MedDevLog.warn(TAG, "PixelCopy not available, saving overlay only");
            Bitmap overlayBitmap = captureViewAsBitmap(measurementOverlay);
            if (overlayBitmap != null) {
                String savedPath = saveBitmapToProcedure(overlayBitmap);
                if (savedPath != null) {
                    android.util.Log.d(TAG, "Saved overlay to procedure: " + savedPath);
                }
                overlayBitmap.recycle();
            }
            if (onComplete != null) onComplete.run();
        }
    }
    
    /**
     * Save bitmap to procedure directory and update EncounterData
     * @param bitmap The bitmap to save
     * @return Path to saved file, or null if failed
     */
    private String saveBitmapToProcedure(Bitmap bitmap) {
        if (bitmap == null) {
            MedDevLog.error(TAG, "Cannot save null bitmap");
            return null;
        }
        
        if (viewmodel == null || viewmodel.getEncounterData() == null) {
            MedDevLog.error(TAG, "ViewModel or procedure is null");
            return null;
        }
        
        EncounterData procedure = viewmodel.getEncounterData();
        String dataDirPath = procedure.getDataDirPath();
        
        if (dataDirPath == null || dataDirPath.isEmpty()) {
            MedDevLog.error(TAG, "Procedure data directory path is null or empty");
            return null;
        }
        
        try {
            File procedureDir = new File(dataDirPath);
            if (!procedureDir.exists()) {
                MedDevLog.error(TAG, "Procedure directory does not exist: " + dataDirPath);
                return null;
            }
            
            // Update vein size from measurement if present
            if (measurementOverlay != null) {
                java.util.List<MeasurementModel> measurements = measurementOverlay.getMeasurements();
                if (measurements != null && !measurements.isEmpty()) {
                    MeasurementModel lastMeasurement = measurements.get(measurements.size() - 1);
                    float veinSizeMm = lastMeasurement.calculateDistanceMm();
                    // Round to 1 decimal place for database storage
                    double roundedVeinSize = Math.round(veinSizeMm * 10.0) / 10.0;
                    java.util.OptionalDouble veinSize = java.util.OptionalDouble.of(roundedVeinSize);
                    procedure.getPatient().setPatientVeinSize(veinSize);
                    MedDevLog.debug(TAG, "Updated vein size from measurement: " + roundedVeinSize + " mm");
                }
            }
            
            // Mark ultrasound capture as saved
            procedure.setIsUltrasoundCaptureSaved(true);
            MedDevLog.debug(TAG, "Marked ultrasound capture as saved");
            
            // Check if we're replacing an existing capture
            boolean isReplacing = procedure.getUltrasoundCaptureCount() > 0;
            
            // If replacing, delete the old file
            if (isReplacing) {
                String[] existingPaths = procedure.getUltrasoundCapturePaths();
                if (existingPaths.length > 0 && existingPaths[0] != null) {
                    File oldFile = new File(existingPaths[0]);
                    if (oldFile.exists()) {
                        if (oldFile.delete()) {
                            MedDevLog.debug(TAG, "Deleted old ultrasound capture: " + existingPaths[0]);
                        } else {
                            MedDevLog.warn(TAG,
                                    "Failed to delete old ultrasound capture: " + existingPaths[0]);
                        }
                    }
                }
            }
            
            // Generate filename with timestamp and index
            SimpleDateFormat dateFormat = new SimpleDateFormat(
                "yyyyMMdd_HHmmss_SSS", 
                Locale.US
            );
            String timestamp = dateFormat.format(new Date());
            int captureIndex = isReplacing ? 1 : procedure.getUltrasoundCaptureCount() + 1;
            String filename = "ultrasound_capture_" + captureIndex + "_" + timestamp + ".png";
            
            File imageFile = new File(procedureDir, filename);
            
            // Save bitmap to file
            FileOutputStream out = new FileOutputStream(imageFile);
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, out);
            out.flush();
            out.close();
            
            // Add or replace path in EncounterData
            if (isReplacing) {
                // Replace the first capture
                procedure.setUltrasoundCapturePaths(new String[] { imageFile.getAbsolutePath() });
            } else {
                // Add new capture
                procedure.addUltrasoundCapturePath(imageFile.getAbsolutePath());
            }
            
            // Update proceduredata.txt with new paths
            updateProcedureFile();
            
            MedDevLog.debug(TAG, "Saved ultrasound capture: " + imageFile.getAbsolutePath());
            MedDevLog.debug(TAG, "Total captures: " + procedure.getUltrasoundCaptureCount());
            
            return imageFile.getAbsolutePath();
            
        } catch (IOException e) {
            MedDevLog.error(TAG, "Error saving bitmap to procedure directory", e);
            return null;
        }
    }
    
    @Override
    public void onDestroyView() {
        super.onDestroyView();
        
        // Log measurements before cleanup (for debugging/saving)
        if (measurementOverlay != null) {
            java.util.List<MeasurementModel> measurements = measurementOverlay.getMeasurements();
            MedDevLog.debug("UltrasoundCapture", 
                "Fragment destroyed with " + measurements.size() + " measurements");
            for (int i = 0; i < measurements.size(); i++) {
                MeasurementModel m = measurements.get(i);
                MedDevLog.debug("UltrasoundCapture", 
                    String.format("  Measurement %d: %.2f cm", i + 1, m.calculateDistanceCm()));
            }
        }
        
        // Clean up player resources
        if (player != null && player.isRawFileOpen()) {
            player.closeRawFile();
        }
        
        // Detach from surface
        if (viewer != null && surfaceView != null) {
            viewer.setSurface(null);
        }
        
        // Clear measurement overlay
        if (measurementOverlay != null) {
            measurementOverlay.clearMeasurements();
        }
        
        // Clean up magnifier bitmap
        if (currentMergedBitmap != null && !currentMergedBitmap.isRecycled()) {
            currentMergedBitmap.recycle();
            currentMergedBitmap = null;
        }
        
        // Remove any pending magnifier updates
        if (magnifierUpdateHandler != null && magnifierUpdateRunnable != null) {
            magnifierUpdateHandler.removeCallbacks(magnifierUpdateRunnable);
        }
        
        // Unregister USB detachment receiver
        if (usbDetachmentReceiver != null) {
            try {
                requireContext().unregisterReceiver(usbDetachmentReceiver);
                usbDetachmentReceiver = null;
            } catch (IllegalArgumentException e) {
                MedDevLog.debug(TAG, "USB receiver was not registered");
            }
        }
    }
}
