package com.echonous.kLink;

import static com.echonous.kLink.KLinkConstants.DS_PORT_1;
import static com.echonous.kLink.KLinkConstants.DS_PORT_2;
import static com.echonous.kLink.KLinkConstants.DS_PORT_3;
import static com.echonous.kLink.KLinkConstants.KLINK_CMD_ACTIVATE_FW;
import static com.echonous.kLink.KLinkConstants.KLINK_CMD_BATTERY_DETAILS;
import static com.echonous.kLink.KLinkConstants.KLINK_CMD_GET_FW_STATUS;
import static com.echonous.kLink.KLinkConstants.KLINK_CMD_GET_KLINK_STATUS;
import static com.echonous.kLink.KLinkConstants.KLINK_CMD_RESET;
import static com.echonous.kLink.KLinkConstants.KLINK_CMD_SET_PORT_STATUS;
import static com.echonous.kLink.KLinkConstants.KLINK_INDEX_UNUSED;
import static com.echonous.kLink.KLinkConstants.KLINK_REQUEST;
import static com.echonous.kLink.KLinkConstants.KLINK_REQUEST_TYPE_IN;
import static com.echonous.kLink.KLinkConstants.KLINK_REQUEST_TYPE_OUT;
import static com.echonous.kLink.KLinkConstants.KLINK_TRANSACTION_TIMEOUT_MS;
import static com.echonous.kLink.KLinkConstants.HUNDRED_PLACE;
import static com.echonous.kLink.KLinkConstants.HW_REV_LEN;
import static com.echonous.kLink.KLinkConstants.PART_LEN;
import static com.echonous.kLink.KLinkConstants.SERIAL_LEN;
import static com.echonous.kLink.KLinkConstants.TEN_THOUSAND_PLACE;

import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;

import com.echonous.usb.AndroidUsbDevice;
import com.echonous.usb.UsbDeviceState;
import com.echonous.util.LogAreas;
import com.echonous.util.LogUtils;

import java.nio.charset.StandardCharsets;
import java.util.HashMap;

/**
 * Class that handles and manages a kLink USB kLink device
 */
public class KLinkUsbDevice extends AndroidUsbDevice {

    /** USB Vendor ID for Signostics */
    public static final int K_LINK_USB_VID = 0x1dbf;

    /** The USB Product ID of the kLink */
    public static final int K_LINK_USB_PRODUCT_ID = 0xA;
    /** Logging tag */
    private static final String TAG = LogAreas.KLINK;
    /** The current device state */
    private UsbDeviceState mDeviceState = UsbDeviceState.DISCONNECTED;
    /** KLinkFirmwareImage to managed the bundled kLink firmware */
    private final KLinkFirmwareImage mKLinkFirmwareImage;

    /** The default size to provide for USB reads */
    private static final int USB_DEFAULT_READ_SIZE = 64;

    private static final int USB_BATTERY_BUFFER_SIZE = 8;

    /** Listener for kLink notifications */
    private KLinkListener mKLinkListener = null;

    /** Kosmos Link Part Number */
    private static final String K_LINK_PART_NUMBER = "P008049";


    private boolean isPortEnabled = false;
    private boolean isPort2Enabled = false;
    private boolean isPort3Enabled = false;

    private int mPrevPowerStateMask = 0;

    private final HashMap<Integer,Integer> mProbePortMap = new HashMap<>();

    public static final int mPortNo1 = 1;
    public static final int mPortNo2 = 2;
    public static final int mPortNo3 = 3;

    /**
     * Interface that defines notifications that can be generated on kLink events
     */
    public interface KLinkListener
    {
        /** A kLink has just connected */
        void kLinkConnected(boolean updateStatus);

        /** A kLink has just disconnected */
        void kLinkDisconnected();

        /** Stop the Probe detection progress indicator */
        void stopProgress();
    }

    public void addProbeMap(int probePid){
        if(isConnected() && !mProbePortMap.containsKey(probePid)){
            int portNumber = -1;
            for (int i = mPortNo1; i <= mPortNo3; i++) {
                if (!mProbePortMap.containsValue(i)) {
                    if (isPortOn(i)) {
                        portNumber = i;
                        break;
                    }
                }
            }
            if (portNumber > 0) {
                mProbePortMap.put(probePid, portNumber);
            }
        }
    }

    /**
     * Remove probe by turning off DS Port
     * @param probePid The probe ID
     */
    public void removeProbe(int probePid) {
        if (isConnected()) {
            int portFound = -1;

            if (mProbePortMap.containsKey(probePid)) {
                portFound = mProbePortMap.get(probePid);
                //Remove Probe ID from HashMap()
                mProbePortMap.remove(probePid);
            }

            if (portFound > 0) {
                setPortOff(portFound);
            }
        }
    }

    private boolean isPortOn(int portNumber) {
        if (portNumber == mPortNo1) {
            return isPortEnabled;
        } else if (portNumber == mPortNo2) {
            return isPort2Enabled;
        } else if (portNumber == mPortNo3) {
            return isPort3Enabled;
        }
        return false;
    }

    public void setPortOff(int portNumber) {
        if (portNumber == mPortNo1) {
            isPortEnabled = false;
        } else if (portNumber == mPortNo2) {
            isPort2Enabled = false;
        } else if (portNumber == mPortNo3) {
            isPort3Enabled = false;
        }
    }

    public void setPortOn(int portNumber) {
        if (portNumber == mPortNo1) {
            isPortEnabled = true;
        } else if (portNumber == mPortNo2) {
            isPort2Enabled = true;
        } else if (portNumber == mPortNo3) {
            isPort3Enabled = true;
        }
    }

    /**
     * Constructor
     *
     * @param context The context the application is running in.
     */
    public KLinkUsbDevice(Context context)
    {
        super(context);

        mKLinkFirmwareImage = new KLinkFirmwareImage(context);

        mKLinkFirmwareImage.loadFirmwareImage("UsbHub.hex");
    }

    /**
     * Gets whether the currently connected device is for a bootloader.
     */
    @Override
    protected boolean isBootloaderDevice() {
        return false;
    }

    /**
     * Call into the probe's implementation code to notify it that the probe has been connected
     *
     * @param aUsbDeviceConnection The UsbProbeConnection for the device
     * @return Whether the probe connection was successful.
     */
    @Override
    protected boolean deviceConnected(UsbDeviceConnection aUsbDeviceConnection) {
        // Read the firmware information
        KLinkFirmwareStatus status = getKLinkFirmwareStatus();

        mDeviceState = UsbDeviceState.CONNECTED;

        // Notify the listener the kLink has connected
        if (mKLinkListener != null) {
            mKLinkListener.kLinkConnected(checkFirmwareUpdate(status));
        }

        return true;
    }

    /**
     * Call into the probe's implementation code to notify it that the probe has been disconnected
     */
    @Override
    protected void deviceDisconnected() {
        if (mDeviceState == UsbDeviceState.CONNECTED_HIBERNATE_CALLED)
        {
            mDeviceState = UsbDeviceState.DISCONNECTED_HIBERNATE;
        }
        else
        {
            mDeviceState = UsbDeviceState.DISCONNECTED;
        }

        // Notify the listener
        if (mKLinkListener != null) {
            isPortEnabled = false;
            isPort2Enabled = false;
            isPort3Enabled = false;
            mKLinkListener.kLinkDisconnected();
        }
    }

    /**
     * Gets whether the given USB device a device that the current monitor is monitoring.
     *
     * @param aUsbDevice The device to check whether it belongs the monitor
     * @return Whether the given USB device belongs to the monitor.
     */
    @Override
    public boolean matchUsbDevice(UsbDevice aUsbDevice) {
        // Check for a VID/PID match
        int product_id = aUsbDevice.getProductId();
        int vendor_id = aUsbDevice.getVendorId();
        return vendor_id == K_LINK_USB_VID && product_id == K_LINK_USB_PRODUCT_ID;
    }

    @Override
    public boolean isActive() {
        return false;
    }

    /**
     * Sets a listener to be notified of kLink events
     *
     * @param akLinkListener Listener to receive kLink events
     */
    public void setListener(KLinkListener akLinkListener)
    {
        mKLinkListener = akLinkListener;
    }

    /**
     * Gets firmware information status for the kLink
     *
     * @return The retrieved firmware status information, or null on failure
     */
    public KLinkFirmwareStatus getKLinkFirmwareStatus()
    {
        if (!isConnected()) return null;

        // Request the firmware status from the device
        byte[] buffer = new byte[USB_DEFAULT_READ_SIZE];

        int r = getUsbDeviceConnection().controlTransfer(KLINK_REQUEST_TYPE_IN, KLINK_REQUEST, KLINK_CMD_GET_FW_STATUS,
                KLINK_INDEX_UNUSED, buffer, buffer.length,
                KLINK_TRANSACTION_TIMEOUT_MS);

        // Check we have gotten the minimum number of bytes (note later version may have more data)
        final int MIN_FW_LEN = 44;
        if (r < MIN_FW_LEN)
        {
            LogUtils.e(TAG, "Failed to read kLink fw status. Got " + r);
            return null;
        }

        // Extract the encoded information based on the structure return.
        int index = 0;
        KLinkFirmwareStatus fw_status = new KLinkFirmwareStatus();
        fw_status.mActiveImage = KLinkFirmwareImage.getUint8(buffer, index++);
        fw_status.mPriorityImage = KLinkFirmwareImage.getUint8(buffer, index++);
        fw_status.mFw1Valid = buffer[index++] != 0;
        fw_status.mFw1MajorVer = KLinkFirmwareImage.getUint8(buffer, index++);
        fw_status.mFw1MinorVer = KLinkFirmwareImage.getUint8(buffer, index++);
        fw_status.mFw1Rev = KLinkFirmwareImage.getUint8(buffer, index++);
        fw_status.mFw2Valid = buffer[index++] != 0;
        fw_status.mFw2MajorVer = KLinkFirmwareImage.getUint8(buffer, index++);
        fw_status.mFw2MinorVer = KLinkFirmwareImage.getUint8(buffer, index++);
        fw_status.mFw2Rev = KLinkFirmwareImage.getUint8(buffer, index++);
        fw_status.mSerial = readString(buffer, index, SERIAL_LEN);
        index += SERIAL_LEN;
        fw_status.mHwRev = readString(buffer, index, HW_REV_LEN);
        index += HW_REV_LEN;
        fw_status.mConfigurationFlags = KLinkFirmwareImage.getUint8(buffer, index++);
        fw_status.mBatteryPercentage = KLinkFirmwareImage.getUint8(buffer, index++);
        fw_status.mPartNo = readString(buffer, index, PART_LEN);
        index += PART_LEN;

        return fw_status;
    }

    public KLinkStatusInfo getKLinkStatusInfo(){
        if (!isConnected()) return null;

        // Request the firmware status from the device
        byte[] buffer = new byte[USB_DEFAULT_READ_SIZE];

        int readData = getUsbDeviceConnection().controlTransfer(
                KLINK_REQUEST_TYPE_IN, KLINK_REQUEST, KLINK_CMD_GET_KLINK_STATUS,
                KLINK_INDEX_UNUSED, buffer, buffer.length,
                KLINK_TRANSACTION_TIMEOUT_MS);

        // Check we have gotten the minimum number of bytes (note later version may have more data)
        if (readData < 0)
        {
            LogUtils.e(TAG, "Failed to read kLink Status Info " + readData);
            return null;
        }

        KLinkStatusInfo klinkStatusInfo = new KLinkStatusInfo();
        // Extract the encoded information based on the structure return.
        klinkStatusInfo.mDs3CcPolarity = KLinkFirmwareImage.getUint8(buffer, readData - 2);
        klinkStatusInfo.mDs2CcPolarity = KLinkFirmwareImage.getUint8(buffer, readData - 3);
        klinkStatusInfo.mDs1CcPolarity = KLinkFirmwareImage.getUint8(buffer, readData - 4);

        return klinkStatusInfo;
    }


    public KLinkStatusInfo getKLinkBatteryStatus(){
        if (!isConnected()) return null;

        // Request the firmware status from the device
        byte[] buffer = new byte[USB_BATTERY_BUFFER_SIZE];

        int readData = getUsbDeviceConnection().controlTransfer(
                KLINK_REQUEST_TYPE_IN, KLINK_REQUEST, KLINK_CMD_BATTERY_DETAILS,
                KLINK_INDEX_UNUSED, buffer, buffer.length,
                KLINK_TRANSACTION_TIMEOUT_MS);

        // Check we have gotten the minimum number of bytes (note later version may have more data)
        if (readData < 0)
        {
            LogUtils.e(TAG, "Failed to read kLink Battery Status Info " + readData);
            return null;
        }
        KLinkStatusInfo mKLinkInfo = new KLinkStatusInfo();

        mKLinkInfo.mAcStatus = KLinkFirmwareImage.getUint8(buffer, 0);
        mKLinkInfo.mAuthCharger = KLinkFirmwareImage.getUint8(buffer, 1);
        mKLinkInfo.mBatteryPercentage = KLinkFirmwareImage.getUint8(buffer, 2);

        return mKLinkInfo;
    }


    /**
     * Requests the kLink to perform a software reset
     */
    public void resetKLink()
    {
        if (!isConnected()) return;

        // Reset the kLink - no result is expected.
        LogUtils.i(TAG, "USB-KLink: reset");
        getUsbDeviceConnection().controlTransfer(KLINK_REQUEST_TYPE_OUT, KLINK_REQUEST, KLINK_CMD_RESET,
                KLINK_INDEX_UNUSED, null, 0, KLINK_TRANSACTION_TIMEOUT_MS);
    }

    /**
     * Set the power enabled status for each of the downstream USB ports on the kLink
     *
     * @param aPort1Enabled Whether DS1 port should be enabled
     * @param aPort2Enabled Whether DS2 port should be enabled
     *
     * @return Whether the command was successful
     */
    public boolean setPortPowerStatus(boolean aPort1Enabled, boolean aPort2Enabled,boolean aPort3Enabled)
    {
        // Check we are connected
        if (!isConnected()) {
            isPortEnabled = false;
            isPort2Enabled = false;
            isPort3Enabled = false;
            return false;
        }


        // Build the message to send and then send it
        int ports = 0;
        if (aPort1Enabled) ports |= DS_PORT_1;
        if (aPort2Enabled) ports |= DS_PORT_2;
        if (aPort3Enabled) ports |= DS_PORT_3;

        if (ports != mPrevPowerStateMask)
        {
            getUsbDeviceConnection().controlTransfer(KLINK_REQUEST_TYPE_OUT, KLINK_REQUEST, KLINK_CMD_SET_PORT_STATUS,
                    0, null, 0, KLINK_TRANSACTION_TIMEOUT_MS);
            try {
                Thread.sleep(200);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
        if (getUsbDeviceConnection() != null) {

            int r = getUsbDeviceConnection().controlTransfer(KLINK_REQUEST_TYPE_OUT, KLINK_REQUEST, KLINK_CMD_SET_PORT_STATUS,
                    ports, null, 0, KLINK_TRANSACTION_TIMEOUT_MS);
            if (r < 0)
            {
                isPortEnabled = !aPort1Enabled;
                isPort2Enabled = !aPort2Enabled;
                isPort3Enabled = !aPort3Enabled;
                LogUtils.e(TAG, "Failed to set port power status " + r);
                return false;
            }
            else
            {
                LogUtils.i(TAG, "Probe:: DS Port 1 "+aPort1Enabled+ "DS Port 2 "+aPort2Enabled+" DS Port 3 "+aPort3Enabled);
                isPortEnabled = aPort1Enabled;
                isPort2Enabled = aPort2Enabled;
                isPort3Enabled = aPort3Enabled;
                mPrevPowerStateMask = ports;
                return true;
            }
        }
        return false;
    }

    public boolean isPortEnabled(){
        return isPortEnabled;
    }

    public boolean isPort2Enabled(){
        return isPort2Enabled;
    }

    public boolean isPort3Enabled(){
        return isPort3Enabled;
    }

    /**
     * Powers off both the downstream USB ports on the kLink
     *
     */
    public void powerOffAllPorts()
    {
        LogUtils.i(TAG, "USB-K_LINK: DS port power off all ports");
        if (!isConnected()) return;
        setPortPowerStatus(false,false,false);
    }



    /**
     * Read an ASCII string out of a byte buffer
     *
     * @param aBuffer The buffer to read the string from
     * @param aIndex The start index in the buffer to start reading from
     * @param aLength The length of the string to read in bytes
     *
     * @return The read string
     */
    private String readString(byte[] aBuffer, int aIndex, int aLength)
    {
        int len = 0;
        for (int i = 0; i < aLength; i++)
        {
            if (aBuffer[aIndex + i] != 0)
            {
                len++;
            }
            else
            {
                break;
            }
        }
        return new String(aBuffer, aIndex, len, StandardCharsets.US_ASCII);
    }

    private boolean checkFirmwareUpdate(KLinkFirmwareStatus aFwStatus) {
        int fileVersion, fwVersion;

        fileVersion = mKLinkFirmwareImage.getRev();
        fileVersion += (HUNDRED_PLACE * mKLinkFirmwareImage.getMinorVer());
        fileVersion += (TEN_THOUSAND_PLACE * mKLinkFirmwareImage.getMajorVer());
        LogUtils.d(TAG, "fileVersion Value" + fileVersion);

        if (aFwStatus != null && mKLinkFirmwareImage.isValid()) {
            if (aFwStatus.mActiveImage == 1) {
                fwVersion = aFwStatus.mFw1Rev;
                fwVersion += (HUNDRED_PLACE * aFwStatus.mFw1MinorVer);
                fwVersion += (TEN_THOUSAND_PLACE * aFwStatus.mFw1MajorVer);
                LogUtils.d(TAG, "fw1Version Value" + fwVersion);

                // currently downgrading the firmware for verification
                if ((fwVersion >= 0) && (fileVersion >= 0) && (fileVersion > fwVersion)) {
                    LogUtils.e(TAG, "Firmware Update Available");
                    powerOffAllPorts();
                    return true;
                } else {
                    LogUtils.i(TAG, "kLink Firmware is already up-to-date");
                }
            } else if (aFwStatus.mActiveImage == 2) {
                fwVersion = aFwStatus.mFw2Rev;
                fwVersion += (HUNDRED_PLACE * aFwStatus.mFw2MinorVer);
                fwVersion += (TEN_THOUSAND_PLACE * aFwStatus.mFw2MajorVer);
                LogUtils.d(TAG, "fw2Version Value" + fwVersion);

                // currently downgrading the firmware for verification
                if ((fwVersion >= 0) && (fileVersion >= 0) && (fileVersion > fwVersion)) {
                    LogUtils.e(TAG, "kLink Firmware Update Available");
                    powerOffAllPorts();
                    return true;
                } else {
                    LogUtils.i(TAG, "kLink Firmware is already up-to-date");
                }
            }
        }
        return false;

    }

    /**
     * Update appropriate firmware image and activates it.
     *
     */
    public void applyFirmwareUpdate() {
        KLinkFirmwareStatus aFwStatus = getKLinkFirmwareStatus();
        int fileVersion, fwVersion;

        fileVersion = mKLinkFirmwareImage.getRev();
        fileVersion += (HUNDRED_PLACE * mKLinkFirmwareImage.getMinorVer());
        fileVersion += (TEN_THOUSAND_PLACE * mKLinkFirmwareImage.getMajorVer());
        LogUtils.d(TAG, "fileVersion Value" + fileVersion);

        if (aFwStatus != null && mKLinkFirmwareImage.isValid()) {
            if (aFwStatus.mActiveImage == 1) {
                fwVersion = aFwStatus.mFw1Rev;
                fwVersion += (HUNDRED_PLACE * aFwStatus.mFw1MinorVer);
                fwVersion += (TEN_THOUSAND_PLACE * aFwStatus.mFw1MajorVer);
                LogUtils.d(TAG, "fw1Version Value" + fwVersion);

                if ((fwVersion >= 0) && (fileVersion >= 0) && (fileVersion > fwVersion)) {
                    if (mKLinkFirmwareImage.writeFirmwareImage(KLinkFirmwareImage.Firmware.FIRMWARE2, getUsbDeviceConnection()) &&
                            activateFirmware(KLinkFirmwareImage.Firmware.FIRMWARE2)) {
                        LogUtils.i(TAG, "Successfully updated Firmware2 to  " + mKLinkFirmwareImage.getVersionString());
                    } else {
                        LogUtils.e(TAG, "Failed to update Firmware2");
                    }
                    LogUtils.e(TAG, "Firmware Update Available");
                } else {
                    LogUtils.i(TAG, "kLink Firmware is already up-to-date");
                }
            } else if (aFwStatus.mActiveImage == 2) {
                fwVersion = aFwStatus.mFw2Rev;
                fwVersion += (HUNDRED_PLACE * aFwStatus.mFw2MinorVer);
                fwVersion += (TEN_THOUSAND_PLACE * aFwStatus.mFw2MajorVer);
                LogUtils.d(TAG, "fw2Version Value" + fwVersion);

                if ((fwVersion >= 0) && (fileVersion >= 0) && (fileVersion > fwVersion)) {
                    if (mKLinkFirmwareImage.writeFirmwareImage(KLinkFirmwareImage.Firmware.FIRMWARE1, getUsbDeviceConnection()) &&
                            activateFirmware(KLinkFirmwareImage.Firmware.FIRMWARE1)) {
                        LogUtils.i(TAG, "Successfully updated Firmware1 to  " + mKLinkFirmwareImage.getVersionString());
                    } else {
                        LogUtils.e(TAG, "Failed to update Firmware1");
                    }
                    LogUtils.e(TAG, "kLink Firmware Update Available");
                } else {
                    LogUtils.i(TAG, "kLink Firmware is already up-to-date");
                }
            }
        }
    }

    /**
     * Activates a particular firmware image
     *
     * @param aFirmware The firmware image to activate
     *
     * @return Whether activation was successful
     *
     * Note that the device will be reset on successful activation
     */
    private boolean activateFirmware(KLinkFirmwareImage.Firmware aFirmware)
    {
        // Get the current firmware status
        KLinkFirmwareStatus status = getKLinkFirmwareStatus();
        if (status == null) return false;

        // Check that the firmware being activated is reported as being valid
        if (aFirmware == KLinkFirmwareImage.Firmware.FIRMWARE1 && !status.mFw1Valid ||
                aFirmware == KLinkFirmwareImage.Firmware.FIRMWARE2 && !status.mFw2Valid)
        {
            LogUtils.e(TAG, "Unable to activate firmware " + aFirmware + " as it is invalid");
            return false;
        }

        // Send the request to activate the software
        int index = aFirmware == KLinkFirmwareImage.Firmware.FIRMWARE1 ? 1 : 2;
        int cmd_result = getUsbDeviceConnection().controlTransfer(KLINK_REQUEST_TYPE_OUT, KLINK_REQUEST,
                KLINK_CMD_ACTIVATE_FW, index, null, 0,
                KLINK_TRANSACTION_TIMEOUT_MS);

        if (cmd_result == 0)
        {
            LogUtils.i(TAG, "Activate firmware successful.");
            resetKLink();
            return true;
        }
        return false;
    }

    /**
     * Stop the probe detection progress
     */
    public void stopProbeDetectionProgress() {
        mKLinkListener.stopProgress();
    }

    /**
     * Is the Kosmos-Link is supported
     * @param partNumber Kosmos-Link part number
     * @return {@code boolean} is the Kosmos-Link is supported
     */
    public boolean isSupported(String partNumber) {
        return partNumber.equals(K_LINK_PART_NUMBER);
    }

}
