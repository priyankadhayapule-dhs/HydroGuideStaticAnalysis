package com.accessvascularinc.hydroguide.mma.model;

import androidx.annotation.Nullable;

/**
 * This enum defines different sections of a typical heartbeat that can be used to label ECG data.
 */
public enum PeakLabel {
  /**
   * This is a default label for an insignificant or unclassified portion of ECG data
   */
  None(0),
  /**
   * The P peak label.
   */
  P(1),
  /**
   * The Q peak label.
   */
  Q(2),
  /**
   * The R peak label.
   */
  R(3),
  /**
   * The S peak label.
   */
  S(4),
  /**
   * The T peak label.
   */
  T(5);

  private final int numValue;

  PeakLabel(final int numVal) {
    this.numValue = numVal;
  }

  /**
   * Gets the enum peak label of a given effective index.
   *
   * @param numValue the number value or index associated with a peak label.
   *
   * @return the peak label corresponding to the provided index or null if no such label exists.
   */
  @Nullable
  public static PeakLabel valueOfInt(final int numValue) {
    for (final PeakLabel e : values()) {
      if (e.getNumValue() == numValue) {
        return e;
      }
    }
    return null;
  }

  /**
   * Gets the numeric value associated with this peak label.
   *
   * @return the effective index.
   */
  private int getNumValue() {
    return numValue;
  }
}
