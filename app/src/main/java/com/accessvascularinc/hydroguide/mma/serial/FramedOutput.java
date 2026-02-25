package com.accessvascularinc.hydroguide.mma.serial;

public class FramedOutput {
  final private SerialFrameBuilder sfb = new SerialFrameBuilder();

  public byte[] addFraming(final byte pkt_type, final long timestamp, final byte[] payload) {
    sfb.reset();
    sfb.append(pkt_type);
    sfb.append((byte) timestamp);
    sfb.append((byte) (timestamp >> 8));
    final byte len = (byte) payload.length;
    if (len != payload.length) {
      throw new IllegalArgumentException("Payload Maximum length exceeded");
    }
    sfb.append(len);
    sfb.appendMany(payload);
    return sfb.end();
  }
}
