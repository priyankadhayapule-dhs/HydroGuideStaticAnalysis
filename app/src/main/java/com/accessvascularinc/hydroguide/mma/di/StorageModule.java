package com.accessvascularinc.hydroguide.mma.di;

import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.storage.ModeAwareStorageManager;

import dagger.Module;
import dagger.Provides;
import dagger.hilt.InstallIn;
import dagger.hilt.components.SingletonComponent;
import dagger.hilt.android.qualifiers.ApplicationContext;

import javax.inject.Singleton;

@Module
@InstallIn(SingletonComponent.class)
public class StorageModule {
  @Provides
  @Singleton
  public IStorageManager provideStorageManager(ModeAwareStorageManager modeAwareStorageManager) {
    // Single injected entry point that dynamically chooses online/offline
    // based on PrefsManager flag at call time.
    return modeAwareStorageManager;
  }
}
