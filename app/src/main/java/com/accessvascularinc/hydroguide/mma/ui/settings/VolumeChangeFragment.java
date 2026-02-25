package com.accessvascularinc.hydroguide.mma.ui.settings;

import android.content.Context;
import android.media.AudioManager;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.databinding.DataBindingUtil;
import androidx.lifecycle.ViewModelProvider;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.VolumeChangeLayoutBinding;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;

public class VolumeChangeFragment extends BaseSettingBottomSheetFragment {
  private AudioManager audioManager = null;

  @Override
  public View onCreateView(@NonNull final LayoutInflater inflater,
                           @Nullable final ViewGroup container,
                           @Nullable final Bundle savedInstanceState) {
    final VolumeChangeLayoutBinding binding =
            DataBindingUtil.inflate(inflater, R.layout.volume_change_layout, container, false);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);

    binding.setTabletState(viewmodel.getTabletState());
    audioManager = (AudioManager) requireActivity().getSystemService(Context.AUDIO_SERVICE);

    binding.volumeChangeIcon.setOnClickListener(view -> dismiss());
    binding.volumeSlider.addOnChangeListener((slider, value, fromUser) -> {
      final int newVolume = Math.round(value);
      viewmodel.getTabletState().setAudioVolume(newVolume);
      audioManager.setStreamVolume(AudioManager.STREAM_MUSIC, newVolume, 0);
    });

    binding.volumeSlider.setOnKeyListener((view, i, keyEvent) -> {
      if (i == KeyEvent.KEYCODE_ENTER) {
        dismiss();
      }
      return false;
    });

    return binding.getRoot();
  }
}
