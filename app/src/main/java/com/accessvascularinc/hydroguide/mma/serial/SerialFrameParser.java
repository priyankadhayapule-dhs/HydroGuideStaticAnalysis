package com.accessvascularinc.hydroguide.mma.serial;

import androidx.annotation.Nullable;

public class SerialFrameParser {
  private final SerialFrameCRC crc = new SerialFrameCRC();
  public Status Expected = Status.STX;
  public int CalculatedCRC = 0;
  public boolean stufffail = false;
  public boolean fail = false;
  public Status result = null;
  private boolean destuff = false;
  private int payloadRemaining = 0;

  static Byte xor(final Byte a) {
    return (byte) (a ^ SerialFrameStuffing.STUFF_PREFIX);
  }

  public void reset() {
    Expected = Status.STX;
    destuff = false;
    crc.reset();
  }

  @Nullable
  public Byte process(final byte byt) {
    byte by = byt;
    if (by == SerialFrameStuffing.STX) {
      result = Status.STX;
      fail = (Expected != Status.STX);
      Expected = Status.PktID;
      crc.reset();
      return null;
    }
    crc.put(by);
    if (destuff) {
      destuff = false;
      by = xor(by);
      stufffail = (by != SerialFrameStuffing.STUFF_PREFIX) && (by != SerialFrameStuffing.STX);
    } else if (by == SerialFrameStuffing.STUFF_PREFIX) {
      destuff = true;
      result = Status.Continue;
      fail = (Expected == Status.STX);
      return null;
    } else {
      fail = false;
    }

    result = processDestuffed(by);
    return by;
  }

  private Status processDestuffed(final byte by) {
    result = Expected;
    switch (Expected) {
      case PktID:
        Expected = Status.Timestamp_LSB;
        break;

      case Timestamp_LSB:
        Expected = Status.Timestamp_MSB;
        break;

      case Timestamp_MSB:
        Expected = Status.PayloadLength;
        break;

      case PayloadLength:
        payloadRemaining = by;
        Expected = Status.Payload;
        break;

      case Payload:
        --payloadRemaining;
        if (payloadRemaining > 0) {
          return Status.Continue;
        }
        Expected = Status.CRC_LSB;
        CalculatedCRC = crc.read();
        break;

      case CRC_LSB:
        fail = by != (byte) CalculatedCRC;
        Expected = Status.CRC_MSB;
        if (fail) {
          return Status.MismatchedCRC;
        }
        break;

      case CRC_MSB:
        fail = by != (byte) (CalculatedCRC >> 8);
        Expected = Status.STX;
        if (fail) {
          return Status.MismatchedCRC;
        }
        break;

      default:
        fail = true;
        return Status.Continue;
    }
    return result;
  }

  public enum Status {
    Continue,
    STX,
    PktID,
    Timestamp_LSB,
    Timestamp_MSB,
    PayloadLength,
    Payload,
    CRC_LSB,
    CRC_MSB,
    ImproperStuffTrailByte,
    MismatchedCRC
  }

  /// <summary>
  /// Packet sub identification for determining type. Used with mask of 0x3 for the 2 lowest bytes
  /// (1-0) of PacketID
  /// </summary>
  public enum PacketSubId {
    Data((byte) 0x0),
    Command((byte) 0x1),
    Waveform((byte) 0x2),
    Alarm((byte) 0x3);

    private final byte byteValue;

    PacketSubId(final byte byteVal) {
      this.byteValue = byteVal;
    }

    @Nullable
    public static PacketSubId valueOfByte(final byte byteVal) {
      for (final PacketSubId e : values()) {
        if (e.getByteValue() == byteVal) {
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
