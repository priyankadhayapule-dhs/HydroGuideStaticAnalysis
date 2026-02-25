package com.accessvascularinc.hydroguide.mma.db;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.util.Log;
import com.accessvascularinc.hydroguide.mma.exceptions.ErrorDispatcherActivity;
import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse;
import com.accessvascularinc.hydroguide.mma.sync.SyncManager;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import android.os.Process;
import javax.inject.Inject;

import dagger.hilt.android.HiltAndroidApp;

@HiltAndroidApp
public class HydroGuide extends Application {

  @Inject
  protected SyncManager syncManager;

  @Inject
  protected IStorageManager storageManager;
  private ConnectivityManager.NetworkCallback networkCallback;
  private Thread.UncaughtExceptionHandler defaultErrHandler;

  @Override
  public void onCreate() {
    super.onCreate();
    initiateErrHandler();
    MedDevLog.init(getApplicationContext());

    ConnectivityManager cm = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
    NetworkRequest request = new NetworkRequest.Builder()
            .addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
            .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
            .build();

    networkCallback = new ConnectivityManager.NetworkCallback() {
      @Override
      public void onAvailable(Network network) {
        Log.d("Logs", "Internet Available");
        // Call a static or singleton sync trigger
        // SyncHelper.triggerSync(getApplicationContext());
        if (storageManager != null) {
          storageManager.syncAllData((success, errorMessage) -> {
            if (!success) {
              MedDevLog.error("HydroGuide", "Sync failed: " + errorMessage);
              //TODO: Do we need sync failures, as well as successes, in audit trail?
              MedDevLog.audit("HydroGuide", "Sync failed: " + errorMessage);
            } else {
              MedDevLog.audit("HydroGuide", "Sync successful");
            }
          });
          LoginResponse loginResponse = PrefsManager.getLoginResponse(getApplicationContext());
          android.content.SharedPreferences prefs =
                  getApplicationContext().getSharedPreferences(PrefsManager.getPrefsName(),
                          android.content.Context.MODE_PRIVATE);
          long tokenSavedTime = prefs.getLong(PrefsManager.getTokenSavedTime(), 0);
          long now = System.currentTimeMillis();

          if (tokenSavedTime > 0 && (now - tokenSavedTime) > PrefsManager.getTokenExpiryDurationMs()) {
            storageManager.refreshToken(loginResponse.getData().getRefereshToken(),
                    ((response, errorMessage) -> {
                      if (response != null && response.getData() != null && errorMessage == null) {
                        PrefsManager.saveLoginResponse(getApplicationContext(), response);
                        prefs.edit().putLong(PrefsManager.getTokenSavedTime(),
                                System.currentTimeMillis()).apply();
                        Log.d("RefreshToken", "onResult: Successful Login");
                      }
                    }));
          }
        } else {
          MedDevLog.error("HydroGuide", "IStorageManager not injected");
        }
      }
    };
    cm.registerNetworkCallback(request, networkCallback);
  }

  private void initiateErrHandler()
  {
    defaultErrHandler = Thread.getDefaultUncaughtExceptionHandler();
    Thread.setDefaultUncaughtExceptionHandler((thread, uncaughtException) ->
    {
      try
      {
        Intent crashedIntent = new Intent(getApplicationContext(), ErrorDispatcherActivity.class);
        crashedIntent.putExtra(Utility.KEY_CRASH_TYPE, uncaughtException.getClass().getName());
        crashedIntent.putExtra(Utility.KEY_CRASH_LOGS, Log.getStackTraceString(uncaughtException));
        crashedIntent.putExtra(Utility.KEY_CRASH_TRACE,uncaughtException);
        crashedIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
        startActivity(crashedIntent);
        Thread.sleep(500);
      }
      catch (Exception e)
      {
        MedDevLog.info(HydroGuide.this.getClass().getSimpleName(), "Error in crash handler: " + e.getMessage());
      }
      finally
      {
        Process.killProcess(Process.myPid());
        System.exit(1);
      }
    });
  }
}
