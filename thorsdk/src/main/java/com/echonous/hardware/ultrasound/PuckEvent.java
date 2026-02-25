/*
 * Copyright (C) 2018 EchoNous, Inc.
 */

package com.echonous.hardware.ultrasound;

/**
 * <p>Object used to report puck events.  Includes button presses for left and 
 * right buttons, palm detection, and scroll direction / distance.</p> 
 */
public final class PuckEvent {
    private int mLeftButtonAction;
    private int mRightButtonAction;
    private int mPalmAction;
    private int mScrollAction;
    private int mScrollAmount;
    private int mUpdateCurPos;
    private int mUpdateMaxPos;
    private ThorError mUpdateStatus;

    protected PuckEvent() {
    }

    public PuckEvent(int leftButtonAction,
                        int rightButtonAction,
                        int palmAction,
                        int scrollAction,
                        int scrollAmount,
                        int updateCurPos,
                        int updateMaxPos,
                        int updateStatus) {
        mLeftButtonAction = leftButtonAction;
        mRightButtonAction = rightButtonAction;
        mPalmAction = palmAction;
        mScrollAction = scrollAction;
        mScrollAmount = scrollAmount;
        mUpdateCurPos = updateCurPos;
        mUpdateMaxPos = updateMaxPos;
        mUpdateStatus = ThorError.fromCode(updateStatus);
    }

    /**
     * Returns the Action of the left button.
     * 
     * @return int Left Button Action.  See BUTTON_ACTION_* constants.
     */
    public int getLeftButtonAction() {
        return mLeftButtonAction;
    }

    /**
     * Returns the Action of the right button.
     * 
     * @return int Right Button Action.  See BUTTON_ACTION_* constants.
     */
    public int getRightButtonAction() {
        return mRightButtonAction;
    }

    /**
     * Returns the Palm action.
     * 
     * @return int Palm Action.  See PALM_ACTION_* constants.
     */
    public int getPalmAction() {
        return mPalmAction;
    }

    /**
     * Returns the scroll action.
     * 
     * @return int Scroll Action.  See SCROLL_ACTION_* constants.
     */
    public int getScrollAction() {
        return mScrollAction;
    }

    /**
     * Returns the amount scrolled.  The default possible values are 3, 5, 7, and 9. 
     * Actual values are determined by the programmed configuration. 
     * 
     * @return int Scroll Amount.
     */
    public int getScrollAmount() {
        return mScrollAmount;
    }

    /**
     * Returns the current firmware update position. This value will always be 0 
     * unless a firmware udpate is in progress. During a firmware update, the 
     * current position will increase from 0 to getUpdateMaxPos().
     * 
     * @return int current update position
     */
    public int getUpdateCurPos() {
        return mUpdateCurPos;
    }

    /**
     * Returns the maximum firmware update position. This value will always be 0 
     * unless a firmware update is in progress. 
     * 
     * @return int max update position
     */
    public int getUpdateMaxPos() {
        return mUpdateMaxPos;
    }

    /**
     * Returns the firmware update status. 
     * THOR_OK: Firmware update completed successfully, or no firmware update in 
     * progress. 
     * TER_TABLET_PUCK_UPDATE_FIRMWARE_BUSY: Firmware update is in progress 
     * TER_TABLET_PUCK_UPDATE_FIRMWARE: Error occurred while updating the firmware 
     * 
     * @return ThorError update status
     */
    public ThorError getUpdateStatus() {
        return mUpdateStatus;
    }

    //
    // Button Actions
    //

    /**
     * A button state is unchanged.
     */
    public static final int BUTTON_ACTION_NA = 0x0;

    /**
     * A button was single clicked.
     */
    public static final int BUTTON_ACTION_SINGLE = 0x20;

    /**
     * A button was double clicked.
     */
    public static final int BUTTON_ACTION_DOUBLE = 0x22;

    /**
     * A button was long pressed.
     */
    public static final int BUTTON_ACTION_LONG = 0x40;


    //
    // Palm Actions
    //

    /**
     * The palm state is unchanged
     */
    public static final int PALM_ACTION_NA = 0x0;

    /**
     * The palm is touched
     */
    public static final int PALM_ACTION_TOUCHED = 0x20;


    //
    // Scroll Actions
    //

    /**
     * The scroll state is unchanged.
     */
    public static final int SCROLL_ACTION_NA = 0x0;

    /**
     * Scroll up.
     */
    public static final int SCROLL_ACTION_UP = 0x50;

    /**
     * Scroll down.
     */
    public static final int SCROLL_ACTION_DOWN = 0x58;
}

