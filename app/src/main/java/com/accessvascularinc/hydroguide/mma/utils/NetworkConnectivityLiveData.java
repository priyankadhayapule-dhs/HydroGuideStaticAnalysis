package com.accessvascularinc.hydroguide.mma.utils;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.os.Build;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

public class NetworkConnectivityLiveData extends LiveData<Boolean> {
  private final Context context;
  private ConnectivityManager connectivityManager;
  private ConnectivityManager.NetworkCallback networkCallback;
  private BroadcastReceiver networkReceiver;

  public NetworkConnectivityLiveData(Context context) {
    this.context = context.getApplicationContext();
    connectivityManager =
            (ConnectivityManager) this.context.getSystemService(Context.CONNECTIVITY_SERVICE);
  }

  @Override
  protected void onActive() {
    super.onActive();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
      networkCallback = new ConnectivityManager.NetworkCallback() {
        @Override
        public void onAvailable(Network network) {
          postValue(true);
        }

        @Override
        public void onLost(Network network) {
          postValue(false);
        }
      };
      connectivityManager.registerDefaultNetworkCallback(networkCallback);
    } else {
      networkReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
          boolean connected = isConnected();
          postValue(connected);
        }
      };
      IntentFilter filter = new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION);
      context.registerReceiver(networkReceiver, filter);
    }
    postValue(isConnected());
  }

  @Override
  protected void onInactive() {
    super.onInactive();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
      if (networkCallback != null) {
        connectivityManager.unregisterNetworkCallback(networkCallback);
      }
    } else {
      if (networkReceiver != null) {
        context.unregisterReceiver(networkReceiver);
      }
    }
  }

  private boolean isConnected() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
      Network network = connectivityManager.getActiveNetwork();
      if (network == null) {
        return false;
      }
      NetworkCapabilities capabilities = connectivityManager.getNetworkCapabilities(network);
      return capabilities != null &&
              (capabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) ||
                      capabilities.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR) ||
                      capabilities.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET));
    } else {
      android.net.NetworkInfo activeNetwork = connectivityManager.getActiveNetworkInfo();
      return activeNetwork != null && activeNetwork.isConnected();
    }
  }
}
