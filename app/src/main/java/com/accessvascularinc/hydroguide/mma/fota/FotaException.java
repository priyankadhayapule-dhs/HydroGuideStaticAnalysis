package com.accessvascularinc.hydroguide.mma.fota;


public class FotaException extends Exception {
    
    private final FotaStatus status;
    private final String phase;

    public FotaException(String message) {
        super(message);
        this.status = null;
        this.phase = null;
    }

    public FotaException(String message, Throwable cause) {
        super(message, cause);
        this.status = null;
        this.phase = null;
    }

    public FotaException(String message, FotaStatus status) {
        super(message);
        this.status = status;
        this.phase = null;
    }

    public FotaException(String phase, String message) {
        super(message);
        this.status = null;
        this.phase = phase;
    }

    public FotaException(String phase, String message, FotaStatus status) {
        super(message);
        this.status = status;
        this.phase = phase;
    }

    public FotaException(String phase, String message, Throwable cause) {
        super(message, cause);
        this.status = null;
        this.phase = phase;
    }
    public FotaStatus getStatus() {
        return status;
    }

    public String getPhase() {
        return phase;
    }

    public boolean hasStatus() {
        return status != null;
    }

    public boolean hasPhase() {
        return phase != null;
    }

    public static FotaException timeout(String phase) {
        return new FotaException(phase, "Response timeout", FotaStatus.ERR_TIMEOUT);
    }
    public static FotaException deviceError(String phase, FotaStatus status) {
        return new FotaException(phase, "Device error: " + status.name(), status);
    }

    public static FotaException unexpectedOpcode(int expected, int actual) {
        return new FotaException(String.format("Unexpected opcode: got 0x%02X, expected 0x%02X",
            actual, expected));
    }
    
    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder("DFUException");
        if (phase != null) {
            sb.append("[").append(phase).append("]");
        }
        sb.append(": ").append(getMessage());
        if (status != null) {
            sb.append(" (status=").append(status.name()).append(")");
        }
        return sb.toString();
    }
}
