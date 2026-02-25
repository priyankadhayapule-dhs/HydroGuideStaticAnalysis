package com.accessvascularinc.hydroguide.mma.ui.input;

import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.ArrayAdapter;
import android.widget.Spinner;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.databinding.DataBindingUtil;
import androidx.fragment.app.DialogFragment;
import androidx.lifecycle.ViewModelProvider;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.UsbSelectionDialogLayoutBinding;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;

import java.util.Objects;

public class StorageSelectionDialogFragment extends DialogFragment {

  protected MainViewModel viewmodel = null;
  protected String[] externalDrives = null;
  protected String[] selectedDrive = null;
  protected Spinner storageSelectionSpinner = null;
  UsbSelectionDialogLayoutBinding binding;
  Runnable exitRunnable = null;
  private final View.OnClickListener confirmBtn_OnClickListener = view -> confirmSelection();

  public StorageSelectionDialogFragment(final String[] externalDrives, final String[] selectedDrive,
                                        final Runnable onExit) {
    this.externalDrives = externalDrives;
    this.selectedDrive = selectedDrive;
    exitRunnable = onExit;
  }

  @Override
  public View onCreateView(@NonNull final LayoutInflater inflater,
                           @Nullable final ViewGroup container,
                           @Nullable final Bundle savedInstanceState) {
    binding = DataBindingUtil.inflate(inflater, R.layout.usb_selection_dialog_layout, container,
            false);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);

    storageSelectionSpinner = binding.extStoreSpinner;

    final ArrayAdapter<CharSequence> externalsArray =
            new ArrayAdapter<>(requireContext(), R.layout.simple_spinner_item);

    for (final String drive : externalDrives) {
      externalsArray.add(drive);
    }
    externalsArray.setDropDownViewResource(R.layout.simple_spinner_dropdown_item);
    storageSelectionSpinner.setAdapter(externalsArray);

    binding.confirmBtnStorage.setOnClickListener(confirmBtn_OnClickListener);
    binding.cancelButtonSelect.setOnClickListener(view -> dismiss());

    return binding.getRoot();
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

  private void confirmSelection() {
    selectedDrive[0] = storageSelectionSpinner.getSelectedItem().toString();
    exitRunnable.run();
    dismiss();
  }
}
