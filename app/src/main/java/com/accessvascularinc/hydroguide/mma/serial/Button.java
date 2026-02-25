package com.accessvascularinc.hydroguide.mma.serial;

import android.util.Log;

import androidx.annotation.Nullable;

import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

public class Button {
  public final int BYTE_LENGTH = 6;
  public final ButtonCollection buttons = new ButtonCollection();

  public Button(final byte[] payload) {
    if (payload.length == BYTE_LENGTH) {
      for (int i = 0; i < ButtonIndex.NumButtons.getIntValue(); i++) {
        buttons.setButtonStateAtKey(i, ButtonState.valueOfByte(payload[i]));
      }
    } else {
      MedDevLog.error("Button Unpacking", String.format(
              "Length mismatch for Button, got %s bytes, expected 6", payload.length));
    }
  }

  public enum ButtonIndex {
    Up(0),
    Down(1),
    Left(2),
    Right(3),
    Select(4), // AKA: Action
    Capture(5),
    NumButtons(6);

    private final int numberValue;

    ButtonIndex(final int numVal) {
      this.numberValue = numVal;
    }

    @Nullable
    public static ButtonIndex valueOfInt(final int newNumValue) {
      for (final ButtonIndex e : values()) {
        if (e.getIntValue() == newNumValue) {
          return e;
        }
      }
      return null;
    }

    public int getIntValue() {
      return numberValue;
    }
  }

  public static class ButtonCollection {
    private final ButtonState[] content = new ButtonState[ButtonIndex.NumButtons.getIntValue()];

    public ButtonState getButtonStateAtKey(final ButtonIndex key) {
      if ((key.getIntValue() < 0) || (key.getIntValue() >= ButtonIndex.NumButtons.getIntValue())) {
        throw new IllegalArgumentException(key.name());
      }

      return content[key.getIntValue()];
    }

    public void setButtonStateAtKey(final int btnIdx, final ButtonState state) {
      content[btnIdx] = state;
    }

    public ButtonState[] getButtonContent() {
      return content.clone();
    }
  }
}
