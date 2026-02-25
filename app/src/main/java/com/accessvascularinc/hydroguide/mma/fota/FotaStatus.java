package com.accessvascularinc.hydroguide.mma.fota;

public enum FotaStatus {
    OK(0x0000),
    ERR_TIMEOUT(0x000B);
    
    private final int value;
    
    FotaStatus(int value) {
        this.value = value;
    }
    
    public int getValue() {
        return value;
    }

    public boolean isSuccess() {
        return this == OK;
    }

    public boolean isError() {
        return this != OK;
    }

    public static FotaStatus fromValue(int value) {
        for (FotaStatus status : values()) {
            if (status.value == value) {
                return status;
            }
        }
        return null;
    }

    public static String getName(int value) {
        FotaStatus status = fromValue(value);
        if (status != null) {
            return status.name();
        }
        return String.format("UnknownStatus(0x%04X)", value);
    }
}
