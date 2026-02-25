package com.accessvascularinc.hydroguide.mma.model;


import androidx.annotation.Nullable;

// Low numerical priority indicates high importance
public enum AlarmType {
  SupplyRangeFault(6),
  FuelGaugeStatusFault(5),
  EcgDataMissing(4),
  ControllerStatusTimeout(3),
  BatterySubsystemFault(2),
  InternalLeadOff(1),
  ExternalLeadOff(0);
  private final int numVal;

  AlarmType(final int numVal) {
    this.numVal = numVal;
  }

  @Nullable
  public static AlarmType valueOfInt(final int numValue) {
    for (final AlarmType e : values()) {
      if (e.getNumVal() == numValue) {
        return e;
      }
    }
    return null;
  }

  public int getNumVal() {
    return numVal;
  }
}

