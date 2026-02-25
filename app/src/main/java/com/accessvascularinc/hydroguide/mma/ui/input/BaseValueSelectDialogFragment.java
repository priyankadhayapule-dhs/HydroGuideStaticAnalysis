package com.accessvascularinc.hydroguide.mma.ui.input;

import android.app.Dialog;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.inputmethod.BaseInputConnection;
import android.widget.NumberPicker;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.DialogFragment;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.serial.Button;
import com.accessvascularinc.hydroguide.mma.serial.ButtonState;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;

import java.util.Objects;

public class BaseValueSelectDialogFragment extends DialogFragment implements ButtonInputListener {
  protected MainViewModel viewmodel = null;
  protected NumberPicker numberPicker = null;
  protected NumberPicker numberPicker_internal = null;

  protected String[] wheelValues = null;
  protected String[] wheelValuesInternal = null;

  protected NumberPickerArrowButton upArrowBtn = null;
  protected NumberPickerArrowButton downArrowBtn = null;
  protected NumberPickerArrowButton upArrowBtn_internal = null;
  protected NumberPickerArrowButton downArrowBtn_internal = null;
  protected BaseInputConnection inputConnection = null;

  private static View.OnTouchListener getOnTouchListener(final NumberPicker numPicker,
                                                         final int keyCode) {
    return new View.OnTouchListener() {
      @Nullable
      private Handler mHandler = null;
      final private Runnable mAction = new Runnable() {
        @Override
        public void run() {
          numPicker.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, keyCode));
          mHandler.postDelayed(this, 100);
        }
      };

      @Override
      public boolean onTouch(final View view, final MotionEvent motionEvent) {
        view.performClick();
        switch (motionEvent.getAction()) {
          case MotionEvent.ACTION_DOWN:
            if (mHandler == null) {
              mHandler = new Handler(Looper.getMainLooper());
              mHandler.post(mAction);
              break;
            } else {
              return true;
            }
          case MotionEvent.ACTION_UP:
            if (mHandler == null) {
              return true;
            }
            mHandler.removeCallbacks(mAction);
            mHandler = null;
            break;
        }
        return false;
      }
    };
  }

  @Override
  public void onViewCreated(@NonNull final View view, @Nullable final Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    inputConnection = new BaseInputConnection(requireView(), true);
    viewmodel.getControllerCommunicationManager().addButtonInputListener(this);
    //TODO : Uncomment when BLE is supported
    //viewmodel.getBleControllerManager().addButtonInputListener(this);
  }

  @Override
  public void onDestroyView() {
    viewmodel.getControllerCommunicationManager().removeButtonInputListener(this);
    //TODO : Uncomment when BLE is supported
    //viewmodel.getBleControllerManager().removeButtonInputListener(this);
    super.onDestroyView();
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

  protected void initNumberPicker(final int selectedValueIdx) {
    numberPicker.setMinValue(0);
    numberPicker.setMaxValue(wheelValues.length - 1);
    numberPicker.setWrapSelectorWheel(false);
    numberPicker.setDisplayedValues(wheelValues);
    numberPicker.setValue(selectedValueIdx);

    upArrowBtn.setOnTouchListener(getOnTouchListener(numberPicker, KeyEvent.KEYCODE_DPAD_UP));
    downArrowBtn.setOnTouchListener(getOnTouchListener(numberPicker, KeyEvent.KEYCODE_DPAD_DOWN));
  }

  protected void initNumberPickerInternal(final int selectedValueIdx) {
    numberPicker_internal.setMinValue(0);
    numberPicker_internal.setMaxValue(wheelValuesInternal.length - 1);
    numberPicker_internal.setWrapSelectorWheel(false);
    numberPicker_internal.setDisplayedValues(wheelValuesInternal);
    numberPicker_internal.setValue(selectedValueIdx);

    upArrowBtn_internal.setOnTouchListener(getOnTouchListener(numberPicker_internal, KeyEvent.KEYCODE_DPAD_UP));
    downArrowBtn_internal.setOnTouchListener(getOnTouchListener(numberPicker_internal, KeyEvent.KEYCODE_DPAD_DOWN));
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
