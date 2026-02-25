package com.accessvascularinc.hydroguide.mma.serial;

public class ImpedanceSample {
  public long timestamp_ms;
  public long impedanceRAWhite_milliohms;
  public long impedanceLABlack_milliohms;
  public long impedanceLLRed_milliohms;

  public ImpedanceSample(final long timestamp, final long ra, final long la, final long ll) {
    timestamp_ms = timestamp;
    //TODO: Verify the order and offset for unpacking
    impedanceRAWhite_milliohms = ra;
    impedanceLABlack_milliohms = la;
    impedanceLLRed_milliohms = ll;
  }
}
