package com.echonous.usb;

import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;

import com.echonous.hardware.ultrasound.UltrasoundManager;
import com.echonous.util.LogAreas;
import com.echonous.util.LogUtils;


/**
 * Abstract class to provide common functionality for wrapped native probes.
 *
 */
public abstract class AndroidUsbDevice implements UltrasoundManager.UsbMonitor {

    /** Reference to the UsbManager */
    private UsbManager mUsbManager = null;

    //==========================================================================
    // Connected device data

    /** The USB device details of the current device (or null if no open device */
    private UsbDeviceDetails mUsbDeviceDetails = null;

    /** USB Connection object to the current device */
    private UsbDeviceConnection mUsbDeviceConnection = null;

    /** The USB Interface we are using with the current device */
    private UsbInterface mUsbInterface = null;

    //==========================================================================
    // Constants

    /** Logging Tag */
    private static final String TAG = LogAreas.KLINK;

    /** The interface number of the probe that we should use */
    private static final int USB_INTERFACE_NUMBER = 0;

    private UsbProbeState mUsbProbeState = UsbProbeState.INACTIVE;

    /**
     * Constructor
     *
     * @param aContext Context for accessing system services
     */
    public AndroidUsbDevice(Context aContext)
    {
        mUsbManager = (UsbManager)aContext.getSystemService(Context.USB_SERVICE);
    }

    //==========================================================================
    // Generic Functions

    /**
     * Cleans up a device if a disconnection notification is received
     *
     * @param aDevice The details of the device.
     */
    public synchronized void checkCleanupDevice(UsbDevice aDevice)
    {
        String device_name = aDevice.getDeviceName();
        // If we currently have the specified device open, then process and close it.
        if (mUsbDeviceDetails != null && mUsbDeviceDetails.getUsbDevice().getDeviceName().equals(device_name))
        {
            LogUtils.v(TAG, "Processing Disconnect of : " + aDevice.getDeviceName());
            closeDevice();
        }
    }

    /**
     * Gets the USB Device associated with the probe
     */
    public UsbDeviceDetails getUsbDevice()
    {
        return mUsbDeviceDetails;
    }

    //==========================================================================
    // UsbMonitor Overrides

    @Override
    public void onDeviceConnected(UsbDevice aUsbDevice)
    {
        // Open the device.
        openDevice(aUsbDevice);
    }

    @Override
    public void onDeviceDisconnected(UsbDevice aUsbDevice)
    {
        checkCleanupDevice(aUsbDevice);
    }

    @Override
    public UsbDevice getCurrentUsbDevice()
    {
        return mUsbDeviceDetails != null ? mUsbDeviceDetails.getUsbDevice() : null;
    }

    @Override
    public AndroidUsbDevice getCurrentAndroidUsbDevice(){
        return this;
    }

    @Override
    public boolean isConnected()
    {
        return mUsbDeviceDetails != null;
    }

    //==========================================================================
    // Protected Section
    //==========================================================================

    /**
     * Result codes used when processing the bootloader of the probe
     */
    protected enum BootloaderResult
    {
        /** Processing of the bootloader succeded */
        PROCESS_SUCCESS,

        /** Processing of the bootloader failed */
        PROCESS_FAILED,

        /** No special processing of the bootlader was needed */
        PROCESSING_NOT_NEEDED
    }

    //==========================================================================
    // Methods to be used by derived probes

    /**
     * Gets the USB connection associated with the probe.
     */
    public UsbDeviceConnection getUsbDeviceConnection()
    {
        return mUsbDeviceConnection;
    }

    /**
     * Gets whether the currently connected device is for a bootloader.
     */
    protected abstract boolean isBootloaderDevice();

    /**
     * Processes a connected bootloader device, downloading firmware to it.
     *
     * @param aDevice The USB device that is connected
     *
     * @return The result of processing the bootloader
     */
    protected BootloaderResult processBootloader(UsbDevice aDevice)
    {
        return BootloaderResult.PROCESSING_NOT_NEEDED;
    }

    public void setProbeState(UsbProbeState usbProbeState){
        mUsbProbeState = usbProbeState;
    }

    public UsbProbeState getProbeState() {
        return mUsbProbeState;
    }
    /**
     * Whether to wait for a full probe during the Initial Setup Wizard.
     *
     * @return Whether to wait for the full USB device to connect (rather
     *         than in bootloader mode)
     */
    protected boolean waitForFullUsbDevice()
    {
        return false;
    }

    //==========================================================================
    // Connectivity methods

    /**
     * Attempts to open a USB probe device
     *
     * @param aDevice The details of the USB device to open
     *
     * @return Whether the device was successfully opened.
     */
    protected boolean openDevice(UsbDevice aDevice)
    {
        synchronized (this)
        {
            // Open a connection
            mUsbDeviceConnection = mUsbManager.openDevice(aDevice);
            if (mUsbDeviceConnection == null)
            {
                LogUtils.d(TAG, "Failed to open USB device" + aDevice.toString());
                return false;
            }

            mUsbDeviceDetails = new UsbDeviceDetails(aDevice);

            // Process the bootloader (if needed)
            if (isBootloaderDevice())
            {
                BootloaderResult processing_result = processBootloader(aDevice);
                if (processing_result != BootloaderResult.PROCESSING_NOT_NEEDED)
                {
                    return processing_result == BootloaderResult.PROCESS_SUCCESS;
                }
            }

            byte[] descriptors = mUsbDeviceConnection.getRawDescriptors();

            // Here putting check of length greater than 4 because we need the value of index 3 for usb version.
            if (descriptors != null && descriptors.length > 4)
            {
                // Usb version information is available on index 2 & 3 of descriptors byte array.
                // Right now we only need  usb version in format like "2" or "3". that's why using only index 3.
                // In future if we want usb version like 2.0 0r 3.1 format then also use index 2 as well.
                int usb_version = ((int)descriptors[3] & 0xff);
                mUsbDeviceDetails.setUsb3(usb_version == 3);
                LogUtils.i(TAG, "Usb version is " + usb_version);
            }

            // The count of interfaces on the device must be greater than the interface number we are trying to access.
            // This is because interfaces of a device are stored in a zero indexed array.
            if (aDevice.getInterfaceCount() <= USB_INTERFACE_NUMBER)
            {
                LogUtils.d(TAG, "USB device does not have expected number of interfaces " + aDevice.toString());
                closeDevice();
                return false;
            }

            mUsbInterface = aDevice.getInterface(USB_INTERFACE_NUMBER);
            if (!mUsbDeviceConnection.claimInterface(mUsbInterface, true))
            {
                LogUtils.d(TAG, "Failed to claim USB device interface for " + aDevice.toString());
                closeDevice();
                return false;
            }
        }

        // Notify the native probe that we have connected
        LogUtils.i(TAG, "USB device connected with USB ID " + aDevice.getProductId());
        boolean result = deviceConnected(mUsbDeviceConnection);

        if (!result)
        {
            LogUtils.e(TAG, "Native USB failed to connect");
            closeDevice();
        }

        return result;
    }

    /**
     * Closes the currently open device
     */
    protected synchronized void closeDevice()
    {
        // Send through a disconnection notification if the native probe thinks it is still connected.
        deviceDisconnected();

        // Clean up the interface and device connection
        if (mUsbDeviceConnection != null)
        {
            if (mUsbInterface != null) mUsbDeviceConnection.releaseInterface(mUsbInterface);
            mUsbDeviceConnection.close();
            mUsbDeviceConnection = null;
        }
        mUsbDeviceDetails = null;
    }

    //==========================================================================
    // Abstract methods

    /**
     * Call into the probe's implementation code to notify it that the probe has been connected
     *
     * @param aUsbDeviceConnection The UsbProbeConnection for the device
     *
     * @return Whether the probe connection was successful.
     */
    protected abstract boolean deviceConnected(UsbDeviceConnection aUsbDeviceConnection);

    /**
     * Call into the probe's implementation code to notify it that the probe has been disconnected
     */
    protected abstract void deviceDisconnected();

}
