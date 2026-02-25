package com.accessvascularinc.hydroguide.mma.ui.captures;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import com.accessvascularinc.hydroguide.mma.databinding.CaptureLayoutBinding;
import com.accessvascularinc.hydroguide.mma.model.Capture;
import java.util.List;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.ui.plot.PlotUtils;
import com.accessvascularinc.hydroguide.mma.ui.plot.PlotStyle;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

public class AllCapturesAdapter extends RecyclerView.Adapter<AllCapturesAdapter.CaptureViewHolder> {
    private static final String TAG = "AllCapturesAdapter";
    private final List<Capture> captures;
    private static final int TYPE_EXTERNAL = 0;
    private static final int TYPE_INTRAVASCULAR = 1;
    private Capture selectedCapture = null;
    private OnCaptureSelectionListener selectionListener;

    public interface OnCaptureSelectionListener {
        void onCaptureSelected(Capture capture);
    }

    public AllCapturesAdapter(List<Capture> captures) {
        this.captures = captures;
    }

    public void setSelectionListener(OnCaptureSelectionListener listener) {
        this.selectionListener = listener;
    }

    public void setSelectedCapture(Capture capture) {
        android.util.Log.d(TAG, "setSelectedCapture: captureId=" + (capture != null ? capture.getCaptureId() : "null"));
        this.selectedCapture = capture;
        notifyDataSetChanged();
    }

    public Capture getSelectedCapture() {
        android.util.Log.d(TAG, "getSelectedCapture: returning captureId="
                + (selectedCapture != null ? selectedCapture.getCaptureId() : "null"));
        return selectedCapture;
    }

    @Override
    public int getItemViewType(int position) {
        android.util.Log.d(TAG, "getItemViewType: position=" + position + ", isIntravascular="
                + captures.get(position).getIsIntravascular());
        Capture capture = captures.get(position);
        return capture.getIsIntravascular() ? TYPE_INTRAVASCULAR : TYPE_EXTERNAL;
    }

    @NonNull
    @Override
    public CaptureViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        android.util.Log.d(TAG, "onCreateViewHolder: viewType=" + viewType);
        LayoutInflater inflater = LayoutInflater.from(parent.getContext());
        CaptureLayoutBinding binding = CaptureLayoutBinding.inflate(inflater, parent, false);
        return new CaptureViewHolder(binding);
    }

    @Override
    public void onBindViewHolder(@NonNull CaptureViewHolder holder, int position) {
        android.util.Log.d(TAG,
                "onBindViewHolder: position=" + position + ", captureId=" + captures.get(position).getCaptureId());
        Capture capture = captures.get(position);
        boolean isSelected = capture != null && capture.equals(selectedCapture);
        android.util.Log.d(TAG,
                "onBindViewHolder: isSelected=" + isSelected + " for captureId=" + capture.getCaptureId());
        holder.bind(capture, isSelected, v -> {
            // Toggle selection
            if (selectedCapture != null && selectedCapture.equals(capture)) {
                android.util.Log.d(TAG, "Deselecting capture: " + capture.getCaptureId());
                selectedCapture = null;
            } else {
                android.util.Log.d(TAG, "Selecting capture: " + capture.getCaptureId());
                selectedCapture = capture;
            }
            notifyDataSetChanged();
            if (selectionListener != null) {
                android.util.Log.d(TAG, "Calling selectionListener with captureId="
                        + (selectedCapture != null ? selectedCapture.getCaptureId() : "null"));
                selectionListener.onCaptureSelected(selectedCapture);
            }
        });
    }

    @Override
    public int getItemCount() {
        return captures.size();
    }

    static class CaptureViewHolder extends RecyclerView.ViewHolder {
        private final CaptureLayoutBinding binding;

        public CaptureViewHolder(CaptureLayoutBinding binding) {
            super(binding.getRoot());
            this.binding = binding;
        }

        public void bind(Capture capture, boolean isSelected, View.OnClickListener clickListener) {
            android.util.Log.d(TAG,
                    "bind: captureId=" + capture.getCaptureId() + ", isIntravascular=" + capture.getIsIntravascular()
                            + ", isSelected=" + isSelected);
            if (capture == null) {
                MedDevLog.error(TAG, "bind: capture is null");
                return;
            }

            // Set visual indication for selection
            if (isSelected) {
                android.util.Log.d(TAG, "bind: Setting SELECTED visual state for captureId=" + capture.getCaptureId()
                        + " (1.05x scale, full opacity, elevation 16f)");
                binding.getRoot().setAlpha(1.0f);
                binding.getRoot().setScaleX(1.05f);
                binding.getRoot().setScaleY(1.05f);
                binding.getRoot().setElevation(16f);
            } else {
                android.util.Log.d(TAG, "bind: Setting UNSELECTED visual state for captureId=" + capture.getCaptureId()
                        + " (1.0x scale, 0.7 opacity, elevation 4f)");
                binding.getRoot().setAlpha(0.7f);
                binding.getRoot().setScaleX(1.0f);
                binding.getRoot().setScaleY(1.0f);
                binding.getRoot().setElevation(4f);
            }

            // android.util.Log.d(TAG, "bind: OnClickListener attached for captureId=\" +
            // capture.getCaptureId());
            binding.getRoot().setOnClickListener(clickListener);
            binding.capturePlotGraph.clear();
            // android.util.Log.d(TAG, "bind: cleared plot graph for captureId=\" +
            // capture.getCaptureId());
            if (capture.getIsIntravascular()) {
                // android.util.Log.d(TAG, "bind: intravascular capture, captureId=\" +
                // capture.getCaptureId());
                // Set heart icon for intravascular
                binding.captureIcon.setImageResource(R.drawable.logo_capture_internal);
                // Show captureId on icon
                binding.captureId.setVisibility(View.VISIBLE);
                binding.captureId.setText(String.valueOf(capture.getCaptureId()));
                // Show both inserted and exposed length
                binding.captureHeaderLabel.setVisibility(View.VISIBLE);
                binding.captureInsertedLength.setVisibility(View.VISIBLE);
                binding.captureFooterLabel.setVisibility(View.VISIBLE);
                binding.captureExposedLength.setVisibility(View.VISIBLE);
                // Set text for inserted and exposed length
                binding.captureInsertedLength.setText(String.format("%.1f cm", capture.getInsertedLengthCm()));
                binding.captureExposedLength.setText(String.format("%.1f cm", capture.getExposedLengthCm()));
            } else {
                android.util.Log.d(TAG, "bind: external capture");
                // Set lock icon for external
                binding.captureIcon.setImageResource(R.drawable.logo_external);
                // Hide captureId for external
                binding.captureId.setVisibility(View.INVISIBLE);
                // Only show inserted length
                binding.captureHeaderLabel.setVisibility(View.VISIBLE);
                binding.captureInsertedLength.setVisibility(View.VISIBLE);
                binding.captureFooterLabel.setVisibility(View.INVISIBLE);
                binding.captureExposedLength.setVisibility(View.INVISIBLE);
                binding.captureInsertedLength.setText(String.format("%.1f cm", capture.getInsertedLengthCm()));
            }
            // Null checks before plotting
            if (capture.getCaptureData() == null || capture.getCaptureData().getData() == null) {
                MedDevLog.error(TAG, "Capture data is null for captureId=" + capture.getCaptureId());
                return;
            }
            // Ensure formatter is set
            if (capture.getFormatter() == null) {
                PlotStyle style = capture.getIsIntravascular() ? PlotStyle.Intravascular : PlotStyle.External;
                int trailSize = capture.getCaptureData().getData().length;
                capture.setFormatter(com.accessvascularinc.hydroguide.mma.ui.plot.PlotUtils
                        .getFormatter(binding.getRoot().getContext(), style, 0));
                android.util.Log.d(TAG, "Formatter set for captureId=" + capture.getCaptureId());
            }
            if (capture.getFormatter() == null) {
                MedDevLog.error(TAG, "Formatter is null for captureId=" + capture.getCaptureId());
                return;
            }
            com.accessvascularinc.hydroguide.mma.ui.plot.PlotUtils.drawCaptureGraph(binding.capturePlotGraph, capture);
            PlotUtils.stylePlot(binding.capturePlotGraph);
            android.util.Log.d(TAG, "bind: called drawCaptureGraph for captureId=" + capture.getCaptureId());
        }
    }
}
