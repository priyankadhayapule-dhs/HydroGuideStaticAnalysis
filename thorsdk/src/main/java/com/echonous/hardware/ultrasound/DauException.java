//
// Copyright 2017 EchoNous Inc.
//
//
package com.echonous.hardware.ultrasound;

import android.content.Context;

/**
 * <p><code>DauException</code> is thrown if a Dau could not be initialized or 
 * created by the {@link UltrasoundManager}, or if the connection to an opened 
 * {@link Dau} is no longer valid. Contained within is the source of the 
 * exception in the form of a {@link ThorError}.</p> 
 *  
 * @see ThorError 
 * @see UltrasoundManager 
 * @see Dau 
 */
public class DauException extends Exception {
    private ThorError mThorError;

    private DauException() {
    }

    /**
     * @hide
     */
    protected DauException(ThorError thorError, Throwable cause) {
        super(cause);
        mThorError = thorError;
    }

    /**
     * @hide
     */
    protected DauException(ThorError thorError) {
        super(thorError.toString());
        mThorError = thorError;
    }

    /**
     * Return the raw error code
     * 
     */
    public int getCode() {
        return mThorError.getCode();
    }

    /**
     * Returns the name of the error causing the exception.
     * 
     */
    public String getName() {
        return mThorError.getName();
    }

    /**
     * Returns the description of error causing the exception.
     * 
     */
    public String getDescription() {
        return mThorError.getDescription();
    }

    /**
     * Returns the source of the error causing the exception.
     * 
     */
    public ThorError getSource() {
        return mThorError.getSource();
    }

    /**
     * Returns a localized description of this error.
     * 
     * 
     * @param context The application context for retrieving the localized string 
     *                resource. The name of the resource string should be the same
     *                as the ThorError enumeration.
     * 
     * @return String The localized description.  If one does not exist, then null 
     *         is returned.
     */
    public String getLocalizedMessage(Context context) {
        return mThorError.getLocalizedMessage(context);
    }

    /**
     * Returns the ThorError that caused the exception.
     * 
     */
    public ThorError getError() {
        return mThorError;
    }
}
