package com.accessvascularinc.hydroguide.mma.model;

import androidx.annotation.Nullable;

public enum EncounterState {
  Idle(0),
  Uninitialized(1),
  Active(3),
  Suspended(4),
  IntraSuspended(5),
  ExternalSuspended(6),
  DualSuspended(7),
  CompletedSuccessful(8),
  CompletedUnsuccessful(9),
  ConcludedIncomplete(10);
  private final int numValue;

  EncounterState(final int numVal) {
    this.numValue = numVal;
  }

  @Nullable
  public static EncounterState valueOfInt(final int numValue) {
    for (final EncounterState e : values()) {
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
