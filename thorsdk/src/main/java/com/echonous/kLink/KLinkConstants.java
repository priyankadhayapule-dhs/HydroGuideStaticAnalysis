package com.echonous.kLink;

/**
 * Class containing constants relating to the interface between the
 * Hub and the display
 */
public final class KLinkConstants
{

    //==========================================================================
    // Public Section
    //==========================================================================

    //==========================================================================
    // Hub Commands

    /** Gets the current firmware status information (IN) */
    public static final int KLINK_CMD_GET_FW_STATUS = 1;

    /** Flashes a row in the hub flash (OUT - 128 bytes - flash row) */
    public static final int KLINK_CMD_FLASH_ROW = 2;

    /** Activates a firmware image (OUT - no data) */
    public static final int KLINK_CMD_ACTIVATE_FW = 3;

    /** Performs a software reset on the hub (OUT - no data) */
    public static final int KLINK_CMD_RESET = 4;

    /** Sets the enabled status of the downstream ports (OUT - no data) */
    public static final int KLINK_CMD_SET_PORT_STATUS = 0x06;

    /** Gets the current status of the hub (IN) */
    public static final int KLINK_CMD_GET_KLINK_STATUS = 10;

    /**
     * Get Battery status from Pac-Man
     */
    public static final int KLINK_CMD_BATTERY_DETAILS = 15;

    //==========================================================================
    // Message Parameter constants

    /** The length of the hub serial number field in bytes */
    public static final int SERIAL_LEN = 16;

    /** The length of the hub hardware revision field in bytes */
    public static final int HW_REV_LEN = 16;

    /** The length of the hub part number field in bytes */
    public static final int PART_LEN = 16;

    /** Identifier of the first downstream port in port enabled bitmasks */
    public static final int DS_PORT_1 = (1 << 0);

    /** Identifier of the second downstream port in port enabled bitmasks */
    public static final int DS_PORT_2 = (1 << 1);

    public static final int DS_PORT_3 = (1 << 2);



    //==========================================================================
    // USB Constants

    /** The USB request type for a vendor defined USB OUT request */
    public static final int KLINK_REQUEST_TYPE_OUT = 0x40;

    /** The USB request type for a vendor defined USB IN request */
    public static final int KLINK_REQUEST_TYPE_IN = 0xC0;

    /** The request ID for requests to the hub */
    public static final int KLINK_REQUEST = 1;

    /** The setup index value used for commands where it is unused */
    public static final int KLINK_INDEX_UNUSED = 0;

    /** The hub transaction timeout in ms */
    public static final int KLINK_TRANSACTION_TIMEOUT_MS = 2000;

    /** The number at hundredth place*/
    public static final int HUNDRED_PLACE = 100;

    /** The number at ten thousands place*/
    public static final int TEN_THOUSAND_PLACE = 10000;

    //==========================================================================
    // Private Section
    //==========================================================================

    //==========================================================================
    // Constructors and Object Created

    /**
     * Constructor - unused
     */
    private KLinkConstants()
    {
        // Empty
    }

}
