package com.accessvascularinc.hydroguide.mma.serial;

import androidx.annotation.Nullable;

public enum HeartbeatEventStates {
  Good((byte) 0),
  Bad((byte) 1),
  InputError((byte) 2),
  Initializing((byte) 3),
  TooMuchUptime((byte) 4),
  HeartRateTooLow((byte) 5),
  HeartRateTooHigh((byte) 6),
  HeartRateTooVariable((byte) 7),
  EcgNoisy((byte) 9),
  InvalidBaseline((byte) 10),
  InitializingFirstBeat((byte) 20),
  BadQrsDetection((byte) 21),
  InvalidRR((byte) 22),
  BadBaseline((byte) 23),
  BadPDetection((byte) 24),
  BaselineOutOfRange((byte) 30),
  ExternalBaselineOutOfRange((byte) 31),
  InternalBaselineOutOfRange((byte) 32),
  InternalInputError((byte) 30),
  InternalExternalDesync((byte) 41),
  NoExternalPWaveRegionOfInterest((byte) 50),
  ShortExternalPWaveRegionOfInterest((byte) 51),
  ExternalPRIntervalOutOfBounds((byte) 52),
  ExternalQSOverlap((byte) 53),
  PWaveTransferFailure((byte) 54);

  private final byte byteValue;

  HeartbeatEventStates(final byte byteVal) {
    this.byteValue = byteVal;
  }

  @Nullable
  public static HeartbeatEventStates valueOfByte(final byte byteValue) {
    for (final HeartbeatEventStates e : values()) {
      if (e.getByteValue() == byteValue) {
        return e;
      }
    }
    return null;
  }

  private byte getByteValue() {
    return byteValue;
  }
}
