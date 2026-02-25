package com.accessvascularinc.hydroguide.mma.ui;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.navigation.Navigation;

import com.accessvascularinc.hydroguide.mma.R;

/**
 * First-time tablet configuration screen.
 *
 * When the app is installed and no mode has been configured yet,
 * SplashFragment will navigate here. The user can choose between
 * Online and Offline modes.
 *
 * Note: Online selection goes directly to the login screen.
 * Offline selection navigates to OfflineAdminSetupFragment where
 * the first local admin user will be created.
 */
public class ModeSetupFragment extends Fragment {

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_mode_setup, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        LinearLayout tileOnline = view.findViewById(R.id.tile_mode_online);
        LinearLayout tileOffline = view.findViewById(R.id.tile_mode_offline);

        View.OnClickListener offlineClickListener = v -> {
            // Do not set the offline flag yet; we only mark the
            // device as offline once the admin user has been
            // successfully created in OfflineAdminSetupFragment.
            Navigation.findNavController(v)
                    .navigate(R.id.action_navigation_mode_setup_to_offline_admin_setup);
        };

        if (tileOffline != null) {
            tileOffline.setOnClickListener(offlineClickListener);
        }

        View.OnClickListener onlineClickListener = v -> {
            if (getContext() == null)
                return;
            com.accessvascularinc.hydroguide.mma.utils.PrefsManager
                    .setTabletConfigurationStateIsOffline(getContext(), false);
            Navigation.findNavController(v)
                    .navigate(R.id.action_navigation_mode_setup_to_login);
        };

        if (tileOnline != null) {
            tileOnline.setOnClickListener(onlineClickListener);
        }
    }
}
