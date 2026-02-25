//
// Copyright 2017 EchoNous Inc.
//
// Thor specific error codes
//
// This file was automatically generated.  Do not edit!
//
package com.echonous.hardware.ultrasound;

import java.util.HashMap;
import android.content.Context;

/**
 * <code>ThorError</code> is an enumeration of Thor Error Sources and their Errors.
 * An Error Source is way of grouping errors from a common origin,
 * i.e., software component or hardware device.
 * <p>
 * Error Sources begin with <code>TES_</code> and Errors begin
 * with <code>TER_</code>.
 * <p>
 * All Error Sources and Errors are defined as unsigned integers
 * that are unique and encoded.  They are typically displayed as hexadecimal values.
 * For each error, there are methods for obtaining the raw code, 
 * name, description, and source.
 * <p>
 * There are two special or uncategorized Errors: {@link ThorError#THOR_ERROR}
 * and {@link ThorError#THOR_OK}.
 * <p>
 * <code>THOR_ERROR</code> is a generic error code.
 * <p>
 * <code>THOR_OK</code> is a success code.  However it is best to use
 * the {@link ThorError#isOK()} method
 * in case additional success codes are added in the future.
 * <p>
 */
public enum ThorError {
    //
    // Generic Values
    //
    /**
     * Operation successful
     */
    THOR_OK(0x0, "OK", "Operation successful"),

    /**
     * Unspecified or uncategorized error
     */
    THOR_ERROR(0x40000000, "Error", "Unspecified or uncategorized error"),

    //
    // Thor Error Sources (TES)
    //
    /** 
     * Contains all miscellaneous and generic errors
     */
    TES_MISC(0x1, "Miscellaneous", "Contains all miscellaneous and generic errors"),

    /** 
     * Standard Linux errors from errno
     */
    TES_LINUX(0x2, "Linux", "Standard Linux errors from errno"),

    /** 
     * Errors generated from the Dau hardware
     */
    TES_DAU_HW(0x3, "Dau Hardware", "Errors generated from the Dau hardware"),

    /** 
     * Errors unique to the Thor tablet hardware
     */
    TES_TABLET_HW(0x4, "Tablet Hardware", "Errors unique to the Thor tablet hardware"),

    /** 
     * Thermal warnings/errors from Thor tablet and Dau
     */
    TES_THERMAL(0x5, "Thermal", "Thermal warnings/errors from Thor tablet and Dau"),

    /** 
     * Errors from the Ultrasound Imaging Engine
     */
    TES_IMAGE_ENG(0x6, "Imaging Engine", "Errors from the Ultrasound Imaging Engine"),

    /** 
     * Errors from the Ultrasound Manager
     */
    TES_ULTRASOUND_MGR(0x7, "Ultrasound Manager", "Errors from the Ultrasound Manager"),

    /** 
     * Errors from the AP_I Manager
     */
    TES_API_MANAGER(0x8, "AP_I Manager", "Errors from the AP_I Manager"),

    /** 
     * Errors from the AI System
     */
    TES_AI_SYSTEM(0x9, "AI System", "Errors from the AI System"),

    //
    // Thor Error Definitions
    //

    //
    // Miscellaneous
    //
    /**
     * The operation was aborted
     */
    TER_MISC_ABORT(0x40010001, "Operation Aborted", "The operation was aborted"),

    /**
     * The file could not be created
     */
    TER_MISC_CREATE_FILE_FAILED(0x40010002, "File Create Failed", "The file could not be created"),

    /**
     * The file could not be opened
     */
    TER_MISC_OPEN_FILE_FAILED(0x40010003, "File Open Failed", "The file could not be opened"),

    /**
     * The file has invalid format
     */
    TER_MISC_INVALID_FILE(0x40010004, "Invalid File", "The file has invalid format"),

    /**
     * The file is not open
     */
    TER_MISC_FILE_NOT_OPEN(0x40010005, "File Not Open", "The file is not open"),

    /**
     * The write operation failed
     */
    TER_MISC_WRITE_FAILED(0x40010006, "Write Failed", "The write operation failed"),

    /**
     * The method is not implemented
     */
    TER_MISC_NOT_IMPLEMENTED(0x40010007, "Not Implemented", "The method is not implemented"),

    /**
     * Busy: The operation could not be performed
     */
    TER_MISC_BUSY(0x40010008, "Busy", "Busy: The operation could not be performed"),

    /**
     * One or more parameters are either null or out of range
     */
    TER_MISC_PARAM(0x40010009, "Invalid Parameter", "One or more parameters are either null or out of range"),

    /**
     * Permission is not granted to perform the operation
     */
    TER_MISC_PERMISSION(0x4001000A, "Invalid Permission", "Permission is not granted to perform the operation"),

    //
    // Dau Hardware
    //
    /**
     * HV power supply monitor fault detected
     */
    TER_DAU_PWR(0x40030001, "Power Error", "HV power supply monitor fault detected"),

    /**
     * There was an MSI error in the Dau
     */
    TER_DAU_MSI(0x40030002, "MSI Error", "There was an MSI error in the Dau"),

    /**
     * There was a fault in the transmitter
     */
    TER_DAU_TXC(0x40030003, "Transmitter Fault", "There was a fault in the transmitter"),

    /**
     * There was PCIe error detected from the Dau
     */
    TER_DAU_PCIE(0x40030004, "PCIe Error", "There was PCIe error detected from the Dau"),

    /**
     * An error occurred in the Dau while Streaming
     */
    TER_DAU_STREAMING(0x40030005, "Streaming Error", "An error occurred in the Dau while Streaming"),

    /**
     * An error occurred in the Dau while processing the Sequence
     */
    TER_DAU_SEQUENCE(0x40030006, "Sequence Error", "An error occurred in the Dau while processing the Sequence"),

    /**
     * Invalid WHO_AM_I register reading
     */
    TER_DAU_MOTION_SENSOR(0x40030007, "Motion Sensor Error", "Invalid WHO_AM_I register reading"),

    /**
     * Dau Register or memory access error
     */
    TER_DAU_MEM_ACCESS(0x40030008, "Memory Error", "Dau Register or memory access error"),

    /**
     * Unknown Dau error
     */
    TER_DAU_UNKNOWN(0x40030009, "Unknown Error", "Unknown Dau error"),

    /**
     * ASIC integrity check fail after multiple power up retry cycles
     */
    TER_DAU_ASIC_CHECK(0x4003000A, "ASIC Error", "ASIC integrity check fail after multiple power up retry cycles"),

    //
    // Tablet Hardware
    //
    /**
     * Failed to access the Puck
     */
    TER_TABLET_PUCK_OPEN(0x40040001, "Puck Open Failure", "Failed to access the Puck"),

    /**
     * Failed to put the Puck in scanning mode
     */
    TER_TABLET_PUCK_SCAN(0x40040002, "Puck Scan Failure", "Failed to put the Puck in scanning mode"),

    /**
     * Failed to configure the Puck
     */
    TER_TABLET_PUCK_CONFIG(0x40040003, "Puck Configuration Failure", "Failed to configure the Puck"),

    /**
     * The Puck firmware needs to be updated
     */
    TER_TABLET_PUCK_FIRMWARE(0x40040004, "Puck Firmware", "The Puck firmware needs to be updated"),

    /**
     * The Puck firmware update failed
     */
    TER_TABLET_PUCK_UPDATE_FIRMWARE(0x40040005, "Puck Update Firmware", "The Puck firmware update failed"),

    /**
     * The Puck firmware is already in progress
     */
    TER_TABLET_PUCK_UPDATE_FIRMWARE_BUSY(0x40040006, "Puck Update Firmware Busy", "The Puck firmware is already in progress"),

    /**
     * The Puck firmware update completed
     */
    TER_TABLET_PUCK_UPDATE_FIRMWARE_COMPLETE(0x40007, "Puck Update Firmware Update Complete", "The Puck firmware update completed"),

    //
    // Thermal
    //
    /**
     * The probe is too hot
     */
    TER_THERMAL_PROBE(0x40050001, "Probe Thermal High", "The probe is too hot"),

    /**
     * The tablet is too hot
     */
    TER_THERMAL_TABLET(0x40050002, "Tablet Thermal High", "The tablet is too hot"),

    /**
     * The probe thermal sensor failed
     */
    TER_THERMAL_PROBE_FAIL(0x40050003, "Probe Thermal Fail", "The probe thermal sensor failed"),

    /**
     * The tablet thermal sensor failed
     */
    TER_THERMAL_TABLET_FAIL(0x40050004, "Tablet Thermal Fail", "The tablet thermal sensor failed"),

    //
    // Imaging Engine
    //
    /**
     * Data Underflow
     */
    TER_IE_UNDERFLOW(0x40060001, "Underflow", "Data Underflow"),

    /**
     * Data Overflow
     */
    TER_IE_OVERFLOW(0x40060002, "Overflow", "Data Overflow"),

    /**
     * Unable to load the Scan Converter Sources
     */
    TER_IE_SCAN_CONVERTER_SOURCES(0x40060003, "Scan Converter Sources", "Unable to load the Scan Converter Sources"),

    /**
     * Unable to extract the UPS
     */
    TER_IE_UPS_EXTRACT(0x40060004, "UPS Extract", "Unable to extract the UPS"),

    /**
     * Unable to open the UPS
     */
    TER_IE_UPS_OPEN(0x40060005, "UPS Open", "Unable to open the UPS"),

    /**
     * Error accessing the UPS
     */
    TER_IE_UPS_ACCESS(0x40060006, "UPS Access", "Error accessing the UPS"),

    /**
     * Unable to open the DMA buffer
     */
    TER_IE_DMA_OPEN(0x40060007, "DMA Open", "Unable to open the DMA buffer"),

    /**
     * Unable to power up the DAU
     */
    TER_IE_DAU_POWER(0x40060008, "DAU Power", "Unable to power up the DAU"),

    /**
     * A reboot is needed to restore connection to Dau
     */
    TER_IE_DAU_POWER_REBOOT(0x40060009, "DAU Power", "A reboot is needed to restore connection to Dau"),

    /**
     * Unable to map DAU memory
     */
    TER_IE_DAU_MEM(0x4006000A, "DAU Memory", "Unable to map DAU memory"),

    /**
     * Unable to initialize the Signal Handler
     */
    TER_IE_SIGHANDLE_INIT(0x4006000B, "Signal Handler Init", "Unable to initialize the Signal Handler"),

    /**
     * Unable to initialize the TBF
     */
    TER_IE_TBF_INIT(0x4006000C, "TBF Init", "Unable to initialize the TBF"),

    /**
     * Unable to initialize the SSE
     */
    TER_IE_SSE_INIT(0x4006000D, "SSE Init", "Unable to initialize the SSE"),

    /**
     * The DAU is closed
     */
    TER_IE_DAU_CLOSED(0x4006000E, "DAU Closed", "The DAU is closed"),

    /**
     * Gain value must be between 0 and 100
     */
    TER_IE_GAIN(0x4006000F, "Invalid Gain Value", "Gain value must be between 0 and 100"),

    /**
     * Cannot locate an Imaging Case from the supplied parameters
     */
    TER_IE_IMAGE_PARAM(0x40060010, "Invalid Image Parameters", "Cannot locate an Imaging Case from the supplied parameters"),

    /**
     * The operation is invalid while running
     */
    TER_IE_OPERATION_RUNNING(0x40060011, "Invalid Operation - Running", "The operation is invalid while running"),

    /**
     * The operation is invalid while stopped
     */
    TER_IE_OPERATION_STOPPED(0x40060012, "Invalid Operation - Stopped", "The operation is invalid while stopped"),

    /**
     * Failed to initialize Image Processor
     */
    TER_IE_IMAGE_PROCESSOR_INIT(0x40060013, "Image Processor - Init Failed", "Failed to initialize Image Processor"),

    /**
     * Failed to initialize Cine Buffer
     */
    TER_IE_CINE_BUFFER_INIT(0x40060014, "Cine Buffer - Init Failed", "Failed to initialize Cine Buffer"),

    /**
     * Failed to initialize Cine Viewer
     */
    TER_IE_CINE_VIEWER_INIT(0x40060015, "Cine Viewer - Init Failed", "Failed to initialize Cine Viewer"),

    /**
     * Failed to initialize Inference Engine
     */
    TER_IE_INFERENCE_ENGINE_INIT(0x40060016, "Inference Engine - Init Failed", "Failed to initialize Inference Engine"),

    /**
     * The DMA FIFO is Null
     */
    TER_IE_DMA_FIFO_NULL(0x40060017, "Null FIFO", "The DMA FIFO is Null"),

    /**
     * Failed to refresh Cine Viewer
     */
    TER_IE_CINE_VIEWER_REFRESH(0x40060018, "Cine Viewer - Refresh Failed", "Failed to refresh Cine Viewer"),

    /**
     * Unable to initialize the USB Dau
     */
    TER_IE_USB_INIT(0x400600B0, "USB Init", "Unable to initialize the USB Dau"),

    /**
     * Unable to start imaging on USB Dau
     */
    TER_IE_USB_START(0x400600B1, "USB Start", "Unable to start imaging on USB Dau"),

    /**
     * There was a USB error communicating with the Dau
     */
    TER_IE_USB_ERROR(0x400600B2, "USB Error", "There was a USB error communicating with the Dau"),

    /**
     * USB transfer timed out
     */
    TER_IE_USB_TIMED_OUT(0x400600B3, "USB Timed Out", "USB transfer timed out"),

    /**
     * USB transfer stalled
     */
    TER_IE_USB_STALL(0x400600B4, "USB Stalled", "USB transfer stalled"),

    /**
     * USB data overflowed
     */
    TER_IE_USB_OVERFLOW(0x400600B5, "USB Overflow", "USB data overflowed"),

    /**
     * Sequencer did not stop in expected time frame
     */
    TER_IE_SEQ_STALLED(0x400600B6, "Sequencer Stalled", "Sequencer did not stop in expected time frame"),

    /**
     * Failed to read sequence blob from UPS
     */
    TER_IE_UPS_NULL_BLOB(0x400600B7, "Sequence blob error", "Failed to read sequence blob from UPS"),

    /**
     * Size of sequence blob is invalid
     */
    TER_IE_UPS_INVALID_BLOB(0x400600B8, "Invalid sequence blob", "Size of sequence blob is invalid"),

    /**
     * Failed to load sequence linked list
     */
    TER_IE_SEQ_LLE_LOAD_FAILURE(0x400600B9, "LLE Sequence load error", "Failed to load sequence linked list"),

    /**
     * Failed to load sequence payload
     */
    TER_IE_SEQ_PLD_LOAD_FAILURE(0x400600BA, "PLD Sequence load error", "Failed to load sequence payload"),

    /**
     * Error occurred retrieving a parameter from UPS
     */
    TER_IE_UPS_READ_FAILURE(0x400600BB, "UPS read error", "Error occurred retrieving a parameter from UPS"),

    /**
     * Error occurred retrieving FOV
     */
    TER_IE_INVALID_FOV(0x400600BC, "Invalid FOV", "Error occurred retrieving FOV"),

    /**
     * Image Processor is engaged with no active processing stage
     */
    TER_IE_NO_ACTIVE_STAGES(0x400600BD, "No Active Stages", "Image Processor is engaged with no active processing stage"),

    /**
     * Frame header count did not match the expected value
     */
    TER_IE_INVALID_HEADER(0x400600BE, "Invalid Header", "Frame header count did not match the expected value"),

    /**
     * Unsupported imaging mode
     */
    TER_IE_IMAGING_MODE(0x400600C1, "Unsupported Imaging Mode", "Unsupported imaging mode"),

    /**
     * UPS integrity check failed
     */
    TER_IE_UPS_INTEGRITY(0x400600C2, "UPS Integrity", "UPS integrity check failed"),

    /**
     * Fail to extract the font file
     */
    TER_IE_FONT_EXTRACT(0x400600D0, "Font File Extract", "Fail to extract the font file"),

    /**
     * Unable to open the font file
     */
    TER_IE_FONT_OPEN(0x400600D1, "Font File Open", "Unable to open the font file"),

    /**
     * DA/ECG frame index did not match image frame index
     */
    TER_IE_DAECG_FRAME_INDEX(0x400600D2, "DA/ECG Frame Index Mismatch", "DA/ECG frame index did not match image frame index"),

    /**
     * Error in the computation of colorflow beam parameters
     */
    TER_IE_COLORFLOW_PARAM(0x400600D3, "Invalid Colorflow parameter", "Error in the computation of colorflow beam parameters"),

    /**
     * USB Device disconnected from Host
     */
    TER_IE_USB_NO_DEVICE(0x400600D4, "USB Cable Disconnected", "USB Device disconnected from Host"),

    /**
     * Error in TLV format
     */
    TER_IE_USB_TLV_INVALID(0x400600D5, "Invalid TLV Detected", "Error in TLV format"),

    /**
     * Error in TLV Parsing
     */
    TER_IE_USB_TLV_ERROR(0x400600D6, "Invalid TLV Structure", "Error in TLV Parsing"),

    /**
     * Error in TLV Protocol Parameters
     */
    TER_IE_USB_TLV_BADPARAMETER(0x400600D7, "Invalid TLV parameter", "Error in TLV Protocol Parameters"),

    /**
     * Unsupported probe
     */
    TER_IE_UNSUPPORTED_PROBE(0x400600D8, "Unsupported Probe", "Unsupported probe"),

    //
    // Ultrasound Manager
    //
    /**
     * Unable to create or initialize the Context
     */
    TER_UMGR_CONTEXT(0x40070001, "Context Failure", "Unable to create or initialize the Context"),

    /**
     * There was a RemoteException error.  Check the cause.
     */
    TER_UMGR_REMOTE(0x40070002, "Remote Exception", "There was a RemoteException error.  Check the cause."),

    /**
     * The Dau is closed.  A new instance must be created and opened.
     */
    TER_UMGR_DAU_CLOSED(0x40070003, "Dau Closed", "The Dau is closed.  A new instance must be created and opened."),

    /**
     * One of the required parameters is null
     */
    TER_UMGR_INVALID_PARAM(0x40070004, "Invalid Parameter", "One of the required parameters is null"),

    /**
     * Unable to get native window from supplied surface
     */
    TER_UMGR_NATIVE_WINDOW(0x40070005, "Native Window", "Unable to get native window from supplied surface"),

    /**
     * Unable to get native Asset Manager from supplied parameter
     */
    TER_UMGR_ASSETMGR(0x40070006, "Asset Manager", "Unable to get native Asset Manager from supplied parameter"),

    /**
     * A Dau instance already exists
     */
    TER_UMGR_DAU_EXISTS(0x40070007, "Dau Exists", "A Dau instance already exists"),

    /**
     * Could not access the Ultrasound Service
     */
    TER_UMGR_SVC_ACCESS(0x40070008, "Service Access", "Could not access the Ultrasound Service"),

    /**
     * The Ultrasound Player is not attached to the Cine Buffer
     */
    TER_UMGR_PLAYER_NOT_ATTACHED(0x40070009, "Player Not Attached", "The Ultrasound Player is not attached to the Cine Buffer"),

    /**
     * A Puck instance already exists
     */
    TER_UMGR_PUCK_EXISTS(0x4007000A, "Puck Exists", "A Puck instance already exists"),

    /**
     * This operation is only available on the Thor Tablet
     */
    TER_UMGR_THOR_ONLY(0x4007000B, "Thor Only", "This operation is only available on the Thor Tablet"),

    /**
     * The Puck is closed.  A new instance must be created and opened.
     */
    TER_UMGR_PUCK_CLOSED(0x4007000C, "Puck Closed", "The Puck is closed.  A new instance must be created and opened."),

    /**
     * The Dau is not connected.
     */
    TER_UMGR_DAU_ATTACHED(0x4007000D, "Dau Not Attached", "The Dau is not connected."),

    /**
     * Could not access USB Manager.
     */
    TER_UMGR_USB_MANAGER(0x4007000E, "USB Manager", "Could not access USB Manager."),

    /**
     * Could not open the USB device.
     */
    TER_UMGR_USB_DEVICE(0x4007000F, "USB Device", "Could not open the USB device."),

    //
    // AP_I Manager
    //
    /**
     * Number of UTPs exceeds limit
     */
    TER_APIMGR_NUM_UTPS(0x40080001, "Too many UTPs", "Number of UTPs exceeds limit"),

    /**
     * Parsing of UTP string obtained from API table failed
     */
    TER_APIMGR_UTP_STRING(0x40080002, "Invalid UTP string", "Parsing of UTP string obtained from API table failed"),

    /**
     * Failed to find a matching UTP ID in UPS
     */
    TER_APIMGR_UTPID(0x40080003, "UTP ID not found", "Failed to find a matching UTP ID in UPS"),

    /**
     * Failed to setup input vector
     */
    TER_APIMGR_INPUT_VECTOR(0x40080004, "Input Vector Setup", "Failed to setup input vector"),

    /**
     * AP&I reference data might be corrupted
     */
    TER_APIMGR_CORRUPTED_REF_DATA(0x40080005, "AP&I Reference Data", "AP&I reference data might be corrupted"),

    //
    // AI System
    //
    /**
     * An AI model failed to load
     */
    TER_AI_MODEL_LOAD(0x40090001, "Model load error", "An AI model failed to load"),

    /**
     * An AI model failed to run
     */
    TER_AI_MODEL_RUN(0x40090002, "Model run error", "An AI model failed to run"),

    /**
     * Failed to identify either ED or ES frame (or both)
     */
    TER_AI_FRAME_ID(0x40090003, "Frame ID failure", "Failed to identify either ED or ES frame (or both)"),

    /**
     * Detected ED or ES frame may be invalid. Recommend manual verification
     */
    TER_AI_FRAME_ID_WARN(0x40090004, "Frame ID warning", "Detected ED or ES frame may be invalid. Recommend manual verification"),

    /**
     * Failed to segment the requested item
     */
    TER_AI_SEGMENTATION(0x40090005, "Segmentation failure", "Failed to segment the requested item"),

    /**
     * Current clip is too short for smart capture
     */
    TER_AI_SC_NOFRAMES(0x40090006, "Smart capture clip too short", "Current clip is too short for smart capture"),

    /**
     * No clip range met smart capture requirements
     */
    TER_AI_SC_NO_VALID_RANGE(0x40090007, "Smart capture no range found", "No clip range met smart capture requirements"),

    /**
     * No LV region found by AI
     */
    TER_AI_LV_NOT_FOUND(0x40090008, "LV not found", "No LV region found by AI"),

    /**
     * Multiple possible LV regions found by AI
     */
    TER_AI_MULTIPLE_LV(0x40090009, "Multiple LV", "Multiple possible LV regions found by AI"),

    /**
     * LV region is too small
     */
    TER_AI_LV_TOO_SMALL(0x4009000A, "LV too small", "LV region is too small"),

    /**
     * Uncertainty in LV segmentation above threshold
     */
    TER_AI_LV_UNCERTAINTY_ERROR(0x4009000B, "LV uncertainty error", "Uncertainty in LV segmentation above threshold"),

    /**
     * A4C major axis is significantly shorter than A2C major axis
     */
    TER_AI_A4C_FORESHORTENED(0x4009000C, "A4C foreshortened", "A4C major axis is significantly shorter than A2C major axis"),

    /**
     * A2C major axis is significantly shorter than A4C major axis
     */
    TER_AI_A2C_FORESHORTENED(0x4009000D, "A2C foreshortened", "A2C major axis is significantly shorter than A4C major axis");

    private final int code;
    private final String name;
    private final String description;

    // Build a hash table to map error codes back to the Enum.
    private static final ThorError[] values = ThorError.values();

    private static final HashMap<Integer, ThorError> valToErrorMap = 
        new HashMap<Integer, ThorError>();

    static {
        for (ThorError error : values) {
            valToErrorMap.put(error.getCode(), error);
        }
    }

    private ThorError(int code, String name, String description) {
        this.code = code;
        this.name = name;
        this.description = description;
    }

    /**
     * Returns a ThorError instance from a raw error code.  Will return THOR_ERROR 
     * if the specified raw code is invalid. 
     * 
     * 
     * @param code The raw error code
     * 
     * @return The ThorError enumeration
     */
    public static ThorError fromCode(int code) {
        ThorError   error = valToErrorMap.get(code);

        if (null == error) {
            // A bogus code was submitted, so make a generic error.
            error = THOR_ERROR;
        }
        return error;
    }

    /**
     * Returns the raw error code.
     * 
     */
    public int getCode() {
        return code;
    }

    /**
     * Returns the name of this error.
     * 
     */
    public String getName() {
        return name;
    }

    /**
     * Returns the description of this error.
     * 
     */
    public String getDescription() {
        return description;
    }

    /**
     * Returns a localized description of this error.
     * 
     * @param context The application context for retrieving the localized string 
     *                resource. The name of the resource string should be the same
     *                as the ThorError enumeration.
     * 
     * @return String The localized description.  If one does not exist, then null 
     *         is returned.
     */
    public String getLocalizedMessage(Context context) {
        String  message = null;
        int     resId;

        resId = context.getResources().getIdentifier(this.name(),
                                                     "string",
                                                     context.getPackageName());
        if (0 != resId) {
            message = context.getString(resId);
        }

        return message;
    }

    /**
     * Returns the Source of this error.
     * 
     */
    public ThorError getSource() {
        // Error Sources, OK, and ERROR don't have parent sources.
        if (code <= THOR_ERROR.getCode()) {
            return this;
        }
        else {
            // Mask off everything except the Error Source bits
            return fromCode((code & 0xFFF0000) >> 16);
        }
    }

    /**
     * Returns true if this is a success code, false if an actual error.
     * 
     */
    public boolean isOK() {
        return ((code & THOR_ERROR.getCode()) != THOR_ERROR.getCode());
    }


    /**
     * Returns a string that can be used for display or logging purposes.  The 
     * format is <code>&ltname&gt (raw code): &ltdescription&gt</code>.
     * 
     */
    @Override
    public String toString() {
        return name + "(" + Integer.toHexString(code) + "): " + description;
    }

}
