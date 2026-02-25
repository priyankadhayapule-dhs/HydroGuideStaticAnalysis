package com.accessvascularinc.hydroguide.mma.fota;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class FotaStatusResponse
{
    private final int opcode;
    private final int lastOpcode;
    private final int statusCode;
    private final int dfuState;
    private final int deviceFlags;
    private final int nextExpectedSeq;
    private final int progress;
    private final int negotiatedMtu;
    private final int deviceFeatures;

    public static FotaStatusResponse parse(byte[] payload)
    {
        if (payload == null || payload.length < 16)
        {
            return null;
        }
        
        ByteBuffer buf = ByteBuffer.wrap(payload);
        buf.order(ByteOrder.LITTLE_ENDIAN);
        
        int op = buf.get() & 0xFF;
        int lastOp = buf.get() & 0xFF;
        int statusCode = buf.getShort() & 0xFFFF;
        int dfuState = buf.get() & 0xFF;
        int flags = buf.get() & 0xFF;
        int nextSeq = buf.getShort() & 0xFFFF;
        int progress = buf.getInt();
        int reserved = buf.getInt();
        
        int negotiatedMtu = reserved & 0xFFFF;
        int deviceFeatures = (reserved >> 16) & 0xFFFF;
        
        return new FotaStatusResponse(op, lastOp, statusCode, dfuState, flags, nextSeq, progress, negotiatedMtu, deviceFeatures);
    }
    
    private FotaStatusResponse(int opcode, int lastOpcode, int statusCode, int dfuState, int deviceFlags, int nextExpectedSeq, int progress, int negotiatedMtu, int deviceFeatures)
    {
        this.opcode = opcode;
        this.lastOpcode = lastOpcode;
        this.statusCode = statusCode;
        this.dfuState = dfuState;
        this.deviceFlags = deviceFlags;
        this.nextExpectedSeq = nextExpectedSeq;
        this.progress = progress;
        this.negotiatedMtu = negotiatedMtu;
        this.deviceFeatures = deviceFeatures;
    }
    
    public int getOpcode() {
        return opcode;
    }
    
    public String getOpcodeName() {
        return FotaOpcode.getName(opcode);
    }
    
    public int getLastOpcode() {
        return lastOpcode;
    }
    
    public String getLastOpcodeName() {
        return FotaOpcode.getName(lastOpcode);
    }
    
    public int getStatusCode() {
        return statusCode;
    }
    
    public String getStatusName() {
        return FotaStatus.getName(statusCode);
    }
    
    public FotaStatus getStatus() {
        return FotaStatus.fromValue(statusCode);
    }
    
    public boolean isSuccess() {
        return statusCode == FotaStatus.OK.getValue();
    }
    
    public int getDfuState() {
        return dfuState;
    }
    
    public String getStateName() {
        return FotaState.getName(dfuState);
    }
    
    public FotaState getState() {
        return FotaState.fromValue(dfuState);
    }
    
    public int getDeviceFlags() {
        return deviceFlags;
    }
    
    public boolean isBusy() {
        return (deviceFlags & FotaConstants.DFU_FLAG_BUSY) != 0;
    }
    
    public boolean isRetryRequired() {
        return (deviceFlags & FotaConstants.DFU_FLAG_RETRY_REQUIRED) != 0;
    }
    
    public boolean isRebootRequired() {
        return (deviceFlags & FotaConstants.DFU_FLAG_REBOOT_REQUIRED) != 0;
    }
    
    public boolean isStagingValid() {
        return (deviceFlags & FotaConstants.DFU_FLAG_STAGING_VALID) != 0;
    }
    
    public int getNextExpectedSeq() {
        return nextExpectedSeq;
    }
    
    public int getProgress() {
        return progress;
    }
    
    public int getNegotiatedMtu() {
        return negotiatedMtu;
    }
    
    public boolean hasNegotiatedMtu() {
        return negotiatedMtu > 0;
    }
    
    public int getDeviceFeatures() {
        return deviceFeatures;
    }
    
    @Override
    public String toString()
    {
        return String.format("Fota Commands Response : {op=%s, lastOp=%s, status=%s, state=%s, flags=0x%02X, nextSeq=%d, progress=%d, mtu=%d}", getOpcodeName(), getLastOpcodeName(), getStatusName(), getStateName(), deviceFlags, nextExpectedSeq, progress, negotiatedMtu);
    }
}
