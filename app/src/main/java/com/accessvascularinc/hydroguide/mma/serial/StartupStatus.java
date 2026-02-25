package com.accessvascularinc.hydroguide.mma.serial;

import android.util.Log;

import androidx.annotation.Nullable;

import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

public class StartupStatus {
  public final int HASH_SIZE = 32;
  public final int BYTE_LENGTH = 6 + HASH_SIZE + HASH_SIZE;
  public final int SFU_HASH_OFFSET = 6;
  public final int APP_HASH_OFFSET = 6 + HASH_SIZE;

  public SwInfoStatus sfuStatus = null;
  public SwInfoStatus appStatus = null;

  // Controller Bootloader Info
  public byte sfuMajorVersion;
  public byte sfuMinorVersion;

  // Controller Application Info
  public byte appMajorVersion;
  public byte appMinorVersion;

  // SHA256 hashes of the bootloader and app used for load verification of the respective software.
  public byte[] sfuHash = new byte[HASH_SIZE]; // Can convert to SHA256 string.
  public byte[] appHash = new byte[HASH_SIZE];

  public StartupStatus(final byte[] payload) {
    if (payload.length == BYTE_LENGTH) {
      sfuStatus = SwInfoStatus.valueOfByte(payload[0]);
      appStatus = SwInfoStatus.valueOfByte(payload[1]);
      sfuMajorVersion = payload[2];
      sfuMinorVersion = payload[3];
      appMajorVersion = payload[4];
      appMinorVersion = payload[5];

      System.arraycopy(payload, SFU_HASH_OFFSET, sfuHash, 0, HASH_SIZE);
      System.arraycopy(payload, APP_HASH_OFFSET, appHash, 0, HASH_SIZE);

    } else {
      MedDevLog.error("StartupStatus Unpacking", String.format(
              "Length mismatch for StartupStatus, got %s bytes, expected 70", payload.length));
    }
  }

  public String getControllerAppVersion() {
    return String.format("v%s.%s", Byte.toUnsignedInt(appMajorVersion),
            Byte.toUnsignedInt(appMinorVersion));
  }

  public enum SwInfoStatus {
    Valid((byte) 0),
    Invalid((byte) 1),
    Invalid_LoadError((byte) 5);

    private final byte byteValue;

    SwInfoStatus(final byte byteVal) {
      this.byteValue = byteVal;
    }

    @Nullable
    public static SwInfoStatus valueOfByte(final byte byteValue) {
      for (final SwInfoStatus e : values()) {
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
}
