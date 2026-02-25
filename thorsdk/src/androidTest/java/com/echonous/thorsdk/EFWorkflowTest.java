package com.echonous.thorsdk;


import com.echonous.ai.CardiacFrames;
import com.echonous.ai.CardiacSegmentation;
import com.echonous.ai.CardiacView;
import com.echonous.ai.EFListener;
import com.echonous.ai.EFWorkflow;
import com.echonous.hardware.ultrasound.ThorError;
import com.echonous.hardware.ultrasound.UltrasoundManager;
import com.echonous.hardware.ultrasound.UltrasoundPlayer;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import androidx.test.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import static org.junit.Assert.*;

@RunWith(AndroidJUnit4.class)
public class EFWorkflowTest implements EFListener {

    private static class LooperThread extends Thread {

        private EFWorkflowTest mTest;

        public LooperThread(EFWorkflowTest test) {
            mTest = test;
        }

        public void run() {
            Looper.prepare();

            Handler handler = new Handler() {
                public void handleMessage(Message msg) {
                    // On any message, stop the looper
                    Looper.myLooper().quit();
                }
            };
            mTest.setHandler(handler);
            Looper.loop();
        }
    }

    private UltrasoundManager mManager;
    private UltrasoundPlayer mPlayer;
    private EFWorkflow mWorkflow;
    private LooperThread mLooperThread;
    private Handler mHandler;

    // Number of times each callback was called
    private int mFrameIDUpdates;
    private int mSegmentationUpdates;
    private int mStatisticsUpdates;

    private Lock mLock;
    private Condition mInfoUpdated;

    public EFWorkflowTest() {
        Context appContext = InstrumentationRegistry.getTargetContext();
        mManager = new UltrasoundManager(appContext, UltrasoundManager.DauCommMode.Proprietary);
        mPlayer = mManager.getUltrasoundPlayer();
    }

    @Before
    public void setup() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        mWorkflow = new EFWorkflow(context.getSharedPreferences(EFWorkflow.PREF_KEY_EF_ERROR,Context.MODE_PRIVATE));
        // Set a debug delay to force operations to take some time
        //mWorkflow.debugSetDelay(50000);

        mPlayer.attachCine();
        mPlayer.setStartFrame(0);
        mPlayer.setEndFrame(100);

        mFrameIDUpdates = 0;
        mSegmentationUpdates = 0;
        mStatisticsUpdates = 0;

        mLock = new ReentrantLock();
        mInfoUpdated = mLock.newCondition();

        mLooperThread = new LooperThread(this);
        mLooperThread.start();

        mLock.lock();
        try {
            while (mHandler == null) {
                mInfoUpdated.await();
            }
        } catch (InterruptedException e) {
            fail(e.getMessage());
        } finally {
            mLock.unlock();
        }
        mWorkflow.setListener(this, mHandler);
    }

    @After
    public void teardown() {
        mPlayer.detachCine();
        mHandler.sendEmptyMessage(0);
        try {
            mLooperThread.join();
        } catch (InterruptedException e)
        {
            fail(e.getMessage());
        }
    }

    @Test
    public void getSetUserFrames() {
        CardiacFrames a2cFrames = new CardiacFrames(10,20);
        CardiacFrames a4cFrames = new CardiacFrames(30,40);

        // Set A2C
        mWorkflow.setUserFrames(CardiacView.A2C, a2cFrames);
        CardiacFrames returnedA2CFrames = mWorkflow.getUserFrames(CardiacView.A2C);
        assertEquals(a2cFrames.edFrame, returnedA2CFrames.edFrame);
        assertEquals(a2cFrames.esFrame, returnedA2CFrames.esFrame);

        // Set A4C
        mWorkflow.setUserFrames(CardiacView.A4C, a4cFrames);
        CardiacFrames returnedA4CFrames = mWorkflow.getUserFrames(CardiacView.A4C);
        assertEquals(a4cFrames.edFrame, returnedA4CFrames.edFrame);
        assertEquals(a4cFrames.esFrame, returnedA4CFrames.esFrame);

        // Ensure A2C is not changed
        returnedA2CFrames = mWorkflow.getUserFrames(CardiacView.A2C);
        assertEquals(a2cFrames.edFrame, returnedA2CFrames.edFrame);
        assertEquals(a2cFrames.esFrame, returnedA2CFrames.esFrame);
    }

    @Test
    public void getSetUserSegmentation() {
        CardiacSegmentation a4cESSegmentation = new CardiacSegmentation(new float[]{1.0f, 2.0f, 3.0f, 4.0f});
        CardiacSegmentation a4cEDSegmentation = new CardiacSegmentation(new float[]{-1.0f, -2.0f, -3.0f, -4.0f});


        // Set points for ES frame
        mWorkflow.setUserSegmentation(CardiacView.A4C,10, a4cESSegmentation);
        CardiacSegmentation retA4CES = mWorkflow.getUserSegmentation(CardiacView.A4C, 10);
        assertArrayEquals(a4cESSegmentation.splinePoints, retA4CES.splinePoints, 0.001f);

        // Set points for ED frame
        mWorkflow.setUserSegmentation(CardiacView.A4C,20, a4cEDSegmentation);
        CardiacSegmentation retA4CED = mWorkflow.getUserSegmentation(CardiacView.A4C, 20);
        assertArrayEquals(a4cEDSegmentation.splinePoints, retA4CED.splinePoints, 0.001f);

        // Ensure ES frame is unchanged
        retA4CES = mWorkflow.getUserSegmentation(CardiacView.A4C, 10);
        assertArrayEquals(a4cESSegmentation.splinePoints, retA4CES.splinePoints, 0.001f);
    }

    @Test
    public void asyncFrameID() {
        mWorkflow.beginFrameID(CardiacView.A4C);

        mLock.lock();
        try {
            // await twice as long as the debug delay
            if (mFrameIDUpdates == 0) {
                mInfoUpdated.await(100000, TimeUnit.MICROSECONDS);
            }

            assertEquals(1, mFrameIDUpdates);
        } catch (InterruptedException e) {
            fail(e.getMessage());
        } finally {
            mLock.unlock();
        }
    }

    @Test
    public void getAIFrameID() {
        CardiacFrames frames = mWorkflow.getAIFrames(CardiacView.A4C);
        assertEquals(0, frames.esFrame);
        assertEquals(0, frames.edFrame);
    }

    // Callbacks
    public void onFrameQualityUpdate(int frame) {
    }
    public void onAIFramesUpdate(CardiacView view) {
        mLock.lock();
        try {
            ++mFrameIDUpdates;
            mInfoUpdated.signalAll();
        } finally {
            mLock.unlock();
        }
    }
    public void onAISegmentationUpdate(CardiacView view, int frame) {
        mLock.lock();
        try {
            ++mSegmentationUpdates;
            mInfoUpdated.signalAll();
        } finally {
            mLock.unlock();
        }
    }

    @Override
    public void onStatisticsUpdate(Boolean isFromThorError) {

    }

    public void onStatisticsUpdate() {
        mLock.lock();
        try {
            ++mStatisticsUpdates;
            mInfoUpdated.signalAll();
        } finally {
            mLock.unlock();
        }
    }

    @Override
    public void onThorError(ThorError error) {

    }

    @Override
    public void onAutocaptureStart() {

    }

    @Override
    public void onAutocaptureFail() {

    }

    @Override
    public void onAutocaptureSucceed() {

    }

    @Override
    public void onAutocaptureProgress(float progress) {

    }

    @Override
    public void onSmartCaptureProgress(float progress) {

    }

    public void setHandler(Handler handler) {
        mLock.lock();
        try {
            mHandler = handler;
            mInfoUpdated.signalAll();
        } finally {
            mLock.unlock();
        }
    }

}
