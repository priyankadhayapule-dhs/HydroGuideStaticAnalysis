package com.accessvascularinc.hydroguide.mma.sync;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity;

import com.accessvascularinc.hydroguide.mma.db.repository.EncounterRepository;
import com.accessvascularinc.hydroguide.mma.db.repository.FacilityRepository;
import com.accessvascularinc.hydroguide.mma.db.repository.ProcedureCaptureRepository;
import com.accessvascularinc.hydroguide.mma.db.repository.UsersRepository;
import com.accessvascularinc.hydroguide.mma.network.FacilitySyncModel.FacilitySyncRequest;
import com.accessvascularinc.hydroguide.mma.network.FacilitySyncModel.FacilitySyncResponse;
import com.accessvascularinc.hydroguide.mma.network.NetworkUtils;
import com.accessvascularinc.hydroguide.mma.repository.FacilitySyncRepository;
import com.accessvascularinc.hydroguide.mma.repository.ProcedureSyncRepository;
import com.accessvascularinc.hydroguide.mma.repository.UserSettingsSyncRepository;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockedStatic;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

import javax.inject.Provider;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class SyncManagerTest {

    @Mock
    private Context context;
    @Mock
    private FacilitySyncRepository facilitySyncRepository;
    @Mock
    private ProcedureSyncRepository procedureSyncRepository;
    @Mock
    private FacilityRepository facilityRepository;
    @Mock
    private ProcedureCaptureRepository procedureCaptureRepository;
    @Mock
    private EncounterRepository encounterRepository;
    @Mock
    private Provider<IStorageManager> storageManagerProvider;
    @Mock
    private IStorageManager storageManager;
    @Mock
    private UsersRepository usersRepository;
    @Mock
    private UserSettingsSyncRepository userSettingsSyncRepository;

    private MockedStatic<PrefsManager> prefsManagerMockedStatic;
    private MockedStatic<NetworkUtils> networkUtilsMockedStatic;

    private SyncManager syncManager;

    @Before
    public void setUp() {
        MockitoAnnotations.openMocks(this);
        prefsManagerMockedStatic = org.mockito.Mockito.mockStatic(PrefsManager.class);
        networkUtilsMockedStatic = org.mockito.Mockito.mockStatic(NetworkUtils.class);

        when(storageManagerProvider.get()).thenReturn(storageManager);
        syncManager = new SyncManager(
                context,
                facilitySyncRepository,
                procedureSyncRepository,
                facilityRepository,
                procedureCaptureRepository,
                encounterRepository,
                storageManagerProvider,
                usersRepository,
                userSettingsSyncRepository
        );
    }

    @After
    public void tearDown() {
        prefsManagerMockedStatic.close();
        networkUtilsMockedStatic.close();
    }

    @Test
    public void syncFacility_whenStorageModeNotOnline_shouldReturnEarly() {
        // Arrange
        when(storageManager.getStorageMode()).thenReturn(IStorageManager.StorageMode.OFFLINE);

        // Act
        syncManager.syncFacility();

        // Assert
        verify(facilitySyncRepository, never()).syncFacility(anyString(), any(FacilitySyncRequest.class));
    }

    @Test
    public void syncFacility_whenBlockSyncIsTrue_shouldReturnEarly() {
        // Arrange
        when(storageManager.getStorageMode()).thenReturn(IStorageManager.StorageMode.ONLINE);
        syncManager.setBlockSync(true);

        // Act
        syncManager.syncFacility();

        // Assert
        verify(facilitySyncRepository, never()).syncFacility(anyString(), any(FacilitySyncRequest.class));
    }

    @Test
    public void syncFacility_whenNoNetwork_shouldReturnEarly() {
        // Arrange
        when(storageManager.getStorageMode()).thenReturn(IStorageManager.StorageMode.ONLINE);
        syncManager.setBlockSync(false);
        networkUtilsMockedStatic.when(() -> NetworkUtils.isNetworkAvailable(context)).thenReturn(false);

        // Act
        syncManager.syncFacility();

        // Assert
        verify(facilitySyncRepository, never()).syncFacility(anyString(), any(FacilitySyncRequest.class));
    }

    @Test
    public void syncFacility_whenOrganizationIdIsNull_shouldShowToast() {
        // Arrange
        when(storageManager.getStorageMode()).thenReturn(IStorageManager.StorageMode.ONLINE);
        syncManager.setBlockSync(false);
        networkUtilsMockedStatic.when(() -> NetworkUtils.isNetworkAvailable(context)).thenReturn(true);
        prefsManagerMockedStatic.when(() -> PrefsManager.getOrganizationId(context)).thenReturn(null);

        // Act
        syncManager.syncFacility();

        // Assert
        verify(facilitySyncRepository, never()).syncFacility(anyString(), any(FacilitySyncRequest.class));
        // Toast.show() is called, but we can't verify it without mocking Toast
    }

    @Test
    public void syncFacility_successfulSync_shouldCallRepository() {
        // Arrange
        when(storageManager.getStorageMode()).thenReturn(IStorageManager.StorageMode.ONLINE);
        syncManager.setBlockSync(false);
        networkUtilsMockedStatic.when(() -> NetworkUtils.isNetworkAvailable(context)).thenReturn(true);
        prefsManagerMockedStatic.when(() -> PrefsManager.getOrganizationId(context)).thenReturn("org-id");

        List<FacilityEntity> facilities = new ArrayList<>();
        when(facilityRepository.getAllFacilities()).thenReturn(facilities);

        Call<FacilitySyncResponse> call = mock(Call.class);
        when(facilitySyncRepository.syncFacility(anyString(), any(FacilitySyncRequest.class))).thenReturn(call);
        doNothing().when(call).enqueue(any(Callback.class));

        // Act
        syncManager.syncFacility();

        // Assert
        verify(facilitySyncRepository).syncFacility(eq("org-id"), any(FacilitySyncRequest.class));
    }

    // Add more tests for other methods similarly

    @Test
    public void setBlockSync_shouldSetBlockSyncFlag() {
        // Act
        syncManager.setBlockSync(true);

        // Assert
        when(storageManager.getStorageMode()).thenReturn(IStorageManager.StorageMode.ONLINE);
        networkUtilsMockedStatic.when(() -> NetworkUtils.isNetworkAvailable(context)).thenReturn(true);
        prefsManagerMockedStatic.when(() -> PrefsManager.getOrganizationId(context)).thenReturn("org-id");

        List<FacilityEntity> facilities = new ArrayList<>();
        when(facilityRepository.getAllFacilities()).thenReturn(facilities);

        Call<FacilitySyncResponse> call = mock(Call.class);
        when(facilitySyncRepository.syncFacility(anyString(), any(FacilitySyncRequest.class))).thenReturn(call);
        doNothing().when(call).enqueue(any(Callback.class));

        syncManager.syncFacility();
        verify(facilitySyncRepository, never()).syncFacility(anyString(), any(FacilitySyncRequest.class));
    }
}