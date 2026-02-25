package com.accessvascularinc.hydroguide.mma.serial.flags;

public enum FuelGaugeStatusFlags {
  None(0),
  StateOfChargeTooHigh(1 << 5),
  UndervoltageLockout(1 << 4),
  BatteryVoltageLowWarning(1 << 3),
  StateOfChargeCriticallyLow(1 << 2),
  BatteryNotConnected(1 << 1),
  FuelGaugeHardwareFault(1);

  private final int numValue;

  FuelGaugeStatusFlags(final int numVal) {
    this.numValue = numVal;
  }

  public static BitFlags<FuelGaugeStatusFlags> valueOfInt(final int numValue) {
    return new BitFlags<FuelGaugeStatusFlags>(numValue);
  }

  /**
   * @param combined A bitflags instance where zero or more individual flags have been combined by
   *                 bitwise-OR
   *
   * @return true if "this" flag is set in the combined bitflags
   */
  public boolean isFlaggedIn(final BitFlags<FuelGaugeStatusFlags> combined) {
    return (combined != null) && (combined.toInt() & numValue) != 0;
  }

  private int getNumValue() {
    return numValue;
  }

  public String getCompositeString(final BitFlags<FuelGaugeStatusFlags> combined) {
    String returnString = "";
    if (combined == null) {
      return returnString;
    }

    if ((combined.toInt() & FuelGaugeStatusFlags.StateOfChargeTooHigh.getNumValue()) != 0) {
      returnString += "State of Charge Too High, ";
    }
    if ((combined.toInt() & FuelGaugeStatusFlags.UndervoltageLockout.getNumValue()) != 0) {
      returnString += "Undervoltage Lockout, ";
    }
    if ((combined.toInt() & FuelGaugeStatusFlags.BatteryVoltageLowWarning.getNumValue()) != 0) {
      returnString += "Battery Voltage Low Warning, ";
    }
    if ((combined.toInt() & FuelGaugeStatusFlags.StateOfChargeCriticallyLow.getNumValue()) != 0) {
      returnString += "State of Charge Critically Low, ";
    }
    if ((combined.toInt() & FuelGaugeStatusFlags.BatteryNotConnected.getNumValue()) != 0) {
      returnString += "Battery Not Connected, ";
    }
    if ((combined.toInt() & FuelGaugeStatusFlags.FuelGaugeHardwareFault.getNumValue()) != 0) {
      returnString += "Fuel Gauge Hardware Fault, ";
    }

    if (returnString != "") {
      return returnString.substring(0, returnString.length() - 2);
    }
    return returnString;
  }
}
