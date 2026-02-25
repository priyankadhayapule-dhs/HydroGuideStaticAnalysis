package com.accessvascularinc.hydroguide.mma.serial.flags;

/**
 * This enum is a flag communicated over every controller status that defines the more recent status
 * of the ECG analysis algorithm.
 */
public enum EcgAnalysisStatusFlags {
  /**
   * This is the default flag, where the ECG analysis is performing as expected.
   */
  None(0),
  /**
   * This flag indicates no PWave was detected on either (intravascular and external) ECG waveform
   */
  PWavesNotDetected(1),
  Bad(1 << 1),
  NoInput(1 << 2),
  Initializing(1 << 3),
  TooMuchUptime(1 << 4);

  private final int numValue;

  EcgAnalysisStatusFlags(final int numVal) {
    this.numValue = numVal;
  }

  /**
   * Gets the enum ECG analysis status flag associated with a numeric value or effective index.
   *
   * @param numValue the number value or index of n ECG analysis status flag
   *
   * @return the ECG analysis status flag corresponding to the provided index
   */
  public static BitFlags<EcgAnalysisStatusFlags> valueOfInt(final int numValue) {
    return new BitFlags<EcgAnalysisStatusFlags>(numValue);
  }

  /**
   * @param combined A bitflags instance where zero or more individual flags have been combined by
   *                 bitwise-OR
   *
   * @return true if "this" flag is set in the combined bitflags
   */
  public boolean isFlaggedIn(final BitFlags<EcgAnalysisStatusFlags> combined) {
    return (combined != null) && (combined.toInt() & numValue) != 0;
  }

  private int getNumValue() {
    return numValue;
  }
}
