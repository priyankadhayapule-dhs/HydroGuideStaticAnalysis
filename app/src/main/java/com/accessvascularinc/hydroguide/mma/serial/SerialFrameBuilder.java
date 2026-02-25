package com.accessvascularinc.hydroguide.mma.serial;

public class SerialFrameBuilder {
  public final int BUFFER_LIMIT = 600;
  private final byte[] internalBuffer;
  private final SerialFrameCRC crc = new SerialFrameCRC();
  private int frameLength = 0;

  public SerialFrameBuilder() {
    internalBuffer = new byte[BUFFER_LIMIT];
    internalBuffer[0] = SerialFrameStuffing.STX;
    reset();
  }

  static Byte xor(final Byte a) {
    return (byte) (a ^ SerialFrameStuffing.STUFF_PREFIX);
  }

  public void reset() {
    frameLength = 1;
    crc.reset();
  }

  private void appendStuffed(final byte by) {
    /* insert byte and update CRC */
    internalBuffer[frameLength] = by;
    frameLength++;
    crc.put(by);
  }

  public void append(final byte by) {
    /* perform byte stuffing */
    if (by == SerialFrameStuffing.STX || by == SerialFrameStuffing.STUFF_PREFIX) {
      appendStuffed(SerialFrameStuffing.STUFF_PREFIX);
      appendStuffed(xor(by));
    } else {
      appendStuffed(by);
    }
  }

  public void appendMany(final byte[] aby) {
    for (final byte by : aby) {
      append(by);
    }
  }

  private void appendCRC() {
    final int finalCRC = crc.read();
    append((byte) finalCRC);
    append((byte) (finalCRC >> 8));
  }

  public byte[] end() {
    appendCRC();
    final byte[] retval = new byte[frameLength];
    System.arraycopy(internalBuffer, 0, retval, 0, frameLength);
    reset();
    return retval;
  }
}
