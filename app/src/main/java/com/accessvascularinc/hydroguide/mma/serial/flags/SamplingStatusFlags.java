package com.accessvascularinc.hydroguide.mma.serial.flags;

public enum SamplingStatusFlags {
  None(0),
  InternalSaturatedLow(1 << 11),
  InternalSaturatedHigh(1 << 10),
  LimbSaturatedLow(1 << 9),
  LimbSaturatedHigh(1 << 8),
  InternalLeadOff(1 << 7),
  LeftLegLeadOff(1 << 6),
  LeftArmLeadOff(1 << 5),
  RightArmLeadOff(1 << 4),
  SupplyOutOfRange(1 << 1),
  AdcFaulted(1);

  private final int numValue;

  SamplingStatusFlags(final int numVal) {
    this.numValue = numVal;
  }

  public static BitFlags<SamplingStatusFlags> valueOfInt(final int numValue) {
    return new BitFlags<SamplingStatusFlags>(numValue);
  }

  /**
   * @param combined A bitflags instance where zero or more individual flags have been combined by
   *                 bitwise-OR
   *
   * @return true if "this" flag is set in the combined bitflags
   */
  public boolean isFlaggedIn(final BitFlags<SamplingStatusFlags> combined) {
    return (combined != null) && (combined.toInt() & numValue) != 0;
  }

  private int getNumValue() {
    return numValue;
  }

  public String getCompositeString(final BitFlags<SamplingStatusFlags> combined) {
    String returnString = "";
    if (combined == null) {
      return returnString;
    }

    if ((combined.toInt() & SamplingStatusFlags.InternalSaturatedLow.getNumValue()) != 0) {
      returnString += "Internal Saturated Low, ";
    }
    if ((combined.toInt() & SamplingStatusFlags.InternalSaturatedHigh.getNumValue()) != 0) {
      returnString += "Internal Saturated High, ";
    }
    if ((combined.toInt() & SamplingStatusFlags.LimbSaturatedLow.getNumValue()) != 0) {
      returnString += "Limb Saturated Low, ";
    }
    if ((combined.toInt() & SamplingStatusFlags.LimbSaturatedHigh.getNumValue()) != 0) {
      returnString += "Limb Saturated High, ";
    }
    if ((combined.toInt() & SamplingStatusFlags.InternalLeadOff.getNumValue()) != 0) {
      returnString += "Internal Lead Off, ";
    }
    if ((combined.toInt() & SamplingStatusFlags.LeftLegLeadOff.getNumValue()) != 0) {
      returnString += "Left Leg Lead Off, ";
    }
    if ((combined.toInt() & SamplingStatusFlags.LeftArmLeadOff.getNumValue()) != 0) {
      returnString += "Left Arm Lead Off, ";
    }
    if ((combined.toInt() & SamplingStatusFlags.RightArmLeadOff.getNumValue()) != 0) {
      returnString += "Right Arm Lead Off, ";
    }
    if ((combined.toInt() & SamplingStatusFlags.AdcFaulted.getNumValue()) != 0) {
      returnString += "ADC Faulted, ";
    }

    if (returnString != "") {
      return returnString.substring(0, returnString.length() - 2);
    }
    return returnString;
  }
}
