package com.accessvascularinc.hydroguide.mma.fota;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public final class FotaPacketFactory {
    public static final String LABEL_START = "START";
    public static final String LABEL_PREPARE = "PREPARE";
    public static final String LABEL_VERIFY = "VERIFY";
    public static final String LABEL_INSTALL = "INSTALL";
    public static final String LABEL_END = "END";
    private FotaPacketFactory() {
        // Prevent instantiation
    }
    public static byte[] start(int imageId, int maxMtu) {
        ByteBuffer buf = ByteBuffer.allocate(12);
        buf.order(ByteOrder.LITTLE_ENDIAN);
        buf.put((byte) FotaOpcode.START.getValue());
        buf.put((byte) imageId);
        buf.putShort((short) FotaConstants.DFU_PROTOCOL_VERSION);
        buf.putInt(FotaConstants.DEFAULT_HOST_FEATURES);
        buf.putShort((short) maxMtu);
        buf.putShort((short) 0);
        return buf.array();
    }
    public static byte[] prepare(int imageId, int firmwareSize, int firmwareCrc32) {
        ByteBuffer buf = ByteBuffer.allocate(20);
        buf.order(ByteOrder.LITTLE_ENDIAN);
        buf.put((byte) FotaOpcode.PREPARE.getValue());
        buf.put((byte) imageId);
        buf.putShort((short) FotaConstants.DFU_PREP_REQUIRE_CRC32);
        buf.putInt(firmwareSize);
        buf.putInt(firmwareCrc32);
        buf.putInt(0);
        buf.putInt(0);
        return buf.array();
    }
    public static byte[] data(int seq, byte[] chunk) {
        ByteBuffer buf = ByteBuffer.allocate(4 + chunk.length);
        buf.order(ByteOrder.LITTLE_ENDIAN);
        buf.put((byte) FotaOpcode.DATA.getValue());
        buf.putShort((short) seq);
        buf.put((byte) chunk.length);
        buf.put(chunk);
        return buf.array();
    }

    public static byte[] verify(int imageId) {
        ByteBuffer buf = ByteBuffer.allocate(8);
        buf.order(ByteOrder.LITTLE_ENDIAN);
        buf.put((byte) FotaOpcode.VERIFY.getValue());
        buf.put((byte) imageId);
        buf.putShort((short) 1);
        buf.putInt(0);
        return buf.array();
    }

    public static byte[] install(int imageId) {
        ByteBuffer buf = ByteBuffer.allocate(8);
        buf.order(ByteOrder.LITTLE_ENDIAN);
        buf.put((byte) FotaOpcode.INSTALL.getValue());
        buf.put((byte) imageId);
        buf.putShort((short) FotaConstants.DFU_INSTALL_MARK_PENDING);
        buf.putInt(0);
        return buf.array();
    }

    public static byte[] end() {
        ByteBuffer buf = ByteBuffer.allocate(8);
        buf.order(ByteOrder.LITTLE_ENDIAN);
        buf.put((byte) FotaOpcode.END.getValue());
        buf.put((byte) 0);
        buf.putShort((short) 0);
        buf.putInt(0);
        return buf.array();
    }

    public static byte[] query() {
        ByteBuffer buf = ByteBuffer.allocate(5);
        buf.order(ByteOrder.LITTLE_ENDIAN);
        buf.put((byte) FotaOpcode.QUERY.getValue());
        buf.putShort((short) 0);
        buf.putShort((short) 0);
        return buf.array();
    }

    public static byte[] abort() {
        ByteBuffer buf = ByteBuffer.allocate(5);
        buf.order(ByteOrder.LITTLE_ENDIAN);
        buf.put((byte) FotaOpcode.ABORT.getValue());
        buf.putShort((short) 0);
        buf.putShort((short) 0);
        return buf.array();
    }

    public static byte[] reboot() {
        ByteBuffer buf = ByteBuffer.allocate(5);
        buf.order(ByteOrder.LITTLE_ENDIAN);
        buf.put((byte) FotaOpcode.REBOOT.getValue());
        buf.putShort((short) 0);
        buf.putShort((short) 0);
        return buf.array();
    }
}
