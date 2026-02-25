package com.accessvascularinc.hydroguide.mma.di;

import android.content.Context;

import com.accessvascularinc.hydroguide.mma.security.IEncryptionManager;
import com.accessvascularinc.hydroguide.mma.security.EncryptionManager;

import javax.inject.Singleton;

import dagger.Module;
import dagger.Provides;
import dagger.hilt.InstallIn;
import dagger.hilt.android.qualifiers.ApplicationContext;
import dagger.hilt.components.SingletonComponent;

/**
 * Dagger Hilt module for providing security-related dependencies
 * Provides encryption manager for password encryption/decryption
 */
@Module
@InstallIn(SingletonComponent.class)
public class SecurityModule {

    /**
     * Provides singleton instance of EncryptionManager
     * Uses AndroidKeyStore for secure key storage
     * 
     * @param context Application context for accessing system KeyStore
     * @return IEncryptionManager implementation (EncryptionManager)
     */
    @Provides
    @Singleton
    public IEncryptionManager provideEncryptionManager(@ApplicationContext Context context) {
        return new EncryptionManager(context);
    }
}
