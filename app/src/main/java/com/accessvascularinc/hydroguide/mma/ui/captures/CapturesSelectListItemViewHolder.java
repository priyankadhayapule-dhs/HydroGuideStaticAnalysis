package com.accessvascularinc.hydroguide.mma.ui.captures;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.model.Capture;

import java.util.Locale;

public class CapturesSelectListItemViewHolder extends RecyclerView.ViewHolder {
  public final CheckBox cbSelectCapture;
  public final ImageView ivCaptureIcon;
  public final TextView tvCapNumber, tvCathInsertionLength;

  public CapturesSelectListItemViewHolder(@NonNull final View view) {
    super(view);
    cbSelectCapture = itemView.findViewById(R.id.capture_select_item_checkbox);
    ivCaptureIcon = itemView.findViewById(R.id.capture_select_item_icon);
    tvCapNumber = itemView.findViewById(R.id.capture_select_item_number);
    tvCathInsertionLength = itemView.findViewById(R.id.capture_select_item_length);
  }

  @NonNull
  public static CapturesSelectListItemViewHolder create(@NonNull final ViewGroup parent) {
    return new CapturesSelectListItemViewHolder(LayoutInflater.from(parent.getContext())
            .inflate(R.layout.capture_select_list_item, parent, false));
  }

  public void setCapture(final Capture capture, final boolean useExposedLength) {
    if (capture == null) {
      cbSelectCapture.setChecked(false);
      cbSelectCapture.setEnabled(false);
      ivCaptureIcon.setImageResource(R.drawable.logo_external);
      tvCapNumber.setText("");
      tvCathInsertionLength.setText(R.string.N_A);
    } else {
      cbSelectCapture.setChecked(capture.getShowInReport());
      ivCaptureIcon.setImageResource(
              capture.getIsIntravascular() ? R.drawable.logo_capture_internal :
                      R.drawable.logo_external);
      tvCapNumber.setText(
              capture.getIsIntravascular() ? Integer.toString(capture.getCaptureId()) : "");

      if (useExposedLength) {
        final String newExtText = String.format(Locale.US, "%.1f", capture.getExposedLengthCm());
        tvCathInsertionLength.setText(String.format("%s cm", newExtText));
      } else {
        final String newInsText = String.format(Locale.US, "%.1f", capture.getInsertedLengthCm());
        tvCathInsertionLength.setText(String.format("%s cm", newInsText));
      }
    }
  }

  public void setUltrasound(final String label, final String fileName, final boolean isChecked) {
    cbSelectCapture.setChecked(isChecked);
    cbSelectCapture.setEnabled(true);
    ivCaptureIcon.setImageResource(R.drawable.transparent_ultrasound);
    tvCapNumber.setText("");
    tvCathInsertionLength.setText(R.string.ultrasound_capture_label);
  }
}