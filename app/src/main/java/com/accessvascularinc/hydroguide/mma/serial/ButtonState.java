package com.accessvascularinc.hydroguide.mma.serial;

import android.util.Log;

import androidx.annotation.Nullable;

public enum ButtonState {
  ButtonIdle((byte) 0),
  ReleaseTransition((byte) (ButtonIdle.getByteValue() | ButtonStateMasks.Edge_Mask.getByteValue())),
  ButtonHeld(ButtonStateMasks.Level_Mask.getByteValue()),
  PressTransition((byte) (ButtonHeld.getByteValue() + ButtonStateMasks.Edge_Mask.getByteValue()));

  private final byte byteValue;

  ButtonState(final byte byteVal) {
    this.byteValue = byteVal;
  }

  @Nullable
  public static ButtonState valueOfByte(final byte byteValue) {

    for (final ButtonState e : values()) {
      if (e.getByteValue() == byteValue) {
        return e;
      }
    }
    return null;
  }

  public byte getByteValue() {
    return byteValue;
  }


  private enum ButtonStateMasks {
    Edge_Mask((byte) 0x01),
    Level_Mask((byte) 0x02);

    private final byte byteValue;

    ButtonStateMasks(final byte byteVal) {
      this.byteValue = byteVal;
    }

    @Nullable
    public static ButtonStateMasks valueOfByte(final byte byteValue) {
      for (final ButtonStateMasks e : values()) {
        if (e.getByteValue() == byteValue) {
          return e;
        }
      }
      return null;
    }

    public byte getByteValue() {
      return byteValue;
    }
  }
}