package com.accessvascularinc.hydroguide.mma.ui;

import android.os.Handler;

import androidx.annotation.Nullable;

public class Scheduler {

  private static final String tag = "Scheduler";
  private long delay = 1000; // Default
  private Handler handler = null;
  @Nullable
  private Runnable timeoutRunnable = null;
  private volatile boolean eventActive = false;

  private final Runnable timerResetRunnable = new Runnable() {
    @Override
    public void run() {
      if (eventActive) {
        if (timeoutRunnable != null) {
          timeoutRunnable.run();
        }
      }
    }
  };

  public Scheduler(final Handler handlingThread, final long timeout_delay,
                   final Runnable timeoutFunction) {
    handler = handlingThread;
    delay = timeout_delay;
    this.timeoutRunnable = timeoutFunction;
  }

  public boolean circulateTimeout(final boolean exitTimeout) {
    if (timerResetRunnable != null) {
      handler.removeCallbacks(timerResetRunnable);
    }

    if (exitTimeout) {
      eventActive = false;
      return false;
    } else {
      eventActive = true;
      handler.postDelayed(timerResetRunnable, delay);
      return false;
    }
  }

  public void cleanUp() {
    handler.removeCallbacksAndMessages(null);
  }

  public void beginPolling() {
    handler.postDelayed(timeoutRunnable, delay);
    handler.postDelayed(this::beginPolling, delay);
  }
}