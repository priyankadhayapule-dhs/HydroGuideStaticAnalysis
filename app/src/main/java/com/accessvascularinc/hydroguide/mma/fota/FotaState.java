package com.accessvascularinc.hydroguide.mma.fota;

public enum FotaState {
    IDLE(0x00),
    PREPARING(0x01),
    DOWNLOADING(0x02),
    VERIFYING(0x03),
    INSTALLING(0x04),
    COMPLETE(0x05),
    ERROR(0x06);
    
    private final int value;
    
    FotaState(int value) {
        this.value = value;
    }
    
    public int getValue() {
        return value;
    }

    public boolean isActive() {
        return this == PREPARING || this == DOWNLOADING || this == VERIFYING || this == INSTALLING;
    }

    public boolean isComplete() {
        return this == COMPLETE;
    }

    public boolean isError() {
        return this == ERROR;
    }

    public static FotaState fromValue(int value) {
        for (FotaState state : values()) {
            if (state.value == value) {
                return state;
            }
        }
        return null;
    }

    public static String getName(int value) {
        FotaState state = fromValue(value);
        if (state != null) {
            return state.name();
        }
        return String.format("UnknownState(0x%02X)", value);
    }
}
