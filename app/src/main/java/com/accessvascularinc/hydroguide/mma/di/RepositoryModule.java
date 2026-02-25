package com.accessvascularinc.hydroguide.mma.di;

import com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase;
import com.accessvascularinc.hydroguide.mma.db.repository.FacilityRepository;
import com.accessvascularinc.hydroguide.mma.db.repository.ProcedureCaptureRepository;
import com.accessvascularinc.hydroguide.mma.network.ApiService;
import com.accessvascularinc.hydroguide.mma.repository.ProcedureSyncRepository;
import com.accessvascularinc.hydroguide.mma.repository.ClinicianApiRepository;
import com.accessvascularinc.hydroguide.mma.db.repository.UsersRepository;
import com.accessvascularinc.hydroguide.mma.security.IEncryptionManager;
import javax.inject.Singleton;

import dagger.Module;
import dagger.Provides;
import dagger.hilt.InstallIn;
import dagger.hilt.components.SingletonComponent;

@Module
@InstallIn(SingletonComponent.class)
public class RepositoryModule {

    @Provides
    @Singleton
    public UsersRepository provideUsersRepository(
            HydroGuideDatabase db, IEncryptionManager encryptionManager) {
        return new UsersRepository(db, encryptionManager);
    }

    @Provides
    @Singleton
    public com.accessvascularinc.hydroguide.mma.db.repository.EncounterRepository provideEncounterRepository(
            HydroGuideDatabase db) {
        return new com.accessvascularinc.hydroguide.mma.db.repository.EncounterRepository(db);
    }

    @Provides
    @Singleton
    public FacilityRepository provideFacilityRepository(HydroGuideDatabase db) {
        return new FacilityRepository(db.facilityDao());
    }

    @Provides
    @Singleton
    public ProcedureCaptureRepository provideProcedureCaptureRepository(HydroGuideDatabase db) {
        return new ProcedureCaptureRepository(db.procedureCaptureDao());
    }

    // PatientRepository already uses @Inject constructor; Hilt can create it if DAO
    // is provided.
    // If PatientDao provider exists via the database's abstract method, Hilt can
    // use it.
    // We add an explicit provider for clarity.

    @Provides
    @Singleton
    public ProcedureSyncRepository provideProcedureSyncRepository(ApiService apiService) {
        return new ProcedureSyncRepository(apiService);
    }

    @Provides
    @Singleton
    public ClinicianApiRepository provideClinicianApiRepository(ApiService apiService) {
        return new ClinicianApiRepository(apiService);
    }
}
