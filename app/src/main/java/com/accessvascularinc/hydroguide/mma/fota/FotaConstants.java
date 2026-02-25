package com.accessvascularinc.hydroguide.mma.fota;

public final class FotaConstants {
    
    private FotaConstants() {
    }
    

    public static final int DFU_PROTOCOL_VERSION = 0x0300;  // v3.0
    

    public static final int DFU_FEAT_STATUS_PROGRESS = (1 << 0);
    public static final int DFU_FEAT_RESUME          = (1 << 1);
    public static final int DFU_FEAT_VERIFY          = (1 << 2);
    public static final int DFU_FEAT_INSTALL         = (1 << 3);
    public static final int DFU_FEAT_CRC32_IMAGE     = (1 << 4);
    

    public static final int DFU_FLAG_BUSY            = (1 << 0);
    public static final int DFU_FLAG_RETRY_REQUIRED  = (1 << 1);
    public static final int DFU_FLAG_REBOOT_REQUIRED = (1 << 2);
    public static final int DFU_FLAG_STAGING_VALID   = (1 << 3);
    

    public static final int DFU_PREP_REQUIRE_CRC32   = (1 << 2);
    

    public static final int DFU_INSTALL_MARK_PENDING = (1 << 0);
    

    public static final int IMAGE_ID_APP = 0x00;
    public static final int IMAGE_ID_NET = 0x02;
    public static final int IMAGE_ID_EXT = 0x03;
    

    public static final long RESPONSE_TIMEOUT_MS = 2000;       // 2 seconds
    public static final long KEEPALIVE_INTERVAL_MS = 1200;     // 1 second
    public static final long END_ACK_TIMEOUT_MS = 30000;       // 30 seconds
    public static final long READY_TIMEOUT_MS = 10000;         // 10 seconds
    public static final int MAX_RETRIES = 3;
    

    public static final int DEFAULT_MTU = 117;
    public static final int MAX_MTU = 240;
    

    public static final int DEFAULT_BAUD_RATE = 1000000;
    

    public static final int DEFAULT_HOST_FEATURES = 
        DFU_FEAT_STATUS_PROGRESS | 
        DFU_FEAT_CRC32_IMAGE | 
        DFU_FEAT_VERIFY | 
        DFU_FEAT_INSTALL;
}
