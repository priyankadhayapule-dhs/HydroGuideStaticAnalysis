package com.accessvascularinc.hydroguide.mma.fota;

public enum FotaOpcode {
    START(0x00),
    PREPARE(0x01),
    DATA(0x02),
    VERIFY(0x03),
    INSTALL(0x04),
    END(0x05),
    STATUS(0x06),
    QUERY(0x07),
    ABORT(0x08),
    REBOOT(0x09);
    
    private final int value;
    
    FotaOpcode(int value) {
        this.value = value;
    }
    
    public int getValue() {
        return value;
    }

    public static FotaOpcode fromValue(int value) {
        for (FotaOpcode opcode : values()) {
            if (opcode.value == value) {
                return opcode;
            }
        }
        return null;
    }

    public static String getName(int value) {
        FotaOpcode opcode = fromValue(value);
        if (opcode != null) {
            return opcode.name();
        }
        return String.format("UnknownOpcode(0x%02X)", value);
    }
}
