package com.echonous.kLink;

/**
 * Struct like class that hold firmware details of a Uscan USB Hub
 */
public class KLinkFirmwareStatus
{
    //==========================================================================
    // Public Section
    //==========================================================================

    //==========================================================================
    // Object Overrides

    @Override
    public String toString()
    {
        final int INITIAL_STR_BUFFER_LEN = 128;
        StringBuilder builder = new StringBuilder(INITIAL_STR_BUFFER_LEN);
        builder.append("Hub FW Status:\nActive:");
        builder.append(mActiveImage).append(" Priority: ").append(mPriorityImage);
        builder.append("\nFw1: Valid: ").append(mFw1Valid).append(" Version: ");
        builder.append(mFw1MajorVer).append('.').append(mFw1MinorVer).append('.').append(mFw1Rev);
        builder.append("\nFw2: Valid: ").append(mFw2Valid).append(" Version: ");
        builder.append(mFw2MajorVer).append('.').append(mFw2MinorVer).append('.').append(mFw2Rev);
        builder.append("\nSerial: ").append(mSerial).append(" HW Rev: ").append(mHwRev);
        builder.append("Flags: ").append(mConfigurationFlags);
        builder.append("\nSilegoRev: ").append(mSilegoRev).append('\n');
        builder.append("\nBattery Percentage: ").append(mBatteryPercentage).append('\n');
        builder.append("\nPart: ").append(mPartNo).append('\n');
        return builder.toString();
    }

    //==========================================================================
    // Public members for struct - like class

    /** The firmware image that is currently active (1 or 2) */
    public short mActiveImage = 0;

    /** The firmware image that currently has priority */
    public short mPriorityImage;

    /** Whether the Firmware1 image is valid */
    public boolean mFw1Valid;
    /** The major version of the Firmware1 image */
    public short mFw1MajorVer;
    /** The minor version of the Firmware1 image */
    public short mFw1MinorVer;
    /** The revision of the Firmware1 image */
    public short mFw1Rev;

    /** Whether the Firmware2 image is valid */
    public boolean mFw2Valid;
    /** The major version of the Firmware2 image */
    public short mFw2MajorVer;
    /** The minor version of the Firmware2 image */
    public short mFw2MinorVer;
    /** The revision of the Firmware2 image */
    public short mFw2Rev;

    /** The serial number of the hub */
    public String mSerial;

    /** The hardware revision number of the hub */
    public String mHwRev;

    /** Configuration flags set in the configuration information */
    public short mConfigurationFlags;

    /** Revision information from the Silego device. */
    public short mSilegoRev;

    /** Hub Part number **/
    public String mPartNo;

    /**
     *  Get Battery Percentage during reading Firmware info of the Hub
     */
    public short mBatteryPercentage;

}
