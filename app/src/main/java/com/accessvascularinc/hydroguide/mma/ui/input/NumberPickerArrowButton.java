package com.accessvascularinc.hydroguide.mma.ui.input;

import android.content.Context;
import android.util.AttributeSet;

import androidx.appcompat.widget.AppCompatImageButton;

public class NumberPickerArrowButton extends AppCompatImageButton {

  public NumberPickerArrowButton(final Context context) {
    super(context);
  }

  public NumberPickerArrowButton(final Context context, final AttributeSet attrs) {
    super(context, attrs);
  }

  @Override
  public boolean performClick() {
    super.performClick();
    return true;
  }
}
