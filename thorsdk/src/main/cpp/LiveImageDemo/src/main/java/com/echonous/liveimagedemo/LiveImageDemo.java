package com.echonous.liveimagedemo;

import java.lang.String;

import com.echonous.hardware.ultrasound.Dau;
import com.echonous.hardware.ultrasound.DauException;
import com.echonous.hardware.ultrasound.ThorError;
import com.echonous.hardware.ultrasound.UltrasoundManager;
import com.echonous.hardware.ultrasound.UltrasoundPlayer;
import com.echonous.hardware.ultrasound.UltrasoundViewer;
import com.echonous.hardware.ultrasound.UltrasoundRecorder;
import com.echonous.hardware.ultrasound.Puck;
import com.echonous.hardware.ultrasound.PuckEvent;
import com.echonous.liveimagedemo.FreezeButton;
import com.echonous.liveimagedemo.FileChooser;
import com.echonous.thorsdk.R;
import com.echonous.util.LogUtils;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.ActivityInfo;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.graphics.Color;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.util.AttributeSet;
import android.view.Display;
import android.view.GestureDetector;
import android.view.ScaleGestureDetector;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.*;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.Button;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView.OnEditorActionListener;
import java.util.UUID;
import java.io.File;



//-----------------------------------------------------------------------------
public class LiveImageDemo extends Activity implements SurfaceHolder.Callback,
    OnItemSelectedListener, GestureDetector.OnGestureListener
{
    private static String TAG = "LiveImageDemo";
    private static int DEFAULT_GAIN = 50;
    private static int DEFAULT_AUTO_GAIN = 12;

    final Context mContext = this;
    private UltrasoundManager mUltrasoundMgr;
    private Dau mDau = null;
    private UltrasoundViewer mViewer = null;
    private UltrasoundPlayer mPlayer = null;
    private UltrasoundRecorder mRecorder = null;
    private boolean mFrozen = true;
    private BroadcastReceiver mReceiver;
    private Handler mHandler;
    private SurfaceHolder mSurfaceHolder = null;
    private FreezeButton mFreezeBtn;
    private EditText mEditImageId;
    private int mImagingCaseId = -1;
    private SeekBar mGainControl;
    private SeekBar mColorGainControl;
    private SeekBar mReviewControl;
    private ProbeInterface mProbeInterface = ProbeInterface.Pcie;
    private LinearLayout mManualLayout;
    private LinearLayout mAutoLayout;
    private RadioButton mManualBtn;
    private RadioButton mAutoBtn;
    private SQLiteDatabase  mDb = null;
    private Spinner mOrganSpinner;
    private Spinner mViewSpinner;
    private Spinner mModeSpinner;
    private Spinner mDepthSpinner;
    private CheckBox mAutoGainCheck;
    private CheckBox mDeSpeckleCheck;
    private CheckBox mInferenceCheck;
    private ImageButton mPlayButton;
    private ImageButton mPauseButton;
    private ImageButton mStopButton;
    private ImageButton mVideoButton;
    private ImageButton mCameraButton;
    private ImageButton mOpenButton;
    private CheckBox mLoopingCheck;
    private ImageButton mNextButton;
    private ImageButton mPreviousButton;
    private Spinner mSpeedSpinner;
    private boolean mHaveNewParams = true;
    private boolean mImageWhenReady = false;
    private int mClipDuration = 0;
    private GestureDetector mDetector;
    private ScaleGestureDetector mScaleDetector;
    private Puck mPuck;

    final private static int DAU_DISCONNECTED = 0;
    final private static int DAU_CONNECTED = 1;
    final private static int UNFREEZE = 2;

    //-------------------------------------------------------------------------
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (!isTaskRoot())
        {
            final Intent intent = getIntent();
            if (intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_ATTACHED)) {
                LogUtils.w(TAG, "Ignoring new instance");
                finish();
                return;
            }
        }

        LogUtils.i(TAG, "onCreate()");

        Display display = getWindowManager().getDefaultDisplay();
        if (display.getWidth() > 480) {
            if (display.getWidth() == 3840) {
		// Dell 4K monitor
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
            }
            else {
		// KD080D35 display
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE);
            }

            setContentView(R.layout.main_land);

            mGainControl = (SeekBar) findViewById(R.id.gain);
            mColorGainControl = (SeekBar) findViewById(R.id.color_gain);
            mReviewControl = (SeekBar) findViewById(R.id.reviewId);
            mPlayButton = (ImageButton) findViewById(R.id.playId);
            mPauseButton = (ImageButton) findViewById(R.id.pauseId);
            mStopButton = (ImageButton) findViewById(R.id.stopId);
            mLoopingCheck = (CheckBox) findViewById(R.id.loopingId);
            mNextButton = (ImageButton) findViewById(R.id.nextId);
            mPreviousButton = (ImageButton) findViewById(R.id.previousId);
            mSpeedSpinner = (Spinner) findViewById(R.id.speedId);
            mVideoButton = (ImageButton) findViewById(R.id.videoId);
            mVideoButton.setEnabled(true);
            mCameraButton = (ImageButton) findViewById(R.id.cameraId);
            mCameraButton.setEnabled(true);
            mOpenButton = (ImageButton) findViewById(R.id.openId);

            ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(
                this, R.array.speed_array, android.R.layout.simple_spinner_item);

            adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
            mSpeedSpinner.setAdapter(adapter);
            mSpeedSpinner.setOnItemSelectedListener(this);
        }
        else {
            // Standard Intrinsyc display
            setContentView(R.layout.main);
        }

        SharedPreferences settings = getSharedPreferences("LiveImageDemo", MODE_PRIVATE);

        if (null != settings) {
            mProbeInterface = ProbeInterface.values()[settings.getInt("ProbeInterface",
                                                ProbeInterface.Pcie.getValue())];
        }

        switch (mProbeInterface) {
            case Pcie:
                mUltrasoundMgr = new UltrasoundManager(mContext, UltrasoundManager.DauCommMode.Proprietary);
                break;

            case Usb:
                mUltrasoundMgr = new UltrasoundManager(mContext, UltrasoundManager.DauCommMode.Usb);
                break;

            case Emulation:
                mUltrasoundMgr = new UltrasoundManager(mContext, UltrasoundManager.DauCommMode.Emulation);
                break;
        }

        mViewer = mUltrasoundMgr.getUltrasoundViewer();
        mPlayer = mUltrasoundMgr.getUltrasoundPlayer();
        mRecorder = mUltrasoundMgr.getUltrasoundRecorder();
        mRecorder.registerCompletionCallback(mRecorderCompletionCallback, mHandler);
        mPlayer.setLooping(false);
        mPlayer.registerProgressCallback(mPlayerProgressCallback, mHandler);

        try {
            mPuck = mUltrasoundMgr.openPuck();
        } catch (DauException ex) {
            showErrorDialog(ex);
            return;
        }
        mPuck.registerEventCallback(mPuckCallback, mHandler);

        // These two lines are just to verify that the ThorError class has been
        // generated properly.
        LogUtils.d(TAG, "THOR_OK description: " + ThorError.THOR_OK.getDescription());
        LogUtils.d(TAG, "THOR_ERROR description: " + ThorError.THOR_ERROR.getDescription());

        mReceiver = new MyReceiver();

        mManualLayout = (LinearLayout) findViewById(R.id.manualGroupId);
        mAutoLayout = (LinearLayout) findViewById(R.id.autoGroupId);

        mFreezeBtn = (FreezeButton) findViewById(R.id.freeze);
        if (null == mFreezeBtn) {
            LogUtils.e(TAG, "Can't find freeze button");
        }
        else {
            mFreezeBtn.setSilentChecked(false);
            mFreezeBtn.setEnabled(false);

            mFreezeBtn.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                    if (null == mDau) {
                        LogUtils.i(TAG, "mDau is null");
                        return;
                    }

                    if (mFreezeBtn.ignoreChecked()) {
                        return;
                    }

                    if (mFrozen) {
                        unfreeze();
                    } else {
                        freeze();
                    }
                }
            });
        }


        SurfaceView surfaceView = (SurfaceView) findViewById(R.id.surfaceview);
        surfaceView.getHolder().addCallback(this);
        surfaceView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                mDetector.onTouchEvent(event);
                mScaleDetector.onTouchEvent(event);
                return true;
            }
        });

        mHandler = new Handler(Looper.getMainLooper()) {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case DAU_CONNECTED:
                        mFreezeBtn.setEnabled(true);
                        if (null == mDau && null != mSurfaceHolder) {
                            prepareImaging();
                        }
                        // NOTE: Uncomment to automatically unfreeze
                        //else if (null != mDau) {
                        //    unfreeze();
                        //}
                        break;

                    case DAU_DISCONNECTED:
                        mFreezeBtn.setEnabled(false);
                        closeDau();
                        break;

                    case UNFREEZE:
                        unfreeze();
                        break;

                    default:
                        super.handleMessage(msg);
                }
            }
        };

        mEditImageId = (EditText) findViewById(R.id.editImageId);
        mEditImageId.setTextColor(Color.WHITE);
        mEditImageId.setFocusable(true);
        mEditImageId.setShowSoftInputOnFocus(true);
        if (null != settings) {
            mEditImageId.setText(settings.getString("ImageCaseID", "0"));
        }

        mEditImageId.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                mEditImageId.clearFocus();
                mEditImageId.requestFocus();
                InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.showSoftInput(mEditImageId, InputMethodManager.SHOW_IMPLICIT);
            }
        });

        mEditImageId.setOnEditorActionListener(new OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView view, int actionId, KeyEvent event) {
                boolean handled = false;

                if (EditorInfo.IME_ACTION_DONE == actionId) {
                    LogUtils.d(TAG, "EditorInfo.IME_ACTION_DONE");
                    mHaveNewParams = true;

                    if (null != settings) {
                        SharedPreferences.Editor prefEditor = settings.edit();
                        prefEditor.putString("ImageCaseID", mEditImageId.getText().toString());
                        prefEditor.commit();
                    }

                    unfreeze();
                    handled = true;
                }

                return handled;
            }
        });

        if (null != mGainControl) {
            mGainControl.setProgress(DEFAULT_GAIN);
            mGainControl.setEnabled(false);

            mGainControl.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
                public void onProgressChanged(SeekBar seekBar, int progress, 
                                              boolean fromUser) {
                    if (null != mDau && fromUser) {
                        mDau.setGain(progress);
                    }
                }

                public void onStartTrackingTouch(SeekBar seekBar) {
                }

                public void onStopTrackingTouch(SeekBar seekBar) {
                }
            });
        }

        if (null != mColorGainControl) {
            mColorGainControl.setProgress(DEFAULT_GAIN);
            mColorGainControl.setEnabled(false);

            mColorGainControl.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
                public void onProgressChanged(SeekBar seekBar, int progress, 
                                              boolean fromUser) {
                    if (null != mDau && fromUser) {
                        mDau.setColorGain(progress);
                    }
                }

                public void onStartTrackingTouch(SeekBar seekBar) {
                }

                public void onStopTrackingTouch(SeekBar seekBar) {
                }
            });
        }
        
        if (null != mReviewControl) {
            mReviewControl.setProgress(0);
            mReviewControl.setEnabled(false);

            mReviewControl.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
                public void onProgressChanged(SeekBar seekbar, int progress,
                                              boolean fromUser) {
                        if (null != mPlayer && fromUser) {
                            mPlayer.seekTo(progress);
                        }
                    }

                public void onStartTrackingTouch(SeekBar seekBar) {
                }

                public void onStopTrackingTouch(SeekBar seekBar) {
                }
            });
        }

        mOrganSpinner = (Spinner) findViewById(R.id.organId);
        mViewSpinner = (Spinner) findViewById(R.id.viewId);
        mModeSpinner = (Spinner) findViewById(R.id.modeId);
        mDepthSpinner = (Spinner) findViewById(R.id.depthId);

        mAutoGainCheck = (CheckBox) findViewById(R.id.autogainId);
        mDeSpeckleCheck = (CheckBox) findViewById(R.id.despeckleId);
        mInferenceCheck = (CheckBox) findViewById(R.id.inferenceId);

        mManualBtn = (RadioButton) findViewById(R.id.radioManualId);
        mAutoBtn = (RadioButton) findViewById(R.id.radioAutoId);

        if (null != settings && null != mAutoBtn && null != mManualBtn) {
            if (1 == settings.getInt("Auto", 1)) {
                mAutoBtn.setChecked(true);
                onRadioButtonClicked(mAutoBtn);
            }
            else {
                mManualBtn.setChecked(true);
                onRadioButtonClicked(mManualBtn);
            }
        }

        enableImagingControls(true);

        LogUtils.d(TAG, "getFilesDir() = " + getFilesDir());
        LogUtils.d(TAG, "getFilesDir().getParent() = " + getFilesDir().getParent());
        LogUtils.d(TAG, "getCacheDir() = " + getCacheDir());
        LogUtils.d(TAG, "getDataDir() = " + getDataDir());
        LogUtils.d(TAG, "getExternalCacheDir() = " + getExternalCacheDir());
        LogUtils.d(TAG, "getObbDir() = " + getObbDir());

        mDetector = new GestureDetector(this, this);

        mScaleDetector = new ScaleGestureDetector(this, new
            ScaleGestureDetector.OnScaleGestureListener() {
                @Override
                public void onScaleEnd(ScaleGestureDetector detector) {
                }

                @Override
                public boolean onScaleBegin(ScaleGestureDetector detector) {
                    return true;
                }

                @Override
                public boolean onScale(ScaleGestureDetector detector) {
                    mViewer.handleOnScale(detector.getScaleFactor());
                    return false;
                }
        });

        // Create folder for raw clips and images
        File    rawDir = new File(getRawPath());

        if (!rawDir.exists()) {
            rawDir.mkdirs();
        }
    }

    //-------------------------------------------------------------------------
    @Override
    protected void onStart() {
        super.onStart();
        LogUtils.i(TAG, "onStart()");
    }

    //-------------------------------------------------------------------------
    @Override
    protected void onResume() {
        super.onResume();
        LogUtils.i(TAG, "onResume()");

        IntentFilter filter = new IntentFilter();

        filter.addAction(UltrasoundManager.ACTION_DAU_CONNECTED);
        filter.addAction(UltrasoundManager.ACTION_DAU_DISCONNECTED);
        registerReceiver(mReceiver, filter);

        // NOTE: Uncomment to automatically unfreeze
        //if (null != mDau) {
        //    unfreeze();
        //}
        //else {
            if (null != mSurfaceHolder &&
                null != mUltrasoundMgr) {
            
                if (mUltrasoundMgr.isDauConnected()) {
                    prepareImaging();
                }
            }
        //}
    }
    
    //-------------------------------------------------------------------------
    @Override
    protected void onPause() {
        super.onPause();
        LogUtils.i(TAG, "onPause()");

        if (null != mDau) {
            freeze();
        }

        if (null != mPlayer) {
            if (mPlayer.isRawFileOpen()) {
                mPlayer.closeRawFile();
            }
            mPlayer.detachCine();
            mPlayer.unregisterProgressCallback();
        }

        if (null != mPuck) {
            mPuck.close();
            mPuck = null;
        }

        unregisterReceiver(mReceiver);
    }

    //-------------------------------------------------------------------------
    @Override
    protected void onStop() {
        super.onStop();
        LogUtils.i(TAG, "onStop()");

        closeDau();

        if (null != mUltrasoundMgr) {
            mUltrasoundMgr.cleanup();
            mUltrasoundMgr = null;
        }

        if (null != mDb) {
            mDb.close();
            mDb = null;
        }
    }

    //-------------------------------------------------------------------------
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        LogUtils.i(TAG, "surfaceChanged()");

        mSurfaceHolder = holder;

        if (null != mUltrasoundMgr) {
            mViewer.setSurface(mSurfaceHolder.getSurface());

            if (mUltrasoundMgr.isDauConnected()) {
                prepareImaging();
            }
        }
    }

    //-------------------------------------------------------------------------
    public void surfaceCreated(SurfaceHolder holder) {
    }

    //-------------------------------------------------------------------------
    public void surfaceDestroyed(SurfaceHolder holder) {
        LogUtils.i(TAG, "surfaceDestroyed()");
        mSurfaceHolder = null;
    }

    //-------------------------------------------------------------------------
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);

        return false;
    }

    //-------------------------------------------------------------------------
    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        super.onPrepareOptionsMenu(menu);

       return false;
    }

    //-------------------------------------------------------------------------
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        super.onOptionsItemSelected(item);

        return super.onOptionsItemSelected(item);
    }

    //-------------------------------------------------------------------------
    public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
        if (mOrganSpinner.equals(parent)) {
            Cursor  cursor = (Cursor) parent.getItemAtPosition(pos);
            int     depthColumnIndex = 2;
            int     viewColumnIndex = 3;
            int     newDepth = cursor.getInt(depthColumnIndex);
            int     newView = cursor.getInt(viewColumnIndex);

            // NOTE: Assumes that cursor ID and spinner selection are the same.
            mDepthSpinner.setSelection(newDepth);
            mViewSpinner.setSelection(newView);
            mHaveNewParams = true;
            if (mImageWhenReady) {
                Message msg = mHandler.obtainMessage(UNFREEZE);
                mHandler.sendMessage(msg);
            }
            LogUtils.d(TAG, "onItemSelected (ORGAN)");
        }
        else if (mViewSpinner.equals(parent)) {
            LogUtils.d(TAG, "onItemSelected (VIEW)");
            mHaveNewParams = true;
        }
        else if (mModeSpinner.equals(parent)) {
            LogUtils.d(TAG, "onItemSelected (MODE)");
            mHaveNewParams = true;
        }
        else if (mDepthSpinner.equals(parent)) {
            LogUtils.d(TAG, "onItemSelected (DEPTH)");
            mHaveNewParams = true;
        }
        else if (mSpeedSpinner.equals(parent)) {
            switch (pos) {
                case 0:
                    mPlayer.setPlaybackSpeed(UltrasoundPlayer.PlaybackSpeed.Normal);
                    break;

                case 1:
                    mPlayer.setPlaybackSpeed(UltrasoundPlayer.PlaybackSpeed.Half);
                    break;

                case 2:
                    mPlayer.setPlaybackSpeed(UltrasoundPlayer.PlaybackSpeed.Quarter);
                    break;
            }
        }
    }

    //-------------------------------------------------------------------------
    public void onNothingSelected(AdapterView<?> parent) {
    }

    //-------------------------------------------------------------------------
    public void onRadioButtonClicked(View view) {
        boolean  isChecked = ((RadioButton) view).isChecked();

        switch (view.getId()) {
            case R.id.radioManualId:
                if (isChecked) {
                    mAutoLayout.setVisibility(View.GONE);
                    mManualLayout.setVisibility(View.VISIBLE);

                    SharedPreferences settings = getSharedPreferences("LiveImageDemo", MODE_PRIVATE);
                    if (null != settings) {
                        SharedPreferences.Editor prefEditor = settings.edit();
                        prefEditor.putInt("Auto", 0);
                        prefEditor.commit();
                    }
                }
                break;

            case R.id.radioAutoId:
                if (isChecked) {
                    mManualLayout.setVisibility(View.GONE);
                    mAutoLayout.setVisibility(View.VISIBLE);

                    SharedPreferences settings = getSharedPreferences("LiveImageDemo", MODE_PRIVATE);
                    if (null != settings) {
                        SharedPreferences.Editor prefEditor = settings.edit();
                        prefEditor.putInt("Auto", 1);
                        prefEditor.commit();
                    }
                }
                break;
        }
    }

    //-------------------------------------------------------------------------
    public void onAutoGainClicked(View view) {
        boolean isChecked = ((CheckBox) view).isChecked();

        if (isChecked) {
            mGainControl.setProgress(DEFAULT_AUTO_GAIN);
        }
        else {
            mGainControl.setProgress(DEFAULT_GAIN);
        }
        mDau.enableImageProcessing(Dau.ImageProcessingType.IMAGE_PROCESS_AUTO_GAIN,
                                   isChecked);
    }

    //-------------------------------------------------------------------------
    public void onDeSpeckleClicked(View view) {
        boolean isChecked = ((CheckBox) view).isChecked();

        mDau.enableImageProcessing(Dau.ImageProcessingType.IMAGE_PROCESS_DESPECKLE,
                                   isChecked);
    }

    //-------------------------------------------------------------------------
    public void onInferenceClicked(View view) {
        boolean isChecked = ((CheckBox) view).isChecked();

        mUltrasoundMgr.enableInferenceEngine(isChecked);
    }

    //-------------------------------------------------------------------------
    public void onButtonClicked(View view) {
        if (view.equals(mPlayButton)) {
            LogUtils.d(TAG, "Play Button clicked");
            mPlayer.start();
        }
        else if (view.equals(mPauseButton)) {
            LogUtils.d(TAG, "Pause Button clicked");
            mPlayer.pause();
        }
        else if (view.equals(mStopButton)) {
            LogUtils.d(TAG, "Stop Button clicked");
            mPlayer.stop();
        }
        else if (view.equals(mNextButton)) {
            LogUtils.d(TAG, "Next Button clicked");
            mPlayer.nextFrame();
        }
        else if (view.equals(mPreviousButton)) {
            LogUtils.d(TAG, "Previous Button clicked");
            mPlayer.previousFrame();
        }
        else if (view.equals(mVideoButton)) {
            LogUtils.d(TAG, "Video Button clicked");
            UUID uniqueId = UUID.randomUUID();

            ThorError ret = mRecorder.recordRetrospectiveVideo(getRawPath() +
                                 File.separator + 
                                 uniqueId.toString() + ".thor",
                                 30000);

            // For testing other recording methods...
            //
            //ThorError ret = mRecorder.recordRetrospectiveVideoFrames(getRawPath() +
            //                     File.separator + 
            //                     uniqueId.toString() + ".thor",
            //                     3);
            //ThorError ret = mRecorder.saveVideoFromCine(getRawPath() +
            //                     File.separator + 
            //                     uniqueId.toString() + ".thor",
            //                     60, 120);
            //ThorError ret = mRecorder.saveVideoFromCineFrames(getRawPath() +
            //                     File.separator + 
            //                     uniqueId.toString() + ".thor",
            //                     3, 6);

            if (ret.isOK()) {
                mVideoButton.setEnabled(false);
                mCameraButton.setEnabled(false);
            }
            else {
                showErrorDialog(ret, false);
            }
        }
        else if (view.equals(mCameraButton)) {
            LogUtils.d(TAG, "Camera Button clicked");
            UUID uniqueId = UUID.randomUUID();
            
            ThorError ret = mRecorder.recordStillImage(getRawPath() +
                                 File.separator +
                                 uniqueId.toString() + ".thor");
            
            if (ret.isOK()) {
                mVideoButton.setEnabled(false);
                mCameraButton.setEnabled(false);
            }
            else {
                showErrorDialog(ret, false);
            }

            // Added for testing these methods
            //LogUtils.d(TAG, "Current Frame: " + mPlayer.getCurrentFrame());
            //LogUtils.d(TAG, "Current Position (msec): " + mPlayer.getCurrentPosition());
        }
        else if (view.equals(mOpenButton)) {
            LogUtils.d(TAG, "Open Button clicked");

            FileChooser     fileChooser = new FileChooser(this);

            fileChooser.setPath(getRawPath());
            fileChooser.setExtension(".thor");

            fileChooser.setFileListener(new FileChooser.FileSelectedListener() {
                @Override
                public void fileSelected(final String filename) {
                    ThorError   ret;

                    if (null != mDau && mDau.isCineAttached()) {
                        mDau.detachCine();
                    }

                    if (!mPlayer.isCineAttached()) {
                        mPlayer.attachCine();
                    }

                    if (mPlayer.isRawFileOpen()) {
                        mPlayer .closeRawFile();
                    }

                    LogUtils.d(TAG, "File picked: " + filename);
                    ret = mPlayer.openRawFile(getRawPath() + File.separator + filename);
                    if (!ret.isOK()) {
                        showErrorDialog(ret, false);
                        return;
                    }
                    mClipDuration = mPlayer.getDuration();
                    LogUtils.d(TAG, "Frame Count: " + mPlayer.getFrameCount());
                    if (null != mReviewControl) {
                        mReviewControl.setMax(mClipDuration);
                        LogUtils.d(TAG, "mReviewControl.getMax() = " + mReviewControl.getMax());

                        //
                        // For testing fencing...
                        //
                        //int newMin = (int) (mClipDuration * 0.25);
                        //int newMax = (int) (mClipDuration * 0.75);
                        //
                        //LogUtils.d(TAG, "newMin = " + newMin + ", newMax = " + newMax);
                        //mPlayer.setStartPosition(newMin);
                        //mPlayer.setEndPosition(newMax);
                    }
                }
            });

            fileChooser.showDialog();
        }
    }

    //-------------------------------------------------------------------------
    public void onLoopingClicked(View view) {
        boolean isChecked = ((CheckBox) view).isChecked();

        mPlayer.setLooping(isChecked);
    }

    //-------------------------------------------------------------------------
    @Override
    public boolean onDown(MotionEvent e) {
        return false;
    }

    //-------------------------------------------------------------------------
    @Override
    public void onShowPress(MotionEvent e) {}

    //-------------------------------------------------------------------------
    @Override
    public boolean onSingleTapUp(MotionEvent e) {
        return false;
    }

    //-------------------------------------------------------------------------
    @Override
    public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
        mViewer.handleOnScroll(distanceX, distanceY);

        return false;
    }

    //-------------------------------------------------------------------------
    @Override
    public void onLongPress(MotionEvent e) {}

    //-------------------------------------------------------------------------
    @Override
    public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
        return false;
    }

    //-------------------------------------------------------------------------
    private String getRawPath() {
        return getDataDir() + File.separator + "raw";
    }

    //-------------------------------------------------------------------------
    private void switchProbeInterface(ProbeInterface newInterface) {
        mProbeInterface = newInterface;
        closeDau();

        if (null != mViewer) {
            mViewer = null;
        }
        if (null != mUltrasoundMgr) {
            mUltrasoundMgr.cleanup();
            mUltrasoundMgr = null;
            System.gc();
        }

        SharedPreferences settings = getSharedPreferences("LiveImageDemo", MODE_PRIVATE);
        if (null != settings) {
            SharedPreferences.Editor prefEditor = settings.edit();
            prefEditor.putInt("ProbeInterface", newInterface.getValue());
            prefEditor.commit();
        }

        switch (newInterface) {
            case Pcie:
                mUltrasoundMgr = new UltrasoundManager(mContext, UltrasoundManager.DauCommMode.Proprietary);
                break;

            case Usb:
                mUltrasoundMgr = new UltrasoundManager(mContext, UltrasoundManager.DauCommMode.Usb);
                break;

            case Emulation:
                mUltrasoundMgr = new UltrasoundManager(mContext, UltrasoundManager.DauCommMode.Emulation);
                break;

            default:
                break;
        }

        mViewer = mUltrasoundMgr.getUltrasoundViewer();
        mViewer.setSurface(mSurfaceHolder.getSurface());

        if (mUltrasoundMgr.isDauConnected()) {
            prepareImaging();
        }
    }

    //-------------------------------------------------------------------------
    private void prepareImaging() {
        if (null == mSurfaceHolder) {
            LogUtils.e(TAG, "Surface is null");
            return;
        }

        if (null == mDau) {
            try {
                mDau = mUltrasoundMgr.openDau();
            } catch (DauException ex) {
                showErrorDialog(ex);
                return;
            }
        }
        else {
            return;
        }

        if (null == mDau) {
            showErrorDialog("Unable to open Dau");
        }
        else {
            LogUtils.i(TAG, "Dau ID: " + mDau.getId());

            mDau.registerErrorCallback(mDauErrorCallback, mHandler);
            mDau.registerHidCallback(mDauHidCallback, mHandler);

            if (openDatabase()) {
                // NOTE: Uncomment to automatically unfreeze
                //mImageWhenReady = true;
            }

            mFreezeBtn.setEnabled(true);
        }
    }

    //-------------------------------------------------------------------------
    private void closeDau() {
        if (null != mDau) {
            freeze();
            mDau.unregisterErrorCallback();
            mDau.unregisterHidCallback();
            mDau.close();
            mDau = null;
        }
        mImagingCaseId = -1;
        mHaveNewParams = true;
    }

    //-------------------------------------------------------------------------
    private void showErrorDialog(String message) {
        showErrorDialog(message, true);
    }

    //-------------------------------------------------------------------------
    private void showErrorDialog(String message, boolean doExit) {
        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(mContext);

        alertDialogBuilder.setTitle("Error");
        alertDialogBuilder.setMessage(message);
        alertDialogBuilder.setCancelable(false);
        if (doExit) {
            alertDialogBuilder.setPositiveButton("Close", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int id) {
                    dialog.cancel();
                    finish();
                }
            });
        }
        else {
            alertDialogBuilder.setPositiveButton("Close", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int id) {
                    dialog.cancel();
                }
            });
        }

        AlertDialog alertDialog = alertDialogBuilder.create();

        alertDialog.show();
    }

    //-------------------------------------------------------------------------
    private void showErrorDialog(ThorError thorError) {
        showErrorDialog(thorError, true);
    }

    //-------------------------------------------------------------------------
    private void showErrorDialog(ThorError thorError, boolean doExit) {
        ErrorDialog errorDlg = new ErrorDialog(this);

        errorDlg.setError(thorError, doExit);
        errorDlg.show();
    }

    //-------------------------------------------------------------------------
    private void showErrorDialog(DauException exception) {
        ErrorDialog errorDlg = new ErrorDialog(this);

        errorDlg.setException(exception, true);
        errorDlg.show();
    }

    //-------------------------------------------------------------------------
    private boolean openDatabase() {
        boolean             retVal = false;
        Cursor              organCursor;
        SimpleCursorAdapter organAdapter;
        Cursor              viewCursor;
        SimpleCursorAdapter viewAdapter;
        Cursor              modeCursor;
        SimpleCursorAdapter modeAdapter;
        Cursor              depthCursor;
        SimpleCursorAdapter depthAdapter;

        try {
            
            mDb = SQLiteDatabase.openDatabase(mUltrasoundMgr.getUpsPath(),
                                              null,
                                              SQLiteDatabase.OPEN_READONLY);

            // Assume if this one is not null then neither are the other spinners
            if (null != mOrganSpinner)
            {

                // Organ Selection
                organCursor = mDb.rawQuery("select ID as _id, Name, DefaultDepthIndex, DefaultViewIndex " +
                                           "from Organs",
                                           null);

                organAdapter = new SimpleCursorAdapter(this,
                                                       android.R.layout.simple_spinner_item,
                                                       organCursor,
                                                       new String[] {"Name"},
                                                       new int[] {android.R.id.text1},
                                                       0);

                mOrganSpinner.setAdapter(organAdapter);

                // View Selection
                viewCursor = mDb.rawQuery("select ID as _id, Name " +
                                           "from Views",
                                           null);

                viewAdapter = new SimpleCursorAdapter(this,
                                                       android.R.layout.simple_spinner_item,
                                                       viewCursor,
                                                       new String[] {"Name"},
                                                       new int[] {android.R.id.text1},
                                                       0);

                mViewSpinner.setAdapter(viewAdapter);

                // Mode Selection
                modeCursor = mDb.rawQuery("select ID as _id, Name " +
                                           "from ImagingModes",
                                           null);

                modeAdapter = new SimpleCursorAdapter(this,
                                                       android.R.layout.simple_spinner_item,
                                                       modeCursor,
                                                       new String[] {"Name"},
                                                       new int[] {android.R.id.text1},
                                                       0);

                mModeSpinner.setAdapter(modeAdapter);

                // Depth Selection
                depthCursor = mDb.rawQuery("select ID as _id " +
                                           "from Depths",
                                           null);

                depthAdapter = new SimpleCursorAdapter(this,
                                                       android.R.layout.simple_spinner_item,
                                                       depthCursor,
                                                       new String[] {"_id"},
                                                       new int[] {android.R.id.text1},
                                                       0);

                mDepthSpinner.setAdapter(depthAdapter);


                mOrganSpinner.setOnItemSelectedListener(this);
                mViewSpinner.setOnItemSelectedListener(this);
                mModeSpinner.setOnItemSelectedListener(this);
                mDepthSpinner.setOnItemSelectedListener(this);
            }

            retVal = true;
        } catch (SQLiteException ex) {
            LogUtils.e(TAG, "SQLiteException.  path = " + mUltrasoundMgr.getUpsPath(), ex.getMessage());
            showErrorDialog("Error accessing UPS");
        }

        return retVal;
    }

    //-------------------------------------------------------------------------
    private int getImagingCaseId() {
        int             imagingCaseId = 0;
        Cursor          cursor;

        try {
            cursor = mDb.query("Globals",
                               new String[] {"DefaultImagingCase"},
                               null,
                               null,
                               null,
                               null,
                               null,
                               null);

            cursor.moveToFirst();
            imagingCaseId = cursor.getInt(0);

            LogUtils.d(TAG, "Default imaging case: " + imagingCaseId);
        } catch (SQLiteException ex) {
            LogUtils.e(TAG, "SQLiteException.", ex.getMessage());
            showErrorDialog("Error accessing UPS");
        }

        return imagingCaseId;
    }

    //-------------------------------------------------------------------------
    private boolean isImageCaseValid(int imageCaseId) {
        Cursor          cursor;
        boolean         retVal = false;

        try {
            cursor = mDb.query("ImagingCases",
                               new String[] {"ID"},
                               new String("ID=" + imageCaseId),
                               null,
                               null,
                               null,
                               null,
                               null);

            if (cursor.getCount() > 0) {
                retVal = true;
            }
        } catch (SQLiteException ex) {
            LogUtils.e(TAG, "SQLiteException.", ex.getMessage());
            showErrorDialog("Error accessing UPS");
        }

        return retVal;
    }

    //-------------------------------------------------------------------------
    private void enableImagingControls(boolean enable) {
        if (null != mEditImageId) {
            mEditImageId.setEnabled(enable);
        }

        if (null != mGainControl) {
            mGainControl.setEnabled(!enable);
        }

        if (null != mColorGainControl) {
            mColorGainControl.setEnabled(!enable);
        }
        
        if (null != mOrganSpinner) {
            mOrganSpinner.setEnabled(enable);
        }
        if (null != mViewSpinner) {
            mViewSpinner.setEnabled(enable);
        }
        if (null != mModeSpinner) {
            mModeSpinner.setEnabled(enable);
        }
        if (null != mDepthSpinner) {
            mDepthSpinner.setEnabled(enable);
        }
        if (null != mAutoGainCheck) {
            mAutoGainCheck.setEnabled(!enable);
        }
        if (null != mDeSpeckleCheck) {
            mDeSpeckleCheck.setEnabled(!enable);
        }
        //if (null != mInferenceCheck) {
        //    mInferenceCheck.setEnabled(enable);
        //}
        mInferenceCheck.setEnabled(true);

        if (null != mReviewControl) {
            mReviewControl.setEnabled(enable);
            if (!enable) {
                mReviewControl.setProgress(0);
            }
        }
        if (null != mPlayButton) {
            mPlayButton.setEnabled(enable);
        }
        if (null != mPauseButton) {
            mPauseButton.setEnabled(enable);
        }
        if (null != mStopButton) {
            mStopButton.setEnabled(enable);
        }
        if (null != mLoopingCheck) {
            mLoopingCheck.setEnabled(enable);
        }
        if (null != mNextButton) {
            mNextButton.setEnabled(enable);
        }
        if (null != mPreviousButton) {
            mPreviousButton.setEnabled(enable);
        }
        if (null != mSpeedSpinner) {
            mSpeedSpinner.setEnabled(enable);
        }
        if (null != mOpenButton) {
            mOpenButton.setEnabled(enable);
        }
    }

    //-------------------------------------------------------------------------
    private void freeze() {
        if (mFrozen || null == mDau) {
            return;
        }

        LogUtils.i(TAG, "freeze");
        mDau.stop();
        mFrozen = true;
        mFreezeBtn.setSilentChecked(false);
        enableImagingControls(true);

        mDau.detachCine();

        if (null != mPlayer) {
            mPlayer.attachCine();
            mClipDuration = mPlayer.getDuration();
            if (null != mReviewControl) {
                mReviewControl.setMax(mClipDuration);
                LogUtils.d(TAG, "mReviewControl.getMax() = " + mReviewControl.getMax());
            }
        }
    }

    //-------------------------------------------------------------------------
    private void unfreeze() {
        if (!mFrozen || null == mDau) {
            return;
        }

        LogUtils.i(TAG, "unfreeze");

        if (mHaveNewParams) {
            boolean useManual = false;

            if (null == mManualLayout)
            {
                useManual = true;
            }
            else if (View.VISIBLE == mManualLayout.getVisibility())
            {
                useManual = true;
            }


            if (useManual) {
                String      imageCaseText = mEditImageId.getText().toString();
                int         newImagingCaseId;
                ThorError   errorRet;

                try {
                    newImagingCaseId = Integer.parseInt(imageCaseText);
                } catch (NumberFormatException ex) {
                    showErrorDialog("Invalid Image Case ID", false);
                    return;
                }

                if (newImagingCaseId != mImagingCaseId) {
                    if (!isImageCaseValid(newImagingCaseId)) {
                        showErrorDialog("Invalid Image Case ID", false);
                        mFreezeBtn.setSilentChecked(false);
                        return;
                    }
                    else {
                        mImagingCaseId = newImagingCaseId;
                        errorRet = mDau.setImagingCase(mImagingCaseId);
                        if (!errorRet.isOK()) {
                            showErrorDialog(errorRet);
                            return;
                        }

                    }
                }
            }
            else {
                LogUtils.d(TAG, "Organ ID: " + mOrganSpinner.getSelectedItemPosition() +
                      " View ID: " + mViewSpinner.getSelectedItemPosition() +
                      " Depth ID: " + mDepthSpinner.getSelectedItemPosition() +
                      " Mode ID: " + mModeSpinner.getSelectedItemPosition());

                ThorError   errorRet;

                errorRet = mDau.setImagingCase( mOrganSpinner.getSelectedItemPosition(),
                                     mViewSpinner.getSelectedItemPosition(),
                                     mDepthSpinner.getSelectedItemPosition(),
                                     mModeSpinner.getSelectedItemPosition());

                if (!errorRet.isOK()) {
                    showErrorDialog("No Image Case ID for selected parameters", false);
                    return;
                }
            }

            if (null != mGainControl)
            {
                mDau.setGain(mGainControl.getProgress());
            }
            if (null != mColorGainControl)
            {
                mDau.setColorGain(mColorGainControl.getProgress());
            }
        }

        if (null != mPlayer) {
            if (mPlayer.isRawFileOpen()) {
                mPlayer.closeRawFile();
            }
            mPlayer.detachCine();
        }

        mDau.attachCine();

        mHaveNewParams = false;
        enableImagingControls(false);
        mDau.start();
        mFrozen = false;
        mFreezeBtn.setSilentChecked(true);
    }

    //-------------------------------------------------------------------------
    private class MyReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            LogUtils.d(TAG, "Dau Intent Received: " + intent.getAction());

            if (intent.getAction().contentEquals(UltrasoundManager.ACTION_DAU_CONNECTED)) {
                Message msg = mHandler.obtainMessage(DAU_CONNECTED);
                mHandler.sendMessage(msg);
            }

            if (intent.getAction().contentEquals(UltrasoundManager.ACTION_DAU_DISCONNECTED)) {
                Message msg = mHandler.obtainMessage(DAU_DISCONNECTED);
                mHandler.sendMessage(msg);
            }
        }
    }

    //-------------------------------------------------------------------------
    private final Dau.ErrorCallback mDauErrorCallback = 
        new Dau.ErrorCallback() {

        @Override
        public void onError(ThorError thorError) {
            showErrorDialog(thorError);
        }
    };

    //-------------------------------------------------------------------------
    private final Dau.HidCallback mDauHidCallback = 
        new Dau.HidCallback() {

        @Override
        public void onHid(int hidId) {
            Toast.makeText(mContext, "Button (0x" + Integer.toHexString(hidId) + ") pressed",
                           Toast.LENGTH_SHORT).show();
            mFreezeBtn.performClick();
        }
    };

    //-------------------------------------------------------------------------
    private final UltrasoundPlayer.ProgressCallback mPlayerProgressCallback =
        new UltrasoundPlayer.ProgressCallback() {

        @Override
        public void onProgress(int position) {
            if (null != mReviewControl) {
                mReviewControl.setProgress(position);
            }
        }
    };

    //-------------------------------------------------------------------------
    private final UltrasoundRecorder.CompletionCallback mRecorderCompletionCallback =
        new UltrasoundRecorder.CompletionCallback() {

        @Override
        public void onCompletion(ThorError result) {
            mVideoButton.setEnabled(true);
            mCameraButton.setEnabled(true);

            if (ThorError.THOR_OK != result) {
                ErrorDialog errorDlg = new ErrorDialog(LiveImageDemo.this);

                errorDlg.setError(result, false);
                errorDlg.show();
            }
        }
    };

    //-------------------------------------------------------------------------
    private final Puck.PuckEventCallback mPuckCallback = 
        new Puck.PuckEventCallback() {

        @Override
        public void onPuckEvent(PuckEvent event) {
            if (event.getActionButton() == PuckEvent.BUTTON_TRACKPAD) {
                LogUtils.d(TAG, "Trackpad: X = " +
                      event.getX() + "     Y = " +
                      event.getY());
            }
            else {
                LogUtils.d(TAG, "PuckEvent - Button = 0x" +
                      Integer.toHexString(event.getActionButton()) +
                      " - 0x" + Integer.toHexString(event.getAction()));
            }
        }
    };

    //-------------------------------------------------------------------------
    private enum ProbeInterface {
        Pcie(0),
        Usb(1),
        Emulation(2);

        private final int value;
        private ProbeInterface(int value) {
            this.value = value;
        }

        public int getValue() {
            return value;
        }
    }
}
