package com.echonous.usb;

import android.hardware.usb.UsbDevice;

import com.echonous.util.LogAreas;

/**
 * Class that holds the details of a USB device, including the details
 * of the hub and hub port it is connected to (if connected to a hub)
 */
public class UsbDeviceDetails {

    // Constants

    /** Constant used when no USB level has been identified */
    public static final int INVALID_USB_LEVEL = -1;

    /**
     * The USB device at which we are a hub connected device - note that
     * level 1 is connected directly to the root hub/controller, and
     * then level 2 is connected via hub. If the value is greater than this, the
     * there are more hubs in the path
     */
    public static final int MIN_HUB_CONNECTED_USB_DEVICE_LEVEL = 2;

    /** The basic USB details provided directly by the system */
    private UsbDevice mDevice = null;

    /** The /dev path that is used to access the USB device */
    private String mDevPath = null;

    /** The path name for the USB device in the /sys/bus/usb/devices tree */
    private String mUsbPath = null;

    /** The level in the heirarchy that the device sets - indicates what hubs is connects to */
    private int mUsbLevel = INVALID_USB_LEVEL;

    /** Flag that indicates if this is a USB3 device */
    private boolean mUsb3 = false;

    /** Logging tag for this class */
    private static final String TAG = LogAreas.SYSTEM;

    /**
     * Constructor
     *
     * @param aDevice The basic USB details that this class wraps
     */
    public UsbDeviceDetails(UsbDevice aDevice)
    {
        mDevice = aDevice;
        mDevPath = aDevice.getDeviceName();
    }

    //==========================================================================
    // Accessors

    /**
     * Gets the basic USB details provided directly by Android
     */
    public UsbDevice getUsbDevice()
    {
        return mDevice;
    }

    /**
     * Gets the path of the USB device in the /sys file system - this includes
     * hub/port information in its format
     */
    public String getUsbPath()
    {
        return mUsbPath;
    }

    /**
     * Gets the level of this device in the USB hierarchy - 1 means a root hub,
     * 2 means a direct connected USB device, and 3 means a hub connected device.
     * Higher levels are possible if more hubs are in the chain.
     */
    public int getUsbLevel()
    {
        return mUsbLevel;
    }

    /**
     * Gets whether the device is connected as a SuperSpeed (USB3) device
     */
    public boolean isUsb3()
    {
        return mUsb3;
    }

    /**
     * Set the level of this device in the USB hierarchy - 1 means a root hub,
     * 2 means a direct connected USB device, and 3 means a hub connected device.
     * Higher levels are possible if more hubs are in the chain.
     */
    public void setUsbLevel(int aUsbLevel)
    {
        mUsbLevel = aUsbLevel;
    }

    /**
     * Set the whether usb device is type of 2 or 3
     *
     * @param aIsUsb3 boolean value to set, true if its usb3 false otherwise
     */
    public void setUsb3(boolean aIsUsb3)
    {
        mUsb3 = aIsUsb3;
    }
}
