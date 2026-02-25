package com.accessvascularinc.hydroguide.mma.serial;

import androidx.annotation.Nullable;

public enum PacketId {
  /* data */
  PatientHeartbeat((byte) 0x54), //changed as per change in firmware, was 0x04 before.
  ButtonsState((byte) 0x08),
  ControllerStatus((byte) 0x0C),
  StartUpStatus((byte) 0x10),

  /* commands */
  TabletKeepalive((byte) 0x45),
  StartEcg((byte) 0x15),
  DisconnectBluetooth ((byte) 0x25),

  /* waveforms */
  EcgWaveform((byte) 0x06),
  RawAdcSamples((byte) 0x22),

  BiozData((byte)0x48),

  /* alerts */

  /* SFU */
  SFUKeepAlive((byte) 0x85),
  SFUUpdateStart((byte) 0x89);

  private final byte byteValue;

  PacketId(final byte byteVal) {
    this.byteValue = byteVal;
  }

  @Nullable
  public static PacketId valueOfByte(final byte byteValue) {
    for (final PacketId e : values()) {
      if (e.getByteValue() == byteValue) {
        return e;
      }
    }
    return null;
  }

  public byte getByteValue() {
    return byteValue;
  }
}

