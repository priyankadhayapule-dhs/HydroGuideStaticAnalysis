package com.accessvascularinc.hydroguide.mma.serial.flags;

import androidx.annotation.NonNull;

public class BitFlags<E> {
  public final int numVal;

  public BitFlags(final int combinedValue) {
    numVal = combinedValue;
  }

  @NonNull
  @Override
  public String toString() {
    return "(flags) " + numVal;
  }

  public int toInt() {
    return numVal;
  }
}
