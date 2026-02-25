package com.accessvascularinc.hydroguide.mma.model;

import androidx.annotation.Nullable;

public enum SortMode {
  None(0),
  Name(1),
  Date(2),
  Id(3);
  private final int numValue;

  SortMode(final int numVal) {
    this.numValue = numVal;
  }

  @Nullable
  public static SortMode valueOfInt(final int numVal) {
    for (final SortMode e : values()) {
      if (e.getNumValue() == numVal) {
        return e;
      }
    }
    return null;
  }

  private int getNumValue() {
    return numValue;
  }
}
