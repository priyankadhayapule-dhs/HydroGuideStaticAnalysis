package com.accessvascularinc.hydroguide.mma.serial;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class SerialHelper {
  public static int unpackInt(final byte[] payload, final int offset) {
    return ByteBuffer.wrap(payload, offset, Integer.BYTES).order(ByteOrder.LITTLE_ENDIAN).getInt();
  }

  public static float unpackFloat(final byte[] payload, final int offset) {
    return ByteBuffer.wrap(payload, offset, Float.BYTES).order(ByteOrder.LITTLE_ENDIAN).getFloat();
  }

  public static short unpackShort(final byte[] payload, final int offset) {
    return ByteBuffer.wrap(payload, offset, Short.BYTES).order(ByteOrder.LITTLE_ENDIAN).getShort();
  }

  public static byte[] packInt(final int integer) {
    return ByteBuffer.allocate(Integer.BYTES).putInt(integer).array();
  }
}
