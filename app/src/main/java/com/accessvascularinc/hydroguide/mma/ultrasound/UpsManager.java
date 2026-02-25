package com.accessvascularinc.hydroguide.mma.ultrasound;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;
import android.util.SparseArray;

import androidx.annotation.Nullable;
import com.accessvascularinc.hydroguide.mma.ultrasound.GlobalUltrasoundManager;

import com.accessvascularinc.hydroguide.mma.app.AppConstants;
import com.accessvascularinc.hydroguide.mma.ultrasound.ColorMapModel;
import com.accessvascularinc.hydroguide.mma.ultrasound.RoiConstraints;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/**
 * Created by sjvinay on 2/7/18.
 */

public class UpsManager extends SQLiteOpenHelper {

    //private static final String TAG = UpsManager.class.getSimpleName();
    private static final String TAG = "UpsManager";

    private static final int DATABASE_VERSION = 1;
    private static UpsManager singleton;
    private static final String DATABASE_NAME = "thordatabase";
    private SQLiteDatabase database;

    //version info table columns
    private static final String VERSION_INFO_TABLE = "VersionInfo";
    private static final String KEY_RELEASE_DATE = "ReleaseDate";
    private static final String KEY_RELEASE_TYPE = "ReleaseType";

    //transducer info
    private static final String TRANSDUCER_INFO_TABLE = "TransducerInfo";
    private static final String KEY_NAME = "Name";

    //imaging case table columns
    //private static String IMAGING_CASE_ID_TABLE = "imagingcaseid";
    private static final String ORGANS_TABLE = "Organs";
    private static final String DEPTH_TABLE = "Depths";
    private static final String IMAGING_CASES_TABLE = "ImagingCases";
    private static final String IMAGING_MODES_TABLE = "ImagingModes";
    private static final String GLOBALS_TABLE = "Globals";
    private static final String PRFS_TABLET="PRFs";
    private static final String WORK_FLOWS_TABLE = "WorkFlows";

    //view table
    private static final String KEY_VIEW_ID = "ID";
    private static final String TABLE_VIEW = "Views";

    //workflow table
    private static final String KEY_WORKFLOW_METHOD = "Method";
    private static final String KEY_WORKFLOW_ORGAN_ID = "Organ";
    private static final String KEY_WORKFLOW_VIEW_ID = "View";
    private static final String KEY_WORKFLOW_DEFAULT_DEPTH_EASY_ID = "DefaultDepthEasy";
    private static final String KEY_WORKFLOW_DEFAULT_DEPTH_GENERAL_ID = "DefaultDepthGeneral";
    private static final String KEY_WORKFLOW_DEFAULT_DEPTH_HARD_ID = "DefaultDepthHard";
    private static final String KEY_WORKFLOW_JOB= "Job";

    //organs table columns
    private static final String KEY_ORGAN_ID = "ID";
    private static final String KEY_ORGAN_NAME = "Name";
    private static final String KEY_GLOBAL_ID = "GlobalID";
    private static final String KEY_DEFAULT_DEPTH_INDEX = "DefaultDepthIndex";
    private static final String KEY_DEFAULT_VIEW_INDEX = "DefaultViewIndex";
    private static final String KEY_DEFAULT_PRF_INDEX="TargetPrfIndex";
    private static final String KEY_DEFAULT_PRF_INDEX_VEIN="TargetPrfIndexVein";
    private static final String KEY_DEFAULT_WALLFILTER_INDEX="DefaultWallFilterIndex";
    private static final String KEY_SOUND_SPEED_INDEX="SoundSpeedIndex";
    private static final String KEY_DEFAULT_FLOWSTATE_INDEX="DefaultFlowStateIndex";
    private static final String KEY_MSWEEP_SPEED_FAST="MSweepSpeedFast";
    private static final String KEY_MSWEEP_SPEED_MEDIUM="MSweepSpeedMedium";
    private static final String KEY_MSWEEP_SPEED_SLOW="MSweepSpeedSlow";
    private static final String KEY_VELOCITY_MAP="VelocityMap";
    private static final String KEY_POWER_MAP="PowerMap";
    public static final String KEY_SHOW_COLOR="ShowColor";
    public static final String KEY_SHOW_PW="ShowPW";
    public static final String KEY_SHOW_CW="ShowCW";
    public static final String KEY_SHOW_TDI="ShowTDI";
    public static final String KEY_SHOW_M="ShowM";
    public static final String KEY_SHOW_GATE_ANGLE_ADJUST="ShowGateAngleAdjust";
    // PW Default values
    private static final String KEY_DEFAULT_GATE_SIZE_INDEX="DefaultGateSizeIndex";
    private static final String KEY_DEFAULT_ANGLE_ADJUST_INDEX="DefaultAngleAdjustIndex";
    private static final String KEY_DEFAULT_PW_PRF_INDEX="DefaultPWPrfIndex";
    private static final String KEY_DEFAULT_PW_WALL_FILTER_INDEX="DefaultPWWallFilterIndex";
    private static final String KEY_DEFAULT_PW_BASE_LINE_SHIFT_INDEX="DefaultPWBaselineShiftIndex";
    private static final String KEY_DEFAULT_TDI_PRF_INDEX="DefaultTDIPrfIndex";
    private static final String KEY_DEFAULT_GATE_SIZE_INDEX_TDI="DefaultGateSizeIndexTDI";

    // Linear PW Steering
    private static final String PW_STEERING_ANGLE_TABLE = "PWSteeringAngle";
    private static final String KEY_PW_STEERING_ANGLE_ID = "ID";
    private static final String KEY_PW_STEERING_ANGLE_DEGREE = "AngleDegrees";
    private static final String KEY_DEFAULT_DEFAULT_STEERING_ANGLE_INDEX="DefaultPWSteeringAngleIndex";

    //depth table columns
    private static final String KEY_DEPTH_ID = "ID";
    private static final String KEY_DEPTH = "Depth";

    //ImagingCases columns
    private static final String KEY_IMAGING_CASE_ID = "ID";
    private static final String KEY_IMAGING_CASE_ORGAN_ID = "Organ";
    private static final String KEY_IMAGING_CASE_VIEW_ID = "View";
    private static final String KEY_IMAGING_CASE_DEPTH_ID = "Depth";


    //ImagingModes columns
    private static final String KEY_IMAGING_MODE_ID = "ID";
    private static final String KEY_IMAGING_MODE_NAME = "Name";

    //Globals table FOV constraints
    private static final String KEY_MAX_ROI_AZIMUTH_SPAN = "MaxRoiAzimuthSpan";
    private static final String KEY_MIN_ROI_AZIMUTH_SPAN = "MinRoiAzimuthSpan";
    private static final String KEY_MAX_ROI_AXIAL_SPAN = "MaxRoiAxialSpan";
    private static final String KEY_MIN_ROI_AXIAL_SPAN = "MinRoiAxialSpan";
    private static final String KEY_MIN_ROI_AXIAL_START = "MinRoiAxialStart";
    private static final String KEY_MPERFS = "MPrf";
    private static final String KEY_AUTO_FREEZE_TIME_MIN = "AutofreezeTimeMin";
    private static final String KEY_AUTO_FREEZE_TIME_MIN_B = "AutofreezeTimeMinB";
    private static final String KEY_COOL_DOWN_TIME_MIN = "CooldownTimeMin";
    private static final String KEY_CW_AUTO_FREEZE_TIME_MIN = "AutofreezeTimeMinCW";
    private static final String KEY_MIN_GATE_AXIAL_START_MM_OFFSET = "MinGateAxialStartMmOffset";
    private static final String KEY_MAX_GATE_AXIAL_START_MM_OFFSET = "MaxGateAxialStartMmOffset";
    private static final String KEY_MIN_GATE_AZIMUTH_START_DEGREE_OFFSET = "MinGateAzimuthStartDegreeOffset";
    private static final String KEY_MAX_GATE_AZIMUTH_START_DEGREE_OFFSET = "MaxGateAzimuthStartDegreeOffset";

    private static final String KEY_MIN_GATE_AZIMUTH_START_MM_OFFSET = "MinGateAzimuthStartMmOffset";
    private static final String KEY_MAX_GATE_AZIMUTH_START_MM_OFFSET = "MaxGateAzimuthStartMmOffset";

    //PRFs table columns
    private static final String ID_PRFS="ID";
    private static final String PRF_HZ="PrfHz";

    //Color Map
    private static final String COLOR_MAP_TABLE = "Colormap";
    private static final String KEY_RGB = "RGB";
    private static final String KEY_TYPE = "Type";

    //Sound Speed
    private static final String SOUND_SPEED_TABLE = "SoundSpeed";
    private static final String KEY_SOUND_SPEED_ID= "ID";
    private static final String KEY_SPEED_OF_SOUND = "SpeedOfSound";

    // color mode Default index array indexes
    public static final int COLOR_ARRAY_INDEX_DEFAULT_PRF = 0;
    public static final int COLOR_ARRAY_INDEX_DEFAULT_WALLFILTER = 1;
    public static final int COLOR_ARRAY_INDEX_DEFAULT_FLOWSTATE = 2;
    public static final int COLOR_ARRAY_INDEX_DEFAULT_STERRING_ANGLE = 3;

    // Linear Color
    private static final String COLOR_STEERING_ANGLE_TABLE = "ColorSteeringAngle";
    private static final String KEY_COLOR_STEERING_ANGLE_ID = "ID";
    private static final String KEY_COLOR_STEERING_ANGLE_DEGREE = "AngleDegrees";
    private static final String KEY_DEFAULT_COLOR_STEERING_ANGLE_INDEX="DefaultColorSteeringAngleIndex";
    public static final int COLOR_ARRAY_INDEX_DEFAULT_PRF_VEIN = 4;
    private int probeIdAsPerInitialiaztionOfUps;

    private static final String KEY_ORGAN = "Organ";

    //PW Gate Size
    private static final String PW_GATE_SIZE_TABLE = "PWGateSize";
    private static final String KEY_GATE_SIZE_ID = "ID";
    private static final String KEY_GATE_SIZE_MM = "GateSizeMm";

    //PW Gate Angle
    private static final String PW_GATE_ANGLE_ADJUST_TABLE = "PWAngleAdjust";
    private static final String KEY_GATE_ANGLE_ADJUST_ID = "ID";
    private static final String KEY_ANGLE_DEGREES = "AngleDegrees";

    // PW Prf table
    private static final String PW_PRF_TABLE = "PWPRFs";
    private static final String KEY_PW_PRF_ID = "ID";
    private static final String KEY_PW_PRF_HZ = "PrfHz";

    // CW Prf table
    private static final String CW_PRF_TABLE = "CWPRFs";
    private static final String KEY_CW_PRF_ID = "ID";
    private static final String KEY_CW_PRF_HZ = "PrfHz";

    // TDI Prf table
    private static final String TDI_PRF_TABLE = "TDIPRFs";
    private static final String KEY_TDI_PRF_ID = "ID";
    private static final String KEY_TDI_PRF_HZ = "PrfHz";

    // TxWaveforms table
    private static final String TX_WAVEFORM_TABLE = "TxWaveforms";
    private static final String KEY_TX_ID = "ID";
    private static final String KEY_CENTER_FREQUENCY = "CenterFrequency";
    private static final String KEY_FOCUS = "Focus";
    private static final String KEY_GATE_RANGE = "GateRange";

    // PWBaselineShift table
    private static final String PW_BASELINE_SHIFT_TABLE = "PWBaselineShift";
    private static final String KEY_PW_BASELINE_SHIFT_ID = "ID";
    private static final String KEY_PW_BASELINE_SHIFT_FRACTION = "BaselineshiftFraction";

    // PW Default index array indexes
    public static final int PW_ARRAY_INDEX_GLOBAL_ID = 0;
    public static final int PW_ARRAY_INDEX_SOUND_SPEED = 1;
    public static final int PW_ARRAY_INDEX_GATE_SIZE = 2;
    public static final int PW_ARRAY_INDEX_ANGLE_ADJUST = 3;
    public static final int PW_ARRAY_INDEX_WALL_FILTER = 4;
    public static final int PW_ARRAY_INDEX_PRF = 5;
    public static final int PW_ARRAY_INDEX_BASE_LINE_SHIFT = 6;
    public static final int TDI_ARRAY_INDEX_PRF = 7;
    public static final int PW_ARRAY_INDEX_STEERING_ANGLE = 8;
    public static final int TDI_ARRAY_INDEX_GATE_SIZE = 9;


    private UpsManager(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
    }

    public static UpsManager init(Context context){
        if(singleton != null){
            singleton.getReadableDatabase();
            return singleton;
        }

        singleton = new UpsManager(context);
        singleton.getReadableDatabase();
        return singleton;
    }

    public static @Nullable
    UpsManager getInstance(){
        return singleton;
    }

    @Override
    public SQLiteDatabase getReadableDatabase() {
        String  upsPath = GlobalUltrasoundManager.getInstance().getUpsPath();

        Log.d(TAG, "UPS Selected path: "+upsPath);
        database = SQLiteDatabase.openDatabase(upsPath, null, SQLiteDatabase.OPEN_READONLY);
        return database;
    }

    @Override
    public SQLiteDatabase getWritableDatabase() {
        String  upsPath = GlobalUltrasoundManager.getInstance().getUpsPath();
        //Log.w(TAG, upsPath);
        database = SQLiteDatabase.openDatabase(upsPath, null, SQLiteDatabase.OPEN_READWRITE);
        return database;
    }

    /**
     * get thor db version
     * @return version X.X.X
     */
    public String getThorDbVersion(){
        if(database != null){
            Cursor cursor = database.rawQuery("SELECT * FROM \"VersionInfo\"", null);
            cursor.moveToFirst();
            String mainNumber = cursor.getString(cursor.getColumnIndex("ReleaseNumMain"));
            String majorNumber = cursor.getString(cursor.getColumnIndex("ReleaseNumMajor"));
            String minorNumber = cursor.getString(cursor.getColumnIndex("ReleaseNumMinor"));
            cursor.close();
            return mainNumber+"."+majorNumber+"."+minorNumber;
        }
        return "0.0.0";
    }

    public static int getUpsDbVersion(){
        return DATABASE_VERSION;
    }

    public void createUniqueOrgansList(Context context){
        if (database == null || Ups.getInstance() == null){
            return;
        }
        Cursor imagingCasesCursor = database.rawQuery("SELECT DISTINCT \"Organ\" FROM \"ImagingCases\"", null);

        int organIndex = imagingCasesCursor.getColumnIndexOrThrow(KEY_IMAGING_CASE_ORGAN_ID);

        Ups.getInstance().clearOrgansList();
        Ups.getInstance().createGlobalIdArray(context);

        while (imagingCasesCursor.moveToNext()) {
            String[] idArray = new String[1];
            String organId = imagingCasesCursor.getString(organIndex);
            idArray[0] = organId;

            String[] views = getViews(idArray);
            Integer[] depthList = getDepthList(idArray);

            String globalId = getGlobalId(organId);
            String organName = getOrganName(globalId);

            Ups.getInstance()
                    .getBuilder()
                    .addOrganId(organId)
                    .addOrganGlobalId(globalId)
                    .addOrganName(organName)
                    .addBmi(views)
                    .addDepthList(depthList)
                    .build();
        }
        imagingCasesCursor.close();
    }


    /**
     * create Unique AI Organ list from WorkFlows table
     * @param context : Android context
     */
    public void createUniqueAiOrgansList(Context context){

        String query = "SELECT DISTINCT " + KEY_WORKFLOW_ORGAN_ID + " FROM " + WORK_FLOWS_TABLE + " where " + KEY_WORKFLOW_JOB + "='EF'";
        Cursor imagingCasesCursor = database.rawQuery(query, null);

        int organIndex = imagingCasesCursor.getColumnIndexOrThrow(KEY_WORKFLOW_ORGAN_ID);

        Ups.getInstance().clearAiOrgansList();
        Ups.getInstance().createGlobalIdArray(context);

        while (imagingCasesCursor.moveToNext()) {
            String[] idArray = new String[1];
            String organId = imagingCasesCursor.getString(organIndex);
            idArray[0] = organId;

            String[] views = getViews(idArray);
            Integer[] depthList = getDepthList(idArray);

            String globalId = getGlobalId(organId);
            String organName = getOrganName(globalId);

            Ups.getInstance()
                    .getBuilder()
                    .addOrganId(organId)
                    .addOrganGlobalId(globalId)
                    .addOrganName(organName)
                    .addBmi(views)
                    .addDepthList(depthList)
                    .buildAi();
        }
        imagingCasesCursor.close();
    }

    public Release getRelease() {

        String[] projection = {
                KEY_RELEASE_DATE,
                KEY_RELEASE_TYPE
        };

        Cursor globalIdCursor = database.query(true, VERSION_INFO_TABLE, projection, null, null, null,
                null, null, null);

        int releaseDateColumn = globalIdCursor.getColumnIndexOrThrow(KEY_RELEASE_DATE);
        int releaseTypeColumn = globalIdCursor.getColumnIndexOrThrow(KEY_RELEASE_TYPE);

        globalIdCursor.moveToFirst();

        String releaseDate = globalIdCursor.getString(releaseDateColumn);
        String releaseType = globalIdCursor.getString(releaseTypeColumn);

        Release release = new Release(Integer.valueOf(releaseType), releaseDate);

        globalIdCursor.close();

        return release;
    }

    public String getTransducerName() {

        String[] projection = {
                KEY_NAME
        };

        Cursor globalIdCursor = database.query(true, TRANSDUCER_INFO_TABLE, projection, null, null, null,
                null, null, null);

        int nameColumn = globalIdCursor.getColumnIndexOrThrow(KEY_NAME);

        globalIdCursor.moveToFirst();

        String name = globalIdCursor.getString(nameColumn);

        globalIdCursor.close();

        return name;
    }

    /**
     * get supported mode for specific organ
     * @param organId
     * @return Hashmap of supported modes
     */
    public HashMap<String,Boolean> getSupportedMode(String organId, Boolean isLinearProbe){
        if(isLinearProbe){
            String selection = KEY_ORGAN_ID + "=?";

            String[] projection = {
                    KEY_ORGAN_ID,
                    KEY_SHOW_COLOR,
                    KEY_SHOW_PW,
                    KEY_SHOW_CW,
                    KEY_SHOW_M,
                    KEY_SHOW_TDI,
                    KEY_SHOW_GATE_ANGLE_ADJUST
            };

            String[] idArray = new String[1];
            idArray[0] = organId;

            Cursor showCursor = database.query(true, ORGANS_TABLE, projection, selection, idArray, null,
                    null, null, null);

            int showColorIdColumn = showCursor.getColumnIndexOrThrow(KEY_SHOW_COLOR);
            int showPWIdColumn = showCursor.getColumnIndexOrThrow(KEY_SHOW_PW);
            int showCWIdColumn = showCursor.getColumnIndexOrThrow(KEY_SHOW_CW);
            int showMIdColumn = showCursor.getColumnIndexOrThrow(KEY_SHOW_M);
            int showTDIIdColumn = showCursor.getColumnIndexOrThrow(KEY_SHOW_TDI);
            int showGetAngleAdjustColumn = showCursor.getColumnIndexOrThrow(KEY_SHOW_GATE_ANGLE_ADJUST);

            showCursor.moveToFirst();

            HashMap<String,Boolean> supportModes = new HashMap<>();
            supportModes.put(KEY_SHOW_COLOR,showCursor.getInt(showColorIdColumn) == 1);
            supportModes.put(KEY_SHOW_PW,showCursor.getInt(showPWIdColumn) == 1);
            supportModes.put(KEY_SHOW_CW,showCursor.getInt(showCWIdColumn) == 1);
            supportModes.put(KEY_SHOW_M,showCursor.getInt(showMIdColumn) == 1);
            supportModes.put(KEY_SHOW_TDI,showCursor.getInt(showTDIIdColumn) == 1);
            // Later on organId.equals("2") this comes form thor.db
            supportModes.put(KEY_SHOW_GATE_ANGLE_ADJUST, showCursor.getInt(showGetAngleAdjustColumn) == AppConstants.ONE);
            showCursor.close();
            return supportModes;
        }else {
            String selection = KEY_ORGAN_ID + "=?";

            String[] projection = {
                    KEY_ORGAN_ID,
                    KEY_SHOW_COLOR,
                    KEY_SHOW_PW,
                    KEY_SHOW_CW,
                    KEY_SHOW_TDI,
                    KEY_SHOW_GATE_ANGLE_ADJUST
            };

            String[] idArray = new String[1];
            idArray[0] = organId;

            Cursor showCursor = database.query(true, ORGANS_TABLE, projection, selection, idArray, null,
                    null, null, null);

            int showColorIdColumn = showCursor.getColumnIndexOrThrow(KEY_SHOW_COLOR);
            int showPWIdColumn = showCursor.getColumnIndexOrThrow(KEY_SHOW_PW);
            int showCWIdColumn = showCursor.getColumnIndexOrThrow(KEY_SHOW_CW);
            int showTDIIdColumn = showCursor.getColumnIndexOrThrow(KEY_SHOW_TDI);
            int showGetAngleAdjustColumn = showCursor.getColumnIndexOrThrow(KEY_SHOW_GATE_ANGLE_ADJUST);
            showCursor.moveToFirst();

            HashMap<String,Boolean> supportModes = new HashMap<>();
            supportModes.put(KEY_SHOW_COLOR,showCursor.getInt(showColorIdColumn) == 1);
            supportModes.put(KEY_SHOW_PW,showCursor.getInt(showPWIdColumn) == 1);
            supportModes.put(KEY_SHOW_CW,showCursor.getInt(showCWIdColumn) == 1);
            supportModes.put(KEY_SHOW_TDI,showCursor.getInt(showTDIIdColumn) == 1);
            // TODO For now Thor3.db does not have KEY_SHOW_M
            supportModes.put(KEY_SHOW_M,true);
            // Later on organId.equals("2") this comes form thor.db
            supportModes.put(KEY_SHOW_GATE_ANGLE_ADJUST, showCursor.getInt(showGetAngleAdjustColumn) == AppConstants.ONE);
            showCursor.close();
            return supportModes;
        }

    }

    private String getGlobalId(String organId) {

        String selection = KEY_ORGAN_ID + "=?";

        String[] projection = {
                KEY_GLOBAL_ID,
                KEY_ORGAN_ID,
                KEY_ORGAN_NAME
        };

        String[] idArray = new String[1];
        idArray[0] = organId;

        Cursor globalIdCursor = database.query(true, ORGANS_TABLE, projection, selection, idArray, null,
                null, null, null);

        int globalIdColumn = globalIdCursor.getColumnIndexOrThrow(KEY_GLOBAL_ID);
        String globalId;

        globalIdCursor.moveToFirst();
        globalId = globalIdCursor.getString(globalIdColumn);
        globalIdCursor.close();
        return globalId;
    }


    private Integer getMPerfs(){
        String[] projection = {
                KEY_MPERFS
        };
        Cursor globalsCursor = database.query(true, GLOBALS_TABLE, projection, null, null, null,
                null, null, null);

        int mPerfIndex = globalsCursor.getColumnIndexOrThrow(KEY_MPERFS);

        globalsCursor.moveToFirst();

        Integer mPerfValue = Integer.valueOf(globalsCursor.getString(mPerfIndex));

        globalsCursor.close();
        return mPerfValue;
    }


    /**
     * get auto freeze time in min
     * @return auto freeze time in min
     */
    public Integer getAutoFreezeTimeInMin(){
        String[] projection = {
                KEY_AUTO_FREEZE_TIME_MIN
        };
        Cursor globalsCursor = database.query(true, GLOBALS_TABLE, projection, null, null, null,
                null, null, null);

        int mAutoFreezeTimeMinIndex = globalsCursor.getColumnIndexOrThrow(KEY_AUTO_FREEZE_TIME_MIN);

        globalsCursor.moveToFirst();

        Integer mAutoFreezeTimeMinValue = Integer.valueOf(globalsCursor.getString(mAutoFreezeTimeMinIndex));

        globalsCursor.close();
        Log.d(TAG,"mAutoFreezeTimeMinValue : " + mAutoFreezeTimeMinValue);
        return mAutoFreezeTimeMinValue;
    }

    /**
     * get auto freeze time in min from AutofreezeTimeMinB
     * @return auto freeze time in min
     */
    public Integer getAutoFreezeTimeInMinB(){
        int mAutoFreezeTimeMinValue;
        try{
            String[] projection = {
                    KEY_AUTO_FREEZE_TIME_MIN_B
            };
            Cursor globalsCursor = database.query(true, GLOBALS_TABLE, projection, null, null, null,
                    null, null, null);

            int mAutoFreezeTimeMinIndex = globalsCursor.getColumnIndexOrThrow(KEY_AUTO_FREEZE_TIME_MIN_B);

            globalsCursor.moveToFirst();

            mAutoFreezeTimeMinValue = Integer.parseInt(globalsCursor.getString(mAutoFreezeTimeMinIndex));
            globalsCursor.close();
        }catch (Exception e){
            mAutoFreezeTimeMinValue = getAutoFreezeTimeInMin();
        }

        Log.d(TAG,"mAutoFreezeTimeMinValueB : " + mAutoFreezeTimeMinValue);
        return mAutoFreezeTimeMinValue;
    }


    /**
     * get cool down time in min
     * @return auto cool down in min
     */
    public Integer getCoolDownTimeInMin(){
        String[] projection = {
                KEY_COOL_DOWN_TIME_MIN
        };
        Cursor globalsCursor = database.query(true, GLOBALS_TABLE, projection, null, null, null,
                null, null, null);

        int mCoolDownTimeMinIndex = globalsCursor.getColumnIndexOrThrow(KEY_COOL_DOWN_TIME_MIN);

        globalsCursor.moveToFirst();

        Integer mCoolDownTimeMinValue = Integer.valueOf(globalsCursor.getString(mCoolDownTimeMinIndex));

        globalsCursor.close();
        Log.d(TAG,"mCoolDownTimeMinValue : " + mCoolDownTimeMinValue);
        return mCoolDownTimeMinValue;
    }


    public Float getMModeTimeSpan(Integer organId,Integer sweepSpeed,Integer mModeWindowWidth){
        Float mTimeSpan;
        String selection = KEY_ORGAN_ID + "=?";

        String[] projection = {
                KEY_GLOBAL_ID,
                KEY_ORGAN_NAME,
                KEY_MSWEEP_SPEED_FAST,
                KEY_MSWEEP_SPEED_MEDIUM,
                KEY_MSWEEP_SPEED_SLOW
        };

        String[] idArray = new String[1];
        idArray[0] = organId.toString();

        Cursor globalIdCursor = database.query(true, ORGANS_TABLE, projection, selection, idArray, null,
                null, null, null);

        int mSweepSpeedFastColumn = globalIdCursor.getColumnIndexOrThrow(KEY_MSWEEP_SPEED_FAST);
        int mSweepSpeedMediumColumn = globalIdCursor.getColumnIndexOrThrow(KEY_MSWEEP_SPEED_MEDIUM);
        int mSweepSpeedSlowColumn = globalIdCursor.getColumnIndexOrThrow(KEY_MSWEEP_SPEED_SLOW);
        globalIdCursor.moveToFirst();
        Integer mSweepSpeedFastFactor = Integer.valueOf(globalIdCursor.getString(mSweepSpeedFastColumn));
        Integer mSweepSpeedMediumFactor = Integer.valueOf(globalIdCursor.getString(mSweepSpeedMediumColumn));
        Integer  mSweepSpeedSlowFactor = Integer.valueOf(globalIdCursor.getString(mSweepSpeedSlowColumn));
        globalIdCursor.close();

        Integer sweepSpeedFactor = mSweepSpeedSlowFactor;
        if(sweepSpeed.equals(SweepSpeed.FAST)){
            sweepSpeedFactor = mSweepSpeedFastFactor;
        }else if(sweepSpeed.equals(SweepSpeed.MEDIUM)) {
            sweepSpeedFactor = mSweepSpeedMediumFactor;
        }
        //Number of columns updated per second ColumnsPerSecond = MModePrf/SpeedFactor.
        int columnsPerSecond = getMPerfs()/sweepSpeedFactor;

        //M-Mode time span is (horizontal pixels allocated for M-mode display)/ColumnsPerSecond.
        mTimeSpan = ((float)mModeWindowWidth / columnsPerSecond);
        return mTimeSpan;

    }
    public RoiConstraints getRoiConstraints() {

        String[] projection = {
                KEY_MAX_ROI_AZIMUTH_SPAN,
                KEY_MIN_ROI_AZIMUTH_SPAN,
                KEY_MAX_ROI_AXIAL_SPAN,
                KEY_MIN_ROI_AXIAL_SPAN,
                KEY_MIN_ROI_AXIAL_START
        };

        Cursor globalsCursor = database.query(true, GLOBALS_TABLE, projection, null, null, null,
                null, null, null);

        int roiAzimuthSpanIndex = globalsCursor.getColumnIndexOrThrow(KEY_MAX_ROI_AZIMUTH_SPAN);
        int roiMinAzimuthSpanIndex = globalsCursor.getColumnIndexOrThrow(KEY_MIN_ROI_AZIMUTH_SPAN);
        int roiMaxAxialSpanIndex = globalsCursor.getColumnIndexOrThrow(KEY_MAX_ROI_AXIAL_SPAN);
        int roiMinAxialSpanIndex = globalsCursor.getColumnIndexOrThrow(KEY_MIN_ROI_AXIAL_SPAN);
        int roiMinAxialStartIndex = globalsCursor.getColumnIndexOrThrow(KEY_MIN_ROI_AXIAL_START);

        globalsCursor.moveToFirst();

        Float roiAzimuthSpan = Float.valueOf(globalsCursor.getString(roiAzimuthSpanIndex));
        Float roiMinAzimuthSpan = Float.valueOf(globalsCursor.getString(roiMinAzimuthSpanIndex));
        Float roiMaxAxialSpan = Float.valueOf(globalsCursor.getString(roiMaxAxialSpanIndex));
        Float roiMinAxialSpan = Float.valueOf(globalsCursor.getString(roiMinAxialSpanIndex));
        Float roiMinAxialStart = Float.valueOf(globalsCursor.getString(roiMinAxialStartIndex));

        globalsCursor.close();
        return new RoiConstraints().setMaxRoiAzimuthSpan(roiAzimuthSpan)
                                   .setMinRoiAzimuthSpan(roiMinAzimuthSpan)
                                   .setMaxRoiAxialSpan(roiMaxAxialSpan)
                                   .setMinRoiAxialSpan(roiMinAxialSpan)
                                   .setMinRoiAxialStart(roiMinAxialStart);
    }



    private String getOrganName(String globalId) {

        String selection = KEY_GLOBAL_ID + "=?";

        String[] projection = {
                KEY_GLOBAL_ID,
                KEY_ORGAN_NAME
        };

        String[] idArray = new String[1];
        idArray[0] = globalId;

        Cursor globalIdCursor = database.query(true, ORGANS_TABLE, projection, selection, idArray, null,
                null, null, null);

        int organNameColumn = globalIdCursor.getColumnIndexOrThrow(KEY_ORGAN_NAME);
        globalIdCursor.moveToFirst();
        String organName = globalIdCursor.getString(organNameColumn);
        globalIdCursor.close();
        return organName;
    }


    private String[] getViews(String[] idArray) {
        String[] projection = {
                KEY_IMAGING_CASE_VIEW_ID
        };

        String selection = KEY_IMAGING_CASE_ORGAN_ID + "=?";
        Cursor viewCursor = database.query(true, IMAGING_CASES_TABLE, projection, selection, idArray, null,
                null, null, null);

        int viewIdColumn = viewCursor.getColumnIndexOrThrow(KEY_IMAGING_CASE_VIEW_ID);
        ArrayList<String> viewIDArray = new ArrayList();

        while (viewCursor.moveToNext()) {
            String viewName = viewCursor.getString(viewIdColumn);
            viewIDArray.add(viewName);
        }

        viewCursor.close();
        String[] array = new String[viewIDArray.size()];
        return viewIDArray.toArray(array);
    }


    /**
     * get Views List from Views table
     * @return The views id value list
     */
    public String[] getViewsAi() {
        String query = "SELECT DISTINCT " + KEY_VIEW_ID + " FROM "+ TABLE_VIEW ;

        Cursor viewCursor =database.rawQuery(query, null);

        int viewIdColumn = viewCursor.getColumnIndexOrThrow(KEY_VIEW_ID);
        ArrayList<String> viewIDArray = new ArrayList();

        while (viewCursor.moveToNext()) {
            String viewName = viewCursor.getString(viewIdColumn);
            viewIDArray.add(viewName);
        }

        viewCursor.close();
        String[] array = new String[viewIDArray.size()];
        return viewIDArray.toArray(array);
    }

    /**
     * get Depth List
     * @param idArray The organ ID
     * @return The actual depth value in millimetres
     */
    private Integer[] getDepthList(String[] idArray) {
        String[] projection = {
                KEY_IMAGING_CASE_DEPTH_ID
        };
        String selection = KEY_IMAGING_CASE_ORGAN_ID + "=?";
        Cursor depthIdCursor = database.query(true, IMAGING_CASES_TABLE, projection, selection, idArray, null,
                null, null, null);

        int depthIdColumn = depthIdCursor.getColumnIndexOrThrow(KEY_IMAGING_CASE_DEPTH_ID);
        ArrayList<Integer> depthIDArray = new ArrayList();

        while (depthIdCursor.moveToNext()) {
            String depthId = depthIdCursor.getString(depthIdColumn);
            depthIDArray.add(getDepthInCms(depthId));
        }

        depthIdCursor.close();
        Integer[] array = new Integer[depthIDArray.size()];
        return depthIDArray.toArray(array);
    }


    /**
     * get Depth List from Depths table
     * @return The actual depth value in millimetres
     */
    public Integer[] getDepthListAi() {

        String query = "SELECT DISTINCT ID FROM Depths";

        Cursor depthIdCursor =database.rawQuery(query, null);



        int depthIdColumn = depthIdCursor.getColumnIndexOrThrow(KEY_DEPTH_ID);
        ArrayList<Integer> depthIDArray = new ArrayList();

        while (depthIdCursor.moveToNext()) {
            String depthId = depthIdCursor.getString(depthIdColumn);
            depthIDArray.add(getDepthInCms(depthId));
        }

        depthIdCursor.close();
        Integer[] array = new Integer[depthIDArray.size()];
        return depthIDArray.toArray(array);
    }


    /**
     * get Views Depth Type as per Views Table
     * @param view view type
     */
    private String getViewsDepthType(int view) {
        if(view == 0){
            return KEY_WORKFLOW_DEFAULT_DEPTH_EASY_ID;
        }else if(view == 1){
            return KEY_WORKFLOW_DEFAULT_DEPTH_GENERAL_ID;
        }else if(view == 2){
            return KEY_WORKFLOW_DEFAULT_DEPTH_HARD_ID;
        }
        return KEY_WORKFLOW_DEFAULT_DEPTH_GENERAL_ID;

    }

    public Integer getDepthInCms(String depthId) {

        String[] projection = {
                KEY_DEPTH
        };

        String[] idArray = new String[1];
        idArray[0] = depthId;

        String selection = KEY_DEPTH_ID + "=?";
        Cursor depthCursor = database.query(true, DEPTH_TABLE, projection, selection, idArray, null,
                null, null, null);

        int depthIdColumn = depthCursor.getColumnIndexOrThrow(KEY_DEPTH);
        depthCursor.moveToFirst();
        int depth = depthCursor.getInt(depthIdColumn);
        depthCursor.close();
        return Math.round(depth/10.0F);
    }

    private DefaultValuesObject getDefaultIndices(Integer organId){
        String[] projection = {
                KEY_DEFAULT_DEPTH_INDEX,
                KEY_DEFAULT_VIEW_INDEX
        };

        String[] organIdArray = new String[1];
        organIdArray[0] = String.valueOf(organId);
        String selection = KEY_ORGAN_ID + "=?";
        Cursor organsCursor = database.query(true, ORGANS_TABLE, projection, selection, organIdArray, null,
                null, null, null);

        int defaultDepthColumn = organsCursor.getColumnIndexOrThrow(KEY_DEFAULT_DEPTH_INDEX);
        int defaultViewColumn = organsCursor.getColumnIndexOrThrow(KEY_DEFAULT_VIEW_INDEX);

        organsCursor.moveToFirst();
        int depthId = organsCursor.getInt(defaultDepthColumn);
        int view = organsCursor.getInt(defaultViewColumn);
        DefaultValuesObject returnObject = new DefaultValuesObject(depthId, view);
        organsCursor.close();

        return returnObject;
    }


    private DefaultValuesObject getDefaultAiIndices(Integer organId){
        String[] projection = {
                KEY_WORKFLOW_VIEW_ID,
                KEY_WORKFLOW_DEFAULT_DEPTH_EASY_ID,
                KEY_WORKFLOW_DEFAULT_DEPTH_GENERAL_ID,
                KEY_WORKFLOW_DEFAULT_DEPTH_HARD_ID
        };

        String[] organIdArray = new String[1];
        organIdArray[0] = String.valueOf(organId);
        String selection = KEY_WORKFLOW_METHOD + "='A4C A2C' and "+ KEY_ORGAN_ID + "=?";
        Cursor organsCursor = database.query(true, WORK_FLOWS_TABLE, projection, selection, organIdArray, null,
                null, null, null);

        Cursor viewIdCursor = database.query(true, WORK_FLOWS_TABLE, projection, selection, organIdArray, null,
                null, null, null);

        viewIdCursor.moveToNext();
        String depthColumnTitle = getViewsDepthType(viewIdCursor.getInt(viewIdCursor.getColumnIndexOrThrow(KEY_WORKFLOW_VIEW_ID)));

        int defaultDepthColumn = organsCursor.getColumnIndexOrThrow(depthColumnTitle);
        int defaultViewColumn = organsCursor.getColumnIndexOrThrow(KEY_WORKFLOW_VIEW_ID);

        organsCursor.moveToFirst();
        int depthId = organsCursor.getInt(defaultDepthColumn);
        int view = organsCursor.getInt(defaultViewColumn);
        DefaultValuesObject returnObject = new DefaultValuesObject(depthId, view);
        organsCursor.close();
        viewIdCursor.close();

        return returnObject;
    }

    public boolean isImagingCaseIdValid(Integer imagingCaseId){
        String[] projection = {
                KEY_IMAGING_CASE_ID
        };

        String[] idArray = new String[1];
        idArray[0] = String.valueOf(imagingCaseId);

        String selection = KEY_IMAGING_CASE_ORGAN_ID + "=?";
        Cursor cursor = database.query(true, IMAGING_CASES_TABLE, projection, selection, idArray, null,
                null, null, null);

        if(cursor.getCount() > 0){
            cursor.close();
            return true;
        }
        Log.d(TAG, "Imaging Case ID is invalid");

        cursor.close();
        return false;
    }

    public Integer getImagingCaseId(DefaultValuesObject defaultValues, Organ selectedOrgan){

        String[] projection = {
                KEY_IMAGING_CASE_ID
        };

        String selection = KEY_DEPTH + "=? AND " + KEY_IMAGING_CASE_ORGAN_ID + "=? AND " + KEY_IMAGING_CASE_VIEW_ID + "=?";
        String[] selectionArgs = new String[3];
        selectionArgs[0] = String.valueOf(defaultValues.getDefaultDepthId());
        selectionArgs[1] = String.valueOf(selectedOrgan.getOrganId());
        selectionArgs[2] = String.valueOf(defaultValues.getDefaultView());

        Cursor imagingCaseIdCursor = database.query(true, IMAGING_CASES_TABLE, projection, selection, selectionArgs, null,
                null, null, null);

        int imagingCasesIdColumn = imagingCaseIdCursor.getColumnIndexOrThrow(KEY_IMAGING_CASE_ID);
        imagingCaseIdCursor.moveToFirst();
        Integer imagingCaseId = imagingCaseIdCursor.getInt(imagingCasesIdColumn);
        imagingCaseIdCursor.close();
        return imagingCaseId;
    }

    public Integer getDepthId(Integer depth){
        String[] projection = {
                KEY_DEPTH_ID
        };

        String[] depthArray = new String[1];
        depthArray[0] = String.valueOf(depth * 10);

        String selection = KEY_DEPTH + "=?";
        Cursor depthCursor = database.query(true, DEPTH_TABLE, projection, selection, depthArray, null,
                null, null, null);

        int depthIdColumn = depthCursor.getColumnIndexOrThrow(KEY_DEPTH_ID);
        depthCursor.moveToFirst();
        Integer depthId = depthCursor.getInt(depthIdColumn);
        depthCursor.close();
        return depthId;
    }



    public SparseArray<Float> getPRFs(){

        SparseArray<Float> prfs=new SparseArray<>();

        String query="SELECT * FROM "+PRFS_TABLET;

        try (Cursor prfsCursor = database.rawQuery(query, null)) {

            if (prfsCursor != null) {
                while (prfsCursor.moveToNext()) {
                    int id = prfsCursor.getInt(prfsCursor.getColumnIndex(ID_PRFS));
                    float prfHz = prfsCursor.getFloat(prfsCursor.getColumnIndex(PRF_HZ));
                    prfs.put(id, prfHz);
                }
            }

        } catch (SQLiteException sqlException) {
          MedDevLog.error(TAG, "getPRFs() SQLiteException : " +  sqlException.getMessage());
        }

      Log.i(TAG,"PRFS count is "+ prfs.size());

      return prfs;
    }


    public int[] getDefaultColorFlowIndexes(int organId,Boolean isLinearProbe){
        Log.d(TAG,"getDefaultColorFlowIndexes organId : " +organId);
        Log.d(TAG,"getDefaultColorFlowIndexes isLinearProbe : " +isLinearProbe);
        if(isLinearProbe){
            try {
                return getDefaultColorFlowIndexesLinear(organId);
            }catch (SQLiteException e){
                return getDefaultColorFlowIndexesTorso(organId);
            }
        }else {
           return getDefaultColorFlowIndexesTorso(organId);
        }
    }

    private int[] getDefaultColorFlowIndexesTorso(int organId) {
       String[] projection = {
                KEY_DEFAULT_PRF_INDEX, KEY_DEFAULT_WALLFILTER_INDEX, KEY_DEFAULT_FLOWSTATE_INDEX
        };

        int[] colorFlow = {3, 2, 2, 0, 0};

        String[] organIDArray = new String[1];
        organIDArray[0] = String.valueOf(organId);

        String selection = KEY_ORGAN_ID + "=?";

        Cursor organsCursor = database.query(true, ORGANS_TABLE, projection, selection, organIDArray, null,
                null, null, null);

        if (organsCursor != null && organsCursor.moveToFirst()) {

            colorFlow[COLOR_ARRAY_INDEX_DEFAULT_PRF] = organsCursor.getInt(organsCursor.getColumnIndex(KEY_DEFAULT_PRF_INDEX));
            colorFlow[COLOR_ARRAY_INDEX_DEFAULT_WALLFILTER] = organsCursor.getInt(organsCursor.getColumnIndex(KEY_DEFAULT_WALLFILTER_INDEX));
            colorFlow[COLOR_ARRAY_INDEX_DEFAULT_FLOWSTATE] = organsCursor.getInt(organsCursor.getColumnIndex(KEY_DEFAULT_FLOWSTATE_INDEX));
        }

        if (organsCursor != null) {
            organsCursor.close();
        }

        Log.i(TAG, "PRF index is" + colorFlow[0] + "organ id is" + organId);

        return colorFlow;
    }

    private int[] getDefaultColorFlowIndexesLinear(int organId) {

        String[] projection = {
                KEY_DEFAULT_PRF_INDEX,
                KEY_DEFAULT_WALLFILTER_INDEX,
                KEY_DEFAULT_FLOWSTATE_INDEX,
                KEY_DEFAULT_COLOR_STEERING_ANGLE_INDEX,
                KEY_DEFAULT_PRF_INDEX_VEIN
        };

        int[] colorFlow = {3, 2, 2, 0, 0};

        String[] organIDArray = new String[1];
        organIDArray[0] = String.valueOf(organId);

        String selection = KEY_ORGAN_ID + "=?";

        Cursor organsCursor = database.query(true, ORGANS_TABLE, projection, selection, organIDArray, null,
                null, null, null);

        if (organsCursor != null && organsCursor.moveToFirst()) {

            colorFlow[COLOR_ARRAY_INDEX_DEFAULT_PRF] = organsCursor.getInt(organsCursor.getColumnIndex(KEY_DEFAULT_PRF_INDEX));
            colorFlow[COLOR_ARRAY_INDEX_DEFAULT_WALLFILTER] = organsCursor.getInt(organsCursor.getColumnIndex(KEY_DEFAULT_WALLFILTER_INDEX));
            colorFlow[COLOR_ARRAY_INDEX_DEFAULT_FLOWSTATE] = organsCursor.getInt(organsCursor.getColumnIndex(KEY_DEFAULT_FLOWSTATE_INDEX));
            colorFlow[COLOR_ARRAY_INDEX_DEFAULT_STERRING_ANGLE] = organsCursor.getInt(organsCursor.getColumnIndex(KEY_DEFAULT_COLOR_STEERING_ANGLE_INDEX));
            colorFlow[COLOR_ARRAY_INDEX_DEFAULT_PRF_VEIN] = organsCursor.getInt(organsCursor.getColumnIndex(KEY_DEFAULT_PRF_INDEX_VEIN));
        }

        if (organsCursor != null) {
            organsCursor.close();
        }

        return colorFlow;
    }

    /**
     * get color steering angle array for Linear color mode
     * @return colorSteeringAngleArray
     */
    public ArrayList<String> getColorSteeringAngleArray(){

        ArrayList<String> colorSteeringAngleArray =new ArrayList<>();

        String[] projection = {
                KEY_COLOR_STEERING_ANGLE_ID,
                KEY_COLOR_STEERING_ANGLE_DEGREE
        };

        Cursor globalsCursor = database.query(true, COLOR_STEERING_ANGLE_TABLE, projection, null,null, null,
                null, null, null);

        int colorSteeringAngleIdIndex = globalsCursor.getColumnIndexOrThrow(KEY_COLOR_STEERING_ANGLE_ID);
        int colorSteeringAngleDegreeIndex = globalsCursor.getColumnIndexOrThrow(KEY_COLOR_STEERING_ANGLE_DEGREE);

        while (globalsCursor.moveToNext()) {
            int id = globalsCursor.getInt(colorSteeringAngleIdIndex);
            int angleDegree = globalsCursor.getInt(colorSteeringAngleDegreeIndex);
            colorSteeringAngleArray.add(id, ""+angleDegree);
        }

        Log.i(TAG,"colorSteeringAngleArray size count is "+ colorSteeringAngleArray.size());

        return colorSteeringAngleArray;
    }

    public Float getColorSteeringAngleDegree(int id) {

        String[] projection = {
                KEY_COLOR_STEERING_ANGLE_DEGREE
        };

        String[] idArray = new String[1];
        idArray[0] = ""+id;

        String selection = KEY_COLOR_STEERING_ANGLE_ID + "=?";
        Cursor cursor = database.query(true, COLOR_STEERING_ANGLE_TABLE, projection, selection, idArray, null,
                null, null, null);

        int colorSteeringAngleDegreeIndex = cursor.getColumnIndexOrThrow(KEY_COLOR_STEERING_ANGLE_DEGREE);
        cursor.moveToFirst();
        float angleDegree = cursor.getInt(colorSteeringAngleDegreeIndex);
        cursor.close();
        return angleDegree;
    }

    /**
     * get pw steering angle array for Linear pw mode
     * @return pwSteeringAngleArray
     */
    public ArrayList<String> getPWSteeringAngleArray(){

        ArrayList<String> pwSteeringAngleArray =new ArrayList<>();

        String[] projection = {
                KEY_PW_STEERING_ANGLE_ID,
                KEY_PW_STEERING_ANGLE_DEGREE
        };

        Cursor globalsCursor = database.query(true, PW_STEERING_ANGLE_TABLE, projection, null,null, null,
                null, null, null);

        int pwSteeringAngleIdIndex = globalsCursor.getColumnIndexOrThrow(KEY_PW_STEERING_ANGLE_ID);
        int pwSteeringAngleDegreeIndex = globalsCursor.getColumnIndexOrThrow(KEY_PW_STEERING_ANGLE_DEGREE);

        while (globalsCursor.moveToNext()) {
            int id = globalsCursor.getInt(pwSteeringAngleIdIndex);
            int angleDegree = globalsCursor.getInt(pwSteeringAngleDegreeIndex);
            pwSteeringAngleArray.add(id, ""+angleDegree);
        }

        Log.i(TAG,"pwSteeringAngleArray size count is "+ pwSteeringAngleArray.size());

        return pwSteeringAngleArray;
    }


    public Float getPWSteeringAngleDegree(int id) {

        String[] projection = {
                KEY_PW_STEERING_ANGLE_DEGREE
        };

        String[] idArray = new String[1];
        idArray[0] = ""+id;

        String selection = KEY_PW_STEERING_ANGLE_ID + "=?";
        Cursor cursor = database.query(true, PW_STEERING_ANGLE_TABLE, projection, selection, idArray, null,
                null, null, null);

        int pwSteeringAngleDegreeIndex = cursor.getColumnIndexOrThrow(KEY_PW_STEERING_ANGLE_DEGREE);
        cursor.moveToFirst();
        float angleDegree = cursor.getInt(pwSteeringAngleDegreeIndex);
        cursor.close();
        return angleDegree;
    }

    public float getSpeedOfSound(String globalId) {
        float speedOfSound;

        String selection = KEY_SOUND_SPEED_ID + "=?";

        String[] projection = {
                KEY_SPEED_OF_SOUND
        };

        String[] idArray = new String[1];
        idArray[0] = globalId;

        Cursor globalIdCursor = database.query(true, SOUND_SPEED_TABLE, projection, selection, idArray, null,
                null, null, null);

        globalIdCursor.moveToFirst();
        speedOfSound = globalIdCursor.getFloat(globalIdCursor.getColumnIndex(KEY_SPEED_OF_SOUND));
        globalIdCursor.close();

        return speedOfSound;
    }

    public int[] getDefaultColorMapIndexes(String organId) {
        int[] colorMapIndexes = {0, 0, 0};

        String selection = KEY_ORGAN_ID + "=?";

        String[] projection = {
                KEY_GLOBAL_ID,
                KEY_VELOCITY_MAP,
                KEY_POWER_MAP,
                KEY_SOUND_SPEED_INDEX
        };

        String[] idArray = new String[1];
        idArray[0] = organId;

        Cursor globalIdCursor = database.query(true, ORGANS_TABLE, projection, selection, idArray, null,
                null, null, null);

        globalIdCursor.moveToFirst();
        colorMapIndexes[0]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_VELOCITY_MAP));
        colorMapIndexes[1]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_POWER_MAP));
        colorMapIndexes[2]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_SOUND_SPEED_INDEX));
        globalIdCursor.close();

        return colorMapIndexes;
    }

    /**
     * Get the ColorMap list from the thor.db
     * @param database  Database object point to thor.db
     * @return List of ColorMap from thor.db
     */
    public List<ColorMapModel> getColorMapList(SQLiteDatabase database) {
        List<ColorMapModel> colorMapList = new ArrayList<>();

        String[] projection = {
                KEY_RGB,
                KEY_TYPE
        };

        try (Cursor globalIdCursor = database.query(true, COLOR_MAP_TABLE, projection, null, null, null,
                null, null, null)) {
            if (globalIdCursor != null) {
                while (globalIdCursor.moveToNext()) {
                    int rgbColumn = globalIdCursor.getColumnIndexOrThrow(KEY_RGB);
                    int typeColumn = globalIdCursor.getColumnIndexOrThrow(KEY_TYPE);
                    byte[] rgbMap = globalIdCursor.getBlob(rgbColumn);
                    boolean isVelocity = globalIdCursor.getString(typeColumn).equalsIgnoreCase("vel");
                    boolean isPow = globalIdCursor.getString(typeColumn).equalsIgnoreCase("pow");
                    ColorMapModel colorMapModel = new ColorMapModel(null, null, rgbMap, isVelocity, false, globalIdCursor.getPosition());
                    colorMapModel.setPow(isPow);
                    colorMapList.add(colorMapModel);
                }
            }
        } catch (SQLiteException sqlException) {
            MedDevLog.error(TAG, "getColorMapList() SQLiteException : " +  sqlException.getMessage());
        }

        return colorMapList;
    }

    public List<WorkFlow> getEFWorkFLows() {

        List<WorkFlow> workFlows = new ArrayList<>();

        String query = "Select wf.*, o.Name from "+WORK_FLOWS_TABLE + " wf , "+ORGANS_TABLE+" o where wf.Organ=o.ID and wf.Job='EF' ";

        /*Cursor workFlowCursor = database.query(true, WORK_FLOWS_TABLE, null, null, null, null,
                null, null, null);*/

        Cursor workFlowCursor = database.rawQuery(query,null);

        if(workFlowCursor!=null && workFlowCursor.getCount()>0){
            workFlowCursor.moveToFirst();
            WorkFlow efWorkFlow = new WorkFlow();
            efWorkFlow.setId(workFlowCursor.getInt(0));
            efWorkFlow.setGlobalId(workFlowCursor.getInt(1));
            efWorkFlow.setJob(workFlowCursor.getString(2));
            efWorkFlow.setMethod(workFlowCursor.getString(3));
            efWorkFlow.setOrgan(workFlowCursor.getInt(4));
            efWorkFlow.setView(workFlowCursor.getInt(5));
            efWorkFlow.setMode(workFlowCursor.getInt(6));
            efWorkFlow.setDefaultDepthEasy(workFlowCursor.getInt(7));
            efWorkFlow.setDefaultDepthGeneral(workFlowCursor.getInt(8));
            efWorkFlow.setDefaultDepthHard(workFlowCursor.getInt(9));
            efWorkFlow.setOrganName(workFlowCursor.getString(10));
            workFlows.add(efWorkFlow);
            workFlowCursor.close();
        }

        return workFlows;

    }

    /**
     * get cw prf values float array
     * @return cwPrfArray
     */
    public SparseArray<Double> getCwPRFs(){

        SparseArray<Double> cwPrfArray=new SparseArray<>();

        String[] projection = {
                KEY_CW_PRF_ID,
                KEY_CW_PRF_HZ
        };

        Cursor globalsCursor = database.query(true, CW_PRF_TABLE, projection, null,null, null,
                null, null, null);

        int prfHzIndex = globalsCursor.getColumnIndexOrThrow(KEY_CW_PRF_HZ);
        int prfIdIndex = globalsCursor.getColumnIndexOrThrow(KEY_CW_PRF_ID);

        while (globalsCursor.moveToNext()) {
            int id = globalsCursor.getInt(prfIdIndex);
            Double mPrfHz = Double.valueOf(globalsCursor.getString(prfHzIndex));
            cwPrfArray.put(id, mPrfHz);
        }

        Log.i(TAG,"CW prf count is "+ cwPrfArray.size());

        return cwPrfArray;
    }

    /**
     * get cw prf value
     * @param index
     * @return mPrfHz
     */
    public Double getCWPrf(Integer index){
        String selection = KEY_CW_PRF_ID + "=?";

        String[] projection = {
                KEY_CW_PRF_ID,
                KEY_CW_PRF_HZ
        };


        String[] id = new String[1];
        id[0] = index.toString();

        Cursor globalsCursor = database.query(true, CW_PRF_TABLE, projection, selection, id, null,
                null, null, null);

        int prfHzIndex = globalsCursor.getColumnIndexOrThrow(KEY_CW_PRF_HZ);

        globalsCursor.moveToFirst();

        Double mPrfHz = Double.valueOf(globalsCursor.getString(prfHzIndex));

        globalsCursor.close();
        Log.d(TAG,"mCwPrfHz : " + mPrfHz);
        return mPrfHz;
    }

    /**
     * get cw prf value
     * @param index
     * @return mPrfHz
     */
    public Double getCWPrf(Integer index, Integer probeType){
        String mUpsPath = GlobalUltrasoundManager.getInstance().getUpsPathProbeWise(probeType);
        SQLiteDatabase database = SQLiteDatabase.openDatabase(
                mUpsPath,
                null, SQLiteDatabase.OPEN_READONLY);
        String selection = KEY_CW_PRF_ID + "=?";

        String[] projection = {
                KEY_CW_PRF_ID,
                KEY_CW_PRF_HZ
        };


        String[] id = new String[1];
        id[0] = index.toString();

        Cursor globalsCursor = database.query(true, CW_PRF_TABLE, projection, selection, id, null,
                null, null, null);

        int prfHzIndex = globalsCursor.getColumnIndexOrThrow(KEY_CW_PRF_HZ);

        globalsCursor.moveToFirst();

        Double mPrfHz = Double.valueOf(globalsCursor.getString(prfHzIndex));

        globalsCursor.close();
        database.close();
        Log.d(TAG,"mCwPrfHz : " + mPrfHz);
        return mPrfHz;
    }

    /**
     * get pw prf values float array
     * @return pwPrfArray
     */
    public SparseArray<Double> getPwPRFs(){

        SparseArray<Double> pwPrfArray=new SparseArray<>();

        String[] projection = {
                KEY_PW_PRF_ID,
                KEY_PW_PRF_HZ
        };

        Cursor globalsCursor = database.query(true, PW_PRF_TABLE, projection, null,null, null,
                null, null, null);

        int prfHzIndex = globalsCursor.getColumnIndexOrThrow(KEY_PW_PRF_HZ);
        int prfIdIndex = globalsCursor.getColumnIndexOrThrow(KEY_PW_PRF_ID);

        while (globalsCursor.moveToNext()) {
            int id = globalsCursor.getInt(prfIdIndex);
            Double mPrfHz = Double.valueOf(globalsCursor.getString(prfHzIndex));
            pwPrfArray.put(id, mPrfHz);
        }

        Log.i(TAG,"PW prf count is "+ pwPrfArray.size());

        return pwPrfArray;
    }

    /**
     * get pw prf value
     * @param index
     * @return mPrfHz
     */
    public Double getPWPrf(Integer index){
        String selection = KEY_PW_PRF_ID + "=?";

        String[] projection = {
                KEY_PW_PRF_ID,
                KEY_PW_PRF_HZ
        };


        String[] id = new String[1];
        id[0] = index.toString();

        Cursor globalsCursor = database.query(true, PW_PRF_TABLE, projection, selection, id, null,
                null, null, null);

        int prfHzIndex = globalsCursor.getColumnIndexOrThrow(KEY_PW_PRF_HZ);

        globalsCursor.moveToFirst();

        Double mPrfHz = Double.valueOf(globalsCursor.getString(prfHzIndex));

        globalsCursor.close();
        Log.d(TAG,"mPwPrfHz : " + mPrfHz);
        return mPrfHz;
    }

    /**
     * get pw prf value
     * @param index
     * @return mPrfHz
     */
    public Double getPWPrf(Integer index, Integer probeType){
        String mUpsPath = GlobalUltrasoundManager.getInstance().getUpsPathProbeWise(probeType);
        SQLiteDatabase database = SQLiteDatabase.openDatabase(
                mUpsPath,
                null, SQLiteDatabase.OPEN_READONLY);
        String selection = KEY_PW_PRF_ID + "=?";

        String[] projection = {
                KEY_PW_PRF_ID,
                KEY_PW_PRF_HZ
        };


        String[] id = new String[1];
        id[0] = index.toString();

        Cursor globalsCursor = database.query(true, PW_PRF_TABLE, projection, selection, id, null,
                null, null, null);

        int prfHzIndex = globalsCursor.getColumnIndexOrThrow(KEY_PW_PRF_HZ);

        globalsCursor.moveToFirst();

        Double mPrfHz = Double.valueOf(globalsCursor.getString(prfHzIndex));

        globalsCursor.close();
        database.close();
        Log.d(TAG,"mPwPrfHz : " + mPrfHz);
        return mPrfHz;
    }

    /**
     * get TDI prf values float array
     * @return tdiPrfArray
     */
    public SparseArray<Double> getTdiPRFs(){

        SparseArray<Double> pwPrfArray=new SparseArray<>();

        String[] projection = {
                KEY_TDI_PRF_ID,
                KEY_TDI_PRF_HZ
        };

        Cursor globalsCursor = database.query(true, TDI_PRF_TABLE, projection, null,null, null,
                null, null, null);

        int prfHzIndex = globalsCursor.getColumnIndexOrThrow(KEY_TDI_PRF_HZ);
        int prfIdIndex = globalsCursor.getColumnIndexOrThrow(KEY_TDI_PRF_ID);

        while (globalsCursor.moveToNext()) {
            int id = globalsCursor.getInt(prfIdIndex);
            Double mPrfHz = Double.valueOf(globalsCursor.getString(prfHzIndex));
            pwPrfArray.put(id, mPrfHz);
        }

        Log.i(TAG,"TDI prf count is "+ pwPrfArray.size());

        return pwPrfArray;
    }

    /**
     * get tdi prf value
     * @param index
     * @param  probeType
     * @return mPrfHz
     */
    public Double getTDIPrf(Integer index,Integer probeType){
        Log.d(TAG,"mTDIPrfHz getTDIPrf probeType : " + probeType);
        String mUpsPath = GlobalUltrasoundManager.getInstance().getUpsPathProbeWise(probeType);
        SQLiteDatabase mDatabase = SQLiteDatabase.openDatabase(
                mUpsPath,
                null, SQLiteDatabase.OPEN_READONLY);

        String selection = KEY_PW_PRF_ID + "=?";

        String[] projection = {
                KEY_TDI_PRF_ID,
                KEY_TDI_PRF_HZ
        };


        String[] id = new String[1];
        id[0] = index.toString();

        Cursor globalsCursor = mDatabase.query(true, TDI_PRF_TABLE, projection, selection, id, null,
                null, null, null);

        int prfHzIndex = globalsCursor.getColumnIndexOrThrow(KEY_TDI_PRF_HZ);

        globalsCursor.moveToFirst();

        Double mPrfHz = Double.valueOf(globalsCursor.getString(prfHzIndex));

        globalsCursor.close();
        mDatabase.close();
        Log.d(TAG,"mTDIPrfHz : " + mPrfHz);
        return mPrfHz;
    }


    /**
     * get the total number of base line shift fraction count
     * @return total number of base line shift fraction count
     */
    public int getBaseLineShiftFractionCount() {
        int count = 0;
        String[] projection = {
                KEY_PW_BASELINE_SHIFT_ID
        };
        Cursor cursor = database.query(PW_BASELINE_SHIFT_TABLE, projection, null, null,
                null, null, null);
        count = cursor.getCount();
        cursor.close();
        return count;
    }

    /**
     * get base line shift fraction
     * @param index
     * @return baseLineShiftFraction
     */
    public Double getBaseLineShiftFraction(Integer index){
        String selection = KEY_PW_BASELINE_SHIFT_ID + "=?";

        String[] projection = {
                KEY_PW_BASELINE_SHIFT_ID,
                KEY_PW_BASELINE_SHIFT_FRACTION
        };


        String[] id = new String[1];
        id[0] = index.toString();

        Cursor globalsCursor = database.query(true, PW_BASELINE_SHIFT_TABLE, projection, selection, id, null,
                null, null, null);

        int baseLiftFractionIndex = globalsCursor.getColumnIndexOrThrow(KEY_PW_BASELINE_SHIFT_FRACTION);

        globalsCursor.moveToFirst();

        Double baseLineShiftFraction = Double.valueOf(globalsCursor.getString(baseLiftFractionIndex));

        globalsCursor.close();
        Log.d(TAG,"baseLineShiftFraction : " + baseLineShiftFraction);
        return baseLineShiftFraction;
    }

    /**
     * get base line shift fraction
     * @param index
     * @return baseLineShiftFraction
     */
    public Double getBaseLineShiftFraction(Integer index, Integer probeType){
        String mUpsPath = GlobalUltrasoundManager.getInstance().getUpsPathProbeWise(probeType);
        SQLiteDatabase database = SQLiteDatabase.openDatabase(
                mUpsPath,
                null, SQLiteDatabase.OPEN_READONLY);
        String selection = KEY_PW_BASELINE_SHIFT_ID + "=?";

        String[] projection = {
                KEY_PW_BASELINE_SHIFT_ID,
                KEY_PW_BASELINE_SHIFT_FRACTION
        };


        String[] id = new String[1];
        id[0] = index.toString();

        Cursor globalsCursor = database.query(true, PW_BASELINE_SHIFT_TABLE, projection, selection, id, null,
                null, null, null);

        int baseLiftFractionIndex = globalsCursor.getColumnIndexOrThrow(KEY_PW_BASELINE_SHIFT_FRACTION);

        globalsCursor.moveToFirst();

        Double baseLineShiftFraction = Double.valueOf(globalsCursor.getString(baseLiftFractionIndex));

        globalsCursor.close();
        database.close();
        Log.d(TAG,"baseLineShiftFraction : " + baseLineShiftFraction);
        return baseLineShiftFraction;
    }


    /**
     * get auto freeze time for cw in min
     * @return auto freeze time cw in min
     */
    public Float getAutoFreezeTimeForCwInMin(){
        String[] projection = {
                KEY_CW_AUTO_FREEZE_TIME_MIN
        };
        Cursor globalsCursor = database.query(true, GLOBALS_TABLE, projection, null, null, null,
                null, null, null);

        int mCwAutoFreezeTimeMinIndex = globalsCursor.getColumnIndexOrThrow(KEY_CW_AUTO_FREEZE_TIME_MIN);

        globalsCursor.moveToFirst();

        Float mCwAutoFreezeTimeMinValue = Float.valueOf(globalsCursor.getString(mCwAutoFreezeTimeMinIndex));

        globalsCursor.close();
        Log.d(TAG,"getAutoFreezeTimeForCwInMin : " + mCwAutoFreezeTimeMinValue);
        return mCwAutoFreezeTimeMinValue;
    }

    /**
     * get gate selection line constraints
     * @return mGateSelectionLineConstraints
     */

    public GateSelectionLineConstraints getGateSelectionLineConstraints(boolean isLexsaProbe) {

        String[] projection;
        if(isLexsaProbe) {
            projection = new String[]{
                    KEY_MIN_GATE_AXIAL_START_MM_OFFSET,
                    KEY_MAX_GATE_AXIAL_START_MM_OFFSET,
                    KEY_MIN_GATE_AZIMUTH_START_MM_OFFSET,
                    KEY_MAX_GATE_AZIMUTH_START_MM_OFFSET
            };
        } else {
            projection = new String[] {
                    KEY_MIN_GATE_AXIAL_START_MM_OFFSET,
                    KEY_MAX_GATE_AXIAL_START_MM_OFFSET,
                    KEY_MIN_GATE_AZIMUTH_START_DEGREE_OFFSET,
                    KEY_MAX_GATE_AZIMUTH_START_DEGREE_OFFSET
            };
        }

        Cursor globalsCursor = database.query(true, GLOBALS_TABLE, projection, null, null, null,
                null, null, null);

        int minGateAxialStartMmIndex = globalsCursor.getColumnIndexOrThrow(KEY_MIN_GATE_AXIAL_START_MM_OFFSET);
        int maxGateAxialStartMmIndex = globalsCursor.getColumnIndexOrThrow(KEY_MAX_GATE_AXIAL_START_MM_OFFSET);

        int minGateAzimuthStartDegreeIndex;
        int maxGateAzimuthStartDegreeIndex;
        if(isLexsaProbe) {
            minGateAzimuthStartDegreeIndex = globalsCursor.getColumnIndexOrThrow(KEY_MIN_GATE_AZIMUTH_START_MM_OFFSET);
            maxGateAzimuthStartDegreeIndex = globalsCursor.getColumnIndexOrThrow(KEY_MAX_GATE_AZIMUTH_START_MM_OFFSET);
        } else {
            minGateAzimuthStartDegreeIndex = globalsCursor.getColumnIndexOrThrow(KEY_MIN_GATE_AZIMUTH_START_DEGREE_OFFSET);
            maxGateAzimuthStartDegreeIndex = globalsCursor.getColumnIndexOrThrow(KEY_MAX_GATE_AZIMUTH_START_DEGREE_OFFSET);
        }

        globalsCursor.moveToFirst();

        GateSelectionLineConstraints mGateSelectionLineConstraints = new GateSelectionLineConstraints();

        mGateSelectionLineConstraints.setMinGateAxialStartMmOffset(globalsCursor.getInt(minGateAxialStartMmIndex));
        mGateSelectionLineConstraints.setMaxGateAxialStartMmOffset(globalsCursor.getInt(maxGateAxialStartMmIndex));
        mGateSelectionLineConstraints.setMinGateAzimuthStartDegreeOffset(globalsCursor.getInt(minGateAzimuthStartDegreeIndex));
        mGateSelectionLineConstraints.setMaxGateAzimuthStartDegreeOffset(globalsCursor.getInt(maxGateAzimuthStartDegreeIndex));

        globalsCursor.close();
        return mGateSelectionLineConstraints;
    }

    public Float getTDICenterFrequency(){
        // for TX ID for TDI value is 14 references from UpsReader::getPwWaveformInfo function from UpsReader.cpp
        String TX_ID_FOR_TDI = "14";
        String selection = KEY_TX_ID + "=?";

        String[] projection = {
                KEY_CENTER_FREQUENCY
        };


        String[] id = new String[1];
        id[0]= TX_ID_FOR_TDI;

        Cursor globalsCursor = database.query(true, TX_WAVEFORM_TABLE, projection, selection, id, null,
                null, null, null);

        int centerFrequencyIndex = globalsCursor.getColumnIndexOrThrow(KEY_CENTER_FREQUENCY);

        globalsCursor.moveToFirst();

        Float centerFrequency = Float.valueOf(globalsCursor.getString(centerFrequencyIndex));

        globalsCursor.close();
        Log.d(TAG,"centerFrequency : " + centerFrequency);
        return centerFrequency;
    }


    public Float getPWCenterFrequencyLinear(int organId, float pwFocus, float gateSize,Integer probeType) {
        String selection = KEY_ORGAN + "="+organId+" and "+KEY_FOCUS+"=" +pwFocus+ " and "+KEY_GATE_RANGE+"="+ gateSize;
        String mUpsPath = GlobalUltrasoundManager.getInstance().getUpsPathProbeWise(probeType);
        SQLiteDatabase mDatabase = SQLiteDatabase.openDatabase(
                mUpsPath,
                null, SQLiteDatabase.OPEN_READONLY);

        String[] projection = {
                KEY_CENTER_FREQUENCY
        };

        Cursor globalsCursor = mDatabase.query(true, TX_WAVEFORM_TABLE, projection, selection, null, null,
                null, null, null);

        int centerFrequencyIndex = globalsCursor.getColumnIndexOrThrow(KEY_CENTER_FREQUENCY);

        globalsCursor.moveToFirst();

        Float centerFrequency = Float.valueOf(globalsCursor.getString(centerFrequencyIndex));

        globalsCursor.close();

        Log.d(TAG,"getPWCenterFrequencyLinear centerFrequency : " + centerFrequency);

        return centerFrequency;
    }

    /**
     * get pw gate angle degree
     * @param index
     * @return angle degree
     */
    public Float getGateAngleDegree(Integer index){
        String selection = KEY_GATE_ANGLE_ADJUST_ID + "=?";

        String[] projection = {
                KEY_GATE_ANGLE_ADJUST_ID,
                KEY_ANGLE_DEGREES
        };


        String[] id = new String[1];
        id[0] = index.toString();

        Cursor globalsCursor = database.query(true, PW_GATE_ANGLE_ADJUST_TABLE, projection, selection, id, null,
                null, null, null);

        int pwGateAngleAdjustIndex = globalsCursor.getColumnIndexOrThrow(KEY_ANGLE_DEGREES);

        globalsCursor.moveToFirst();

        Float pwGateAngleAdjust = Float.valueOf(globalsCursor.getString(pwGateAngleAdjustIndex));

        globalsCursor.close();
        Log.d(TAG,"pwGateAngleAdjust : " + pwGateAngleAdjust);
        return pwGateAngleAdjust;
    }

    /**
     * get pw gate angle degree
     * @param index
     * @return angle degree
     */
    public Float getGateAngleDegree(Integer index, Integer probeType){
        String mUpsPath = GlobalUltrasoundManager.getInstance().getUpsPathProbeWise(probeType);
        SQLiteDatabase database = SQLiteDatabase.openDatabase(
                mUpsPath,
                null, SQLiteDatabase.OPEN_READONLY);
        String selection = KEY_GATE_ANGLE_ADJUST_ID + "=?";

        String[] projection = {
                KEY_GATE_ANGLE_ADJUST_ID,
                KEY_ANGLE_DEGREES
        };


        String[] id = new String[1];
        id[0] = index.toString();

        Cursor globalsCursor = database.query(true, PW_GATE_ANGLE_ADJUST_TABLE, projection, selection, id, null,
                null, null, null);

        int pwGateAngleAdjustIndex = globalsCursor.getColumnIndexOrThrow(KEY_ANGLE_DEGREES);

        globalsCursor.moveToFirst();

        Float pwGateAngleAdjust = Float.valueOf(globalsCursor.getString(pwGateAngleAdjustIndex));

        globalsCursor.close();
        database.close();
        Log.d(TAG,"pwGateAngleAdjust : " + pwGateAngleAdjust);
        return pwGateAngleAdjust;
    }

    /**
     * get gate angle id form angle
     * @param angle
     * @return id
     */
    public Integer getGateAngleId(Integer angle){
        String[] projection = {
                KEY_GATE_ANGLE_ADJUST_ID
        };

        String[] angleArray = new String[1];
        angleArray[0] = String.valueOf(angle);

        String selection = KEY_ANGLE_DEGREES + "=?";
        Cursor cursor = database.query(true, PW_GATE_ANGLE_ADJUST_TABLE, projection, selection, angleArray, null,
                null, null, null);

        int angleIdColumn = cursor.getColumnIndexOrThrow(KEY_GATE_ANGLE_ADJUST_ID);
        cursor.moveToFirst();
        Integer angleId = cursor.getInt(angleIdColumn);
        cursor.close();
        return angleId;
    }

    /**
     * get the total number of gate sizes
     * @return total number of gate sizes
     */
    public int getGateSizeCount() {
        int count = 1;
        String[] projection = {
                KEY_GATE_SIZE_MM
        };

        Cursor cursor = database.query(PW_GATE_SIZE_TABLE, projection, null, null,
                null, null, null);
        count = cursor.getCount();

        cursor.close();

        return count;
    }


    /**
     * get pw gate size array array
     * @return pwGateSizeArray
     */
    public SparseArray<Float> getGateSizeArray(int probeType,int organId){

        SparseArray<Float> pwGateSizeArray=new SparseArray<>();

        String[] projection = {
                KEY_GATE_SIZE_ID,
                KEY_GATE_SIZE_MM
        };

        String selection =  null;
        if(probeType == AppConstants.PROBE_LEXSA) {
           selection = KEY_ORGAN + "=" + organId;
        }

        Cursor globalsCursor = database.query(true, PW_GATE_SIZE_TABLE, projection, selection,null, null,
                null, null, null);

        int gateSizeMmIndex = globalsCursor.getColumnIndexOrThrow(KEY_GATE_SIZE_MM);
        int gateSizeIdIndex = globalsCursor.getColumnIndexOrThrow(KEY_GATE_SIZE_ID);

        while (globalsCursor.moveToNext()) {
            int id = globalsCursor.getInt(gateSizeIdIndex);
            float gateSize = globalsCursor.getFloat(gateSizeMmIndex);
            pwGateSizeArray.put(id, gateSize);
        }

        Log.i(TAG,"Gate size count is "+ pwGateSizeArray.size());

        return pwGateSizeArray;
    }

    /**
     * get pw gate size in mm
     * @param index
     * @return gate size in mm
     */
    public float getGateSize(Integer index, Integer probeType){

        String mUpsPath = GlobalUltrasoundManager.getInstance().getUpsPathProbeWise(probeType);
        SQLiteDatabase mDatabase = SQLiteDatabase.openDatabase(
                mUpsPath,
                null, SQLiteDatabase.OPEN_READONLY);

        String selection = KEY_GATE_SIZE_ID + "=?";

        String[] projection = {
                KEY_GATE_SIZE_ID,
                KEY_GATE_SIZE_MM
        };


        String[] id = new String[1];
        id[0] = index.toString();

        Cursor globalsCursor = mDatabase.query(true, PW_GATE_SIZE_TABLE, projection, selection, id, null,
                null, null, null);

        int pwGateSizeIndex = globalsCursor.getColumnIndexOrThrow(KEY_GATE_SIZE_MM);

        globalsCursor.moveToFirst();

        float pwGateSize = globalsCursor.getFloat(pwGateSizeIndex);

        globalsCursor.close();
        Log.d(TAG,"pwGateSize : " + pwGateSize);
        return pwGateSize;
    }

    /**
     * get default pw indexes based on oraganId
     * @param organId
     * @param isLinearProbe
     * @return
     */
    public int[] getDefaultPWIndexes(String organId,Boolean isLinearProbe) {

        if(isLinearProbe) {
            int[] pwIndexes = {0, 0, 0, 0, 0, 0, 0, 0,0};

            String selection = KEY_ORGAN_ID + "=?";

            String[] projection = {
                    KEY_GLOBAL_ID,
                    KEY_SOUND_SPEED_INDEX,
                    KEY_DEFAULT_GATE_SIZE_INDEX,
                    KEY_DEFAULT_ANGLE_ADJUST_INDEX,
                    KEY_DEFAULT_PW_WALL_FILTER_INDEX,
                    KEY_DEFAULT_PW_PRF_INDEX,
                    KEY_DEFAULT_PW_BASE_LINE_SHIFT_INDEX,
                    KEY_DEFAULT_TDI_PRF_INDEX,
                    KEY_DEFAULT_DEFAULT_STEERING_ANGLE_INDEX
            };

            String[] idArray = new String[1];
            idArray[0] = organId;

            Cursor globalIdCursor = database.query(true, ORGANS_TABLE, projection, selection, idArray, null,
                    null, null, null);

            globalIdCursor.moveToFirst();
            pwIndexes[PW_ARRAY_INDEX_GLOBAL_ID]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_GLOBAL_ID));
            pwIndexes[PW_ARRAY_INDEX_SOUND_SPEED]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_SOUND_SPEED_INDEX));
            pwIndexes[PW_ARRAY_INDEX_GATE_SIZE]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_DEFAULT_GATE_SIZE_INDEX));
            pwIndexes[PW_ARRAY_INDEX_ANGLE_ADJUST]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_DEFAULT_ANGLE_ADJUST_INDEX));
            pwIndexes[PW_ARRAY_INDEX_WALL_FILTER]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_DEFAULT_PW_WALL_FILTER_INDEX));
            pwIndexes[PW_ARRAY_INDEX_PRF]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_DEFAULT_PW_PRF_INDEX));
            pwIndexes[PW_ARRAY_INDEX_BASE_LINE_SHIFT]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_DEFAULT_PW_BASE_LINE_SHIFT_INDEX));
            pwIndexes[TDI_ARRAY_INDEX_PRF]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_DEFAULT_TDI_PRF_INDEX));
            pwIndexes[PW_ARRAY_INDEX_STEERING_ANGLE]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_DEFAULT_DEFAULT_STEERING_ANGLE_INDEX));
            globalIdCursor.close();

            return pwIndexes;
        } else {
            int[] pwIndexes = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

            String selection = KEY_ORGAN_ID + "=?";

            String[] projection = {
                    KEY_GLOBAL_ID,
                    KEY_SOUND_SPEED_INDEX,
                    KEY_DEFAULT_GATE_SIZE_INDEX,
                    KEY_DEFAULT_ANGLE_ADJUST_INDEX,
                    KEY_DEFAULT_PW_WALL_FILTER_INDEX,
                    KEY_DEFAULT_PW_PRF_INDEX,
                    KEY_DEFAULT_PW_BASE_LINE_SHIFT_INDEX,
                    KEY_DEFAULT_TDI_PRF_INDEX,
                    KEY_DEFAULT_GATE_SIZE_INDEX_TDI
            };

            String[] idArray = new String[1];
            idArray[0] = organId;

            Cursor globalIdCursor = database.query(true, ORGANS_TABLE, projection, selection, idArray, null,
                    null, null, null);

            globalIdCursor.moveToFirst();
            pwIndexes[PW_ARRAY_INDEX_GLOBAL_ID]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_GLOBAL_ID));
            pwIndexes[PW_ARRAY_INDEX_SOUND_SPEED]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_SOUND_SPEED_INDEX));
            pwIndexes[PW_ARRAY_INDEX_GATE_SIZE]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_DEFAULT_GATE_SIZE_INDEX));
            pwIndexes[PW_ARRAY_INDEX_ANGLE_ADJUST]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_DEFAULT_ANGLE_ADJUST_INDEX));
            pwIndexes[PW_ARRAY_INDEX_WALL_FILTER]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_DEFAULT_PW_WALL_FILTER_INDEX));
            pwIndexes[PW_ARRAY_INDEX_PRF]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_DEFAULT_PW_PRF_INDEX));
            pwIndexes[PW_ARRAY_INDEX_BASE_LINE_SHIFT]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_DEFAULT_PW_BASE_LINE_SHIFT_INDEX));
            pwIndexes[TDI_ARRAY_INDEX_PRF]= globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_DEFAULT_TDI_PRF_INDEX));
            pwIndexes[TDI_ARRAY_INDEX_GATE_SIZE] = globalIdCursor.getInt(globalIdCursor.getColumnIndex(KEY_DEFAULT_GATE_SIZE_INDEX_TDI));
            globalIdCursor.close();

            return pwIndexes;
        }

    }

    /**
     *
     * @param version Pass -1 for NO_RX, 0 For Engineering,1 for Type Clinical, 2 for Production
     */
    public void setVersionValue(int version){
        getInstance().getWritableDatabase();
        database.execSQL("UPDATE `VersionInfo` SET `ReleaseType`='"+version+"' WHERE `_rowid_`='1';");
        getInstance().getReadableDatabase();
    }

    public DefaultValuesObject getDefaultIndices(){
        return getDefaultIndices(Ups.getInstance().getSelectedOrgan().getOrganId());
    }

    public DefaultValuesObject getDefaultIndicesByOrgan(int organId){
        return getDefaultIndices(organId);
    }

    public DefaultValuesObject getDefaultAiIndices(Context context){
        return getDefaultAiIndices(Ups.getInstance().getSelectedOrganAi().getOrganId());
    }

    @Override
    public void onCreate(SQLiteDatabase sqLiteDatabase) {}

    @Override
    public void onUpgrade(SQLiteDatabase sqLiteDatabase, int i, int i1) {}

    public void clean(){
        if (database!=null) {
            database.close();
        }
        Ups.getInstance().clean();
        singleton = null;
    }

    public int getProbeIdAsPerInitialiaztionOfUps() {
        return probeIdAsPerInitialiaztionOfUps;
    }

    public void setProbeIdAsPerInitialiaztionOfUps(int probeId) {
        probeIdAsPerInitialiaztionOfUps = probeId;
    }
}
