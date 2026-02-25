package com.accessvascularinc.hydroguide.mma.serial.flags;

public enum ControllerStatusFlags {
  None(0),
  WakeReset(1 << 31),
  WWatchdogReset(1 << 30),
  IndWatchdogReset(1 << 29),
  SoftReset(1 << 28),
  PorReset(1 << 27),
  PinReset(1 << 26),
  BrownoutReset(1 << 25),
  BootloaderIntegrityFailure(1 << 6),
  AppIntegrityFailure(1 << 5),
  ChargerError(1 << 4),
  FuelGaugeError(1 << 3),
  EcgError(1 << 2),
  ChargingActive(1 << 1),
  BatteryLow(1);

  private final int numValue;

  ControllerStatusFlags(final int numVal) {
    this.numValue = numVal;
  }

  public static BitFlags<ControllerStatusFlags> valueOfInt(final int numValue) {
    return new BitFlags<ControllerStatusFlags>(numValue);
  }

  /**
   * @param combined A bitflags instance where zero or more individual flags have been combined by
   *                 bitwise-OR
   *
   * @return true if "this" flag is set in the combined bitflags
   */
  public boolean isFlaggedIn(final BitFlags<ControllerStatusFlags> combined) {
    return (combined != null) && (combined.toInt() & numValue) != 0;
  }

  private int getNumValue() {
    return numValue;
  }
}
