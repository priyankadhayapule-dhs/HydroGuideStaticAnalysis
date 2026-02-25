package com.accessvascularinc.hydroguide.mma.serial;

public class Packet {
  public byte packetType = (byte) 0;
  public long timestamp = 0;
  public byte[] payload = null;

  public Packet() {
  }

  public Packet(final byte id, final long packetTimestamp, final byte[] packetPayload) {
    this.packetType = id;
    this.timestamp = packetTimestamp;
    this.payload = packetPayload.clone();
  }
}