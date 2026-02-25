package com.echonous.kLink;

import androidx.annotation.NonNull;

public class KLinkStatusInfo {

    /**!< The current DS port 2 CC polarity (DPM_POLARITY_NONE, DPM_POLARITY_CC1 or DPM_POLARITY_CC2)**/
    public short mDs1CcPolarity;

    /** !< The current DS port 1 CC polarity (DPM_POLARITY_NONE, DPM_POLARITY_CC1 or DPM_POLARITY_CC2)**/
    public short mDs2CcPolarity;

    public short mDs3CcPolarity;


    public int mBatteryPercentage = 0;

    public int mAcStatus = 0;

    public int mAuthCharger = 0;

    @NonNull
    @Override
    public String toString()
    {
        final int INITIAL_STR_BUFFER_LEN = 128;
        StringBuilder builder = new StringBuilder(INITIAL_STR_BUFFER_LEN);
        builder.append("\nDs1CcPolarity: ").append(mDs1CcPolarity).append('\n');
        builder.append("\nDs2CcPolarity: ").append(mDs2CcPolarity).append('\n');
        builder.append("\nDs3CcPolarity: ").append(mDs3CcPolarity).append('\n');

        return builder.toString();
    }
}
