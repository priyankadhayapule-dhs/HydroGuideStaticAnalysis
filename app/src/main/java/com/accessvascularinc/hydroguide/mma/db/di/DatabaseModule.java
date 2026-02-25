package com.accessvascularinc.hydroguide.mma.db.di;

import android.content.Context;

import com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase;

import dagger.Module;
import dagger.Provides;
import dagger.hilt.InstallIn;
import dagger.hilt.android.qualifiers.ApplicationContext;
import dagger.hilt.components.SingletonComponent;
import javax.inject.Singleton;

/**
 * DI for DB
 */
@Module
@InstallIn(SingletonComponent.class)
public class DatabaseModule {

  /**
   * @param context for DB
   *
   * @return
   */
  @Provides
  @Singleton
  public HydroGuideDatabase provideDatabase(@ApplicationContext final Context context) {
    return HydroGuideDatabase.buildDatabase(context);
  }

}
