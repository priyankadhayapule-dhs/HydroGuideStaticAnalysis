package com.accessvascularinc.hydroguide.mma.ui;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.databinding.DataBindingUtil;
import androidx.fragment.app.Fragment;
import androidx.navigation.Navigation;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentSplashBinding;
import com.accessvascularinc.hydroguide.mma.model.TabletState;
import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;

import javax.inject.Inject;

import dagger.hilt.android.AndroidEntryPoint;

@AndroidEntryPoint
public class SplashFragment extends Fragment {
  static final long SPLASH_DELAY = 3000; // ms
  private final Handler splashTimerHandler = new Handler(Looper.getMainLooper());
  @Inject
  IStorageManager storageManager;
  private final Runnable splashRunnable = new Runnable() {
    @Override
    public void run() {
      if (!isAdded() || getView() == null) {
        return;
      }
      LoginResponse loginResponse = PrefsManager.getLoginResponse(requireContext());
      boolean isLoggedIn = loginResponse != null && loginResponse.getData() != null &&
              loginResponse.getData().getToken() != null &&
              !loginResponse.getData().getToken().isEmpty();

      // Token expiry check (18 hours)
      boolean isTokenExpired = false;
      android.content.SharedPreferences prefs =
              requireContext().getSharedPreferences(PrefsManager.getPrefsName(),
                      android.content.Context.MODE_PRIVATE);
      long tokenSavedTime = prefs.getLong(PrefsManager.getTokenSavedTime(), 0);
      long now = System.currentTimeMillis();
      if (tokenSavedTime > 0 && (now - tokenSavedTime) > PrefsManager.getTokenExpiryDurationMs()) {
        isTokenExpired = true;
      }
      androidx.navigation.NavController navController = Navigation.findNavController(getView());
      if (navController.getCurrentDestination() != null &&
              navController.getCurrentDestination().getId() == R.id.navigation_splash) {
        IStorageManager.StorageMode mode = PrefsManager.getConfiguredStorageMode(requireContext());
        if (mode == IStorageManager.StorageMode.UNKNOWN) {
          navController.navigate(R.id.action_navigation_splash_to_mode_setup);
          return;
        }
        if (isLoggedIn) {
          if (isTokenExpired &&
                  storageManager.getStorageMode() == IStorageManager.StorageMode.ONLINE) {
            // Call refresh token API
            storageManager.refreshToken(loginResponse.getData().getRefereshToken(),
                    ((response, errorMessage) -> {
                      if (response != null && response.getData() != null && errorMessage == null) {
                        Log.d("RefreshToken", "onResult: Successful Login");
                        // Save new token and update saved time
                        PrefsManager.saveLoginResponse(requireContext(), response);
                        prefs.edit().putLong(PrefsManager.getTokenSavedTime(), System.currentTimeMillis()).apply();
                        String role = response.getData().getRoles().get(0).getRole();
                        if (IStorageManager.UserType.Organization_Admin.toString().equals(role)) {
                          navController.navigate(R.id.action_navigation_splash_to_admin_home);
                        } else {
                          navController.navigate(R.id.action_navigation_splash_to_home);
                        }
                      } else {
                        navController.navigate(R.id.action_navigation_splash_to_login);
                      }
                    }));
            return;
          }
          String role = loginResponse.getData().getRoles().get(0).getRole();
          if (IStorageManager.UserType.Organization_Admin.toString().equals(role)) {
            navController.navigate(R.id.action_navigation_splash_to_admin_home);
          } else {
            navController.navigate(R.id.action_navigation_splash_to_home);
          }
        } else {
          navController.navigate(R.id.action_navigation_splash_to_login);
        }
      }
    }
  };

  @Override
  public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container,
                           final Bundle savedInstanceState) {
    final FragmentSplashBinding binding =
            DataBindingUtil.inflate(inflater, R.layout.fragment_splash, container, false);
    binding.splashSwVers.setText(TabletState.getAppVersion(requireContext()));

    super.onCreateView(inflater, container, savedInstanceState);

    return binding.getRoot();
  }

  @Override
  public void onViewCreated(@NonNull final View view, @Nullable final Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
  }

  @Override
  public void onStart() {
    super.onStart();
    splashTimerHandler.postDelayed(splashRunnable, SPLASH_DELAY);
  }

  @Override
  public void onStop() {
    super.onStop();
    splashTimerHandler.removeCallbacks(splashRunnable);
  }
}