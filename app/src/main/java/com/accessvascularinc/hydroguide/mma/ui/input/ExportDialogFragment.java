package com.accessvascularinc.hydroguide.mma.ui.input;

import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.ImageButton;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.databinding.DataBindingUtil;
import androidx.fragment.app.DialogFragment;
import androidx.lifecycle.ViewModelProvider;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.ExportDialogBinding;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;

import java.util.Objects;

public class ExportDialogFragment extends DialogFragment {

  protected MainViewModel viewmodel = null;
  protected ImageButton exportButton = null;
  protected ImageButton deleteAndExportButton = null;
  protected ImageButton quitButton = null;
  protected String description = "";
  ExportDialogBinding binding;
  Runnable exportRunnable = null;
  Runnable deleteRunnable = null;
  private final View.OnClickListener exitBtn_OnClickListener = view -> dismiss();
  private final View.OnClickListener eadBtn_OnClickListener = view -> exportAndDelete();
  private final View.OnClickListener eBtn_OnClickListener = view -> exportRunnable.run();

  public ExportDialogFragment() {
  }

  public void setRunnables(final Runnable onExport, final Runnable onDelete, final String desc) {
    exportRunnable = onExport;
    deleteRunnable = onDelete;
    description = desc;
  }

  @Override
  public View onCreateView(@NonNull final LayoutInflater inflater,
                           @Nullable final ViewGroup container,
                           @Nullable final Bundle savedInstanceState) {
    binding = DataBindingUtil.inflate(inflater, R.layout.export_dialog, container,
            false);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    super.setCancelable(false);

    exportButton = binding.exportButton;
    deleteAndExportButton = binding.buttonDeleteAndExport;
    quitButton = binding.exportQuitButton;

    final TextView descView = binding.exportDialogDescription;
    descView.setText(description);

    exportButton.setOnClickListener(eBtn_OnClickListener);
    deleteAndExportButton.setOnClickListener(eadBtn_OnClickListener);
    quitButton.setOnClickListener(exitBtn_OnClickListener);

    return binding.getRoot();
  }

  public void toggleProgressBar(final boolean barShown) {
    if (barShown) {
      binding.exportSelectionLayout.setVisibility(View.GONE);
      binding.progressLayout.setVisibility(View.VISIBLE);
    } else {
      binding.exportSelectionLayout.setVisibility(View.VISIBLE);
      binding.progressLayout.setVisibility(View.GONE);
    }
  }

  public void completeProgress() {
    binding.inProgressMessage.setText("Export Complete");
    binding.progressQuitButton.setVisibility(View.VISIBLE);
    binding.progressQuitButton.setOnClickListener(view -> dismiss());
  }

  public void updateProgressBar(final double value) {
    binding.exportProgressBar.setProgress((int) Math.ceil(value));
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(final Bundle savedInstanceState) {
    final Dialog dialog = super.onCreateDialog(savedInstanceState);
    dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
    Objects.requireNonNull(dialog.getWindow()).setBackgroundDrawable(
            ContextCompat.getDrawable(requireContext(), R.drawable.capture_dialog));
    return dialog;
  }

  private void exportAndDelete() {
    exportRunnable.run();
    deleteRunnable.run();
  }

}
