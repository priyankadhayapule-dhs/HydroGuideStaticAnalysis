/*
 * Copyright (C) 2020 EchoNous, Inc.
 */

package com.echonous.hardware.ultrasound;

/**
 * <p>Object that contains all of the configuration parameters that control 
 * detection of single click, double click, long press, and scrolling. See 
 * the protocol document for more information.</p> 
 */
public final class PuckParams {
    private static final String TAG = "PuckParams";

    /**
     * There are four scroll position thresholds and step values.  Use this 
     * enumeration to specify which value is being read or modified. 
     */
    public enum ScrollPosition {
        POS1(0),
        POS2(1),
        POS3(2),
        POS4(3);

        private int index;

        ScrollPosition(int index) {
            this.index = index;
        }

        protected int getIndex() {
            return this.index;
        }
    }

    private static final int MIN_CLICK = 1;
    private static final int MAX_CLICK = 65535;
    private static final int MIN_THRESHOLD = 0;
    private static final int MAX_THRESHOLD = 255;
    private static final int MIN_STEP = 0;
    private static final int MAX_STEP = 255;

    private int mSingleClickMin = MIN_CLICK;
    private int mSingleClickMax = MIN_CLICK;

    private int mDoubleClickMin = MIN_CLICK;
    private int mDoubleClickMax = MIN_CLICK;

    private int mLongClickMax = MIN_CLICK;

    private int[] mScrollThresholds = {MIN_THRESHOLD, MIN_THRESHOLD, MIN_THRESHOLD, MIN_THRESHOLD};
    private int[] mScrollSteps = {MIN_STEP, MIN_STEP, MIN_STEP, MIN_STEP};


    public PuckParams() {
    }

    //
    // Single Click
    //

    /**
     * Sets the minimum time a button must be pressed to be considered a single 
     * click. 
     * 
     * @param min minimum press time in milliseconds
     */
    public void setSingleClickMin(int min) throws IllegalArgumentException {
        if (min < MIN_CLICK || min > MAX_CLICK)
        {
            throw new IllegalArgumentException();
        }

        mSingleClickMin = min;
    }

    /**
     * Returns the minimum touch time for a single click
     * 
     * @return int minimum press time in milliseconds
     */
    public int getSingleClickMin() {
        return mSingleClickMin;
    }

    /**
     * Sets the maximum time a button must be pressed to be considered a single 
     * click. 
     * 
     * @param max maximum press time in milliseconds
     */
    public void setSingleClickMax(int max) throws IllegalArgumentException {
        if (max < MIN_CLICK || max > MAX_CLICK)
        {
            throw new IllegalArgumentException();
        }

        mSingleClickMax = max;
    }

    /**
     * Returns the maximum touch time for a single click
     * 
     * @return int maximum press time in milliseconds
     */
    public int getSingleClickMax() {
        return mSingleClickMax;
    }


    //
    // Double Click
    //

    /**
     * Sets the minimum time a button must be pressed after the first click to be 
     * considered a double click. 
     * 
     * @param min minimum press time in milliseconds
     */
    public void setDoubleClickMin(int min) throws IllegalArgumentException {
        if (min < MIN_CLICK || min > MAX_CLICK)
        {
            throw new IllegalArgumentException();
        }

        mDoubleClickMin = min;
    }

    /**
     * Returns the minimum touch time for a double click
     * 
     * @return int minimum press time in milliseconds
     */
    public int getDoubleClickMin() {
        return mDoubleClickMin;
    }

    /**
     * Sets the maximum time a button must be pressed after the first click to be 
     * considered a double click. 
     * 
     * @param min maximum press time in milliseconds
     */
    public void setDoubleClickMax(int max) throws IllegalArgumentException {
        if (max < MIN_CLICK || max > MAX_CLICK)
        {
            throw new IllegalArgumentException();
        }

        mDoubleClickMax = max;
    }

    /**
     * Returns the maximum touch time for a double click
     * 
     * @return int maximum press time in milliseconds
     */
    public int getDoubleClickMax() {
        return mDoubleClickMax;
    }


    //
    // Long Click
    //

    /**
     * Sets the time a button must be pressed for a long press to be detected.
     * 
     * @param max time in milliseconds
     */
    public void setLongClickMax(int max) throws IllegalArgumentException {
        if (max < MIN_CLICK || max > MAX_CLICK)
        {
            throw new IllegalArgumentException();
        }

        mLongClickMax = max;
    }

    /**
     * Returns the time a button must be pressed to be considered a long press.
     * 
     * @return int time in milliseconds
     */
    public int getLongClickMax() {
        return mLongClickMax;
    }


    //
    // Scroll Positions
    //

    /**
     * Sets the scroll threshold for the specified scroll position.
     * 
     * @param pos the scroll position to modify
     * @param threshold the new threshold
     */
    public void setScrollThreshold(ScrollPosition pos, int threshold) throws IllegalArgumentException {
        if (threshold < MIN_THRESHOLD || threshold > MAX_THRESHOLD)
        {
            throw new IllegalArgumentException();
        }

        mScrollThresholds[pos.getIndex()] = threshold;
    }

    /**
     * Returns the scroll threshold for the specified scroll position.
     * 
     * @param pos the scroll position
     * 
     * @return int the threshold
     */
    public int getScrollThreshold(ScrollPosition pos) {
        return mScrollThresholds[pos.getIndex()];
    }


    //
    // Scroll Steps
    //

    /**
     * Sets the scroll step for the specified scroll position.
     * 
     * @param pos the scroll position to modify
     * @param step the new step value
     */
    public void setScrollStep(ScrollPosition pos, int step) throws IllegalArgumentException {
        if (step < MIN_STEP || step > MAX_STEP)
        {
            throw new IllegalArgumentException();
        }

        mScrollSteps[pos.getIndex()] = step;
    }

    /**
     * Returns the scroll step value for the specified scroll position.
     * 
     * @param pos the scroll position
     * 
     * @return int the step value
     */
    public int getScrollStep(ScrollPosition pos) {
        return mScrollSteps[pos.getIndex()];
    }
}
