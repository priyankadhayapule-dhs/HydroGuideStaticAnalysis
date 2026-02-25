package com.accessvascularinc.hydroguide.mma.ui.captures;

import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.accessvascularinc.hydroguide.mma.ui.input.BaseValueSelectDialogFragment;

import androidx.fragment.app.DialogFragment;
import androidx.recyclerview.widget.RecyclerView;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.model.Capture;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;
import android.view.inputmethod.BaseInputConnection;

import androidx.lifecycle.ViewModelProvider;

public class AllCapturesDialogFragment extends BaseValueSelectDialogFragment {
    private static final String TAG = "AllCapturesDialogFragment";
    private static final String ARG_CAPTURE_IDS = "captureIds";
    private static final String ARG_CAPTURE_IS_INTRA = "captureIsIntra";
    private static final String ARG_SELECTED_CAPTURE_ID = "selectedCaptureId";
    private Capture[] captures;
    private Capture selectedCapture;
    private AllCapturesAdapter adapter;

    public static AllCapturesDialogFragment newInstance(Capture[] captures) {
        android.util.Log.d(TAG, "newInstance() called with " + (captures != null ? captures.length : "null")
                + " captures (no selected capture)");
        return newInstance(captures, null);
    }

    public static AllCapturesDialogFragment newInstance(Capture[] captures, Capture selectedCapture) {
        android.util.Log.d(TAG, "newInstance: captures=" + (captures != null ? captures.length : "null")
                + ", selectedCapture=" + (selectedCapture != null ? selectedCapture.getCaptureId() : "null"));
        if (captures != null) {
            android.util.Log.d(TAG, "Capture details: ");
            for (int i = 0; i < captures.length; i++) {
                android.util.Log.d(TAG, "  [" + i + "] captureId=" + captures[i].getCaptureId() +
                        ", isIntravascular=" + captures[i].getIsIntravascular());
            }
        }
        AllCapturesDialogFragment fragment = new AllCapturesDialogFragment();
        Bundle args = new Bundle();

        // Convert Capture objects to IDs for serialization
        if (captures != null) {
            int[] captureIds = new int[captures.length];
            boolean[] captureIsIntra = new boolean[captures.length];
            for (int i = 0; i < captures.length; i++) {
                captureIds[i] = captures[i].getCaptureId();
                captureIsIntra[i] = captures[i].getIsIntravascular();
            }
            args.putIntArray(ARG_CAPTURE_IDS, captureIds);
            args.putBooleanArray(ARG_CAPTURE_IS_INTRA, captureIsIntra);
        }

        if (selectedCapture != null) {
            args.putInt(ARG_SELECTED_CAPTURE_ID, selectedCapture.getCaptureId());
            android.util.Log.d(TAG,
                    "newInstance: Added selectedCaptureId=" + selectedCapture.getCaptureId() + " to bundle");
        } else {
            args.putInt(ARG_SELECTED_CAPTURE_ID, -1);
            android.util.Log.d(TAG, "newInstance: No selectedCapture provided");
        }
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        android.util.Log.d(TAG, "onCreate called");
        super.onCreate(savedInstanceState);
        if (getArguments() != null) {
            int[] captureIds = getArguments().getIntArray(ARG_CAPTURE_IDS);
            boolean[] captureIsIntra = getArguments().getBooleanArray(ARG_CAPTURE_IS_INTRA);

            if (captureIds != null && captureIsIntra != null) {
                // We'll reconstruct Capture objects in onCreateView when we have access to
                // ViewModel
                android.util.Log.d(TAG, "onCreate: Retrieved " + captureIds.length + " capture IDs from bundle");
            } else {
                android.util.Log.d(TAG, "onCreate: No capture IDs in bundle");
            }

            int selectedId = getArguments().getInt(ARG_SELECTED_CAPTURE_ID, -1);
            android.util.Log.d(TAG, "onCreate: selectedCaptureId from bundle=" + selectedId);
        } else {
            android.util.Log.d(TAG, "onCreate: No arguments provided");
        }
        setStyle(DialogFragment.STYLE_NO_FRAME, R.style.AllCapturesDialogTheme);
        android.util.Log.d(TAG, "onCreate completed");
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        android.util.Log.d(TAG, "onCreateView: called");

        // Initialize ViewModel to access EncounterData
        viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);

        // Reconstruct Capture objects from IDs stored in Bundle
        if (getArguments() != null) {
            int[] captureIds = getArguments().getIntArray(ARG_CAPTURE_IDS);
            boolean[] captureIsIntra = getArguments().getBooleanArray(ARG_CAPTURE_IS_INTRA);
            int selectedId = getArguments().getInt(ARG_SELECTED_CAPTURE_ID, -1);

            if (captureIds != null && captureIsIntra != null) {
                // Look up Capture objects from EncounterData using IDs
                java.util.List<Capture> captureList = new java.util.ArrayList<>();
                Capture externalCapture = viewmodel.getEncounterData().getExternalCapture();
                Capture[] intravCaptures = viewmodel.getEncounterData().getIntravCaptures();

                for (int i = 0; i < captureIds.length; i++) {
                    int id = captureIds[i];
                    if (captureIsIntra[i]) {
                        // Find intravascular capture by ID
                        if (intravCaptures != null) {
                            for (Capture c : intravCaptures) {
                                if (c.getCaptureId() == id) {
                                    captureList.add(c);
                                    break;
                                }
                            }
                        }
                    } else {
                        // External capture
                        if (externalCapture != null && externalCapture.getCaptureId() == id) {
                            captureList.add(externalCapture);
                        }
                    }
                }
                captures = captureList.toArray(new Capture[0]);
                android.util.Log.d(TAG, "onCreateView: Reconstructed " + captures.length + " captures from IDs");

                // Set selected capture if ID is valid
                if (selectedId >= 0) {
                    for (Capture c : captures) {
                        if (c.getCaptureId() == selectedId) {
                            selectedCapture = c;
                            android.util.Log.d(TAG, "onCreateView: Found selectedCapture with id=" + selectedId);
                            break;
                        }
                    }
                }
            }
        }

        android.util.Log.d(TAG,
                "onCreateView: called with " + (captures != null ? captures.length : "null") + " captures");
        android.util.Log.d(TAG, "onCreateView: Current selectedCapture="
                + (selectedCapture != null ? selectedCapture.getCaptureId() : "null"));

        View view = inflater.inflate(R.layout.dialog_all_captures, container, false);

        // Set up a single RecyclerView for all captures (external + internal) in a
        // 4-column grid
        RecyclerView recyclerView = view.findViewById(R.id.all_captures_recycler);
        if (recyclerView != null && captures != null) {
            android.util.Log.d(TAG, "RecyclerView found, setting up with " + captures.length + " captures");
            recyclerView.setLayoutManager(
                    new androidx.recyclerview.widget.GridLayoutManager(getContext(), 4));
            adapter = new AllCapturesAdapter(java.util.Arrays.asList(captures));
            android.util.Log.d(TAG, "AllCapturesAdapter created");

            // Set selected capture if it exists
            if (selectedCapture != null) {
                android.util.Log.d(TAG, "Setting adapter selectedCapture to: " + selectedCapture.getCaptureId());
                adapter.setSelectedCapture(selectedCapture);
                android.util.Log.d(TAG,
                        "Adapter selectedCapture is now: "
                                + (adapter.getSelectedCapture() != null ? adapter.getSelectedCapture().getCaptureId()
                                        : "null"));
            } else {
                android.util.Log.d(TAG, "No selectedCapture to set in adapter");
            }

            // Set selection listener
            adapter.setSelectionListener(capture -> {
                selectedCapture = capture;
                android.util.Log.d(TAG,
                        "[SELECTION EVENT] onCaptureSelected callback: captureId="
                                + (capture != null ? capture.getCaptureId() : "null") +
                                ", isIntravascular=" + (capture != null ? capture.getIsIntravascular() : "N/A"));

                // Update EncounterData with the selected capture
                if (viewmodel != null && viewmodel.getEncounterData() != null) {
                    android.util.Log.d(TAG, "Updating EncounterData.selectedCapture to: "
                            + (selectedCapture != null ? selectedCapture.getCaptureId() : "null"));
                    viewmodel.getEncounterData().setSelectedCapture(selectedCapture);
                    android.util.Log.d(TAG,
                            "EncounterData.selectedCapture is now: "
                                    + (viewmodel.getEncounterData().getSelectedCapture() != null
                                            ? viewmodel.getEncounterData().getSelectedCapture().getCaptureId()
                                            : "null"));
                } else {
                    android.util.Log.e(TAG, "ERROR: viewmodel or encounterData is null!");
                }
            });
            android.util.Log.d(TAG, "Selection listener set on adapter");

            recyclerView.setAdapter(adapter);
            android.util.Log.d(TAG, "Adapter attached to RecyclerView");

            // Add vertical spacing between rows
            int verticalSpace = (int) (getResources().getDisplayMetrics().density * 16); // 16dp
            recyclerView.addItemDecoration(new androidx.recyclerview.widget.RecyclerView.ItemDecoration() {
                @Override
                public void getItemOffsets(android.graphics.Rect outRect, View view, RecyclerView parent,
                        RecyclerView.State state) {
                    int position = parent.getChildAdapterPosition(view);
                    int spanCount = 4;
                    if (position >= spanCount) {
                        outRect.top = verticalSpace;
                    }
                }
            });
        } else {
            android.util.Log.e(TAG, "ERROR: RecyclerView not found or captures is null!");
        }

        view.findViewById(R.id.close_button).setOnClickListener(v -> {
            android.util.Log.d(TAG, "Close button clicked");
            dismiss();
        });
        android.util.Log.d(TAG, "Dialog view created and configured");

        // Set transparent background for dialog
        if (getDialog() != null && getDialog().getWindow() != null) {
            getDialog().getWindow().setBackgroundDrawableResource(android.R.color.transparent);
        }

        // Setup ButtonController navigation (same as CaptureDialogFragment)
        // viewmodel already initialized above in onCreateView
        android.util.Log.d(TAG, "ViewModel already initialized");
        inputConnection = new BaseInputConnection(view, true);
        viewmodel.getControllerCommunicationManager().addButtonInputListener(this);

        return view;
    }

    @Override
    public void onStart() {
        android.util.Log.d(TAG, "onStart called");
        super.onStart();
        Dialog dialog = getDialog();
        if (dialog != null) {
            Window window = dialog.getWindow();
            if (window != null) {
                float density = requireContext().getResources().getDisplayMetrics().density;
                int width = ViewGroup.LayoutParams.MATCH_PARENT;
                int contentHeight = (int) (700 * density);
                int screenHeight = requireContext().getResources().getDisplayMetrics().heightPixels;
                // Center the dialog vertically between top and bottom bars
                int topBarHeight = (int) requireContext().getResources().getDimension(R.dimen.top_bar_height);
                int bottomBarHeight = (int) requireContext().getResources().getDimension(R.dimen.bottom_bar_height);
                int availableHeight = screenHeight - topBarHeight - bottomBarHeight;
                int yOffset = topBarHeight + (availableHeight - contentHeight) / 2;
                window.setLayout(width, contentHeight);
                android.view.WindowManager.LayoutParams params = window.getAttributes();
                params.y = yOffset;
                window.setAttributes(params);
            }
        }
    }

    @Override
    public void onDestroyView() {
        if (viewmodel != null) {
            viewmodel.getControllerCommunicationManager().removeButtonInputListener(this);
        }
        super.onDestroyView();
    }
}
