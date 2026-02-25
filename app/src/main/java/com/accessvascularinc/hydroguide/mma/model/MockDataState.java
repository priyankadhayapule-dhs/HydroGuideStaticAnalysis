package com.accessvascularinc.hydroguide.mma.model;

import androidx.annotation.Nullable;

/**
 * This enum defines available mock states that showcase different ECG data scenarios that you can
 * cycle through on the procedure screen for testing the UI in absence of an ECG controller.
 */
public enum MockDataState {
  /**
   * This is the default state with mock data disabled so the live data is only plotted with data
   * from the controller.
   */
  None(0, MockFiles.ECG_NORMAL, MockFiles.PEAKS_NORMAL, MockFiles.ECG_NORMAL,
          MockFiles.PEAKS_NORMAL),
  /**
   * This is the first mock state that simulates normal ECG waveforms.
   */
  Normal(1, MockFiles.ECG_NORMAL, MockFiles.PEAKS_NORMAL, MockFiles.ECG_NORMAL,
          MockFiles.PEAKS_NORMAL),
  /**
   * In this state, P-wave peaks are slightly elevated.
   */
  SlightIncrease(2, MockFiles.ECG_SLIGHT_INCREASED, MockFiles.PEAKS_SLIGHT_INCREASED,
          MockFiles.ECG_SLIGHT_INCREASED, MockFiles.PEAKS_SLIGHT_INCREASED),
  /**
   * In this state, intravascular P-wave peaks are even more elevated.
   */
  GreatIncrease(3, MockFiles.ECG_SLIGHT_INCREASED, MockFiles.PEAKS_SLIGHT_INCREASED,
          MockFiles.ECG_GREATLY_INCREASED, MockFiles.PEAKS_GREATLY_INCREASED),
  /**
   * In this state, intravascular P-wave peaks are as elevated as possible.
   */
  Max(4, MockFiles.ECG_SLIGHT_INCREASED, MockFiles.PEAKS_SLIGHT_INCREASED, MockFiles.ECG_MAX,
          MockFiles.PEAKS_MAX),
  /**
   * In this state, intravascular P-wave peaks start to deflect, starting to appear upside down.
   */
  SlightDeflection(5, MockFiles.ECG_SLIGHT_INCREASED, MockFiles.PEAKS_SLIGHT_INCREASED,
          MockFiles.ECG_SLIGHT_DEFLECT, MockFiles.PEAKS_DEFLECT),
  /**
   * In this state, intravascular P-wave peaks have the highest degree of deflection.
   */
  Deflection(6, MockFiles.ECG_SLIGHT_INCREASED, MockFiles.PEAKS_SLIGHT_INCREASED,
          MockFiles.ECG_DEFLECT, MockFiles.PEAKS_DEFLECT);

  private final int numValue;
  private final String externalEcg;
  private final String externalPeaks;
  private final String intravascularEcg;
  private final String intravascularPeaks;

  MockDataState(final int numVal, final String extEcg, final String extPeaks,
                final String intravEcg, final String intravPeaks) {
    this.numValue = numVal;
    this.externalEcg = extEcg;
    this.externalPeaks = extPeaks;
    this.intravascularEcg = intravEcg;
    this.intravascularPeaks = intravPeaks;
  }

  /**
   * Gets the enum mock data state associated with a numeric value or effective index.
   *
   * @param numVal the number value or index of a mock data state.
   *
   * @return the mock data state corresponding to the provided index or null if no such stat exists.
   */
  @Nullable
  public static MockDataState valueOfInt(final int numVal) {
    for (final MockDataState e : values()) {
      if (e.getNumValue() == numVal) {
        return e;
      }
    }
    return null;
  }

  private int getNumValue() {
    return numValue;
  }

  /**
   * Gets the file used for external ECG data points of this mock data state.
   *
   * @return the file name that is the source of mock external ECG data.
   */
  public String getExternalEcg() {
    return externalEcg;
  }

  /**
   * Gets the file used for external P-Wave peak data points of this mock data state.
   *
   * @return the file name that is the source of mock external P-Wave peak data.
   */
  public String getExternalPeaks() {
    return externalPeaks;
  }

  /**
   * Gets the file used for intravascular ECG data points of this mock data state.
   *
   * @return the file name that is the source of mock intravascular ECG data.
   */
  public String getIntravascularEcg() {
    return intravascularEcg;
  }

  /**
   * Gets the file used for intravascular P-Wave peak data points of this mock data state.
   *
   * @return the file name that is the source of mock intravascular P-Wave peak data.
   */
  public String getIntravascularPeaks() {
    return intravascularPeaks;
  }

  /**
   * Gets the next mock data state in this enum, and will loop around at the last state.
   *
   * @return the next mock data state in this enum.
   */
  public MockDataState next() {
    final int nextNumVal = numValue + 1;
    return valueOfInt(nextNumVal > 6 ? 0 : nextNumVal);
  }
}
