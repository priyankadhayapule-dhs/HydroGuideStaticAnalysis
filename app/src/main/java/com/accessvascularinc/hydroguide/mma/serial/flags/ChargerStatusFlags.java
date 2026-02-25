package com.accessvascularinc.hydroguide.mma.serial.flags;

/**
 * This enum is a flag communicated over every controller status that defines different charger
 * status errors that the controller could be experiencing, to be displayed on relevant screens of
 * the UI.
 */
public enum ChargerStatusFlags {
  /**
   * This is the default flag, where there is no problem with the charger.
   */
  None(0),
  /**
   * Low voltage timeout charger status flag.
   */
  LowVoltageTimeout(1 << 9),
  /**
   * Overcurrent shutdown charger status flag.
   */
  OvercurrentShutdown(1 << 8),
  /**
   * Input overvoltage charger status flag.
   */
  InputOvervoltage(1 << 7),
  /**
   * Input undervoltage charger status flag.
   */
  InputUndervoltage(1 << 6),
  /**
   * Input voltage invalid charger status flag.
   */
  InputVoltageInvalid(1 << 5),
  /**
   * Battery voltage low charger status flag.
   */
  BatteryVoltageLow(1 << 4),
  /**
   * Battery temperature too high charger status flag.
   */
  BatteryTemperatureTooHigh(1 << 3),
  /**
   * Battery temperature high warning charger status flag.
   */
  BatteryTemperatureHighWarning(1 << 2),
  /**
   * Battery temperature too low charger status flag.
   */
  BatteryTemperatureTooLow(1 << 1),
  /**
   * Charger hardware fault charger status flag.
   */
  ChargerHardwareFault(1);

  private final int numValue;

  ChargerStatusFlags(final int numVal) {
    this.numValue = numVal;
  }

  public static BitFlags<ChargerStatusFlags> valueOfInt(final int numValue) {
    return new BitFlags<ChargerStatusFlags>(numValue);
  }

  /**
   * @param combined A bitflags instance where zero or more individual flags have been combined by
   *                 bitwise-OR
   *
   * @return true if "this" flag is set in the combined bitflags
   */
  public boolean isFlaggedIn(final BitFlags<ChargerStatusFlags> combined) {
    return (combined != null) && (combined.toInt() & numValue) != 0;
  }

  private int getNumValue() {
    return numValue;
  }

  public String getCompositeString(final BitFlags<ChargerStatusFlags> combined) {
    String returnString = "";
    if (combined == null) {
      return returnString;
    }

    if ((combined.toInt() & ChargerStatusFlags.LowVoltageTimeout.getNumValue()) != 0) {
      returnString += "Low Voltage Timeout, ";
    }
    if ((combined.toInt() & ChargerStatusFlags.OvercurrentShutdown.getNumValue()) != 0) {
      returnString += "Overcurrent Shutdown, ";
    }
    if ((combined.toInt() & ChargerStatusFlags.InputOvervoltage.getNumValue()) != 0) {
      returnString += "Input Overvoltage, ";
    }
    if ((combined.toInt() & ChargerStatusFlags.InputUndervoltage.getNumValue()) != 0) {
      returnString += "Input Undervoltage, ";
    }
    if ((combined.toInt() & ChargerStatusFlags.InputVoltageInvalid.getNumValue()) != 0) {
      returnString += "Input Voltage Invalid, ";
    }
    if ((combined.toInt() & ChargerStatusFlags.BatteryVoltageLow.getNumValue()) != 0) {
      returnString += "Battery Voltage Low, ";
    }
    if ((combined.toInt() & ChargerStatusFlags.BatteryTemperatureTooHigh.getNumValue()) != 0) {
      returnString += "Battery Temperature Too High, ";
    }
    if ((combined.toInt() & ChargerStatusFlags.BatteryTemperatureHighWarning.getNumValue()) != 0) {
      returnString += "Battery Temperature High Warning, ";
    }
    if ((combined.toInt() & ChargerStatusFlags.BatteryTemperatureTooLow.getNumValue()) != 0) {
      returnString += "Battery Temperature Too Low, ";
    }
    if ((combined.toInt() & ChargerStatusFlags.ChargerHardwareFault.getNumValue()) != 0) {
      returnString += "Charger Hardware Fault, ";
    }

    if (returnString != "") {
      return returnString.substring(0, returnString.length() - 2);
    }
    return returnString;
  }
}
