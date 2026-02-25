package com.accessvascularinc.hydroguide.mma.serial;

import android.util.Log;

import com.accessvascularinc.hydroguide.mma.serial.flags.HeartbeatStatusFlags;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

public class Heartbeat {
  public final int BYTE_LENGTH = 40;

  public int pWaveStartTime_ms = 0;
  public int pWavePeakTime_ms = 0;
  public int pWaveEndTime_ms = 0;
  public float pWaveInternalAmplitude_V = 0.0F; // amplitude of IC P wave
  public float pWaveExternalAmplitude_V = 0.0F; // amplitude of surface P wave
  public float internalBaseline_V = 0.0F; // baseline of IC channel
  public float externalBaseline_V = 0.0F; // baseline of surface channel
  public int rWaveTime_ms = 0; // sample index of QRS complex R wave
  public int rWavePreviousTime_ms = 0; // sample index of R wave from previous QRS complex
  public HeartbeatStatusFlags heartbeatStatus = null;
  public HeartbeatEventStates eventStatus = null;

  public Heartbeat(final byte[] payload) {
    if (payload.length == BYTE_LENGTH) {
      pWaveStartTime_ms = SerialHelper.unpackInt(payload, 0);
      pWavePeakTime_ms = SerialHelper.unpackInt(payload, 4);
      pWaveEndTime_ms = SerialHelper.unpackInt(payload, 8);
      pWaveInternalAmplitude_V = SerialHelper.unpackFloat(payload, 12);
      pWaveExternalAmplitude_V = SerialHelper.unpackFloat(payload, 16);
      internalBaseline_V = SerialHelper.unpackFloat(payload, 20);
      externalBaseline_V = SerialHelper.unpackFloat(payload, 24);
      rWaveTime_ms = SerialHelper.unpackInt(payload, 28);
      rWavePreviousTime_ms = SerialHelper.unpackInt(payload, 32);
      heartbeatStatus = HeartbeatStatusFlags.valueOfInt(SerialHelper.unpackShort(payload, 36));
      eventStatus = HeartbeatEventStates.valueOfByte(payload[39]);
    } else {
      MedDevLog.error("Heartbeat Unpacking", String.format(
              "Length mismatch for PatientHeartbeat, got %s bytes, expected 24", payload.length));
    }
  }
}