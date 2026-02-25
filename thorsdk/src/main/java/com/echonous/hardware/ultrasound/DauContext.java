/*
 * Copyright (C) 2016 EchoNous, Inc.
 */

package com.echonous.hardware.ultrasound;

import java.io.IOException;

import android.content.Context;
import android.os.Parcel;
import android.os.ParcelFileDescriptor;
import android.os.Parcelable;

import com.echonous.util.LogUtils;


/**
 * Provides access to Dau resources. 
 * 
 */
public class DauContext implements Parcelable {
    private static final String TAG = "DauContext";

    public Context mAndroidContext;
    public boolean mIsUsbContext = false;

    //
    // The following are for Proprietary interface
    //
    public ParcelFileDescriptor mEibIoctlFd;
    public ParcelFileDescriptor mIonFd;

    public ParcelFileDescriptor mDauPowerFd;
    public ParcelFileDescriptor mDauDataLinkFd;
    public ParcelFileDescriptor mDauDataIrqFd;
    public ParcelFileDescriptor mDauErrorIrqFd;
    public ParcelFileDescriptor mDauResourceFd;

    //
    // The following are for Usb interface
    //
    public int mNativeUsbFd = -1;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        out.writeFileDescriptor(mEibIoctlFd.getFileDescriptor());
        out.writeFileDescriptor(mIonFd.getFileDescriptor());

        if (null != mDauPowerFd) {
            out.writeFileDescriptor(mDauPowerFd.getFileDescriptor());
        }
        if (null != mDauDataLinkFd) {
            out.writeFileDescriptor(mDauDataLinkFd.getFileDescriptor());
        }
        if (null != mDauDataIrqFd) {
            out.writeFileDescriptor(mDauDataIrqFd.getFileDescriptor());
        }
        if (null != mDauErrorIrqFd) {
            out.writeFileDescriptor(mDauErrorIrqFd.getFileDescriptor());
        }
        if (null != mDauResourceFd) {
            out.writeFileDescriptor(mDauResourceFd.getFileDescriptor());
        }
    }

    public void readFromParcel(Parcel in) {
        LogUtils.d(TAG, "readFromParcel: (in) parcel size: " + in.dataSize());

        try {
            
        mEibIoctlFd = in.readFileDescriptor().dup();
        mIonFd = in.readFileDescriptor().dup();

        mDauPowerFd = in.readFileDescriptor().dup();
        mDauDataLinkFd = in.readFileDescriptor().dup();
        mDauDataIrqFd = in.readFileDescriptor().dup();
        mDauErrorIrqFd = in.readFileDescriptor().dup();
        mDauResourceFd = in.readFileDescriptor().dup();
        } catch (IOException ex) {
            LogUtils.e(TAG, "failed to duplicate file descriptor", ex);
        }
    }

    public static final Parcelable.Creator<DauContext> CREATOR =
            new Parcelable.Creator<DauContext>() {
        @Override
        public DauContext createFromParcel(Parcel in) {
            DauContext context = new DauContext();
            context.readFromParcel(in);

            return context;
        }

        @Override
        public DauContext[] newArray(int size) {
            return new DauContext[size];
        }
    };
}

