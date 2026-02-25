// Copyright (C) 2018 EchoNous, Inc.
package com.accessvascularinc.hydroguide.mma.app


import com.accessvascularinc.hydroguide.mma.ultrasound.UpsManager

object AppConstants {

    const val APP_NAME: String = "ECHONOUS_APP"
    const val FREE_FLOW = "FF"
    const val AI_FLOW = "AI"
    const val BLADDER_FLOW = "BF"
    const val TYPE_AI = "AI"
    const val ORGAN_HEART: String = "Heart"
    const val ORGAN_LUNG: String = "Lung"
    const val ORGAN_ABDOMEN: String = "Abdomen"
    const val A4C_STRING: String = "A4C"
    const val A2C_STRING: String = "A2C"
    const val EVEN = 2
    const val PATIENT_MRN: String = "PATIENT_MRN"
    const val SELECTED_PATIENT_MRN: String = "SELECTED_PATIENT_MRN"
    const val PATIENT_NAME: String = "PATIENT_NAME"
    const val PATIENT_FIRST_NAME: String = "PATIENT_FIRST_NAME"
    const val PATIENT_LAST_NAME: String = "PATIENT_LAST_NAME"
    const val PATIENT_DOB: String = "PATIENT_DOB"
    const val PATIENT_GENDER: String = "PATIENT_GENDER"
    const val EXAM_ID: String = "EXAM_ID"
    const val SCAN_ID: String = "SCAN_ID"
    const val SCREEN_MODE: String = "SCREEN_MODE"
    const val SCREEN_MODE_EDIT: String = "SCREEN_MODE_EDIT"
    const val SCREEN_MODE_NEW_CHANGE: String = "SCREEN_MODE_NEW_CHANGE"
    const val SCREEN_MODE_MERGE_PATIENT: String = "SCREEN_MODE_MERGE_PATIENT"
    const val SPACE_DELIMITER: String = " "
    const val SCREEN_MODE_PATIENT_SEARCH: String = "SCREEN_MODE_PATIENT_SEARCH"
    const val PREVIOUS_FRAGMENT: String = "PREVIOUS_FRAGMENT"
    const val SELECTED_MEDIA_INDEX: String = "SELECTED_MEDIA_INDEX"
    const val TOTAL_MEDIA: String = "TOTAL_MEDIA"
    const val COMPLETED: Int = 2
    const val IN_PROGRESS: Int = 1
    const val NORMALIZED_BATTERY_THRESHOLD: Int = 15
    const val LOW_BATTERY_LEVEL: Int = 15
    const val CRITICAL_BATTERY_LEVEL: Int = 4
    const val CRITICAL_STORAGE_SPACE: Int = 150
    const val ARCHIVAL_CLOUD_STATUS_WAIT: Int = 1
    const val ARCHIVAL_CLOUD_STATUS_IN_PROGRESS: Int = 2
    const val ARCHIVAL_CLOUD_STATUS_COMPLETED: Int = 3
    const val ARCHIVAL_CLOUD_STATUS_FAIL: Int = 4
    const val ARCHIVAL_CLOUD_STATUS_NON:Int=0
    const val JOB_CANCELLED=1
    const val JOB_ACTIVE=0
    const val EXPORT_STATUS_NON: Int=0
    const val EXPORT_STATUS_WAIT: Int = 1
    const val EXPORT_STATUS_IN_PROGRESS: Int = 2
    const val EXPORT_STATUS_COMPLETED: Int = 3
    const val EXPORT_STATUS_FAIL: Int = 4
    /**
     * Special USB Export for EF Job research
     */
    const val SPECIAL_EXPORT_NON:Int=10
    const val SPECIAL_EXPORT_WAIT:Int=11
    const val SPECIAL_EXPORT_IN_PROGRESS:Int=12
    const val SPECIAL_EXPORT_FAIL:Int=13
    const val SPECIAL_EXPORT_COMPLETED:Int=14
    const val TYPE_ARCHIVAL: Int = 1
    const val TYPE_EXPORT: Int = 2
    const val TYPE_ARCHIVAL_EXPORT: Int = -1
    const val SEARCH_EXAM = 11
    const val SEARCH_PATIENT = 12
    const val SEARCH_JOB = 13
    const val MRN_ECLIPSIZE = 12
    const val OVERALL_GAIN = 50
    const val NEAR_GAIN = 50
    const val FAR_GAIN = 50
    const val COLOR_GAIN = 50
    const val ON_TOUCH_GAIN_SCALE = 2
    const val ON_TOUCH_GAIN_RADIUS = 5
    const val JPG: String = ".jpg"
    const val THOR: String = ".thor"
    const val IMAGE: String = "Image"
    const val CLIP: String = "Clip"
    const val MP4: String = ".mp4"
    const val RAW:String=".rgb"
    const val MODE_TYPE="ModeType"
    const val DOPPLER_MODE_TYPE="DopplerModeType"
    const val B_Mode="BMode"
    const val C_Mode="CMode"
    const val PW_Mode="PwMode"
    const val CW_Mode="CwMode"
    const val TDI_Mode="TdiMode"
    const val M_Mode = "MMode"
    const val JOB_STATUS_WAIT: String = "Wait"
    const val JOB_STATUS_IN_PROGRESS: String = "In Progress"
    const val JOB_STATUS_COMPLETED: String = "Completed"
    const val JOB_STATUS_FAIL: String = "Fail"
    const val ACTION_USB_PERMISSION = "com.echonous.kosmosapp.USB_PERMISSION"
    const val APP_USB_ROOT_DIR: String = "Echonous"
    const val APP_USB_SPECIAL_EXPORT_ENCRYPTED_FILE: String = "special_export.enc"
    const val PREF_NAME="KosmosApp"
    const val PREF_OTS_REGISTERED_PROBE_SN_KEY_SET="PREF_OTS_REGISTERED_PROBE_SN_KEY_SET"
    const val PREF_DEVICE_IS_REGISTER_KEY="PREF_DEVICE_IS_REGISTER_KEY"
    const val PREF_DEVICE_PRIVATE_KEY="PREF_DEVICE_PRIVATE_KEY"
    const val PREF_DEVICE_CERT_KEY="PREF_DEVICE_CERT_KEY"
    const val PREF_DEVICE_SERIAL_NUMBER="PREF_DEVICE_SERIAL_NUMBER"
    const val PREF_SIGN_IN_SESSION_RESET_TIME="PREF_SIGN_IN_SESSION_RESET_TIME"
    const val PREF_ADMIN_SIGN_IN_SESSION_RESET_TIME = "PREF_ADMIN_SIGN_IN_SESSION_RESET_TIME"
    const val PREF_HASH_PREF_RESET_FOR_RELEASE="PREF_HASH_PREF_RESET_FOR_RELEASE"
    const val PREF_AUTO_POWEROFF="AUTOPOWER"
    const val PREF_MWL_LAST_QUERY_TIME = "MWL_QUERY_TIME"
    const val PREF_MWL_QUERY_DATA ="MWL_QUERY_DATA"
    const val PREF_SAME_AUTHOR_FOR_FETAL_AGE_GROWTH ="PREF_SAME_AUTHOR_FOR_FETAL_AGE_GROWTH"
    const val PREF_FETAL_WEIGHT_CALCULATION_ID ="PREF_FETAL_WEIGHT_CALCULATION_ID"
    const val ECHONOUS_DIR_PATH="/Echonous"
    const val KEY_APP_DIRECTORY="echonousdir"
    const val EMPTY=""
    const val UTF_8 = "utf-8"
    const val KEY_DEPTH_THICKNESS="keyDepthThick"
    const val DP_38: Int = 38
    const val DP_64: Int = 64
    const val DP_156: Int = 156
    const val MARGIN_TOP_OF_ECG_DA_TIME_SCALE_VIEW_MODE_6_1_1: Int = 135
    const val MARGIN_TOP_OF_ECG_DA_TIME_SCALE_VIEW_MODE_2_1_1: Int = 270
    const val MARGIN_TOP_OF_ECG_DA_TIME_SCALE_VIEW_MODE_1_1: Int = 540
    const val DEFAULT_TIME_SCALE_VALUE_IN_SECOND: Float = 5.454f
    const val DEFAULT_TIME_SCALE_SPEED_INDEX: Int = 1
    const val DEFAULT_ECG_GAIN_VALUE_IN_PERCENT: Int = 50
    const val DEFAULT_DA_GAIN_VALUE_IN_PERCENT: Int = 50
    const val EF: String = "EF"
    const val EFSCAN_STATUS_A4C_CAPTURED: Int = 1
    const val EFSCAN_STATUS_A4C_A2C_CAPTURED: Int = 2
    const val EFSCAN_STATUS_CALCULATION_COMPLETED: Int = 3
    const val CLIP_TYPE_A4C = 1
    const val CLIP_TYPE_A2C = 2
    const val CURRENT_FLOW_FRAGMENT_PREF : String = "current_flow_fragment_pref"
    const val CURRENT_FLOW_FRAGMENT : String = "current_flow_fragment"
    const val EF_SCAN_METHOD_A4C= 1
    const val EF_SCAN_METHOD_A4C_A2C= 2
    const val EF_SCAN_ES_FRAME = 1
    const val EF_SCAN_ED_FRAME = 2
    const val EF_SCAN_ES_OLD_FRAME = 3
    const val EF_SCAN_ED_OLD_FRAME = 4
    const val OTHER_FRAME = -1
    const val DISABLE = 0
    const val ON=1
    const val OFF=0
    const val NO_OF_SIGNAL_ONE: Int = 1
    const val NO_OF_SIGNAL_TWO: Int = 2
    const val NO_OF_SIGNAL_THREE: Int = 3
    const val KEY_THOR_FILE_NAME="KEY_THOR_FILE_NAME"
    const val KEY_IS_ECG_ON_JSON="KEY_IS_ECG_ON_JSON"
    const val UNLOCK: Int = 0
    const val LOCK: Int = 1
    const val EF_SCAN_METHOD : String = "EF_SCAN_METHOD"
    const val KEY_TEXT_NOTE_DETAIL: String = "textnote"
    const val KEY_SCANID_SCAN_LAYOUT:String = "scanId"
    const val KEY_ZOOM_BOX="KeyZoomBox"
    const val KEY_SCALE_BOX="KeyScaleBox"
    const val ED: String = "ED"
    const val ES: String = "ES"
    const val SEGMENTATION: String = "segmentation"
    const val FRAME_SELECTION: String = "frame selection"
    const val NONE_STRING : String = "none"
    const val ACTION_SET_ED : Int = 1
    const val ACTION_SET_ES : Int = 2
    const val ACTION_MOVE : Int = 3
    const val ACTION_SELECT : Int = 4
    const val ACTION_DESELECT : Int = 5
    const val ACTION_UNDO_ENABLE : Int = 6
    const val ACTION_UNDO_DISABLE : Int = 7
    const val ACTION_REDO_ENABLE : Int = 8
    const val ACTION_REDO_DISABLE : Int = 9
    const val ACTION_UNDO_DISABLE_REDO_ENABLE : Int = 10
    const val ACTION_UNDO_ENABLE_REDO_DISABLE : Int = 11
    const val ACTION_UNDO_ENABLE_REDO_ENABLE : Int = 12
    const val ACTION_ENABLE_SET_ES_ED_BUTTON : Int = 13
    const val ACTION_DISABLE_SET_ES_ED_BUTTON : Int = 14
    const val ACTION_RESET : Int = 15
    const val ACTION_RESET_UNDO : Int = 16
    const val ACTION_CLEAR_CHANGE_LIST : Int = 17
    const val ACTION_REMOVE : Int = 18
    const val ACTION_REMOVE_UNDO : Int = 19
    const val ACTION_REMOVE_LAST_ADDED : Int = 20
    const val ACTION_SET_TAB_A4C = 21
    const val ACTION_SET_TAB_A2C = 22
    const val ACTION_REMOVE_ES : Int = 23
    const val ACTION_REMOVE_ED : Int = 24
    const val ACTION_SET_SELECT_ECG : Int = 24
    const val ACTION_SET_DESELECT_ECG : Int = 25
    const val ACTION_SAVED : Int = 26
    const val CLIP_TYPE: String = "CLIP_TYPE"
    const val FRAME_TYPE: String = "FRAME_TYPE"
    const val WAKE_LOCK="KosmosApp::LiveImaging"
    const val EF_SEGMENTATION_ES_FRAME = 1
    const val EF_SEGMENTATION_ED_FRAME = 2
    const val FAIL : Int = 0
    const val SUCCESS : Int = 1
    const val FIRMWARE_PROPERTY_DEVICE_ID = "deviceId"
    const val FIRMWARE_PROPERTY_JOB_ID = "jobId"
    const val FIRMWARE_PROPERTY_STATUS = "status"
    const val FIRMWARE_PROPERTY_TYPE = "type"
    const val CLIP_MODE = "clipmode"
    const val READ_ONLY_MODE:Int = 1
    const val SCAN_TYPE: String = "SCAN_TYPE"
    const val TYPE_SCAN_A4C = "SCAN A4C"
    const val TYPE_SCAN_A2C = "SCAN A2C"
    const val TYPE_SCAN_A4C_A2C = "SCAN A4C and A2C"
    const val LEAD_I = 1
    const val LEAD_II = 2
    const val LEAD_III = 3
    const val MODE_M="2D + M"
    const val MODE_B="2D"
    const val MODE_C="2D + C"
    const val MODE_CPD="2D + CPD"
    const val MODE_PW="PW"
    const val MODE_CW="CW"
    const val MODE_TDI="TDI"
    const val GAIN_VALUE_TYPE_ECG = 0
    const val GAIN_VALUE_TYPE_DA = 1
    const val UP = 0
    const val DEFAULT_EXTERNAL_CONNECTED: Boolean = false
    const val DEFAULT_EXTERNAL_LEAD_SELECTED:Int = LEAD_II
    const val EXTERNAL_LEAD_SELECTED_KEY = "externalLead"
    const val EXTERNAL_CONNECTED_KEY = "isExternalConnected"
    const val aiExternalLeadSelectedKey = "aiExternalLead"
    const val DEFAULT_RADIOBUTTON_LEAD_SELECTION:Int = LEAD_II
    const val PREF_EF_RECORD_CLIP_TYPE="PREF_EF_RECORD_CLIP_TYPE"
    const val PREF_ALWAYS_SHOW_DEV_OPTIONS="PREF_ALWAYS_SHOW_DEV_OPTIONS"
    const val INVALID_HEART_RATE = -1.0F
    const val CLOUD_JOB_STATUS_FAIL = 0
    const val CLOUD_JOB_STATUS_WAIT = 1
    const val CLOUD_JOB_STATUS_IN_PROGRESS = 2
    const val CLOUD_JOB_STATUS_COMPLETED = 3
    const val PREF_PRIVACY_LOCATION_KEY ="PREF_PRIVACY_LOCATION"
    const val PREF_PRIVACY_SEND_USAGE_DATA_KEY ="PREF_PRIVACY_SEND_USAGE_DATA"
    const val PREF_PRIVACY_SEND_ANONYMIZED_IMAGE_DATA_KEY ="PREF_PRIVACY_SEND_ANONYMIZED_IMAGE_DATA"
    const val PREF_PRIVACY_MANUALLY_SEND_IMAGE_DATE_KEY ="PREF_PRIVACY_MANUALLY_SEND_IMAGE_DATE"
    const val PREF_SLEEP_EXAM_COMPLETE_ON_SLEEP_DATE_KEY ="PREF_PRIVACY_EXAM_COMPLETE_ON_SLEEP_DATE_KEY"
    const val MEDIA_IMG_CLIP=0
    const val MEDIA_REPORT=1
    const val KEY_DIALOG_TYPE = "DIALOG_TYPE"
    const val KEY_DIALOG_TITLES = "KEY_DIALOG_TITLES"
    const val KEY_USER_TYPE = "KEY_USER_TYPE"
    const val KEY_HASH_RESET = "KEY_HASH_RESET"
    const val DIALOG_TYPE_ADD_USER = "ADD_SECURITY_USER"
    const val DIALOG_TYPE_EDIT_USER = "EDIT_SECURITY_USER"
    const val ACCESS_TYPE_ADMIN_USER = "ADMIN"
    const val ACCESS_TYPE_BASIC_USER = "BASIC"
    const val ACCESS_TYPE_EMERGENCY= "EMERGENCY"
    const val DIALOG_TYPE_SIGN_IN_TIME_OUT= "DIALOG_TYPE_SIGN_IN_TIME_OUT"
    const val DIALOG_TYPE_SIGN_IN_NORMAL= "DIALOG_TYPE_SIGN_IN_NORMAL"
    const val ACCESS_TYPE_HOME_SCREEN_ADMIN_PIN = "HOME_SCREEN_ADMIN_PIN"
    const val EXTRA_SHOW_FRAGMENT_AS_SUBSETTING = ":settings:show_fragment_as_subsetting"
    const val USB_DAU_DETECT_ALLOW = true
    const val ARCHIVE_EF="EF"//EjectionFraction
    const val ARCHIVE_BF="BF"
    const val DIALOG_TYPE_ADMIN_AUTH_OTA_NOTIFICATION = "DIALOG_TYPE_ADMIN_AUTH_OTA_NOTIFICATION"
    const val DIALOG_TYPE_ADMIN_AUTH_OTA_USB = "DIALOG_TYPE_ADMIN_AUTH_OTA_USB"
    const val DIALOG_TYPE_ADMIN_AUTH_OTA_UPDATE = "DIALOG_TYPE_ADMIN_AUTH_OTA_USB"
    const val LAST_ARCHIVE_DAYS_LIMIT = 7
    const val ONE_DAY_MILLISECONDS = 86400000
    const val ADMIN_TIMEOUT_ACTION = "ADMIN_TIMEOUT_ACTION"
    const val ACTION_ADMIN_TIME_OUT = "ACTION_ADMIN_TIME_OUT"
    const val PREF_PROBE_INFORMATION_JSON = "PREF_PROBE_INFORMATION_JSON"
    const val EF_LOWER_RANGE = 0
    const val EF_UPPER_RANGE = 100
    const val EDV_UPPER_RANGE = 500
    const val ESV_UPPER_RANGE = 400
    const val EDV_ESV_MAX_DIFF = 30
    const val LEGEND_MAX_LENGTH = 999
    const val LEGEND_MAX_DEFAULT_VALUE = "999+"
    // Screen will be time out after 4 minute and 30 second. We did not use 5 minute because screen slip dialog will showing after 5 minute so both alert dialog will overlap each other
    const val ADMIN_SCREEN_TIME_OUT = 258000L
    const val DOUBLE_TAP_INTERVAL_TIME = 1000 // 1 second
    const val TAGGED=1
    const val IMAGING_WIDTH=1
    const val IMAGING_HEIGHT=1
    var TIME_OUT_FOR_SHOW_WARRNING_FOR_CW = (1*60*1000).toLong() // In Mile-seconds
    const val AUTO_FREEZE_TIMEOUT_CW_15M = (15*60*1000).toLong()
    var SECOND_TIME_AUTO_FREEZE_COUNTDOWN_DURATION = (UpsManager.getInstance()!!.coolDownTimeInMin*60).toLong()//In seconds
    const val PREF_AUTO_FREEZE_COUNT="PREF_AUTO_FREEZE_COUNT"
    const val DEFAULT_AUTO_FREEZE_COUNT = 0
    const val FIRST_TIME_AUTO_FREEZE = 1
    const val SECONDS_TIME_AUTO_FREEZE = 2
    const val KEY_DATE_FORMAT = "KEY_DATE_FORMAT"
    const val KEY_LANGUAGE = "KEY_LANGUAGE"
    const val DEFAULT_LANGUAGE_POSITION : Int = 0
    const val DEFAULT_DATE_FORMAT_POSITION : Int = 1
    const val FIGS_DATE_FORMAT = "dd/MM/yyyy"
    const val JAPANESE_DATE_FORMAT = "yyyy/MM/dd"
    const val FILE_TYPE_JPEG = 1
    const val FILE_TYPE_DICOM = 2
    const val FILE_TYPE_DICOMDIR = 3
    //THSW-2316, Increased timeout AI calculation
    const val AI_CALCULATION_TIME_OUT = 120000L // Ai calculation timeout 120 seconds
    // This is use to store and show the use correct date when user change time zone.
    const val TIMEZONE_UTC = "UTC"
    const val EXPORT_IMAGE_SIZE_RATIO= 4
    const val EXPORT_CLIP_SIZE_RATIO= 5
    const val EXPORT_EF_CLIP_RATIO=6
    const val EXPORT_DICOM_CLIP_RATIO=8
    const val EXPORT_EF_IMAGES_SIZE=307200
    const val ESTIMATED_PDF_SIZE = 35840 // 35 KB, Approx PDF size, Later it will be change
    const val EXPORT_FF_DICOM_STILL=307200
    const val EXPORT_MJPEG_DICOM_RATIO=1.87
    const val PHYSICAL_COORDINATE_DIFFERENCE = 2.0f
    const val KEY_ACCESS_TOKEN_EXPIRE_ON = "expires_on"
    const val KEY_ACCESS_TOKEN = "access_token"
    const val DICOM_CLIP_H264=0
    const val DICOM_CLIP_MJPEG=1
    const val PERSIST_INFO_DIR = "/persist/thor_config/"
    const val KEY = "7854156156611111"
    const val INIT_VECTOR = "0000000000000000" // 16 bytes IV
    //TODO We need to revisit this
    const val DCM_HEART_IMG_TYPE="DERIVED\\PRIMARY\\CARDIAC"
    const val DCM_OTHER_IMG_TYPE="DERIVED\\PRIMARY"
    const val BF_PROTOCOL="Bladder Workflow"
    const val EF_PROTOCOL="EF Workflow"
    const val FF_PROTOCOL="Free Flow"
    const val TABLET_PARAMETERS_JSON = "device_parameters.json"
    const val TABLET_TEMPERATURE_UPPER_LIMIT = 48.0
    const val TABLET_TEMPERATURE_RESUME_THRESHOLD = 42.0
    const val TABLET_TEMPERATURE_MONITOR_INTERVAL_IN_SECONDS = 60.0
    const val KEY_APP_VERSION : String = "key_app_version"
    const val KEY_SYSTEM_SOFTWARE_VERSION : String = "key_system_software_version"
    const val KEY_APPLICATION_LAST_STATE : String = "application_last_state"
    const val KEY_DEFAULT_STATE : String = "INIT"
    const val KEY_CREATE_STATE : String = "CREATE"
    const val KEY_DESTROY_STATE : String = "DESTROY"
    const val USB_DETECTING = -2
    const val ANNOTATION_TOUCH_PADDING = 45
    const val ANNOTATION_TEXT_LENGTH = 32
    /**
     * ECG Control delay for SignalsControlFragment to be initial and load so that UI can be updated properly
     * when ECG external connect/disconnect
     */
    const val ECG_CONTROL_DELAY=700L
    const val TIME_500_MILLISECOND = 500L

    const val UL_CONTROL_MODE_B = 1
    // For B + PW/CW
    const val UL_CONTROL_MODE_B_LIVE_PW = 2
    const val UL_CONTROL_MODE_B_LIVE_CW = 3
    const val UL_CONTROL_MODE_DOPPLER_LIVE_B = 4
    // For B+C + PW/CW
    const val UL_CONTROL_MODE_COLOR_LIVE_C = 5
    const val UL_CONTROL_MODE_COLOR_LIVE_B = 6
    // For B + TDI
    const val UL_CONTROL_MODE_B_LIVE_TDI = 7

    const val WHEEL_TYPE_DEPTH = 1
    const val WHEEL_TYPE_PRF = 2
    const val WHEEL_TYPE_DOPPLER_PRF = 4
    const val WHEEL_TYPE_COLOR_STEERING_ANGLE = 3
    const val WHEEL_TYPE_PW_LEXSA_STEERING_ANGLE = 5

    const val TIME_FOR_SCROLL_IDEAL = 300L
    const val TIMEOUT_FOR_POPUP_DISMISS = 2000L // 2 Seconds
    const val IMAGING_MODE_B_INDEX=0
    const val IMAGING_MODE_M_INDEX=1
    const val IMAGING_MODE_C_INDEX=2
    const val IMAGING_MODE_PW_INDEX=3
    const val IMAGING_MODE_CW_INDEX=4
    const val IMAGING_MODE_TDI_INDEX=5
    const val IMAGING_MODE_CPD_INDEX = 6
    const val FIRMWARE="firmware"
    const val PREF_KOSMOS_STATION_NAME="KosmosStationName"
    const val ED_FRAME=0
    const val ES_FRAME=1
    const val RESULT_FRAME=2
    const val PREF_ED_A4CFRAME_KEY="%s_A4CED"
    const val PREF_ES_A4CFRAME_KEY="%s_A4CES"
    const val PREF_ED_A2CFRAME_KEY="%s_A2CED"
    const val PREF_ES_A2CFRAME_KEY="%s_A2CES"
    const val PREF_RESULTFRAME_KEY="%s_RESULT"
    const val FLOW_TYPE : String = "FLOW_TYPE"
    const val SECTION_VIEW: Int = 1
    const val RAW_VIEW: Int = 2
    const val PATIENT_SCREEN_ELLIPSE_POSITION_PORTRAIT = 0
    const val CHANGE_PATIENT_SCREEN_ELLIPSE_POSITION_PORTRAIT = 3
    const val CHANGE_PATIENT_SCREEN_ELLIPSE_POSITION_LANDSCAPE = 5
    const val PATIENT_SCREEN_ELLIPSE_POSITION_LANDSCAPE = 1
    const val PATIENT_REVIEW_ELLIPSE_POSITION_LANDSCAPE = 1
    const val PATIENT_REVIEW_ELLIPSE_POSITION_PORTRAIT = 2
    const val EXAM_SCREEN_ELLIPSE_POSITION_PORTRAIT = 0
    const val EXAM_SCREEN_ELLIPSE_POSITION_LANDSCAPE = 2
    const val NAVIGATE_FROM_SCREEN: String = "NAVIGATE_FROM_SCREEN"
    const val CURRENT_SELECTED_TAB: String = "CURRENT_SELECTED_TAB"
    const val TAB_PATIENT: String = "PATIENT"
    const val TAB_EXAM: String = "EXAM"
    const val TAB_PROCEDURE ="Procedure"
    const val IS_MWL : String = "IsMwl"
    const val DEVICE_REGISTRATION_STATUS_NOT_RUNNING = 0
    const val DEVICE_REGISTRATION_STATUS_RUNNING = 1
    const val PREF_DEVICE_CERTIFICATE_EXPIRE_ON="PREF_DEVICE_CERTIFICATE_EXPIRE_ON"
    const val ROTATE_CERTIFICATE_DAYS : Int = 7 //days
    const val TRIO_TIMER: Long = 4000
    const val HAS_PORTRAIT = "hasPortrait"
    const val MWL_PatientName="PatientName"
    const val MWL_PatientID="PatientID"
    const val MWL_AccessionNumber="AccessionNumber"
    const val MWL_PatientBirthDate="PatientBirthDate"
    const val MWL_PatientSex="PatientSex"
    const val MWL_EthnicGroup="EthnicGroup"
    const val MWL_PatientSize ="PatientSize"
    const val MWL_PatientWeight ="PatientWeight"
    const val MWL_ReferringPhysicianName="ReferringPhysicianName"
    const val MWL_Prefix="NamePrefix"
    const val MWL_Suffixes="NameSuffixes"
    const val MWL_PatientComments="PatientComments"
    const val MWL_PhysiciansOfRecord ="PhysiciansOfRecord"
    const val ORGAN_NAME: String = "organ_name"
    const val IMAGE_WIDTH_LANDSCAPE_UNCLIPPED: String = "IMAGE_WIDTH_LANDSCAPE_UNCLIPPED"
    const val IMAGE_HEIGHT_LANDSCAPE_UNCLIPPED: String = "IMAGE_HEIGHT_LANDSCAPE_UNCLIPPED"
    const val IMAGE_WIDTH_PORTRAIT_UNCLIPPED: String = "IMAGE_WIDTH_PORTRAIT_UNCLIPPED"
    const val IMAGE_HEIGHT_PORTRAIT_UNCLIPPED: String = "IMAGE_HEIGHT_PORTRAIT_UNCLIPPED"
    const val COLOR_MAP_DIALOG_WIDTH_PERCENT: Double = 0.81
    const val COLOR_MAP_DIALOG_WIDTH_PERCENT_MULTI_WINDOW: Double = 0.7
    const val IMAGING_ROI_RATIO: Float = 1.41F
    const val IMG_THOR_WIDTH = 1520
    const val IMG_THOR_HEIGHT = 1164
    const val IMG_DICOM_WIDTH = 1280
    const val IMG_DICOM_HEIGHT = 720
    const val TOOLBAR_HEIGHT_LANDSCAPE: String = "TOOLBAR_HEIGHT_LANDSCAPE"
    const val TOOLBAR_HEIGHT_PORTRAIT: String = "TOOLBAR_HEIGHT_PORTRAIT"
    const val CONTROL_FRAGMENTS_WIDTH_LANDSCAPE: String = "CONTROL_FRAGMENTS_WIDTH_LANDSCAPE"
    const val CONTROL_FRAGMENTS_HEIGHT_PORTRAIT: String = "CONTROL_FRAGMENTS_HEIGHT_PORTRAIT"
    const val DEVICE_WIDTH_LANDSCAPE: String = "DEVICE_WIDTH_LANDSCAPE"
    const val DEVICE_HEIGHT_LANDSCAPE: String = "DEVICE_HEIGHT_LANDSCAPE"
    const val DEVICE_WIDTH_PORTRAIT: String = "DEVICE_WIDTH_PORTRAIT"
    const val DEVICE_HEIGHT_PORTRAIT: String = "DEVICE_HEIGHT_PORTRAIT"
    const val SAMSUNG_MANUFACTURER: String = "samsung"
    const val SAMSUNG_DEVICE_SERIAL_PROPERTY_KEY: String = "ril.serialnumber"
    const val FRAGMENT_PREVIOUS = "fragmentPrevious"
    const val IS_ULTRASOUND_UNCLIPPED: String = "IS_ULTRASOUND_UNCLIPPED"
    const val IMAGE_WIDTH_LANDSCAPE_CLIPPED: String = "IMAGE_WIDTH_LANDSCAPE_CLIPPED"
    const val IMAGE_HEIGHT_LANDSCAPE_CLIPPED: String = "IMAGE_HEIGHT_LANDSCAPE_CLIPPED"
    const val IMAGE_WIDTH_PORTRAIT_CLIPPED: String = "IMAGE_WIDTH_PORTRAIT_CLIPPED"
    const val IMAGE_HEIGHT_PORTRAIT_CLIPPED: String = "IMAGE_HEIGHT_PORTRAIT_CLIPPED"
    const val IDEAL_EXAM_IN_PROGRESS_TIME = "IDEAL_EXAM_IN_PROGRESS_TIME"
    const val HAS_LANGUAGE = "HAS_LANGUAGE"
    const val PREVIOUS_LANGUAGE = "PREVIOUS_LANGUAGE"
    const val NUMBER_OF_COLUMNS: Int = 4
    const val PREF_ACCESS_FIRST_LAUNCH ="PREF_FIRST_LAUNCH"
    const val PREF_TUTORIAL_NAME ="PREF_FIRST_START_TUTORIAL"
    const val TORSO_TEST_CASE ="TORSO_TEST_CASE"
    const val ENABLED ="Enabled"
    const val DISABLED ="Disabled"
    const val SNACKBAR_DURATION_IN_MS: Int = 2000
    const val DISABLE_IMAGING_WHEN_CHARGED ="DISABLE_IMAGING_WHEN_CHARGED"
    const val DEFAULT_BRIGHTNESS_LEVEL = 204
    const val MINIMUM_BRIGHTNESS_LEVEL = 52
    const val SNACKBAR_MARGIN_DEVIATION = 24
    const val BLACK_COLOR_HEX: String = "#000000"
    const val GREY_COLOR_HEX: String = "#515151"
    const val LIGHT_BLACK_COLOR_HEX: String = "#757575"
    const val UI_ALERT_WARNING_ICON_SIZE = 45
    const val TIME_CONTROLLER_FIRST_LEGEND_POINT_X: Int = 60
    const val TIME_CONTROLLER_SECOND_LEGEND_POINT_X: Int = 200
    const val TIME_CONTROLLER_LEGEND_POINT_Y: Int = 30
    const val IMAGING_DEFAULT_BRIGHTNESS_LEVEL: Float = 0.8F
    const val SYSTEM_DEFAULT_BRIGHTNESS_LEVEL: Float = -1.0F
    const val ULTRASOUND_SCALE_FACTOR: Float = 1.0F
    const val NETWORK_WATCH_SERVICE_START_DELAY: Long = 5000
    const val ENABLE_ALPHA: Float = 1.0F
    const val DISABLE_ALPHA_HALF: Float = 0.5F
    const val DISABLE_ALPHA: Float = 0.38F
    const val RULER_NO_VALUE: Float = -1.0F
    const val BASIC_CONTROL_FRAGMENT_TAG: String = "BasicControlsFragment"
    const val TERTIARY_PINK_COLOR: String = "#e8707f"
    const val MINIMUM_WIDTH_FOR_SPLIT_SCREEN_MODE: Int = 49
    const val DEVICE_WIDTH_MULTI_WINDOW_MODE: String = "DEVICE_WIDTH_MULTI_WINDOW_MODE"
    const val DEVICE_HEIGHT_MULTI_WINDOW_MODE: String = "DEVICE_HEIGHT_MULTI_WINDOW_MODE"
    const val TOOLBAR_HEIGHT_MULTI_WINDOW_MODE: String = "TOOLBAR_HEIGHT_MULTI_WINDOW_MODE"
    const val CONTROL_FRAGMENTS_HEIGHT_MULTI_WINDOW_MODE: String = "CONTROL_FRAGMENTS_HEIGHT_MULTI_WINDOW_MODE"
    const val IMAGE_WIDTH_MULTI_WINDOW_MODE: String = "IMAGE_WIDTH_MULTI_WINDOW_MODE"
    const val IMAGE_HEIGHT_MULTI_WINDOW_MODE: String = "IMAGE_HEIGHT_MULTI_WINDOW_MODE"
    const val AI_CHANGES_ROW_COUNT_GRIDLAYOUT: Int = 2
    const val GRID_COLUMN_SPAN_FULL_SCREEN = 4
    const val GRID_COLUMN_SPAN_SPLIT_SCREEN = 2

    // Language
    const val ENGLISH = "en" // English (EN) - English
    const val DANISH = "da" //  Danish (DA) - Dansk
    const val DUTCH = "nl" // Dutch (NL) - Nederlands
    const val FRENCH = "fr" //  French (FR) - Francais
    const val GERMAN = "de" //  German (DE) - Deutsch
    const val ITALIAN = "it" // Italian (IT) - italiano
    const val NORWEGIAN = "nb" //  Norway (NO) - Norsk bokmal
    const val PORTUGESE_BRAZIL = "pt" //  Portuguese (PT_BR) - português
    const val SPANISH = "es"  //  Spanish (ES) - español
    const val SWEDISH = "sv" //  Swedish (SV) - svenska
    const val RUSSIAN = "ru"    // Русский

    const val UNZOOM_SCALE = 1.0F

    const val TEST_CLIP_BIPLANE_A4C = "BiplaneA4C.thor"
    const val TEST_CLIP_BIPLANE_A2C = "BiplaneA2C.thor"
    const val TEST_CLIP_A4C_ECG = "ECG_BiplaneA4C.thor"
    const val TEST_CLIP_A2C_ECG = "ECG_BiplaneA2C.thor"
    const val INCH_HIGHEST_POSSIBLE_VALUE = 11.99F
    const val MAX_HEIGHT_IN_CMS_POSSIBLE_VALUE = 1005.81F
    const val MAX_WEIGHT_IN_KG_POSSIBLE_VALUE = 999.00F
    const val MAX_WEIGHT_IN_LBS_POSSIBLE_VALUE = 2202.42F
    const val MAX_HEIGHT_FEET_POSSIBLE_VALUE = 32
    const val CONSTANT_ZERO_STRING = "0"
    const val CONSTANT_ZERO_FLOAT_STRING = "0.0"
    const val EXAM_WEIGHT_UNIT_INDEX_0 = 0
    const val EXAM_WEIGHT_UNIT_INDEX_1 = 1
    const val EXAM_HEIGHT_UNIT_INDEX_0 = 0
    const val EXAM_HEIGHT_UNIT_INDEX_1 = 1
    const val SPECIAL_CHAR_POPUP_TIMER: Long = 4000
    const val SPECIAL_CHAR_POPUP_DELAY:Long =  400


    const val MEDED_UPLOAD_STATUS_NON: Int = 0

    const val MEDED_SUBMIT_STATUS_NON: Int = 0

    const val LOGIN_META_AUTH_REALM = "authRealm"
    const val LOGIN_META_ID = "id"
    const val LOGIN_META_DOMAIN_NAME = "domainName"
    const val LOGIN_META_CODE = "code"
    const val LOGIN_META_CLIENT_ID = "clientId"
    const val LOGIN_META_AUTH_URL = "keycloakAuthURL"
    const val LOGIN_META_TENANT_NAME = "tenantName"
    const val LOGIN_META_JSON = "keycloakJson"
    const val LOGIN_META_UIURL = "uiURL"
    const val LOGIN_META_TENANT_DISPLAY_NAME = "tenantDisplayName"
    const val LOGIN_META_TENANT_TYPE = "tenantType"

    const val PREF_CURRENT_LOGGED_IN_USER = "PREF_CURRENT_LOGGED_IN_USER"
    const val PREF_CACHE_CREDENTIALS = "pref_cache_credentials"

    const val USER_MAIL_ANONYMOUS = "anonymous@locale.com"

    const val MED_ED_INITIAL_EXAM = "Initial Exam"

    const val SHOW_SCAN_ID = "SHOW_SCAN_ID"
    const val SCAN_ID_Exam = "SCAN_ID_Exam"

    const val TRUE: String = "true"
    const val FALSE: String = "false"

    const val TYPE_SEND_DATA_CLOUD: Int = 3

    const val SELECTED_SCAN_ID = "SELECTED_SCAN_ID"
    const val SELECTED_MEDIA_ID = "SELECTED_MEDIA_ID"

    const val MED_ED_SAS_URL = "MED_ED_SAS"
    const val KEY_REFRESH_TOKEN = "refresh_token"
    const val ERROR_SERVER_TIMEOUT = "timeout"

    const val IS_SUBMIT_TRUE = 1
    const val IS_SUBMIT_FALSE = 0

    const val MAX_EMBED_FRAMES_SELECTION = 6
    const val EXAM_EXPORT_SNACK_BAR_TIMEOUT : Int = 4000
    const val FONT_SIZE_SCAN_REPORT_MULTI = 16.7F
    const val FONT_SIZE_SCAN_REPORT = 26.7F


    //Decimal patterns for annotations value
    const val DEFAULT_PATTERN_DIST = "0.00"
    const val DEFAULT_PATTERN_TIME = "#0.##"
    const val DEFAULT_PATTERN_SLOPE = "#0.##"
    const val DEFAULT_PATTERN_AREA = "##0.00"
    const val DEFAULT_PATTERN_FOR_INITIAL_DIST = "0.00"
    const val DEFAULT_PATTERN = "#.##"
    const val SAFETY_INFO_DISPLAY_PATTERN = "0.0#"
    const val COLOR_BAR_INFO_FREQ_DISPLAY_PATTERN = "0.0"
    const val COLOR_BAR_INFO_DISPLAY_PATTERN = "#.##"
    const val DOPPLER_SAFETY_INFO_FREQ_DISPLAY_PATTERN = "0.0"
    const val DEFAULT_PATTERN_SINGLE_DIGIT = "#.#"


    const val PROBE_TORSO_ONE = 4
    const val PROBE_TORSO_THREE = 3
    const val PROBE_LEXSA = 5
    const val PROBE_INVALID = -99
    const val DATABASE_FILE_EXTENSION = ".db"

    const val AUTO_FREEZE_TIMEOUT_60M = (60*60*1000).toLong()
    const val TCD_AUTO_FREEZE_TIMEOUT_3M = (3*60*1000).toLong()
    const val PREF_AUTO_FREEZE = "AutoFreeze"
    const val DEFAULT_PREF_EF_RECORD_CLIP_TYPE: Boolean = false
    const val DEFAULT_PREF_TORSO_TEST_CASE: Boolean = true
    const val DEFAULT_PREF_DISABLE_IMAGING_WHEN_CHARGED: Boolean = true
    const val DEFAULT_PREF_AUTO_FREEZE: Boolean = false
    const val KEY_IS_ARCHIVED = "IS_ARCHIVED"
    const val LEGEND_COLLAPSE = "collapse"
    const val LEGEND_EXPAND = "expand"
    const val AI_IMG_CONTROL_PANEL_WIDTH = 400

    const val TEXT_STYLE_BOLD: String = "bold"
    const val TEXT_STYLE_NORMAL: String = "normal"

    const val TORSO_TYPE: String = "Torso1, USB"
    const val TORSO_3_TYPE: String = "Torso3"
    const val LEXSA_TYPE: String = "Lexsa"
    const val TORSO_PCIe: String = "Torso1"

    const val DEBUG_PROBE_TORSO_ONE = 2147483652
    const val DEBUG_PROBE_TORSO_THREE = 2147483651
    const val DEBUG_PROBE_LEXSA = 2147483653

    const val PREF_ELEMENT_TEST = "ElementTest"

    const val ORGAN_GLOBAL_ID_HEART_FREE_FLOW = 0
    const val ORGAN_GLOBAL_ID_LUNG_FREE_FLOW = 1
    const val ORGAN_GLOBAL_ID_ABDOMEN_FREE_FLOW = 2
    const val ORGAN_GLOBAL_ID_MSK_LINEAR = 3
    const val ORGAN_GLOBAL_ID_NERVE_LINEAR = 4
    const val ORGAN_GLOBAL_ID_VASCULAR_LINEAR = 5
    const val ORGAN_GLOBAL_ID_LUNG_LINEAR = 6
    const val ORGAN_GLOBAL_ID_OB = 7
    const val ORGAN_GLOBAL_ID_GYN= 8
    const val ORGAN_GLOBAL_ID_BLADDER = 9
    const val ORGAN_GLOBAL_ID_TCD = 10

    const val TORSO_TOTAL_TEST = 64
    const val LEXSA_TOTAL_TEST = 128

    const val FLIP_LINEAR_ORIENTATION = false

    const val SURFACEVIEW_HEIGHT = "SURFACEVIEW_HEIGHT"

    const val TIS_LOWER_LIMIT = 0.01F
    const val SINGLE_DIGIT_DECIMAL_FORMAT = "%1.1f"
    const val DOUBLE_DIGIT_DECIMAL_FORMAT = "%1.2f"

    const val ORGAN_MSK: String = "MSK"
    const val ORGAN_NERVE: String = "Nerve"
    const val ORGAN_VASCULAR: String = "Vascular"
    const val ARRAY_SIZE_TWO = 2
    const val ARRAY_SIZE_THREE = 3

    const val PREF_ENABLE_SPLIT_SCREEN = "EnableSplitScreen"
    const val PREF_CONNECTION_ERROR = "PREF_CONNECTION_ERROR_WARN"

    const val DEFAULT_PREF_ENABLE_SPLIT_SCREEN: Boolean = false
    const val DEFAULT_LICENSE_ENABLE: Boolean = true
    const val DIV_BY_100 = 100F

    const val MAX_VALUE_99999F = 99999f
    const val MAX_VALUE_9999F = 9999f
    const val MAX_VALUE_999F = 999f
    const val MAX_VALUE_99F = 99f
    const val MAX_VALUE_9F = 9f
    const val IS_PROBE_REGISTERED_IN_ABOUT = "IS_PROBE_REGISTERED_IN_ABOUT"
    const val DEFAULT_PROBE_REGISTERED_IN_ABOUT = false
    const val MAX_MATRIX_SIZE = 6

    // Variable to avoid multiple click at same time interval
    const val DEBOUNCE_TIME = 500L
    const val TORSO_INDICATOR = "T"
    const val LINEAR_INDICATOR = "L"
    const val MAX_PRF_ARRAY_INDEX: Int = 3
    const val MAX_VALUE_FOR_AREA_CIRCUMFERENCE = "999.99"
    const val MIN_VALUE_FOR_AREA_CIRCUMFERENCE = "9"
    const val CHARACTER_AFTER_CIRCUMFERENCE_FOR_MARGIN = 2
    const val CHARACTER_AFTER_SEPARATOR_FOR_MARGIN = 3
    const val DEFAULT_DEBOUNCE_TIME:Long = 0L

    const val A2C_ED = "A2C_ED"
    const val A4C_ED = "A4C_ED"
    const val A4C_ES = "A4C_ES"
    const val A2C_ES = "A2C_ES"
    const val EF_RESULT = "EFResult"
    const val MAIN_ACTIVITY_SNACKBAR_TIMEOUT: Int = 4000
    const val MAIN_ACTIVITY_EXPORT_ARCHIVE_COMBO_SNACKBAR_TIMEOUT: Int = 10000
    const val VIEW_VISIBILITY_DELAY:Long = 200
    const val NO_OF_FILES_PER_EF_CLIP_IN_ARCHIVE_EXPORT: Int = 2
    const val DEACTIVE_PROFILE:Int = 0
    const val ACTIVE_PROFILE:Int = 1
    const val DELAY_IN_FOCUS_OF_EDITTEXT_FOR_REPORT_ADD_TEXT_DIALOG: Long = 350
    const val ECG_BIPLANEA4C = "ECG_BiplaneA4C"
    const val BIPLANEA4C ="BiplaneA4C"
    const val ECG_BIPLANEA2C = "ECG_BiplaneA2C"
    const val BIPLANEA2C ="BiplaneA2C"
    const val EXAM_IN_PROGRESS_TIME_OUT =  29 * 60 * 1000 //29 Minutes
    const val CLIP_DURATION = "30000"
    const val TIME_ELAPSE = 10
    const val TOTAL_TIME_ELAPSE = 100
    const val AI_IMAGING_FRAGMENT_BACKGROUND_DELAY: Long = 300
    const val ONE_SEC_IN_MS: Long = 1000
    const val SPLINE_ARRAY_SIZE: Int = 28

    const val CONTROL_POINTS_LIST_MIN_SIZE: Int = 14
    const val AI_EXAM_EDIT_SNACKBAR_TIMEOUT: Int = 4000
    const val AI_EXAM_EDIT_SNACKBAR_TEXTSIZE: Float = 16F
    const val AI_EXAM_EDIT_SNACKBAR_MARGIN_IN_DP: Int = 50
    const val THOR_FILE_EXTENSION: String = ".thor"
    const val A4C_GUIDE_PERCENT_WHEN_A4C_SELECTED: Float = 0.2f

    const val A2C_GUIDE_PERCENT_WHEN_A4C_SELECTED: Float = 0.66f
    const val A4C_GUIDE_PERCENT_WHEN_A2C_SELECTED: Float = 0.11f
    const val A2C_GUIDE_PERCENT_WHEN_A2C_SELECTED: Float = 0.56f
    const val COLOR_MAP_VELOCITY_CONSTANT: Float = 100.0F
    const val ABSTRACTDISPLAYFRAGMENT_SNACKBAR_TEXTSIZE: Float = 16F
    const val ABSTRACTDISPLAYFRAGMENT_SNACKBAR_ACTIONSIZE: Float = 20F
    const val ABSTRACTDISPLAYFRAGMENT_SNACKBAR_MARGIN_IN_DP: Int = 28
    const val ABSTRACTDISPLAYFRAGMENT_SNACKBAR_HOLDER_HEIGHT_IN_DP: Int = 64
    const val ABSTRACTDISPLAYFRAGMENT_SNACKBAR_HOLDER_BOTTOM_MARGIN_IN_DP: Int = 86
    const val ABSTRACTDISPLAYFRAGMENT_EXPORT_SNACKBAR_TIMEOUT: Int = 4000
    const val ABSTRACTDISPLAYFRAGMENT_ANIMATION_STARTOFFSET: Long = 1500
    const val ABSTRACTDISPLAYFRAGMENT_ANIMATION_DURATION: Long = 1000
    const val PRF_READABLE_CONSTANT: Int = 1000
    const val ANGLE_360 = 360.0f


    // Arrow Width
    const val ARROW_WIDTH: Int = 150
    const val DATABASE_TORSO = "thor.db"
    const val DATABASE_LINEAR = "l38.db"
    const val MM_TO_CM = 0.1
    const val ANGLE_TO_DEGREE: Float = 180f/Math.PI.toFloat()
    const val userValueColumnNumber = 0
    const val dividerColumnNumber = 1
    const val aiValueColumnNumber = 2
    const val visible = false
    const val gone = true

    const val Y0_PARAM_WIN_ARRAY: Float = 0.072165F

    const val MIN_TIME_FOR_DOPPLER_LIVE = 500L

    const val MESURED_HEIGHT_DOPPLER_BASELINE_VIEW = 870 // based on s7
    const val DOPPLER_LIVE_SELECTION_LAYOUT_DISPLAY_DELAY = 200L
    const val COUNT_DOWN_INTERVAL = 10L

    const val PW_AUDIO_GAIN = 50
    const val PW_DOPPLER_GAIN = 50
    const val TDI_AUDIO_GAIN = 0
    const val TDI_DOPPLER_GAIN = 50

    const val DOPPLER_PRF_CHANGE_ID = 0
    const val DOPPLER_WALL_CHANGE_ID = 1
    const val DOPPLER_AUDIO_GAIN_CHANGE_ID = 2
    const val DOPPLER_DOPPLER_GAIN_CHANGE_ID = 3
    const val DOPPLER_SWEEP_SPEED_CHANGE_ID = 4
    const val DOPPLER_PRF_UI_CHANGE_ID = 5

    const val SAFETY_INFO_PW_MODE = "PW"
    const val SAFETY_INFO_CW_MODE = "CW"
    const val SAFETY_INFO_TDI_MODE = "TDI"

    const val GATE_FOCAL_POINT_ADJUST_SNACKBAR_TIMEOUT = 3000
    const val DEFAULT_DOPPLER_GATE_AND_ROI_COUPLED_SETTINGS = false
    const val VTI_SNACKBAR_TIMEOUT = 5000
    const val MM_10 = 10

    const val LESS_THAN_SYMBOL = "<"
    const val UNIT_SEC = " s"
    const val UNIT_CM_S = " cm/s"
    const val UNIT_CM = " cm"

    const val TIME_LEGEND_LOWER_RANGE = "0.01"
    const val BLUETOOTH_DEVICE_VOLUME_DELAY: Long = 4000
    const val REGULAR_REGISTRATION_BACKGROUND_DELAY: Long = 4000
    const val SECOND_LINE_HEIGHT_MULTIPLIER: Int = 8
    const val MILI_SECOND_LINE_HEIGHT_MULTIPLIER: Int = 4
    const val ENC = ".enc"
    const val CERTIFICATE_DIR = "tlsCertificates"
    const val LICENSE_DIR = "licenses"
    const val PREF_LIC_LAST_PREFIX = "PREF_LIC_TIME_"
    const val PREF_LIC_FIRST_REMINDER_LIST_PREFIX = "PREF_LIC_FIRST_REMINDER_LIST_"
    const val PREF_LIC_SECOND_REMINDER_LIST_PREFIX = "PREF_LIC_SECOND_REMINDER_LIST_"
    const val LONG_TOAST_DURATION: Int = 5
    const val APP_USB_LICENSE_DIR: String = "EchonousLicense"
    const val APP_USB_TLS_CERTIFICATE_DIR: String = "EchonousTls"

    const val EF_ORGAN_INDEX = 0

    const val LICENSE_GENERATED_TIME = "LICENSE_GENERATED_TIME"
    const val LAST_CHECK_IN_DATE = "LAST_CHECK_IN_DATE"

    const val COUNT_DOWN_100L_INTERVAL = 100L
    const val INITIAL_PROGRESS = "0"
    const val MIN_PROGRSSBAR = 0
    const val MAX_PROGRSSBAR = 100
    const val APP_RESTART_DELAY: Long = 500
    const val CLOUD_SELECTED_ENV: String = "CLOUD_SELECTED_ENV"

    const val HALF_SEC_IN_MS: Long = 500

    const val ACTIVE_MODE_POSITION = 0F

    const val AUTO_CAPTURE_CLIP_DURATION = 3000
    const val EF_CLIP_DURATION = "5000"
    const val CLIP_RECORDING_3_4TH_PROGRESS = 75
    const val CLIP_RECORDING_FULL_PROGRESS = 100
    const val ANDROID_11_SDK_INT = 30
    const val SEC_1 = 60000

    const val US2_AUTO_UPLOAD_IMAGES_CLIPS: Int = 1
    const val US2_AUTO_UPLOAD_EXAM_COMPLETE: Int = 2

    const val US2_CLOUD_STATUS_NON: Int = 0
    const val US2_CLOUD_STATUS_WAIT: Int = 1
    const val US2_CLOUD_STATUS_IN_PROGRESS: Int = 2
    const val US2_CLOUD_STATUS_COMPLETED: Int = 3
    const val US2_CLOUD_STATUS_FAIL: Int = 4
    const val US2_CLOUD_DISCARD: Int = 5

    const val PREF_US2_ENV_CUSTOMER_NAME = "PREF_US2_ENV_CUSTOMER_NAME"
    const val PREF_US2_ENV_TRAIL = "PREF_US2_ENV_TRAIL"
    const val PREF_US2_SHOW_EXPIRED_SUBS_ALERT = "PREF_US2_SHOW_EXPIRED_SUBS_ALERT"
    const val PREF_US2_SHOW_SECOND_SUBS_WARN = "PREF_US2_SHOW_SECOND_SUBS_WARN"
    const val PREF_US2_SHOW_FIRST_SUBS_WARN = "PREF_US2_SHOW_FIRST_SUBS_WARN"
    const val US2_REPORT_STATUS_DEFAULT = 0

    const val US2_REPORT_STATUS_LOADING = 1
    const val US2_REPORT_STATUS_READY = 2
    const val US2_REPORT_UNIT_SQUARE = 2

    const val US2_REPORT_UNIT_CUBE = 3
    const val US2_REPORT_UNIT_POWER_4 = 4
    const val US2_REPORT_UNIT_POWER_5 = 5
    const val US2_REPORT_STATUS_CHECK_TIME_DELAY_10_SECOND = 10000L

    const val US2_REPORT_STATUS_CHECK_TIME_DELAY_3_SECOND = 3000L
    const val GENDER_MALE = "M";

    const val GENDER_FEMALE = "F";
    const val MEASUREMENT_ONE_DECIMAL_PLACE : Int = 1
    const val MEASUREMENT_TWO_DECIMAL_PLACES : Int = 2
    const val US2_OFFLINE_SNACKBAR_TIMEOUT = 2000
    const val MILLIS_MULTIPLIER: Long = 1000

    const val REPORT_LOCK_ERROR: String = "report lock failed"
    const val BLANK_SPACE = " "
    const val REPORT_LOCK_UNAUTHORIZED = "report lock failed unauthorized"
    const val CONSTANT_ZERO_DOUBLE = 0.0
    const val PREF_PROTOCOL_JSON_DATA ="PREF_PROTOCOL_JSON_DATA"

    const val PREF_KEY_US2_ENV_FIRST_TIME = "PREF_KEY_US2_ENV_FIRST_TIME"
    const val PREF_US2_ENV_URL = "PREF_US2_ENV_URL"

    const val FLIP_ORIENTATION_VALUE = -1.0F

    const val NEGATIVE_ONE = -1

    const val ZERO = 0
    const val ONE = 1
    const val TWO = 2
    const val THREE = 3
    const val FOUR = 4
    const val FIVE = 5
    const val SIX = 6
    const val SEVEN = 7
    const val EIGHT = 8
    const val NINE = 9
    const val TEN = 10
    const val ELEVEN = 11
    const val TWELVE = 12
    const val THIRTEEN = 13
    const val FOURTEEN = 14

    const val TWENTY = 20
    const val HUNDRED = 100
    //For US2 Report screen because clip view has 720px height
    const val US2_REPORT_MARGIN_TOP_OF_ECG_DA_TIME_SCALE_VIEW_MODE_6_1_1: Int = 90

    const val US2_REPORT_MARGIN_TOP_OF_ECG_DA_TIME_SCALE_VIEW_MODE_2_1_1: Int = 180
    const val US2_REPORT_MARGIN_TOP_OF_ECG_DA_TIME_SCALE_VIEW_MODE_1_1: Int = 360
    //For US2 Report Edit screen because clip view has 937px height
    const val US2_REPORT_EDIT_MARGIN_TOP_OF_ECG_DA_TIME_SCALE_VIEW_MODE_6_1_1: Int = 117

    const val US2_REPORT_EDIT_MARGIN_TOP_OF_ECG_DA_TIME_SCALE_VIEW_MODE_2_1_1: Int = 234
    const val US2_REPORT_EDIT_MARGIN_TOP_OF_ECG_DA_TIME_SCALE_VIEW_MODE_1_1: Int = 468
    const val POSITION_ZERO = 0

    const val ORGAN_ID_HEART_FREE_FLOW = 0
    const val ORGAN_ID_LUNG_FREE_FLOW = 1
    const val ORGAN_ID_ABDOMEN_FREE_FLOW = 2
    const val ORGAN_ID_MSK_LINEAR = 0
    const val ORGAN_ID_NERVE_LINEAR = 1
    const val ORGAN_ID_VASCULAR_LINEAR = 2
    const val ORGAN_ID_LUNG_LINEAR = 3
    const val ORGAN_ID_OB = 3
    const val ORGAN_ID_GYN = 4
    const val ORGAN_ID_BLADDER = 5
    const val ORGAN_ID_TCD = 6


    const val KEY_US2_REPORT_VIDEO="Us2ReportVideoDir"

    const val KEY_US2_REPORT_DELETE_EXAM_ID="_US2_Delete"
    const val KEY_US2_REPORT_NOTE_DETAIL: String = "notes"
    const val MIN_CONFIDENCE_MEASUREMENT = 3

    const val TEXT_EDITOR_DELAY_TIME: Long = 350
    const val DOUBLE_CLICK_TIME_DELTA: Long = 500
    const val ROTATE_180_DEGREE: Int = 180
    const val TOTAL_MEDIA_COUNT_ZERO = 0
    const val MAX_PROGRESS_COUNT: Int = 100
    const val MIN_PROGRESS_COUNT: Int = 0
    const val TYPE_US2: Int = 4
    const val LINE = "line"
    const val SANS_SERIF = "sans-serif"
    const val SANS_SERIF_LIGHT = "sans-serif-light"
    const val SANS_SERIF_MEDIUM = "sans-serif-medium"
    // Us2.ai Constants
    const val PROGRESS_PERCENT_95: Int = 95


    const val PATIENT_ORIENTATION = "A\\P"
    const val UNKNOWN = "Unknown"
    const val PREF_EF_RECORD_CLIP_TYPE_TEST_DATA_SET_1 = "PREF_EF_RECORD_CLIP_TYPE_TEST_DATA_SET_1"

    const val PREF_EF_RECORD_CLIP_TYPE_TEST_DATA_SET_2 = "PREF_EF_RECORD_CLIP_TYPE_TEST_DATA_SET_2"
    const val PREF_EF_RECORD_CLIP_TYPE_TEST_DATA_SET_3 = "PREF_EF_RECORD_CLIP_TYPE_TEST_DATA_SET_3"
    const val PREF_EF_RECORD_CLIP_TYPE_TEST_DATA_SET_4 = "PREF_EF_RECORD_CLIP_TYPE_TEST_DATA_SET_4"
    const val PREF_EF_RECORD_CLIP_TYPE_TEST_DATA_SET_EXIST = "PREF_EF_RECORD_CLIP_TYPE_TEST_DATA_SET_EXIST"
    const val PREF_EF_RECORD_CLIP_TYPE_TEST_DATA_SET_SDCARD_EXIST = "PREF_EF_RECORD_CLIP_TYPE_TEST_DATA_SET_SDCARD_EXIST"
    const val ECHONOUS_US2_ROOT_DIR_TEST_DATA_SET = "/US2_AI/TEST_DATA"

    const val ECHONOUS_ROOT_DIR_TEST_DATA_SET_1 = "Test_Data_Set_1"
    const val ECHONOUS_ROOT_DIR_TEST_DATA_SET_2 = "Test_Data_Set_2"
    const val ECHONOUS_ROOT_DIR_TEST_DATA_SET_3 = "Test_Data_Set_3"
    const val ECHONOUS_ROOT_DIR_TEST_DATA_SET_4 = "Test_Data_Set_4"
    const val ECHONOUS_ROOT_DIR_TEST_DATA_SET_5 = "Test_Data_Set_5"
    const val TEST_DATA_SET_1_CLIP_2D_1 = "TDS1_Clip_Heart_2D_1.thor"

    const val TEST_DATA_SET_1_CLIP_2D_2 = "TDS1_Clip_Heart_2D_2.thor"
    const val TEST_DATA_SET_1_CLIP_2D_3 = "TDS1_Clip_Heart_2D_3.thor"
    const val TEST_DATA_SET_1_IMAGE_2D_M = "TDS1_Image_Heart_2D_M.thor"
    const val TEST_DATA_SET_1_IMAGE_CW = "TDS1_Image_Heart_CW.thor"
    const val TEST_DATA_SET_1_IMAGE_PW = "TDS1_Image_Heart_PW.thor"
    const val TEST_DATA_SET_1_IMAGE_TDI_1 = "TDS1_Image_Heart_TDI_1.thor"
    const val TEST_DATA_SET_1_IMAGE_TDI_2 = "TDS1_Image_Heart_TDI_2.thor"
    const val TEST_DATA_SET_1_CLIP_2D_Biplane_A4C = "TDS1_Clip_Heart_2D_Biplane_A4C.thor"
    const val TEST_DATA_SET_1_CLIP_2D_Biplane_A2C = "TDS1_Clip_Heart_2D_Biplane_A2C.thor"
    const val TEST_DATA_SET_2_CLIP_2D_1 = "TDS2_Clip_Heart_2D_1.thor"

    const val TEST_DATA_SET_2_CLIP_2D_2 = "TDS2_Clip_Heart_2D_2.thor"
    const val TEST_DATA_SET_2_IMAGE_2D_M = "TDS2_Image_Heart_2D_M.thor"
    const val TEST_DATA_SET_2_IMAGE_CW = "TDS2_Image_Heart_CW.thor"
    const val TEST_DATA_SET_2_IMAGE_PW = "TDS2_Image_Heart_PW.thor"
    const val TEST_DATA_SET_2_IMAGE_TDI_1 = "TDS2_Image_Heart_TDI_1.thor"
    const val TEST_DATA_SET_2_IMAGE_TDI_2 = "TDS2_Image_Heart_TDI_2.thor"
    const val TEST_DATA_SET_3_CLIP_2D_1 = "TDS3_Clip_Heart_2D_1.thor"

    const val TEST_DATA_SET_3_CLIP_2D_2 = "TDS3_Clip_Heart_2D_2.thor"
    const val TEST_DATA_SET_3_CLIP_2D_3 = "TDS3_Clip_Heart_2D_3.thor"
    const val TEST_DATA_SET_3_IMAGE_2D_M = "TDS3_Image_Heart_2D_M.thor"
    const val TEST_DATA_SET_3_IMAGE_CW = "TDS3_Image_Heart_CW.thor"
    const val TEST_DATA_SET_3_IMAGE_PW = "TDS3_Image_Heart_PW.thor"
    const val TEST_DATA_SET_3_IMAGE_TDI_1 = "TDS3_Image_Heart_TDI_1.thor"
    const val TEST_DATA_SET_3_IMAGE_TDI_2 = "TDS3_Image_Heart_TDI_2.thor"
    const val TEST_DATA_SET_4_CLIP_2D_1 = "TDS4_Clip_Heart_2D_1.thor"

    const val TEST_DATA_SET_4_CLIP_2D_2 = "TDS4_Clip_Heart_2D_2.thor"
    const val TEST_DATA_SET_4_CLIP_2D_3 = "TDS4_Clip_Heart_2D_3.thor"
    const val TEST_DATA_SET_4_IMAGE_2D_M = "TDS4_Image_Heart_2D_M.thor"
    const val TEST_DATA_SET_4_IMAGE_CW = "TDS4_Image_Heart_CW.thor"
    const val TEST_DATA_SET_4_IMAGE_PW = "TDS4_Image_Heart_PW.thor"
    const val TEST_DATA_SET_4_IMAGE_TDI_1 = "TDS4_Image_Heart_TDI_1.thor"
    const val TEST_DATA_SET_4_IMAGE_TDI_2 = "TDS4_Image_Heart_TDI_2.thor"
    const val TEST_DATA_SET_5_CLIP_2D_1 = "TDS5_Clip_Heart_2D_1.thor"

    const val TEST_DATA_SET_5_CLIP_2D_2 = "TDS5_Clip_Heart_2D_2.thor"
    const val TEST_DATA_SET_5_CLIP_2D_3 = "TDS5_Clip_Heart_2D_3.thor"
    const val TEST_DATA_SET_5_IMAGE_2D_M = "TDS5_Image_Heart_2D_M.thor"
    const val TEST_DATA_SET_5_IMAGE_CW = "TDS5_Image_Heart_CW.thor"
    const val TEST_DATA_SET_5_IMAGE_Color_PW = "TDS5_Image_Heart_Color_PW.thor"
    const val TEST_DATA_SET_5_IMAGE_PW_1 = "TDS5_Image_Heart_PW_1.thor"
    const val TEST_DATA_SET_5_IMAGE_PW_2 = "TDS5_Image_Heart_PW_2.thor"
    const val TEST_DATA_SET_5_CLIP_CW_2 = "TDS5_Image_Heart_CW_CLIP.thor"
    const val TEST_DATA_SET_5_IMAGE_CW_1 = "TDS5_Image_Heart_CW_1.thor"
    const val EXTENSION_JSON: String = ".json"
    const val ANDROID_12_SDK_INT = 31

    const val NETWORK_WATCH_WORKER_TAG = "NETWORK_WATCH_WORKER"
    const val DAILY_TOMORROW_ALARM = 1000 * 60 * 60 * 24
    const val WEEKLY_ALARM = 1000 * 60 * 60 * 24 * 7
    const val EXAM_AUTO_DELETE_ST = 0

    const val EXAM_AUTO_DELETE_NON_ST = 1
    const val LOGIN_EMAIL = "login_email"
    const val MAGIC_CONST_3600 = 3600L

    const val WEIGHT_UNIT_LBS = "lbs"
    const val ECHONOUS_DIR_US2_REPORT_VIDEO= "$ECHONOUS_DIR_PATH/Us2ReportVideo"

    const val US2_BITMAP_WIDTH = "US2_BITMAP_WIDTH"
    const val US2_BITMAP_HEIGHT = "US2_BITMAP_HEIGHT"
    const val ACCESS_TYPE = "Bearer"

    const val SINGLE_SPACE_STRING = " "

    const val APP_USB_DICOMEF_DIR: String = "EFCLIPS"
    const val APP_USB_DICOMDIR_FILE="DICOMDIR"
    const val APP_USB_DICOM_ROOT="DICOM"
    const val APP_USB_DICOM_DIR: String = "DICOM"
    const val TYPE_DICOM_SR = 2
    const val TYPE_DICOM_CAL_SR=3
    const val TYPE_DICOM_OB_SR=4
    const val TYPE_DICOM_GYN_SR=5
    const val TYPE_DICOM_VASCULAR_SR=6
    const val TYPE_DICOM_ABDOMEN_SR=7
    const val EXPORT_JPEG = 1
    const val EXPORT_DICOM = 2
    const val EXPORT_DICOM_DIR = 3
    const val DICOM_SR = "SR"
    const val DATE_PATTERN_MM_DD_YYYY = "MM-dd-yyyy"
    const val SYNC_DATE_TIME_VALUE = "DATE_TIME_SYNCED_VALUE"
    const val DATE_SHIFT_THRESHOLD = 2
    const val EXT_DICOM = ".dcm"
    const val PREF_DICOM_DIR="DicomPatient"
    // USB Dicom SR export file identifier
    const val SR_US2 = "US2"

    const val DICOM_SR_HEART = "_Heart_SR"
    const val DICOM_SR_OB = "_OB_SR"
    const val DICOM_SR_GYN = "_GYN_SR"
    const val DICOM_SR_VAS = "_Vascular_SR"
    const val DICOM_SR_ABDOMEN = "_Abdomen_SR"

    const val MINUS_ONE_DEPTH = -1.0F
    const val MAX_HEIGHT_US2_MEDIA_PX = 1080

    const val GALE: String = "com.nineteenlabs.gale"

    const val FILE_PROVIDER_AUTHORITY: String = "com.echonous.kosmosapp.provider"
    const val DATA_TYPE_TO_GALE: String = "application/zip"
    const val GALE_DIRECTORY_PATH_RELATIVE: String = "/Echonous/gale"
    const val EXPORT_DIRECTORY_PATH_RELATIVE: String = "/Echonous/export"
    const val PATH_SEPARATOR: String = "/"
    const val CALLER_INTENT_PARAM_FROM_GALE: String = "caller"
    const val PATIENT_INFO_JSON_FROM_GALE: String = "patientInfo"
    const val GALE_CONSTANT_FEMALE: String = "Female"
    const val GALE_CONSTANT_MALE: String = "Male"
    const val GALE_CONSTANT_HEIGHT_FEET: String = "ft"
    const val GALE_CONSTANT_HEIGHT_CM: String = "cm"
    const val GALE_CONSTANT_WEIGHT_LBS: String = "lbs"
    const val GALE_CONSTANT_WEIGHT_KG: String = "kg"
    const val ACTIVE_PROFILE_ID_DEACTIVATED_DUE_TO_GALE: String = "ACTIVE_PROFILE_ID_DEACTIVATED_DUE_TO_GALE"
    const val DEFAULT_VALUE_FOR_ACTIVE_PROFILE_ID_DEACTIVATED_DUE_TO_GALE: Int = -1
    const val DELAY_EXAM_JOB_DATA_OBSERVER: Long = 30

    const val PREF_ENABLE_PACS_DEACTIVATED_DUE_TO_GALE = "PREF_ENABLE_PACS_DEACTIVATED_DUE_TO_GALE"
    const val DEFAULT_PREF_ENABLE_PACS_DEACTIVATED_DUE_TO_GALE: Boolean = false

    //Special Export directory Name inside USB drive
    const val SPECIAL_EXPORT_DIR_DICOM = "DICOM"
    const val SPECIAL_EXPORT_DIR_JPEG =  "MJPEG"
    const val SPECIAL_EXPORT_DIR_THOR =  "THOR"
    const val SPECIAL_EXPORT_DIR_JSON = "JSON"
    const val SPECIAL_EXPORT_DIR_BI_PLANE_EF = "Bi-plane_EF"
    const val SPECIAL_EXPORT_DIR_A4C_EF = "A4C_EF"
    const val SPECIAL_EXPORT_FILE_A4C = "a4c"
    const val SPECIAL_EXPORT_FILE_A2C = "a2c"

    //File extensions
    const val EXT_JPEG = ".jpg"
    const val EXT_THOR = ".thor"
    const val EXT_JSON = ".json"

    const val CRASH_LOG_DIR = "/Crash"
    const val STACKTRACE_EXTENSION = ".stacktrace"

    const val RBT_NONE = 0
    const val RBT_TEMPERATURE = 1
    const val RBT_WATCHDOG = 2
    const val RBT_OVERFLOW = 3
    const val RBT_UNDERFLOW = 4
    const val PREF_DAU_SAFETY_TEST_OPTION: String = "PREF_DAU_SAFETY_TEST_OPTION"
    const val ZERO_FLOAT: Float = 0.0F
    const val DECIMAL = '.'

    const val PLAY_STORE_ANDROID_VENDING = "com.android.vending"
    const val PLAY_STORE_GOOGLE_ANDROID_FEEDBACK = "com.google.android.feedback"
    const val TDS_US2AI_TEST_FILE = "TDS"

    const val KEY_BEARER = "Bearer"

    const val TRANSITION_DEBOUNCE_TIME: Long = 1000L
    const val GREATER_THAN_SYMBOL = ">"
    const val INVALID_ID = -1
    const val DASH_CHARACTER = "_"
    const val KEY_CURRENT_EXAM = "KEY_CURRENT_EXAM"
    const val ZERO_F = 0F
    const val CONSTANT_ZERO_FLOAT = 0.0F

    // WAVEFORM
    const val WF_TWO_PEAK_WAVEFORM = 101
    const val WF_QUERY_Y_COORD = 201

    const val ONE_F = 1.0F
    const val TWO_F = 2.0F
    const val PREF_TOP_FRAGMENT = "PREF_TOP_FRAGMENT"

    const val HEIGHT_UNIT_FEET: String = "feet"
    const val HEIGHT_UNIT_INCHES: String = "inches"
    const val HEIGHT_UNIT_CMS: String = "cms"
    const val WEIGHT_UNIT_KGS: String = "kgs"
    const val HEART_RATE_UNIT = "bpm"

    const val DP_24: Int = 24
    const val DP_28: Int = 28

    const val ZERO_FIVE_F = 0.5F
    const val NEGATIVE_ONE_F = -1F

    const val NEW_TEXT = "Add text"
    const val LBL_TEXT_ID = 1001
    const val VASCULAR_TEXT_ID = 1011
    const val VASCULAR_PROTOCOL_LABEL_ID = 1012
    const val LBL_TEXT_DISPLAY_NAME = "LBLTEXT"
    const val MAX_LABEL_COUNT = 6

    const val STR_MAXPG = "MaxPg"
    const val STR_MEANPG = "MeanPg"
    const val STR_VMAX = "Vmax"
    const val STR_VMEAN = "Vmean"
    const val STR_VMAX_VMEAN = "Vmax   | Vmean"
    const val STR_MEAN_MAX_PG = "MaxPg | MeanPg"
    const val CLIP_DEFAULT_DUR="30000"
    const val PREF_SELECTED_MEDIA_ORGAN_ID = "PREF_SELECTED_MEDIA_ORGAN_ID"
    const val SELECTED_MEDIA_ORGAN_ID: String = "SELECTED_MEDIA_ORGAN_ID"
    const val PHT:Float = 0.293F
    const val LEGEND_START_Y_LEXSA = 40
    const val TEXT_ANNOTATION_SNACKBAR_DELAY = 4000

    const val PERSIST_LAST_MAX_AVG_ACTION = true
    const val PREF_KEY_VTI_SUB_MEASUREMENT_MAX_VALUE = "vti_sub_measurement_max_value_"
    const val DOT_DELIMITER: String = "."

    const val CALC_EXPORT_PATH =  "/Echonous/calc"
    const val TEST_IMAGE_CAL_2D_PLAX = "THUMB_2D_PLAX.jpg"
    const val TEST_IMAGE_CAL_2D_AROTIC_VALVE = "THUMB_2D_AROTIC_VALVE.jpg"
    const val TEST_IMAGE_CAL_2DC_AROTIC_VALVE = "THUMB_2DC_AROTIC_VALVE.jpg"
    const val TEST_IMAGE_CAL_2D_MITRAL_VALVE = "THUMB_2D_MITRAL_VALVE.jpg"
    const val TEST_IMAGE_CAL_2D_RIGHT_HEART = "THUMB_2D_RIGHT_HEART.jpg"
    const val TEST_IMAGE_CAL_2D_IVC = "THUMB_2D_IVC.jpg"
    const val TEST_IMAGE_CAL_PW_DIASTOLOGY = "THUMB_PW_DIASTOLOGY.jpg"
    const val TEST_IMAGE_CAL_PW_MITRAL_VALVE = "THUMB_PW_MITRAL_VALVE.jpg"
    const val TEST_IMAGE_CAL_PW_AROTIC_VALVE = "THUMB_PW_AROTIC_VALVE.jpg"
    const val TEST_IMAGE_CAL_PW_RIGHT_HEART = "THUMB_PW_RIGHT_HEART.jpg"
    const val TEST_IMAGE_CAL_CW_DIASTOLOGY = "THUMB_CW_DIASTOLOGY.jpg"
    const val TEST_IMAGE_CAL_CW_MITRAL_VALVE = "THUMB_CW_MITRAL_VALVE.jpg"
    const val TEST_IMAGE_CAL_CW_AROTIC_VALVE = "THUMB_CW_AROTIC_VALVE.jpg"
    const val TEST_IMAGE_CAL_CW_RIGHT_HEART = "THUMB_CW_RIGHT_HEART.jpg"
    const val TEST_IMAGE_CAL_TDI_DIASTOLOGY = "THUMB_TDI_DIASTOLOGY.jpg"
    const val TEST_IMAGE_CAL_TDI_MITRAL_VALVE = "THUMB_TDI_MITRAL_VALVE.jpg"
    const val TEST_IMAGE_CAL_TDI_RIGHT_HEART = "THUMB_TDI_RIGHT_HEART.jpg"
    const val TEST_IMAGE_CAL_M_MODE = "THUMB_M_M_MODE.jpg"
    const val TEST_IMAGE_CAL_2DC_MITRAL_VALVE = "THUMB_2DC_MITRAL_VALVE.jpg"

    const val TEST_JSON_CAL_2D_PLAX = "calc/CALC_2D_PLAX.json"
    const val TEST_JSON_CAL_2D_AROTIC_VALVE = "calc/CALC_2D_AROTIC_VALVE.json"
    const val TEST_JSON_CAL_2DC_AROTIC_VALVE = "calc/CALC_2DC_AROTIC_VALVE.json"
    const val TEST_JSON_CAL_2D_MITRAL_VALVE = "calc/CALC_2D_MITRAL_VALVE.json"
    const val TEST_JSON_CAL_2D_RIGHT_HEART = "calc/CALC_2D_RIGHT_HEART.json"
    const val TEST_JSON_CAL_2D_IVC = "calc/CALC_2D_IVC.json"
    const val TEST_JSON_CAL_PW_DIASTOLOGY = "calc/CALC_PW_DIASTOLOGY.json"
    const val TEST_JSON_CAL_PW_MITRAL_VALVE = "calc/CALC_PW_MITRAL_VALVE.json"
    const val TEST_JSON_CAL_PW_AROTIC_VALVE = "calc/CALC_PW_AROTIC_VALVE.json"
    const val TEST_JSON_CAL_PW_RIGHT_HEART = "calc/CALC_PW_RIGHT_HEART.json"
    const val TEST_JSON_CAL_CW_DIASTOLOGY = "calc/CALC_CW_DIASTOLOGY.json"
    const val TEST_JSON_CAL_CW_MITRAL_VALVE = "calc/CALC_CW_MITRAL_VALVE.json"
    const val TEST_JSON_CAL_CW_AROTIC_VALVE = "calc/CALC_CW_AROTIC_VALVE.json"
    const val TEST_JSON_CAL_CW_RIGHT_HEART = "calc/CALC_CW_RIGHT_HEART.json"
    const val TEST_JSON_CAL_TDI_DIASTOLOGY = "calc/CALC_TDI_DIASTOLOGY.json"
    const val TEST_JSON_CAL_TDI_MITRAL_VALVE = "calc/CALC_TDI_MITRAL_VALVE.json"
    const val TEST_JSON_CAL_TDI_RIGHT_HEART = "calc/CALC_TDI_RIGHT_HEART.json"
    const val TEST_JSON_CAL_M_MODE = "calc/CALC_M_M_MODE.json"
    const val TEST_JSON_CAL_2DC_MITRAL_VALVE = "calc/CALC_2DC_MITRAL_VALVE.json"

    const val TEST_JSON_ANNOTATIONS_2D_PLAX = "calc/ANNOTATIONS_2D_PLAX.json"
    const val TEST_JSON_ANNOTATIONS_2D_AROTIC_VALVE = "calc/ANNOTATIONS_2D_AROTIC_VALVE.json"
    const val TEST_JSON_ANNOTATIONS_2DC_AROTIC_VALVE = "calc/ANNOTATIONS_2DC_AROTIC_VALVE.json"
    const val TEST_JSON_ANNOTATIONS_2D_MITRAL_VALVE = "calc/ANNOTATIONS_2D_MITRAL_VALVE.json"
    const val TEST_JSON_ANNOTATIONS_2D_RIGHT_HEART = "calc/ANNOTATIONS_2D_RIGHT_HEART.json"
    const val TEST_JSON_ANNOTATIONS_2D_IVC = "calc/ANNOTATIONS_2D_IVC.json"
    const val TEST_JSON_ANNOTATIONS_PW_DIASTOLOGY = "calc/ANNOTATIONS_PW_DIASTOLOGY.json"
    const val TEST_JSON_ANNOTATIONS_PW_MITRAL_VALVE = "calc/ANNOTATIONS_PW_MITRAL_VALVE.json"
    const val TEST_JSON_ANNOTATIONS_PW_AROTIC_VALVE = "calc/ANNOTATIONS_PW_AROTIC_VALVE.json"
    const val TEST_JSON_ANNOTATIONS_PW_RIGHT_HEART = "calc/ANNOTATIONS_PW_RIGHT_HEART.json"
    const val TEST_JSON_ANNOTATIONS_CW_DIASTOLOGY = "calc/ANNOTATIONS_CW_DIASTOLOGY.json"
    const val TEST_JSON_ANNOTATIONS_CW_MITRAL_VALVE = "calc/ANNOTATIONS_CW_MITRAL_VALVE.json"
    const val TEST_JSON_ANNOTATIONS_CW_AROTIC_VALVE = "calc/ANNOTATIONS_CW_AROTIC_VALVE.json"
    const val TEST_JSON_ANNOTATIONS_CW_RIGHT_HEART = "calc/ANNOTATIONS_CW_RIGHT_HEART.json"
    const val TEST_JSON_ANNOTATIONS_TDI_DIASTOLOGY = "calc/ANNOTATIONS_TDI_DIASTOLOGY.json"
    const val TEST_JSON_ANNOTATIONS_TDI_MITRAL_VALVE = "calc/ANNOTATIONS_TDI_MITRAL_VALVE.json"
    const val TEST_JSON_ANNOTATIONS_TDI_RIGHT_HEART = "calc/ANNOTATIONS_TDI_RIGHT_HEART.json"
    const val TEST_JSON_ANNOTATIONS_M_MODE = "calc/ANNOTATIONS_M_M_MODE.json"
    const val TEST_JSON_ANNOTATIONS_2DC_MITRAL_VALVE = "calc/ANNOTATIONS_2DC_MITRAL_VALVE.json"

    const val TEST_DATA_CAL_2D_MODE_IMAGE = "TEST_DATA_CAL_2D_MODE_IMAGE"
    const val TEST_DATA_CAL_2D_COLOR_MODE_IMAGE = "TEST_DATA_CAL_2D_COLOR_MODE_IMAGE"
    const val TEST_DATA_CAL_M_MODE_IMAGE = "TEST_DATA_CAL_M_MODE_IMAGE"
    const val TEST_DATA_CAL_PW_MODE_IMAGE = "TEST_DATA_CAL_PW_MODE_IMAGE"
    const val TEST_DATA_CAL_CW_MODE_IMAGE = "TEST_DATA_CAL_CW_MODE_IMAGE"
    const val TEST_DATA_CAL_TDI_MODE_IMAGE = "TEST_DATA_CAL_TDI_MODE_IMAGE"
    const val CAL_DEMO_EXAM_FIRST_NAME = "Cal Demo"
    const val CAL_DEMO_EXAM_LAST_NAME = "Study"
    const val BACK_SLASH = "/"

    const val ORGAN_GLOBAL_ID_NON: Int = -1
    const val MOVE_LEFT = 1
    const val MOVE_RIGHT = 2
    const val MOVE_NON = 0
    const val ORGAN_OB: String = "OB"
    const val ORGAN_GYN: String = "Gyn"
    const val ORGAN_BLADDER: String = "Bladder"
    const val ORGAN_TCD: String = "TCD"

    const val FETUS_1 = 1
    const val FETUS_2 = 2
    const val FETUS_3 = 3

    const val PAST_ONE_YEAR = -1
    const val FUTURE_ONE_YEAR = 1

    const val COLON = ":"

    const val GLOBAL_ID ="GLOBAL_ID"
    const val OB_GUIDELINE_PERCENTAGE = 0.7F
    const val GYN_GUIDELINE_PERCENTAGE = 0.6F

    const val FETUS_ID_A = "A"
    const val PROTOCOL_FETUS_A = "10"
    const val FETUS_ID_B = "B"
    const val PROTOCOL_FETUS_B = "11"
    const val FETUS_ID_C = "C"
    const val PROTOCOL_FETUS_C = "12"

    const val STATIC_REPORT_LEFT_PANEL_WEIGHT = 0.85F
    const val PREF_ELEMENT_TEST_INTERVAL="ELEMENT_INTERVAL"
    const val ELEMENT_TEST_DEFAULT_INTERVAL = 480

    const val COMMA_DELIMITER: String = ","
    const val APP_RESTART_DELAY_CONFIG_CHANGED = 200L

    val MP4_RENDERING_ISSUE_EXPORT_IN_DEVICE: Array<String> = arrayOf(
        "gts8wifi",
        "gts8",
        "gts8pwifi",
        "gts8p",
        "gts8uwifi",
        "gts8u"
    )
    val MP4_RENDERING_ISSUE_EXPORT_IN_DEVICE_MODEL: Array<String> = arrayOf(
        "SM-X700",
        "SM-X706",
        "SM-X800",
        "SM-X806",
        "SM-X900",
        "SM-X906"
    )

    const val DAYS_IN_YEAR = 365
    const val DAYS_IN_WEEK = 7

    const val PREF_IS_OVERLAY_HIDDEN = "PREF_IS_OVERLAY_HIDDEN"

    const val STR_PARENT = "PARENT"

    const val ADJUSTED_EDD_DAYS = 280
    const val SINGLE_DASH_CHARACTER = "-"

    const val LICENSE_FILE : Int = 0
    const val CERTIFICATE_FILE : Int = 1
    const val CERTIFICATE_FILE_EXTENSION : String = ".pem"

    const val DIR_SHARED_PREF = "shared_prefs"

    const val JPEG_COMPRESSION_BASELINE = 0
    const val JPEG_COMPRESSION_LOSSLESS = 1
    const val PREF_USB_JPEG_COMPRESS_TYPE="EXPORT_JPEG_COMPRESSION"

    const val SAVE_BUTTON_FOCUS_DELAY: Long = 650L
    //EF Error preference key
    const val PREF_KEY_EF_ERROR = "PREF_EF_FORCED_ERROR"
    const val PREF_CALCULATION_MIGRATION_COMPLETED = "CALCULATION MIGRATION"

    const val DOUBLE_DELAY_TIMER: Long = 2000L
    const val DELAY_TIMER: Long = 1000L
    const val ALPHA_ENABLE = 1.0F
    const val ALPHA_DISABLE_HALF = 0.5F
    const val PREF_INSTITUTION_DEPARTMENT_NAME = "InstitutionDepartmentName"

    const val AI_CHANGE_SNACK_BAR_TIMEOUT = 5000
    const val SNACK_BAR_TYPE_AI_CHANGE_GAIN = 0
    const val SNACK_BAR_TYPE_AI_CHANGE_DEPTH = 1
    const val SNACK_BAR_TYPE_AI_CHANGE_PRESET = 2

    const val KEY_DEV_OPTION_AUTO_GAIN = "AUTO_GAIN"
    const val KEY_DEV_OPTION_AUTO_DEPTH = "AUTO_DEPTH"
    const val KEY_DEV_OPTION_AUTO_PRESET = "AUTO_PRESET"
    const val KEY_DEV_OPTION_AUTO_DOPPLER = "AUTO_DOPPLER"
    const val DOPPLER_AI_VIEW_JSON = "auto_doppler_data.json"

    const val LINK_BATTERY_RESET_PERCENTAGE = 255

    const val KLINK_AUTHORIZED_CHARGER_DELAY = 30000L
    const val KLINK_UNAUTHORIZED_CHARGER_DELAY = 1000L

    const val AUTO_ARCHIVE_WORKER_TAG = "AUTO_ARCHIVE_WORKER"
    const val IN_PROGRESS_EXAM_WORKER_TAG = "IN_PROGRESS_EXAM_WORKER"

    const val PREF_ENABLE_BLADDER_FREEZE_MODE = "PREF_ENABLE_BLADDER_FREEZE_MODE"
    const val DEFAULT_PREF_ENABLE_BLADDER_FREEZE_MODE: Boolean = false
    const val NEW_PATIENT_ERROR_DIALOG_WIDTH: Int = 484
    const val KEY_MATRIX = "matrix"

    const val VASCULAR_DEMO_EXAM_FIRST_NAME = "VascularDemo"
    const val VASCULAR_DEMO_EXAM_LAST_NAME = "Patient"

    const val VASCULAR_POPUP_TIMER: Long = 60000L
    const val PW_MODE_ID : Int = 2
}
