package com.accessvascularinc.hydroguide.mma.serial;

import android.util.Log;

import com.accessvascularinc.hydroguide.mma.serial.flags.BitFlags;
import com.accessvascularinc.hydroguide.mma.serial.flags.SamplingStatusFlags;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

public class EcgSample {
  public static final int BYTE_LENGTH = 88;
  public static final int NUMBER_OF_ADC_CHANNELS = 2;
  public short timestamp_ms = (short) 0;
  public BitFlags<SamplingStatusFlags> samplingStatus = null;
  public int startTime_ms = 0;
  public int numberOfSamples = 0;
  public float[][] samples = null;

  public EcgSample(final long timestamp, final byte[] payload) {
    if (payload.length == BYTE_LENGTH) {
      final var status = SamplingStatusFlags.valueOfInt(SerialHelper.unpackInt(payload, 0));
      final int sampleIndex = SerialHelper.unpackInt(payload, 4);
      final int totalSamples = (payload.length - 8) / Float.BYTES;
      final float[] allSamples = new float[totalSamples];

      for (int i = 0; i < totalSamples; i++) {
        final int payloadPos = (i * Float.BYTES) + 8;
        allSamples[i] = SerialHelper.unpackFloat(payload, payloadPos);
      }

      timestamp_ms = (short) timestamp;
      samplingStatus = status;
      startTime_ms = sampleIndex;
      numberOfSamples = allSamples.length / 2;
      samples = new float[NUMBER_OF_ADC_CHANNELS][];

      buildFrom(allSamples);
    } else {
      MedDevLog.error("EcgSample Unpacking", String.format(
              "Length mismatch for EcgWaveform, got %s bytes, expected 88", payload.length));
    }
  }

  void buildFrom(final float[] allSamples) {
    int i = 0;
    for (int k = 0; k < samples.length; k++) {
      samples[k] = new float[numberOfSamples];
    }
    for (int j = 0; j < numberOfSamples; j++) {
      for (int k = 0; k < samples.length; k++) {
        samples[k][j] = allSamples[i];
        i++;
      }
    }
  }
}
