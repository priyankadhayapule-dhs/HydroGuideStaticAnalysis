package com.accessvascularinc.hydroguide.mma.ui.input;

import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.inputmethod.BaseInputConnection;
import android.widget.ArrayAdapter;
import android.widget.Spinner;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.databinding.DataBindingUtil;
import androidx.fragment.app.DialogFragment;
import androidx.lifecycle.ViewModelProvider;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.ConfirmationDialogLayoutBinding;
import com.accessvascularinc.hydroguide.mma.model.EncounterState;
import com.accessvascularinc.hydroguide.mma.serial.Button;
import com.accessvascularinc.hydroguide.mma.serial.ButtonState;
import com.accessvascularinc.hydroguide.mma.sync.SyncManager;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import java.util.Objects;

import javax.inject.Inject;

import dagger.hilt.android.AndroidEntryPoint;

@AndroidEntryPoint
public class ConfirmProcedureDialogFragment extends DialogFragment implements ButtonInputListener {

  protected MainViewModel viewmodel = null;
  ConfirmationDialogLayoutBinding binding;
  Runnable exitRunnable = null;
  Spinner conclusionSpinner = null;
  private BaseInputConnection inputConnection = null;
  private final View.OnClickListener confirmBtn_OnClickListener = view -> confirmSelection();

  public ConfirmProcedureDialogFragment(final Runnable onExit) {
    exitRunnable = onExit;
  }

  @Inject
  protected SyncManager syncManager;

  @Override
  public View onCreateView(@NonNull final LayoutInflater inflater,
                           @Nullable final ViewGroup container,
                           @Nullable final Bundle savedInstanceState) {
    binding = DataBindingUtil.inflate(inflater, R.layout.confirmation_dialog_layout, container,
            false);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);

    final ArrayAdapter<CharSequence> statesArray =
            new ArrayAdapter<>(requireContext(), R.layout.simple_spinner_item);

    statesArray.add("Completed Successfully");
    statesArray.add("Completed Unsuccessfully");
    statesArray.add("Abandoned");
    statesArray.setDropDownViewResource(R.layout.simple_spinner_dropdown_item);
    binding.conclusionSpinner.setAdapter(statesArray);
    conclusionSpinner = binding.conclusionSpinner;

    binding.confirmBtnConclude.setOnClickListener(confirmBtn_OnClickListener);
    binding.cancelButtonConclude.setOnClickListener(view -> dismiss());

    return binding.getRoot();
  }

  @Override
  public void onViewCreated(@NonNull final View view, @Nullable final Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    inputConnection = new BaseInputConnection(requireView(), true);
    viewmodel.getControllerCommunicationManager().addButtonInputListener(this);
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

  private void updateViewModel() {
    switch (conclusionSpinner.getSelectedItem().toString()) {
      case "Completed Successfully" -> {
        viewmodel.getEncounterData().setState(EncounterState.CompletedSuccessful);
        MedDevLog.audit("Procedure Completion", "Procedure Completed Successfully");
      }
      case "Completed Unsuccessfully" -> {
        viewmodel.getEncounterData().setState(EncounterState.CompletedUnsuccessful);
        MedDevLog.audit("Procedure Completion", "Procedure Completed Unsuccessfully");
      }
      case "Abandoned" -> {
        viewmodel.getEncounterData().setState(EncounterState.ConcludedIncomplete);
        MedDevLog.audit("Procedure Completion", "Procedure Abandoned");
      }
      default -> {
      }
    }
  }

  private void confirmSelection() {
    if (syncManager != null) {
      syncManager.setBlockSync(true);
    }
    updateViewModel();
    exitRunnable.run();
    dismiss();
  }

  @Override
  public void onButtonsStateChange(final Button newButton, final String buttonLogMsg) {
    if (getDialog() == null || !getDialog().isShowing()) {
      return;
    }

    for (int idx = 0; idx < Button.ButtonIndex.NumButtons.getIntValue(); idx++) {
      final Button.ButtonIndex buttonIdx = Button.ButtonIndex.valueOfInt(idx);
      final ButtonState buttonState = newButton.buttons.getButtonStateAtKey(
              Objects.requireNonNull(buttonIdx));

      if ((buttonIdx == Button.ButtonIndex.Capture) || (buttonState == ButtonState.ButtonIdle)) {
        continue;
      }

      InputUtils.simulateKeyInput(buttonIdx, buttonState, inputConnection);
    }
  }
}
