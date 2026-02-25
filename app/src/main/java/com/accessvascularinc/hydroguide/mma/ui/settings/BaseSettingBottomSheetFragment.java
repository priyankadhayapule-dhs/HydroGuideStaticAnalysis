package com.accessvascularinc.hydroguide.mma.ui.settings;

import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.inputmethod.BaseInputConnection;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.accessvascularinc.hydroguide.mma.serial.Button;
import com.accessvascularinc.hydroguide.mma.serial.ButtonState;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;
import com.accessvascularinc.hydroguide.mma.ui.input.ButtonInputListener;
import com.accessvascularinc.hydroguide.mma.ui.input.InputUtils;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import java.util.Objects;

public class BaseSettingBottomSheetFragment extends BottomSheetDialogFragment
        implements ButtonInputListener {
  protected MainViewModel viewmodel = null;
  private BaseInputConnection inputConnection = null;

  @Override
  public void onViewCreated(@NonNull final View view, @Nullable final Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    viewmodel.getControllerCommunicationManager().addButtonInputListener(this);
    //viewmodel.getBleControllerManager().addButtonInputListener(this);
    inputConnection = new BaseInputConnection(requireView(), true);
  }

  @Override
  public void onDestroyView() {
    viewmodel.getControllerCommunicationManager().removeButtonInputListener(this);
    //viewmodel.getBleControllerManager().removeButtonInputListener(this);
    super.onDestroyView();
  }

  @Override
  public void onStart() {
    final var sheet = (BottomSheetDialog) getDialog();
    final var behavior = Objects.requireNonNull(sheet).getBehavior();
    behavior.setMaxWidth(-1);
    behavior.setState(BottomSheetBehavior.STATE_EXPANDED);

    final Window window = getDialog().getWindow();
    Objects.requireNonNull(window).setDimAmount(0);

    super.onStart();
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
