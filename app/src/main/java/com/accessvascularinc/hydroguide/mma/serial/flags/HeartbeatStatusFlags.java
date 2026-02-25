package com.accessvascularinc.hydroguide.mma.serial.flags;

import androidx.annotation.Nullable;

public enum HeartbeatStatusFlags {
  None(0),
  PWaveAbsent(1);

  private final int numValue;

  HeartbeatStatusFlags(final int numVal) {
    this.numValue = numVal;
  }

  @Nullable
  public static HeartbeatStatusFlags valueOfInt(final int numValue) {
    for (final HeartbeatStatusFlags e : values()) {
      if (e.getNumValue() == numValue) {
        return e;
      }
    }
    return null;
  }

  private int getNumValue() {
    return numValue;
  }
}
