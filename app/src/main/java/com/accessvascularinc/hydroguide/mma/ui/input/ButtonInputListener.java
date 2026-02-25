package com.accessvascularinc.hydroguide.mma.ui.input;

import com.accessvascularinc.hydroguide.mma.serial.Button;

@FunctionalInterface
public interface ButtonInputListener {
  void onButtonsStateChange(Button newButton, String buttonLogMsg);
}
