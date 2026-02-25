
/*
 * Copyright (C) 2016 EchoNous, Inc.
 */

package com.echonous.hardware.ultrasound;

import android.annotation.SuppressLint;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.hardware.usb.UsbConfiguration;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.view.Surface;
import android.widget.Toast;

import com.echonous.ai.MLManager;
import com.echonous.hardware.service.UltrasoundServiceProxy;
import com.echonous.kLink.KLinkUsbDevice;
import com.echonous.thorsdk.BuildConfig;
import com.echonous.thorsdk.R;
import com.echonous.usb.AndroidUsbDevice;
import com.echonous.util.LogUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;

/**
 * <p>A system service for detecting and connecting to a {@link Dau}
 * (Data Acquisition Unit) / ultrasound probe.</p> 
 *  
 * <p>On a Thor device, obtain an instance of this service by calling {@link
 * android.content.Context#getSystemService(String) 
 * Context.getSystemService()}.</p> 
 *  
 * <pre>UltrasoundManager manager = (UltrasoundManager) getSystemService(Context.ULTRASOUND_SERVICE);</pre> 
 *  
 * <p>On platforms that do not have the service (ex: commercial Android 
 * tablets), it is possible to instantiate a manager directly.  This allows for 
 * use of USB based probes and for emulation purposes. Under emulation, {@link
 * UltrasoundManager#isDauConnected()} will always return {@code true} and the
 * created Dau instance will display a simple ramp pattern as ultrasound 
 * images.</p> 
 * 
 * <pre>UltrasoundManager manager = new UltrasoundManager(context, true);</pre> 
 */
public final class UltrasoundManager {
    private static final String TAG = "UltrasoundManager";

    private final Context mContext;
    private final UltrasoundServiceProxy mService;
    private final DauCommMode mMode;
    private Dau mDau = null;
    private String mUpsPath;
    private Puck mPuck = null;
    private UltrasoundViewer mViewer = null;
    private UltrasoundPlayer mPlayer = null;
    private UltrasoundRecorder mRecorder = null;
    private UltrasoundEncoder mEncoder = null;
    private Object mCineProducer = null;
    private boolean mIsClosed = false;
    private PendingIntent mPermissionIntent;
    private boolean mHavePermission = false;
    private boolean mCheckingPermission = false;
    private boolean mIsPermissionChecked = false;
    private BroadcastReceiver mDauReceiver = null;
    private SvcCallback mSvcCallback = new SvcCallback();
    private UsbManager mUsbManager;
    private MLManager mMLManager = null;

    public static Boolean hasFirmwareUpdate = false;

    //
    // Usb Related
    //
    public static final int mVid = 0x1dbf;
    private static final int DFU_CLASS = 254;
    private static final int APP_CLASS = 255;
    private static final int INTF_DEFINE_CLASS = 0;

    public static final int PROBE_TORSO_ONE = 0x4;
    public static final int PROBE_TORSO_THREE = 0x3;
    public static final int PROBE_LEXSA = 0x5;

    public static final int TORSO_ONE_PID = 0x0101;
    public static final int TORSO_THREE_PID = 0x0100;
    public static final int LEXSA_PID = 0x0102;

    public static ArrayList<UsbProbeData> mUsbProbeList = new ArrayList();
    
    private static final String SP_NAME = "PackageInfo";
    private static final String SP_APP_VERSION = "CurrentAppVersion";
    private static final String[] mDatabaseFileNameArray = {"thor.db", "l38.db"};
    public static final Boolean ENABLE_PROBE_TYPE_CHANGE = false;

    public static final int IOS_USB_DEVICE_CLASS = 239;
    public static final int IOS_USB_CONFIGURATION_INDEX = 1;
    public static final int SET_USB_CONFIGURATION_DELAY = 1000;
    public static final int FLAG_MUTABLE = 33554432;

    /**
     * This USB Device must be always App Class object
     */
    private UsbDevice mUsbDevice = null;
    private UsbDevice mBootloaderUsbDevice = null;

    /**
     * Sent as broadcast Intent when the Dau is connected to the system in bootloader mode.
     */
    public static final String ACTION_DAU_BOOT_CONNECTED =
            "echonous.intent.action.ACTION_DAU_BOOT_CONNECTED";

    /**
     * Sent as broadcast Intent when the Dau is connected to the system.
     */
    public static final String ACTION_DAU_CONNECTED =
            "echonous.intent.action.ACTION_DAU_CONNECTED";

    /**
     * Sent as broadcast Intent when the Dau is disconnected from the system.
     */
    public static final String ACTION_DAU_DISCONNECTED =
            "echonous.intent.action.ACTION_DAU_DISCONNECTED";

    /**
     * Sent as extra data for ACTION_DAU_CONNECTED and ACTION_DAU_DISCONNECTED. This
     * extra value will be a member of DauCommMode (Either Proprietary or Usb).
     */
    public static final String EXTRA_DAU_COMM_MODE =
            "echonous.intent.extra.EXTRA_DAU_COMM_MODE";

    /**
     * Sent as extra data for ACTION_DAU_CONNECTED. This extra value will specify
     * DAU PId.
     */
    public static final String EXTRA_PROBE_PID = "echonous.intent.extra.EXTRA_PROBE_PID";

    private static final String ACTION_USB_PERMISSION
            = "com.echonous.hardware.ultrasound.USB_PERMISSION";

    public static final String ACTION_DAU_PERMISSION_NOT_GRANTED =
            "echonous.intent.action.ACTION_DAU_PERMISSION_NOT_GRANTED";

    /** The object that manages the kLink device */
    private KLinkUsbDevice mKLinkUsbDevice = null;

    /** The list of registered USB monitors */
    private final List<UsbMonitor> mMonitors = new ArrayList<>();

    /**
     * Whether device has been processed. A device is regarded as processed if
     * we have requested permission for it or the permission is already granted.
     */
    private final Set<Integer> mProcessedDevices = new HashSet<>();
    private final Set<Integer> mApprovedProductIdList = new HashSet<>();
    private final Set<Integer> mRejectedProductIdList = new HashSet<>();

    private int mActiveProbePid = TORSO_ONE_PID;

    int dauSafetyTestSelectedOption = 0;
    float dauThresholdTemperatureForTest = 0.0F;

    public void setDauSafetyTestOption(int selectedOption, float temperature) {
        dauSafetyTestSelectedOption = selectedOption;
        if (selectedOption == 1) {
            dauThresholdTemperatureForTest = temperature;
        } else {
            dauThresholdTemperatureForTest = 0.0F;
        }
    }

    public float getDauThresholdTemperatureForTest() {
        return dauThresholdTemperatureForTest;
    }

    public int getSafetTestSelectedOption() {
        return dauSafetyTestSelectedOption;
    }

    /**
     * The UltrasoundManager can provide access to one of several ultrasound probe
     * interfaces.
     */
    public enum DauCommMode {
        /**
         * The Thor tablet has a proprietary communication link to the ultrasound probe
         * (Dau).
         */
        Proprietary,

        /**
         * Commercial Android tablets only have USB ports.  Select this option to use
         * USB based ultrasound probes.
         */
        Usb,

        /**
         * Emulation mode when no ultrasound probe is available for developement.
         * Currently no longer supported.
         */
        Emulation,

        /**
         * Proprietary and Usb probes can be used.
         */
        Combined
    }

    /**
     * Main Constructor.
     *
     * @param context Context providing access to Android system services.
     * @param mode    Specifies the operational mode: Proprietary, Usb, or Emulation.
     */
    public UltrasoundManager(Context context, DauCommMode mode) {
        LogUtils.d(TAG, "UltrasoundManager.constructor() mode = " + mode.toString());
        mContext = context;
        mMode = mode;
        mUsbManager = (UsbManager) mContext.getSystemService(Context.USB_SERVICE);
        createProbeList();
        if (DauCommMode.Proprietary == mMode) {
            UltrasoundServiceProxy service;

            try {
                service = new UltrasoundServiceProxy();
                if (null == service) {
                    LogUtils.d(TAG, "Unable to locate UltrasoundService");
                } else {
                    service.setCallback(mSvcCallback);
                }
            } catch (UltrasoundServiceProxy.ServiceException ex) {
                LogUtils.e(TAG, "Unable to get instance of Ultrasound Service");
                service = null;
            }

            mService = service;
        } else {
            mService = null;
        }
        handleChangeInSoftwareVersion();
        openManagerNative(context, context.getAssets(), context.getDataDir().getAbsolutePath());
        registerReceiver();

        // Create the kLink device and register it with USB monitor for USB device
        // attach and detach event.
        mKLinkUsbDevice = new KLinkUsbDevice(mContext);
        registerMonitor(mKLinkUsbDevice);

        mMLManager = new MLManager(context);
    }

    /**
     * create probe list with supported probe
     * ====================
     * Supported USB probe
     * ====================
     * Torso One
     * Torso Three
     */
    private static void createProbeList() {
        mUsbProbeList = new ArrayList();
        UsbProbeData torsoOne = new UsbProbeData();
        torsoOne.setPid(TORSO_ONE_PID);
        torsoOne.setVid(mVid);
        torsoOne.setProbeType(PROBE_TORSO_ONE);

        // TODO : Need to conform the USB Torso Three support we required not
        UsbProbeData torsoThree = new UsbProbeData();
        torsoThree.setPid(TORSO_THREE_PID);
        torsoThree.setVid(mVid);
        torsoThree.setProbeType(PROBE_TORSO_THREE);

        UsbProbeData lexsa = new UsbProbeData();
        lexsa.setPid(LEXSA_PID);
        lexsa.setVid(mVid);
        lexsa.setProbeType(PROBE_LEXSA);

        mUsbProbeList.add(torsoOne);
        mUsbProbeList.add(torsoThree);
        mUsbProbeList.add(lexsa);
    }

    public static ArrayList<UsbProbeData> getUsbProbeList() {
        if (mUsbProbeList.size() <= 0) {
            createProbeList();
        }
        return mUsbProbeList;
    }

    /**
     * check the is supported probe
     *
     * @param vId
     * @param pId
     * @return if supported then return UsbProbeData otherwise null
     */
    public static UsbProbeData isInSupportedProbes(ArrayList<UsbProbeData> mUsbProbeList,int vId, int pId) {
        UsbProbeData mUsbData = null;
        if (mUsbProbeList != null) {
            for (int i = 0; i < mUsbProbeList.size(); i++) {
                UsbProbeData probeData = mUsbProbeList.get(i);
                if (probeData.getVid() == vId && probeData.getPid() == pId) {
                    mUsbData = probeData;
                    break;
                }
            }
        }
        return mUsbData;
    }

    public void handleChangeInSoftwareVersion() {

        //Fetching Older Version of App From Shared Preferences
        SharedPreferences packageInfo = mContext.getSharedPreferences(SP_NAME, Context.MODE_PRIVATE);
        String currentAppVersion = packageInfo.getString(SP_APP_VERSION, null);

        //Fetching Newer Version Of App From PackageInfo
        String newAppVersion = "";
        try {
            PackageInfo pInfo = mContext.getPackageManager().getPackageInfo(mContext.getPackageName(), 0);
            newAppVersion = ""+pInfo.versionCode;
        } catch (PackageManager.NameNotFoundException e) {
            LogUtils.e(TAG, "handleChangeInSoftwareVersion() NameNotFoundException : " +  e.getMessage());
        }

        //If Fresh Installation, Upgrade CurrentAppVersion in shared Preferences
        if (currentAppVersion == null) {
            packageInfo.edit().putString(SP_APP_VERSION, newAppVersion).commit();
            return;
        }

        LogUtils.i(TAG, "CurrentAppVersion:" + currentAppVersion + "NewerAppVersion:" + newAppVersion);
        //If Any Change in App Version, Delete existing extracted database
        if (!currentAppVersion.equals(newAppVersion)) {
            for (String mDatabaseFileName: mDatabaseFileNameArray) {
                String databasePath = mContext.getApplicationInfo().dataDir + "/" + mDatabaseFileName;
                File databaseFile = new File(databasePath);
                if (databaseFile.exists()) {
                    if (databaseFile.delete()) {
                        LogUtils.i(TAG, "File Deleted :" + databasePath);
                        //Updating App Version in Shared Preferences
                        packageInfo.edit().putString(SP_APP_VERSION, newAppVersion).commit();
                    } else {
                        LogUtils.e(TAG, "File not Deleted :" + databasePath);
                    }
                } else {
                    LogUtils.e(TAG, "File does not exist :" + databasePath);
                }
            }
        }
    }

    /**
     * Set USB Configuration.
     * @param device
     */
    public void setUsbConfiguration(UsbDevice device) {
        int deviceClass = device.getDeviceClass();
        LogUtils.d(TAG,"setConfiguration deviceClass : " +deviceClass);
        if(deviceClass == IOS_USB_DEVICE_CLASS) {
            try {
                UsbConfiguration usbConfig;
                Thread.sleep(SET_USB_CONFIGURATION_DELAY);
                // 0 -> For IOS configuration index
                // 1 -> For Android USB configuration index
                usbConfig = device.getConfiguration(IOS_USB_CONFIGURATION_INDEX);

                UsbDeviceConnection usbDeviceConnection = mUsbManager.openDevice(device);
                if (usbDeviceConnection == null) {
                    LogUtils.e(TAG, "setConfiguration failed usbDeviceConnection is null");
                    return;
                }
                int usbFd = usbDeviceConnection.getFileDescriptor();
                // Set Android Configuration from Native side
                nativeSetUsbConfiguration(usbFd);

                // Required time to set configuration
                Thread.sleep(SET_USB_CONFIGURATION_DELAY);
                LogUtils.d(TAG, "setConfiguration call done");
            } catch (InterruptedException e) {
                LogUtils.e(TAG, "setConfiguration call failed" + e.getMessage());
            }
        }
    }

    /**
     * Release resources.  Clients should call this method when finished with the
     * Manager.
     */
    public void cleanup() {
        LogUtils.d(TAG, "UltrasoundManager.cleanup()");
        mIsPermissionChecked = false;
        LogUtils.v(TAG, "cleanup: mIsPermissionChecked is false");
        if (!mIsClosed) {
            if (null != mService) {
                mService.clearCallback();
            }

            if (null != mDauReceiver) {
                mContext.unregisterReceiver(mDauReceiver);
                mDauReceiver = null;
            }

            if (null != mDau) {
                mDau.close();
                mDau = null;
            }

            if (null != mPlayer) {
                mPlayer.detachCine();
                mPlayer = null;
            }

            if (null != mViewer) {
                mViewer.releaseSurface();
                mViewer.unregisterHeartRateCallback();
                mViewer = null;
            }

            if (null != mRecorder) {
                mRecorder.cleanup();
                mRecorder = null;
            }

            if (null != mEncoder) {
                mEncoder.cleanup();
                mEncoder = null;
            }

            if (null != mPuck) {
                mPuck.close();
                mPuck = null;
            }

            mCineProducer = null;

            closeManagerNative();

            mIsClosed = true;
            LogUtils.d(TAG, "UltrasoundManager closed");
        }
    }

    /**
     * Called by Dau instance when it is closed.
     *
     * @hide
     */
    protected void releaseDau(Object dau) {
        if (dau.equals(mDau)) {
            mDau = null;
        }
    }

    /**
     * Called by Puck instance when it is closed.
     *
     * @hide
     */
    protected void releasePuck(Object puck) {
        if (puck.equals(mPuck)) {
            mPuck = null;
        }
    }

    /**
     * Get the full pathname of the UPS (Ultrasound Parameter Storage) database that
     * contains information for selecting imaging cases and workflows.
     *
     * @return The full pathname to the database
     */
    public String getUpsPath() {
        if (null == mUpsPath) {
            // Hardcode a path for now
            return mContext.getDataDir().getAbsolutePath() + "/thor.db";
        } else {
            // Return the cached (i.e. from most recently opened Dau) UPS path
            return mUpsPath;
        }
    }

    /**
     * get ups path based on probeType
     * @param probeType
     * @return ups path
     */
    public String getUpsPathProbeWise(Integer probeType) {
        Integer PROBE_LEXSA = 5;
        if(probeType.equals(PROBE_LEXSA)){
            return mContext.getDataDir().getAbsolutePath() + "/l38.db";
        }else {
            return mContext.getDataDir().getAbsolutePath() + "/thor.db";
        }
    }

    /**
     * Set UPS database as per connected probe. We need to set this path to get organ list based on active probe
     *
     * @param databaseName specifies the database name.
     */
    public void setUpsPathProbeWise(String databaseName) {
        mUpsPath = mContext.getDataDir().getAbsolutePath() + "/" + databaseName;
    }

    public boolean isDauConnected(DauCommMode mode, UsbDevice usbDevice) {

        return switch (mode) {
            case Emulation -> true;
            case Proprietary -> isPciDauConnected();
            case Usb -> isUsbDauConnected(usbDevice);
            case Combined -> isPciDauConnected() || isUsbDauConnected(usbDevice);
            default ->
                    throw new IllegalStateException("This block should not be reached - unhandled DauCommMode");
        };
    }

    private boolean isPciDauConnected() {
        boolean pciDauConnected = false;

        LogUtils.d(TAG, "Looking for Proprietary Dau");
        if (null != mService) {
            pciDauConnected = mService.isDauConnected();
        }

        return (pciDauConnected);
    }

    private boolean isUsbDauConnected(UsbDevice device) {
        if (null == mService) {
            // Check for Usb probe
            LogUtils.v(TAG, "Device permission running " + device.getProductId());
            if (mCheckingPermission || mIsPermissionChecked) {
                LogUtils.v(TAG, "isUsbDauConnected: mCheckingPermission is " + mCheckingPermission + "and mIsPermissionChecked is " + mIsPermissionChecked + " so returning");
                return false;
            }
            LogUtils.d(TAG, "OTS:: Looking for USB Dau-------> isDauConnected called");

            if (isInSupportedProbes(mUsbProbeList, device.getVendorId(), device.getProductId()) != null) {
                LogUtils.d(TAG, "Found USB Probe Product ID: " + device.getProductId());
                if (device.getInterface(0).getInterfaceClass() == DFU_CLASS) {
                    boolean isGranted = getUsbManager().hasPermission(device);
                    if (isGranted) {
                        LogUtils.d(TAG, "OTS:: USB Device Found DFU Permission Granted ");
                        mBootloaderUsbDevice = device;
                        startDFU(device);
                        LogUtils.d(TAG, "OTS:: Start DFU from isUsbDauConnected Method");
                    } else {
                        LogUtils.d(TAG, "OTS:: USB Device Found DFU Permission Request Access  ID" + device.getInterface(0).getInterfaceClass());
                        mCheckingPermission = true;
                        mHavePermission = false;
                        getUsbManager().requestPermission(device, mPermissionIntent);
                    }
                } else {
                    LogUtils.d(TAG, "OTS:: Application Image");
                    if (!getUsbManager().hasPermission(device) && !mRejectedProductIdList.contains(device.getProductId())) {
                        LogUtils.d(TAG, "OTS:: USB Device  Request Permission App Imaging for: "+ device.getProductId());
                        mCheckingPermission = true;
                        getUsbManager().requestPermission(device, mPermissionIntent);
                    } else {
                        LogUtils.d(TAG, "OTS:: USB Device  Permission Granted App ");
                        setUsbConfiguration(device);
                        mUsbDevice = device;
                        if (mApprovedProductIdList.contains(device.getProductId())) {
                            LogUtils.d(TAG, "OTS:: USB Device is already connected device");
                        } else {
                            mApprovedProductIdList.add(mUsbDevice.getProductId());
                            mKLinkUsbDevice.addProbeMap(mUsbDevice.getProductId());
                            sendDauConnectEvent(DauCommMode.Usb, mUsbDevice.getProductId());
                        }
                    }
                }
            } else {
                LogUtils.e(TAG, "Found USB Probe Product ID: " + device.getProductId() + " is Not supported");
            }
            return (null != mUsbDevice);
        } else {
            // Check for probe on proprietary interface
            boolean isConnected;
            LogUtils.d(TAG, "Looking for Proprietary Dau");
            isConnected = mService.isDauConnected();
            return isConnected;
        }
    }

    /**
     * check firmware update usb
     * @return true if firmware update available else false
     */
    public Boolean checkFirmwareUpdateUsb() {
        LogUtils.d(TAG, "checkFwUpdateUsb called");

        UsbDevice device = getActiveUsbProbeDevice();

        LogUtils.d(TAG, "Usb Device 0x" +
                Integer.toHexString(device.getVendorId()) +
                ":0x" +
                Integer.toHexString(device.getProductId())
        );

        if (isInSupportedProbes(mUsbProbeList, device.getVendorId(), device.getProductId()) != null) {
            UsbDeviceConnection usbDeviceConnection = getUsbManager().openDevice(device);
            int fdId = usbDeviceConnection.getFileDescriptor();
            boolean isUpdateAvailable = checkFwUpdateAvailableNativeUsb(fdId, false);
            int deviceClass = device.getDeviceClass();
            LogUtils.d(TAG, "isUpdateAvailable Flag in checkFirmwareUpdateUsb()" + isUpdateAvailable);
            if ((deviceClass == APP_CLASS) && isUpdateAvailable) {
                //Update available
                LogUtils.d(TAG, "Update found during App Imaging");
                rebootDau(usbDeviceConnection);
            }
            return isUpdateAvailable;
        }

        return false;
    }

    /**
     * send reboot command to dau
     * @param usbDeviceConnection
     */
    private void rebootDau(UsbDeviceConnection usbDeviceConnection){
        byte[] buffer = new byte[16];
        Arrays.fill(buffer, (byte) 0);
        buffer[0] = 14;
        LogUtils.i(TAG, "Buffer Value " + Arrays.toString(buffer));
        int result = usbDeviceConnection.controlTransfer(0x40,
                0x1,
                0,
                0, buffer,
                buffer.length,
                0);
        LogUtils.d(TAG, "Reboot value" + result);
    }

    /**
     * usb probe firmware update
     */
    public void usbProbeFwUpdate() {
        appFwUpdateNativeUsb();
    }

    public boolean setProbeInformation(Integer probeType){
        LogUtils.d(TAG,"setProbeInformation: called");

        if (mBootloaderUsbDevice == null) {
            return false;
        }
        UsbDeviceConnection usbDeviceConnection = mUsbManager.openDevice(mBootloaderUsbDevice);
        if (usbDeviceConnection == null){
            LogUtils.e(TAG, "Permission not granted because of device disconnection.");
            return false;
        }
        int fd = usbDeviceConnection.getFileDescriptor();
        LogUtils.d(TAG,"setProbeInformation: USB FD Value" + fd);
       return nativeSetProbeInformation(probeType,fd);
    }
    /**
     * usb probe firmware update
     */
    public ProbeInformation getProbeInformation() {
        ProbeInformation probeInformation = null;
        if (mUsbDevice == null) {
            return null;
        }
        UsbDeviceConnection usbDeviceConnection = getUsbManager().openDevice(mUsbDevice);
        if (usbDeviceConnection == null) {
            LogUtils.e(TAG, "Permission not granted because of device disconnection.");
            return null;
        }
        int fd = usbDeviceConnection.getFileDescriptor();
        LogUtils.d(TAG, "GetProbeInfo: USB FD Value" + fd);
        probeInformation = getProbeInformationNativeUsb(fd);
        LogUtils.d(TAG, "GetProbeInfo: USB probe type" + probeInformation.type);
        return probeInformation;
    }

    private UsbManager getUsbManager() {
        if (mUsbManager == null) {
            mUsbManager = (UsbManager) mContext.getSystemService(Context.USB_SERVICE);
        }
        return mUsbManager;
    }

    /**
     * set active probe type
     * @param probeType
     */
    public void setActiveProbe(int probeType){
        LogUtils.d(TAG, "setActiveProbe() called with: probeType = [" + probeType + "]");
        if(probeType == PROBE_TORSO_ONE){
            mActiveProbePid = TORSO_ONE_PID;
        }else if(probeType == PROBE_LEXSA){
            mActiveProbePid = LEXSA_PID;
        }
    }

    private UsbDevice getActiveUsbProbeDevice(){
        HashMap<String, UsbDevice> devices = getUsbManager().getDeviceList();
        for(UsbDevice device: devices.values()){
            if(device.getProductId() == mActiveProbePid){
                if(getUsbManager().hasPermission(device)){
                    LogUtils.d(TAG, "Found Active USB Device "+ device);
                    return device;
                }
            }
        }
        return null;
    }

    /**
     * Enumerate all the connected USB devices and notify the corresponding monitors.
     *
     */
    public void enumerateDevices()
    {
        LogUtils.d(TAG, "enumerateDevices() called");
        HashMap<String, UsbDevice> deviceMap = getUsbManager().getDeviceList();
        // Loop to check whether a device is already detached
        for (UsbMonitor monitor : mMonitors)
        {
            if (monitor.isConnected() && !deviceMap.containsValue(monitor.getCurrentUsbDevice()))
            {
                // Cannot find the device any more. Remove it from
                // the processed device list and notify the monitor
                LogUtils.e(TAG,"Cannot find the device any more. Remove it the processed device list and notify the monitor");
                mProcessedDevices.remove(monitor.getCurrentUsbDevice().getDeviceId());
                monitor.onDeviceDisconnected(monitor.getCurrentUsbDevice());
            }
        }

        // Process all the connected USB devices. Notify the monitors in case
        // there are newly attached devices
        for (UsbDevice device :  deviceMap.values())
        {
            if (!mProcessedDevices.contains(device.getDeviceId()))
            {
                deviceAttached(device);
            }
        }
    }

    /**
     * Notify a USB device is attached.
     * @param aDevice The attached device
     */
    private void deviceAttached(UsbDevice aDevice)
    {
        // Add the device to the processed devices list, since we will request
        // permission for the device if necessary.
        mProcessedDevices.add(aDevice.getDeviceId());
        LogUtils.d(TAG, "USB device attached  " + getDeviceDetailsString(aDevice));

        for (UsbMonitor monitor : mMonitors)
        {
            // We only notify monitors which are not already connected with a device and match
            // its expected USB device class
            if (!monitor.isConnected() && monitor.matchUsbDevice(aDevice))
            {
                // If we don't yet have permission for the device, request it.
                if (!getUsbManager().hasPermission(aDevice))
                {
                    LogUtils.d(TAG, "Requesting USB device permission " + getDeviceDetailsString(aDevice));
                    //Intent intent = new Intent(ACTION_USB_PERMISSION);
                    //PendingIntent permission_intent = PendingIntent.getBroadcast(mContext, 0, intent, 0);
                    getUsbManager().requestPermission(aDevice, mPermissionIntent);
                    return;
                }
                else
                {
                    // Need to call for the IOS Probe ( Greater Probe firmware version 1.1.6 )
                    setUsbConfiguration(aDevice);
                    LogUtils.d(TAG, "Processing connected device with permission " + getDeviceDetailsString(aDevice));
                    monitor.onDeviceConnected(aDevice);
                }
            }
        }
    }

    /**
     * Register a USB monitor to receive USB device connect/disconnect events
     *
     * @param aMonitor The monitor to be registered.
     */
    private void registerMonitor(UsbMonitor aMonitor)
    {
        mMonitors.add(aMonitor);
    }

    /**
     * Gets a debug String containing a summary of the details of a USB device
     *
     * @param aDevice The UsbDevice to get the summary string for
     *
     * @return Summary string of the device
     */
    private String getDeviceDetailsString(UsbDevice aDevice)
    {
        return aDevice.getDeviceName() + " PID:VID " + aDevice.getVendorId() + ":" + aDevice.getProductId();
    }

    /**
     * Gets the kLink device.
     */
    public KLinkUsbDevice getKLink()
    {
        return mKLinkUsbDevice;
    }

    //-------------------------------------------------------------------------
    /**
     * Interface for a USB event monitor. A monitor registered to the USB
     * device manager can get device connection/disconnection notifications.
     */
    public interface UsbMonitor
    {
        /**
         * Gets whether the given USB device a device that the current monitor is monitoring.
         *
         * @param aUsbDevice The device to check whether it belongs the monitor
         *
         * @return Whether the given USB device belongs to the monitor.
         */
        boolean matchUsbDevice(UsbDevice aUsbDevice);

        /**
         * Check whether a USB device is currently connected to the USB monitor.
         *
         * @return Whether a USB device has been connected to the USB monitor.
         */
        boolean isConnected();

        /**
         * Notification for when a matching USB device is connected.
         *
         * @param aUsbDevice The connected device
         */
        void onDeviceConnected(UsbDevice aUsbDevice);

        /**
         * Notification for when a monitored USB device is disconnected.
         *
         * @param aUsbDevice The disconnected device
         */
        void onDeviceDisconnected(UsbDevice aUsbDevice);

        /**
         * Gets the currently connected USB device
         */
        UsbDevice getCurrentUsbDevice();

        AndroidUsbDevice getCurrentAndroidUsbDevice();

        boolean isActive();
    }


    /**
     * start DFU
     * @param usbDevice
     */
    private void startDFU(UsbDevice usbDevice) {
        boolean isUpdateAvailable = false;

        LogUtils.d(TAG,"OTS:: startDFU Called");
        if(BuildConfig.DEBUG && ENABLE_PROBE_TYPE_CHANGE){
            Toast.makeText(mContext, mContext.getString(R.string.device_in_bootloader_mode),Toast.LENGTH_LONG).show();
        }else {
            UsbDeviceConnection usbDeviceConnection = mUsbManager.openDevice(usbDevice);
            if (usbDeviceConnection == null) {
                LogUtils.e(TAG, "startDFU: Permission not granted because of device disconnection.");
                return;
            }
            int fd = usbDeviceConnection.getFileDescriptor();
            // ===========================================================================
            // TODO : Later on after 2 or 3 major releases we can remove this code
            ProbeInformation mProbeInformation = getProbeInformationNativeUsb(fd);
            if(mProbeInformation.type == 0x4268){
                LogUtils.d(TAG, "startDFU:: Found old version of torso one probe");
                setProbeInformation(PROBE_TORSO_ONE);
                return;
            }
            // ===========================================================================
            int deviceClass = usbDevice.getDeviceClass();
            isUpdateAvailable = checkFwUpdateAvailableNativeUsb(fd, true);
            LogUtils.d(TAG,"isUpdateAvailable Flag in startDFU" + isUpdateAvailable);
            if ((deviceClass == APP_CLASS) && isUpdateAvailable) {
                //Update available
                LogUtils.d(TAG, "Update found during App Imaging");
                rebootDau(usbDeviceConnection);
            }
            else if((deviceClass == INTF_DEFINE_CLASS) && isUpdateAvailable) {
                LogUtils.d(TAG, "OTS:: Update Available");
                sendDauConnectEvent(DauCommMode.Usb,usbDevice.getProductId());
            }
            else {
                LogUtils.i(TAG, "OTS:: startDFU Load Imaging App ------------>");
                bootAppImgNative(fd);
            }
        }
    }

    /**
     * send dau connect event
     * @param mode mode of the USB device
     * @param probePid Probe part ID
     */
    private void sendDauConnectEvent(DauCommMode mode, int probePid) {
        Intent connectIntent = new Intent(ACTION_DAU_CONNECTED);
        connectIntent.putExtra(EXTRA_DAU_COMM_MODE, mode);
        connectIntent.putExtra(EXTRA_PROBE_PID,probePid);
        connectIntent.setPackage(mContext.getPackageName());
        mContext.sendBroadcast(connectIntent);
        LogUtils.e(TAG, "ACTION_DAU_CONNECTED: event called probe id "+ probePid);
    }

    /**
     * <p>Creates an instance of a Dau.  This method will not succeed if a Dau is not
     * connected.  Will return null if no Dau exists or an error occurred.</p>
     *
     * <p>The Dau will draw the ultrasound imaging stream onto the Surface
     * provided.</p>
     *
     * <p>If an emulated instance of the manager is used, then the returned Dau will
     * draw a canned set of images onto the Surface.</p>
     *
     * @return The newly created Dau.
     * @throws DauException If it fails to initialize the Dau object.
     */
    @Deprecated
    public Dau openDau() throws DauException {
        if (null != mDau) {
            LogUtils.e(TAG, "A Dau instance already exists!");
            throw new DauException(ThorError.TER_UMGR_DAU_EXISTS);
        }

        if (DauCommMode.Emulation == mMode) {
            DauContext dauContext = new DauContext();
            DauEmulated newDau = new DauEmulated(this);

            dauContext.mAndroidContext = mContext;

            newDau.open(dauContext);
            mDau = newDau;
        } else {
            LogUtils.d(TAG, "Creating instance of DauImpl");

            DauContext dauContext = new DauContext();
            UsbDeviceConnection usbConnection = null;

            if (null == mService) {
                // Create a Usb Context
                LogUtils.i(TAG, "openDau() Calling is Connected");
                if (null == mUsbDevice) { // && !isDauConnected(mMode)) {
                    LogUtils.e(TAG, "Probe is not connected");
                    // BUGBUG: Need to throw exception here
                }

                UsbManager manager = (UsbManager) mContext.getSystemService(Context.USB_SERVICE);

                if (mUsbDevice != null) {
                    usbConnection = manager.openDevice(mUsbDevice);
                }
                dauContext.mIsUsbContext = true;
                if (usbConnection != null) {
                    dauContext.mNativeUsbFd = usbConnection.getFileDescriptor();
                }
            }

            dauContext.mAndroidContext = mContext;

            if (null == dauContext) {
                LogUtils.e(TAG, "Failed to get DauContext");
                throw new DauException(ThorError.TER_UMGR_CONTEXT);
            } else {
                mDau = new DauImpl(this);
                mDau.setDebugFlag(BuildConfig.DEBUG);
                mDau.setDauSafetyFeatureTestOption(dauSafetyTestSelectedOption, dauThresholdTemperatureForTest);
                ThorError errorRet = mDau.open(dauContext);

                if (!errorRet.isOK()) {
                    throw new DauException(errorRet);
                }

                mUpsPath = mDau.getUpsPath();
            }
        }
        return mDau;
    }

    public Dau openDau(DauCommMode mode) throws DauException {
        DauContext dauContext = null;

        if (null != mDau) {
            LogUtils.e(TAG, "A Dau instance already exists!");
            throw new DauException(ThorError.TER_UMGR_DAU_EXISTS);
        }

        switch (mode) {
            case Emulation:
                dauContext = new DauContext();
                DauEmulated newDau = new DauEmulated(this);

                dauContext.mAndroidContext = mContext;

                newDau.open(dauContext);
                mDau = newDau;
                break;

            case Proprietary:
                if (null == mService) {
                    LogUtils.e(TAG, "Ultrasound Service doesn't exist!");
                    throw new DauException(ThorError.TER_UMGR_SVC_ACCESS);
                }

                if (!isPciDauConnected())
                {
                    LogUtils.e(TAG, "Proprietary Dau is not connected!");
                    throw new DauException(ThorError.TER_UMGR_DAU_ATTACHED);
                }

                dauContext = new DauContext();
                break;

            case Usb:
                UsbDevice usbDevice = getActiveUsbProbeDevice();

                if ( null == usbDevice) {
                    LogUtils.e(TAG, "USB Dau is not connected!");
                    throw new DauException(ThorError.TER_UMGR_DAU_ATTACHED);
                }

                UsbDeviceConnection usbConnection = null;

                if (null == getUsbManager())
                {
                    LogUtils.e(TAG, "Couldn't open the USB Manager!");
                    throw new DauException(ThorError.TER_UMGR_USB_MANAGER);
                }

                usbConnection = getUsbManager().openDevice(usbDevice);
                if (null == usbConnection) {
                    LogUtils.e(TAG, "Couldn't open the USB device!");
                    throw new DauException(ThorError.TER_UMGR_USB_DEVICE);
                }
                else {
                    dauContext = new DauContext();
                    dauContext.mIsUsbContext = true;
                    dauContext.mNativeUsbFd = usbConnection.getFileDescriptor();
                }
                break;

            default:
                throw new IllegalStateException("This block should not be reached - unsupported DauCommMode");
        }

        if (null != dauContext && null == mDau)
        {
            dauContext.mAndroidContext = mContext;
            mDau = new DauImpl(this);
            mDau.setDebugFlag(BuildConfig.DEBUG);
            mDau.setDauSafetyFeatureTestOption(dauSafetyTestSelectedOption, dauThresholdTemperatureForTest);
            ThorError errorRet = mDau.open(dauContext);

            if (!errorRet.isOK()) {
                //Added null check to avoid null pointer exception
                if(mDau!= null){
                    mDau.close();
                    mDau = null;
                }
                throw new DauException(errorRet);
            }

            mUpsPath = mDau.getUpsPath();
        }
        mDau.setDebugFlag(BuildConfig.DEBUG);
        return mDau;
    }

    /**
     * Create an instance of a Thor Puck.  This method will not succeed if an 
     * instance already exists, or if called on a non-Thor platform. 
     * 
     * @return Puck The newly created Puck
     * @exception DauException If it fails to initialize the Puck object. 
     */
    public Puck openPuck() throws DauException {
        if (null != mPuck) {
            LogUtils.e(TAG, "A Puck instance already exists!");
            throw new DauException(ThorError.TER_UMGR_PUCK_EXISTS);
        }

        if (null == mService) {
            LogUtils.e(TAG, "Puck is only available on Thor tablet.");
            throw new DauException(ThorError.TER_UMGR_THOR_ONLY);
        }

        mPuck = new Puck(this);
        // Ignoring return code and returning Puck to caller to allow for
        // firmware updates.

        return mPuck;
    }

    /**
     * Create an emulated instance of a Dau that will draw a canned set of images 
     * onto the Surface provided. This ensures that an emulated Dau is used even if 
     * the platform supports the Ultrasound service. 
     * 
     * @param surface The Surface that will act as a target for drawing canned 
     *                ultrasound images.
     * @return Dau the newly created emulated Dau
     * @hide
     */
    public Dau openDauEmulated(Surface surface) {
        LogUtils.d(TAG, "Creating instance of DauEmulated");
        DauEmulated newDau = new DauEmulated(this);
        newDau.open(null);
        return newDau;
    }

    /**
     * Get an instance of the UltrasoundViewer.
     * 
     * 
     * @return UltrasoundViewer 
     */
    public UltrasoundViewer getUltrasoundViewer() {
        if (null == mViewer) {
            mViewer = new UltrasoundViewer(this);
        }

        return mViewer;
    }

    /**
     * Get an instance of the UltrasoundPlayer.
     * 
     * 
     * @return UltrasoundPlayer 
     */
    public UltrasoundPlayer getUltrasoundPlayer() {
        if (null == mPlayer) {
            mPlayer = new UltrasoundPlayer(this);
        }

        return mPlayer;
    }

    /**
     * check whether Gale is installed or not.
     *
     * @return boolean
     */
    private boolean checkGaleIsInstalled() {
        try {
            String GALE = "com.nineteenlabs.gale";
            mContext.getPackageManager().getPackageInfo(GALE,0);
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    /**
     * Get an instance of the UltrasoundRecorder.
     * 
     * 
     * @return UltrasoundRecorder 
     */
    public UltrasoundRecorder getUltrasoundRecorder() {
        if (null == mRecorder) {
            mRecorder = new UltrasoundRecorder(this);
        }

        return mRecorder;
    }

    /**
     * Get an instance of the UltrasoundEncoder.
     * 
     * 
     * @return UltrasoundEncoder 
     */
    public UltrasoundEncoder getUltrasoundEncoder() {
        if (null == mEncoder) {
            mEncoder = new UltrasoundEncoder();
        }

        return mEncoder;
    }

    /**
     * Determine if the connected charger is approved for ultrasound imaging.
     * 
     * 
     * @return {@code true} if charger is approved
     */
    public boolean isChargerApproved() {
        // OTS-871: Allow all possible charging sources. Otherwise Zendure kLink
        //          could not be used for imaging.
        //For OTS Linear release, isApproved flag will be false
        boolean isApproved = false;

        if (null != mService) {
            isApproved = mService.isChargerApproved();
        }

        LogUtils.d(TAG, "Charger is " + (isApproved ? "approved" : "not approved"));

        return isApproved;
    }

    //-------------------------------------------------------------------------
    protected boolean attachCineProducer(Object producer) {
        boolean attached = false;

        if (null == mCineProducer) {
            mCineProducer = producer;
            attached = true;
            LogUtils.d(TAG, "Attached Cine Producer: " + producer.toString());
        } else {
            LogUtils.d(TAG, "A cine producer is already attached.  Trying to attach: " + producer.toString());
        }

        return attached;
    }

    //-------------------------------------------------------------------------
    protected void detachCineProducer(Object producer) {
        if (null != mCineProducer && mCineProducer.equals(producer)) {
            mCineProducer = null;
            LogUtils.d(TAG, "Detached Cine Producer: " + producer);
        }
    }

    //-------------------------------------------------------------------------
    protected boolean isPlayerAttached() {
        boolean isAttached = false;

        if (null != mPlayer && mPlayer.isCineAttached()) {
            isAttached = true;
        }

        return isAttached;
    }

    /**
     * Enable the Inference Engine.
     * This is a temporary method of testing the Inference Engine until there exists 
     * a proper interface and/or workflow. 
     * 
     * 
     * @param enable <code>True</code> to enable.
     */
    public void enableInferenceEngine(boolean enable) {
        enableIeNative(enable);
    }

    //-------------------------------------------------------------------------
    private class DauReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            LogUtils.d(TAG, "Probe::  USB Device action "+ action);
            UsbDevice usbDevice;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
                usbDevice = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE, UsbDevice.class);
            } else {
                usbDevice = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
            }
            if (usbDevice != null) {
                LogUtils.d(TAG, "USB product id is " + usbDevice.getProductId());
            }
            if (ACTION_USB_PERMISSION.equals(action)) {
                mIsPermissionChecked = true;
                LogUtils.v(TAG, "onReceive: mIsPermissionChecked is true");
                LogUtils.i(TAG, "OTS:: USB Device Found 5 ACTION_USB_PERMISSION");
                mHavePermission = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED,
                        false);

                if (usbDevice != null && isInSupportedProbes(mUsbProbeList,usbDevice.getVendorId(), usbDevice.getProductId()) != null
                        && mHavePermission && mService == null) {
                    if (usbDevice.getInterface(0).getInterfaceClass() == DFU_CLASS) {
                        LogUtils.i(TAG, "OTS:: USB Device Found 5 DFU Start DFU after DFU PERMISSION Grated");
                        mBootloaderUsbDevice = usbDevice;
                        LogUtils.i(TAG,"OTS:: USB Device Found 5 DFU Start DFU after DFU PERMISSION Grated");
                        startDFU(usbDevice);
                    }
                } else if (usbDevice != null) {
                    LogUtils.e(TAG, "OTS:: USB Device Found 5  " + usbDevice.getProductId() + " is Not supported");
                }
                LogUtils.d(TAG, "OTS:: USB Device Found 7 Permission " + mHavePermission);
                mCheckingPermission = false;
                if (usbDevice != null && mHavePermission) {

                    deviceAttached(usbDevice);
                    /*
                     * It's App class with Permission granted so start the Home screen organs
                     */
                    if (usbDevice.getProductId() != KLinkUsbDevice.K_LINK_USB_PRODUCT_ID
                            && usbDevice.getInterface(0).getInterfaceClass() == APP_CLASS
                            && !mRejectedProductIdList.contains(usbDevice.getProductId())
                            && !mApprovedProductIdList.contains(usbDevice.getProductId())) {
                        mUsbDevice = usbDevice;
                        sendDauConnectEvent(DauCommMode.Usb, mUsbDevice.getProductId());
                    }
                    mApprovedProductIdList.add(usbDevice.getProductId());

                } else {
                    Intent connectIntent = new Intent(ACTION_DAU_PERMISSION_NOT_GRANTED);
                    connectIntent.setPackage(mContext.getPackageName());
                    context.sendBroadcast(connectIntent);
                    if (usbDevice != null && isInSupportedProbes(mUsbProbeList, usbDevice.getVendorId(), usbDevice.getProductId()) != null) {
                        mRejectedProductIdList.add(usbDevice.getProductId());
                    }
                }

                Collection<UsbDevice> deviceArray = getUsbManager().getDeviceList().values();

                if (deviceArray.size() > 1) {
                    for (UsbDevice device : deviceArray) {
                        // whenever there are more than 1 probe connected to the device at
                        // the beginning of the application, probe permission should be
                        // handled 1 after the other
                        if (usbDevice != null
                                && device.getProductId() != usbDevice.getProductId()
                                && isInSupportedProbes(mUsbProbeList, device.getVendorId(), device.getProductId()) != null
                                && mProcessedDevices.contains(device.getDeviceId())
                                && !mRejectedProductIdList.contains(device.getProductId())
                                && !mApprovedProductIdList.contains(device.getProductId())) {
                            getUsbManager().requestPermission(device, mPermissionIntent);
                            mHavePermission = false;
                            break;
                        }
                    }
                }

            } else if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
                /*
                 * Check Attached device has permission granted
                 */
                mIsPermissionChecked = false;

                if (usbDevice == null) {
                    // If the UsbDevice is not found, return
                    return;
                }

                LogUtils.v(TAG, "onReceive: ACTION_USB_DEVICE_ATTACHED mIsPermissionChecked is false");
                mHavePermission = getUsbManager().hasPermission(usbDevice);

                if (isInSupportedProbes(mUsbProbeList,usbDevice.getVendorId(), usbDevice.getProductId()) != null) {
                    LogUtils.d(TAG, "OTS:: USB Dau attached ID " + usbDevice.getInterface(0).getInterfaceClass());
                    mKLinkUsbDevice.addProbeMap(usbDevice.getProductId());
                    mKLinkUsbDevice.stopProbeDetectionProgress();
                    /// When Kosmos-Link is connected ask for the Probe permission.
                    if(mKLinkUsbDevice.isConnected()){
                        if(!mHavePermission){
                            //Not need to ask USB Permission Manually
                            //getUsbManager().requestPermission(usbDevice, mPermissionIntent);
                            LogUtils.d(TAG, "OTS:: Asking Probe permission from code");
                        }
                    }
                    mRejectedProductIdList.remove(usbDevice.getProductId());
                } else if (isKosmosLink(usbDevice.getVendorId(), usbDevice.getProductId())) {
                    // This connection event is for Kosmos-Link connection
                    // Initialise Kosmos-Link connection
                    LogUtils.d(TAG, "OTS:: Kosmos-Link device found");
                    if (checkGaleIsInstalled()) {
                        enumerateDevices();
                    }
                } else {
                    LogUtils.d(TAG, "OTS:: Ignoring attached USB device - Not a Dau");
                }
            } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
                mIsPermissionChecked = false;
                LogUtils.v(TAG, "onReceive: ACTION_USB_DEVICE_DETACHED mIsPermissionChecked is false");
                if (usbDevice == null) {
                    // If the UsbDevice is not found, return
                    return;
                }

                // Notify the monitors
                for (UsbMonitor monitor : mMonitors)
                {
                    if (usbDevice.equals(monitor.getCurrentUsbDevice()))
                    {
                        LogUtils.d(TAG,"Disconnect calling : " + monitor.getCurrentUsbDevice().getProductId());
                        monitor.onDeviceDisconnected(usbDevice);

                        // Assuming if the Kosmos-Link was connected with the Device,
                        // all the Probe(s) attached are connected through it.
                        // Therefor remove all the Probe(s) at once.
                        mProcessedDevices.clear();
                        mApprovedProductIdList.clear();
                        mRejectedProductIdList.clear();
                        return;
                    }
                }

                // Device attached, we will allow permission request again
                mProcessedDevices.remove(usbDevice.getDeviceId());

                if (isInSupportedProbes(mUsbProbeList, usbDevice.getVendorId(), usbDevice.getProductId()) != null) {
                    LogUtils.d(TAG, "OTS:: USB Dau detached");
                    Intent connectIntent = new Intent(ACTION_DAU_DISCONNECTED);
                    connectIntent.putExtra(EXTRA_DAU_COMM_MODE, DauCommMode.Usb);
                    connectIntent.putExtra(EXTRA_PROBE_PID,usbDevice.getProductId());
                    connectIntent.setPackage(mContext.getPackageName());
                    context.sendBroadcast(connectIntent);
                    mApprovedProductIdList.remove(usbDevice.getProductId());
                    mRejectedProductIdList.remove(usbDevice.getProductId());
                    mKLinkUsbDevice.removeProbe(usbDevice.getProductId());
                } else {
                    LogUtils.d(TAG, "Ignoring detached USB device - Not a Dau");
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    private class SvcCallback implements UltrasoundServiceProxy.Callback {
        public void notifyDauStatusChange(UltrasoundServiceProxy.DauConnectionStatus status) {
            if (status == UltrasoundServiceProxy.DauConnectionStatus.CONNECTED) {
                LogUtils.d(TAG, "Dau is connected");
              //  fireDauConnectedEvent();
                sendDauConnectEvent(DauCommMode.Proprietary,-1);
            } else if (status == UltrasoundServiceProxy.DauConnectionStatus.DISCONNECTED) {
                LogUtils.d(TAG, "Dau is disconnected");
                Intent connectIntent = new Intent(ACTION_DAU_DISCONNECTED);
                connectIntent.putExtra(EXTRA_DAU_COMM_MODE, DauCommMode.Proprietary);
                connectIntent.setPackage(mContext.getPackageName());
                mContext.sendBroadcast(connectIntent);
            } else {
                LogUtils.d(TAG, "Received unknown value: " + status);
            }
        }
    }

    //-------------------------------------------------------------------------
    @SuppressLint({"MutableImplicitPendingIntent", "WrongConstant", "UnspecifiedRegisterReceiverFlag"})
    private void registerReceiver() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            Intent permissionIntent = new Intent(ACTION_USB_PERMISSION);
            permissionIntent.setPackage(mContext.getPackageName());
            mPermissionIntent = PendingIntent.getBroadcast(mContext,
                    0,
                    permissionIntent,
                    PendingIntent.FLAG_MUTABLE
            );
        } else {
            mPermissionIntent = PendingIntent.getBroadcast(mContext,
                    0,
                    new Intent(ACTION_USB_PERMISSION),
                    FLAG_MUTABLE
            );
        }

        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        mDauReceiver = new DauReceiver();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            mContext.registerReceiver(mDauReceiver, filter, Context.RECEIVER_EXPORTED);
        } else {
            mContext.registerReceiver(mDauReceiver, filter);
        }
    }

    public void getUltrasoundScreenImage(Bitmap bitmapImage, boolean isForThumbnail, int desiredFrameNumber) {
        mViewer.getUltrasoundScreenImage(bitmapImage, isForThumbnail, desiredFrameNumber);
    }

    //-------------------------------------------------------------------------
    public void setLocale(Locale locale) {
        LogUtils.d(TAG, "Setting locale lang=\"" + locale.getLanguage() + "\" country=\"" + locale.getCountry() + "\" variant=\"" + locale.getVariant() + "\"");
        setLocaleNative(locale.getLanguage());
    }

    //-------------------------------------------------------------------------
    public Collection<Locale> getSupportedLocales() {
        List<String> localeNames = getSupportedLocalesNative();
        List<Locale> locales = new ArrayList<Locale>();
        for (String name : localeNames) {
            locales.add(new Locale(name));
        }
        return locales;
    }

    /**
     * This method read the probe detail from connected Proprietary and USB mode
     *
     * @param mode Specifies the operational mode: Proprietary, Usb, or Emulation.
     * @return ProbeInformation contains probe property
     */
    public ProbeInformation getProbeDataFromDau(DauCommMode mode) throws DauException {
        ProbeInformation probeInformation = null;
        if(mode == DauCommMode.Usb){
            probeInformation = getProbeInformationUsb();
            if (probeInformation == null)
            {
                LogUtils.e(TAG, "Unsupported probe");
                throw new DauException(ThorError.TER_IE_UNSUPPORTED_PROBE);
            }
        } else{
            if (null == mDau) {
                try {
                    LogUtils.d(TAG, "getProbeDataFromDau()... DAU is null and open called ");
                    mDau = openDau(mode);
                    LogUtils.d(TAG,"DEBUGFLAG::BuildConfig.DEBUG" + BuildConfig.DEBUG);
                    mDau.setDebugFlag(BuildConfig.DEBUG);
                    probeInformation = mDau.getProbeInformation();
                    probeInformation.probeUps = mDau.getUpsPath();
                    mDau.close();
                    mDau = null;
                } catch (DauException e) {
                    LogUtils.d(TAG, "Dau detection and get info error: " + e.getMessage());
                    throw new DauException(e.getError());
                }
            }
        }
        return probeInformation;
    }

    /**
     * This method read the probe detail from connected Proprietary and USB mode
     *
     * @param probeId Probe ID
     * @return ProbeInformation contains probe property
     */
    public ProbeInformation getProbeDataFromDau(int probeId) throws DauException {
        ProbeInformation probeInformation = null;
        probeInformation = getProbeInformationUsb(probeId);
        if (probeInformation == null)
        {
            LogUtils.e(TAG, "Unsupported probe");
            throw new DauException(ThorError.TER_IE_UNSUPPORTED_PROBE);
        }
        return probeInformation;
    }

    /**
     * get probe information for USB probe only
     * @return probe information
     */
    private ProbeInformation getProbeInformationUsb(){
        ProbeInformation probeInformation;
        HashMap<String, UsbDevice> devices = getUsbManager().getDeviceList();

        if(devices.isEmpty()) {
            return null;
        }

        for (UsbDevice device: devices.values()) {
            LogUtils.d(TAG, "Usb Device 0x" +
                    Integer.toHexString(device.getVendorId()) +
                    ":0x" +
                    Integer.toHexString(device.getProductId())
            );

            if (isInSupportedProbes(mUsbProbeList,device.getVendorId(), device.getProductId()) != null && getUsbManager().hasPermission(device)) {
                UsbDeviceConnection usbDeviceConnection = getUsbManager().openDevice(device);
                if (usbDeviceConnection == null){
                    return  null;
                }
                int fdId = usbDeviceConnection.getFileDescriptor();
                probeInformation = getProbeInformationNativeUsb(fdId);
                LogUtils.d(TAG, "getProbeInformationUsb: USB probe type" + probeInformation.type);
                probeInformation.probeUps = nativeGetDbName(probeInformation.type);
                return probeInformation;
            }
        }

        return null;
    }

    private ProbeInformation getProbeInformationUsb(int probeId){
        ProbeInformation probeInformation = null;
        HashMap<String, UsbDevice> devices = getUsbManager().getDeviceList();

        if(devices.isEmpty()) {
            return null;
        }

        for (UsbDevice device: devices.values()) {
            LogUtils.d(TAG, "Usb Device 0x" +
                    Integer.toHexString(device.getVendorId()) +
                    ":0x" +
                    Integer.toHexString(device.getProductId())
            );

            int mProbePid = 0;

            if (probeId == PROBE_TORSO_ONE || probeId == TORSO_ONE_PID) {
                mProbePid = TORSO_ONE_PID;
            } else if (probeId == PROBE_LEXSA || probeId == LEXSA_PID) {
                mProbePid = LEXSA_PID;
            } else if (probeId == PROBE_TORSO_THREE || probeId == TORSO_THREE_PID) {
                mProbePid = PROBE_TORSO_THREE;
            }

            if (isInSupportedProbes(mUsbProbeList,device.getVendorId(), device.getProductId()) != null
                    && getUsbManager().hasPermission(device) && device.getProductId() == mProbePid) {
                UsbDeviceConnection usbDeviceConnection = getUsbManager().openDevice(device);
                if (usbDeviceConnection == null){
                    return  null;
                }
                int fdId = usbDeviceConnection.getFileDescriptor();
                probeInformation = getProbeInformationNativeUsb(fdId);
                LogUtils.d(TAG, "getProbeInformationUsb: USB probe type" + probeInformation.type);
                probeInformation.probeUps = nativeGetDbName(probeInformation.type);
                return probeInformation;
            }
        }

        return null;

    }

    /**
     * Get probe detail from PID
     *
     * @param mode Specifies the operational mode: Proprietary, Usb, or Emulation.
     * @param  probePid probe id
     * @return ProbeInformation
     *
     */
    public ProbeInformation getProbeDataFromPID(DauCommMode mode, int probePid) {
        ProbeInformation probeInformation = new ProbeInformation();
        probeInformation.probeUps = "";
        if( mode == DauCommMode.Usb) {
            LogUtils.d(TAG, "DAU is not null and probe attached as USB with PID: "+probePid);
            if(probePid == LEXSA_PID) {
                probeInformation.type = getPidBasedOnProbeType(probePid);
            }else if(probePid == TORSO_THREE_PID) {
                probeInformation.type = getPidBasedOnProbeType(probePid);
            } else if(probePid == TORSO_ONE_PID){
                probeInformation.type = getPidBasedOnProbeType(probePid);
            }
            if(probeInformation.type == 0){
                return null;
            }
        }
        return probeInformation;
    }

    /**
     * @return mUsbDevice to know whether it is null or not
     **/
    public boolean isUsbDeviceConnected() {
        return null != mUsbDevice;
    }

    /**
     * Run ML Verification test(s) on a given input file tree. Output is written to a directory
     * created under [context.getCacheDir()].
     *
     * @param appVersion Application version (for recording metadata)
     * @param inputRoot Root folder of input file tree
     * @return Root of output file tree
     */
    public File runMLVerification(String appVersion, File inputRoot) {
        return runMLVerification(appVersion, inputRoot, mContext.getCacheDir());
    }

    /**
     * Run ML Verification test(s) on a given input file tree. Output is written to the given
     * [outputRoot] folder.
     *
     * @param appVersion Application version (for recording metadata)
     * @param inputRoot Root folder of input file tree
     * @param outputRoot Root folder of output file tree
     * @return Root of output file tree
     */
    public File runMLVerification(String appVersion, File inputRoot, File outputRoot) {
        return mMLManager.runVerification(appVersion, inputRoot, this, outputRoot);
    }

    /**
     * Determine whether connected usb device is kLink device or not
     * @param vId usb device vendor id
     * @param pId usb device product id
     * @return true when kLink is connected, false otherwise
     */
    public static boolean isKosmosLink(int vId, int pId) {
        return vId == KLinkUsbDevice.K_LINK_USB_VID && pId == KLinkUsbDevice.K_LINK_USB_PRODUCT_ID;
    }

    /**
     * this function for getting probe type from actual probe
     * Note : we will remove this function after all USB probe firmware update to version 1.0.13.
     * @return mProbeInfo
     */
    private int getPidBasedOnProbeType(int probePid){
        ProbeInformation mProbeInfo = getProbeInformationUsb(probePid);
        if(mProbeInfo == null){
            return 0;
        }
        return mProbeInfo.type;
    }

    private static native void setLocaleNative(String locale);

    private static native List<String> getSupportedLocalesNative();

    private static native void openManagerNative(Context context,
                                                 AssetManager assetManager,
                                                 String appPath);

    private static native void closeManagerNative();

    private static native void enableIeNative(boolean enable);

    public static native int bootAppImgNative(int devFd);

    public static native boolean checkFwUpdateAvailableNativeUsb(int devFd, boolean isAppFd);

    public static native boolean appFwUpdateNativeUsb();

    public static native ProbeInformation getProbeInformationNativeUsb(int appFd);
    public static native boolean nativeSetProbeInformation(int probeType,int appFd);

    public static native boolean checkFwUpdateAvailableNativePCIe();
    private static native int getProbeTypePcieNative();
    private static native boolean appFwUpdateNativePCIe();
    public static native int nativeSetUsbConfiguration(int devFd);

    private static native String nativeGetDbName(int probeType);

    static {
        System.loadLibrary("c++_shared");
        System.loadLibrary("Ultrasound");

        LibraryLoaderThread loader = new LibraryLoaderThread();
        loader.start();
    }

    static class LibraryLoaderThread extends Thread {
        @Override
        public void run() {
            System.loadLibrary("DcmtkApp");  //Loaded this library here as loading from app module was causing crash in some devices
        }
    }
}
