
package com.accessvascularinc.hydroguide.mma.sync;

import android.util.Log;
import android.widget.Toast;

import com.accessvascularinc.hydroguide.mma.db.entities.UsersEntity;
import com.accessvascularinc.hydroguide.mma.db.repository.EncounterRepository;
import com.accessvascularinc.hydroguide.mma.db.repository.UsersRepository;
import com.accessvascularinc.hydroguide.mma.network.ApiService;
import com.accessvascularinc.hydroguide.mma.network.NetworkUtils;

import androidx.annotation.NonNull;

import com.accessvascularinc.hydroguide.mma.network.RetrofitClient;
import com.accessvascularinc.hydroguide.mma.network.UserSettingsModel;
import com.accessvascularinc.hydroguide.mma.repository.FacilitySyncRepository;
import com.accessvascularinc.hydroguide.mma.network.FacilitySyncModel.FacilitySyncRequest;
import com.accessvascularinc.hydroguide.mma.network.FacilitySyncModel.FacilitySyncResponse;
import com.accessvascularinc.hydroguide.mma.repository.UserSettingsSyncRepository;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;
import com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity;
import com.accessvascularinc.hydroguide.mma.db.repository.FacilityRepository;
import com.accessvascularinc.hydroguide.mma.db.repository.ProcedureCaptureRepository;
import com.accessvascularinc.hydroguide.mma.db.entities.ProcedureCaptureEntity;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

import com.accessvascularinc.hydroguide.mma.network.ProcedureSyncModel.ProcedureSyncRequest;
import com.accessvascularinc.hydroguide.mma.network.ProcedureSyncModel.ProcedureSyncResponse;
import com.accessvascularinc.hydroguide.mma.repository.ProcedureSyncRepository;
import com.google.gson.Gson;
import com.accessvascularinc.hydroguide.mma.utils.Converters;

import javax.inject.Inject;
import javax.inject.Singleton;
import javax.inject.Provider;

import dagger.hilt.android.qualifiers.ApplicationContext;

import android.content.Context;

@Singleton
public class SyncManager {
  private final Context context;
  private final FacilitySyncRepository facilitySyncRepository;
  private final ProcedureSyncRepository procedureSyncRepository;
  private final FacilityRepository facilityRepository;
  private final ProcedureCaptureRepository procedureCaptureRepository;
  private final EncounterRepository encounterRepository;
  private final Provider<IStorageManager> storageManagerProvider;
  private final UsersRepository usersRepository;
  private final UserSettingsSyncRepository userSettingsSyncRepository;

  private boolean blockSync = false;

  public void setBlockSync(boolean value) {
    blockSync = value;
  }

  @Inject
  public SyncManager(
          @ApplicationContext @NonNull Context context,
          @NonNull FacilitySyncRepository facilitySyncRepository,
          @NonNull ProcedureSyncRepository procedureSyncRepository,
          @NonNull FacilityRepository facilityRepository,
          @NonNull ProcedureCaptureRepository procedureCaptureRepository,
          @NonNull EncounterRepository encounterRepository,
          @NonNull Provider<IStorageManager> storageManagerProvider,
          @NonNull UsersRepository usersRepository,
          @NonNull UserSettingsSyncRepository userSettingsSyncRepository) {
    this.context = context.getApplicationContext();
    this.facilitySyncRepository = facilitySyncRepository;
    this.procedureSyncRepository = procedureSyncRepository;
    this.facilityRepository = facilityRepository;
    this.procedureCaptureRepository = procedureCaptureRepository;
    this.encounterRepository = encounterRepository;
    this.storageManagerProvider = storageManagerProvider;
    this.usersRepository = usersRepository;
    this.userSettingsSyncRepository = userSettingsSyncRepository;
  }

  public void syncFacility() {
    syncFacility(null);
  }

  /**
   * Optionally accepts a callback, called on successful sync.
   */
  public void syncFacility(Runnable onSuccessCallback) {
    if (storageManagerProvider.get().getStorageMode() != IStorageManager.StorageMode.ONLINE) {
      return;
    }
    if (blockSync) {
      return;
    }
    if (!NetworkUtils.isNetworkAvailable(context)) {
      return;
    }
    String organizationId = PrefsManager.getOrganizationId(context);

    if (organizationId == null) {
      Toast.makeText(context, "Organization ID not found", Toast.LENGTH_SHORT).show();
      return;
    }
    Log.d("Facility Sync", "syncFacility: " + organizationId);
    // Fetch facilities with syncStatus = "Pending" from local DB
    java.util.List<FacilitySyncRequest.FacilityDto> addedFacilities = new java.util.ArrayList<>();
    java.util.List<FacilitySyncRequest.FacilityDto> updatedFacilities = new java.util.ArrayList<>();
    try {
      java.util.List<FacilityEntity> allFacilities = facilityRepository.getAllFacilities();
      for (FacilityEntity f : allFacilities) {
        if ("Pending".equalsIgnoreCase(f.getSyncStatus())) {
          FacilitySyncRequest.FacilityDto dto = new FacilitySyncRequest.FacilityDto();
          // dto.FacilityId = f.getCloudDbPrimaryKey();
          dto.FacilityName = f.getFacilityName();
          try {
            dto.OrganizationId = java.util.UUID.fromString(f.getOrganizationId());
          } catch (Exception e) {
            dto.OrganizationId = null;
          }
          dto.IsActive = f.isActive();
          dto.IspatientNameRequired = f.isShowPatientName();
          dto.IsinsertionVeinRequired = f.isShowInsertionVein();
          dto.IspatientIDRequired = f.isShowPatientId();
          dto.IspatientSideRequired = f.isShowPatientSide();
          dto.IspatientDobRequired = f.isShowPatientDob();
          dto.IsarmCircumferenceRequired = f.isShowArmCircumference();
          dto.IsveinSizeRequired = f.isShowVeinSize();
          dto.IsreasonForInsertionRequired = f.isShowReasonForInsertion();
          dto.IsCatherSizeRequired = f.isShowCatheterSize();
          dto.IsNumberOfLumensRequired = f.isShowNoOfLumens();
          dto.CreatedBy = null;
          // dto.CreatedByUserName = null; // Set if available in FacilityEntity
          if ("Added".equalsIgnoreCase(f.getActionType())) {
            addedFacilities.add(dto);
          } else if ("Updated".equalsIgnoreCase(f.getActionType())) {
            dto.FacilityId = f.getCloudDbPrimaryKey();
            updatedFacilities.add(dto);
          }
        }
      }
    } catch (Exception e) {
    }
    FacilitySyncRequest request = new FacilitySyncRequest();
    request.Added = addedFacilities;
    request.Updated = updatedFacilities;
    facilitySyncRepository.syncFacility(organizationId, request)
            .enqueue(new Callback<FacilitySyncResponse>() {
              @Override
              public void onResponse(Call<FacilitySyncResponse> call,
                                     Response<FacilitySyncResponse> response) {
                if (response != null && (response.code() == 401 || response.code() == 403)) {
                  if (unauthorizedListener != null) {
                    unauthorizedListener.onUnauthorized();
                  }
                  return;
                }
                if (response != null && response.isSuccessful() && response.body() != null &&
                        response.body().isSuccess()) {
                  // Upsert facilities in local DB
                  if (response.body().getData() != null) {
                    try {
                      java.util.List<FacilityEntity> localFacilities =
                              facilityRepository.getAllFacilities();
                      java.util.List<FacilitySyncResponse.FacilityData> cloudFacilities =
                              response.body().getData();

                      // --- Upsert logic (existing code) ---
                      for (FacilitySyncResponse.FacilityData facilityData : cloudFacilities) {
                        // Upsert FacilityEntity by facilityName
                        FacilityEntity existingEntity = null;
                        for (FacilityEntity f : localFacilities) {
                          if (f.getFacilityName() != null &&
                                  f.getFacilityName().equals(facilityData.getFacilityName())) {
                            existingEntity = f;
                            break;
                          }
                        }
                        if (existingEntity != null) {
                          // Update existing
                          existingEntity.setOrganizationId(facilityData.getOrganizationId());
                          existingEntity.setDateLastUsed(facilityData.getDateLastUsed());
                          existingEntity.setCreatedOn(facilityData.getCreatedOn());
                          existingEntity.setActive(facilityData.isActive());
                          existingEntity.setShowPatientId(
                                  Boolean.TRUE.equals(facilityData.getIsPatientIDRequired()));
                          existingEntity
                                  .setShowInsertionVein(Boolean.TRUE.equals(
                                          facilityData.getIsInsertionVeinRequired()));
                          existingEntity.setShowPatientName(
                                  Boolean.TRUE.equals(facilityData.getIsPatientNameRequired()));
                          existingEntity.setShowPatientSide(
                                  Boolean.TRUE.equals(facilityData.getIsPatientSideRequired()));
                          existingEntity.setShowPatientDob(
                                  Boolean.TRUE.equals(facilityData.getIsPatientDobRequired()));
                          existingEntity
                                  .setShowArmCircumference(Boolean.TRUE.equals(
                                          facilityData.getIsArmCircumferenceRequired()));
                          existingEntity.setShowVeinSize(
                                  Boolean.TRUE.equals(facilityData.getIsVeinSizeRequired()));
                          existingEntity.setShowReasonForInsertion(
                                  Boolean.TRUE.equals(
                                          facilityData.getIsReasonForInsertionRequired()));
                          existingEntity.setShowCatheterSize(
                                  Boolean.TRUE.equals(facilityData.getIsCatherSizeRequired()));
                          existingEntity.setShowNoOfLumens(
                                  Boolean.TRUE.equals(facilityData.getIsNumberOfLumensRequired()));
                          existingEntity.setCloudDbPrimaryKey(
                                  facilityData.getFacilityId() != 0 ?
                                          (long) facilityData.getFacilityId() : null);
                          existingEntity.setSyncStatus("Synced");
                          existingEntity.setActionType(null);
                          facilityRepository.updateFacility(existingEntity);
                        } else {
                          // Insert new
                          FacilityEntity entity = new FacilityEntity(
                                  facilityData.getFacilityName(),
                                  facilityData.getOrganizationId(),
                                  facilityData.getDateLastUsed(),
                                  1, // adding createdBy 1 will need to update the actual Logic
                                  facilityData.getCreatedOn(),
                                  facilityData.isActive(),
                                  Boolean.TRUE.equals(facilityData.getIsPatientNameRequired()),
                                  Boolean.TRUE.equals(facilityData.getIsInsertionVeinRequired()),
                                  Boolean.TRUE.equals(facilityData.getIsPatientIDRequired()),
                                  Boolean.TRUE.equals(facilityData.getIsPatientSideRequired()),
                                  Boolean.TRUE.equals(facilityData.getIsPatientDobRequired()),
                                  Boolean.TRUE.equals(facilityData.getIsArmCircumferenceRequired()),
                                  Boolean.TRUE.equals(facilityData.getIsVeinSizeRequired()),
                                  Boolean.TRUE.equals(
                                          facilityData.getIsReasonForInsertionRequired()),
                                  Boolean.TRUE.equals(facilityData.getIsCatherSizeRequired()),
                                  Boolean.TRUE.equals(facilityData.getIsNumberOfLumensRequired()),
                                  "test@maildrop.cc"
                                  // adding createdByEmail as test@maildrop.cc will need to update the actual
                                  // Logic
                          );
                          entity.setCloudDbPrimaryKey(
                                  facilityData.getFacilityId() != 0 ?
                                          (long) facilityData.getFacilityId() : null);
                          entity.setSyncStatus("Synced");
                          facilityRepository.addFacility(entity);
                        }
                      }

                      // --- Deletion logic: Remove local facilities not present in cloud ---
                      java.util.HashSet<String> cloudFacilityNames = new java.util.HashSet<>();
                      for (FacilitySyncResponse.FacilityData facilityData : cloudFacilities) {
                        if (facilityData.getFacilityName() != null) {
                          cloudFacilityNames.add(facilityData.getFacilityName());
                        }
                      }
                      for (FacilityEntity local : localFacilities) {
                        if (local.getFacilityName() != null &&
                                !cloudFacilityNames.contains(local.getFacilityName())
                                && !"Pending".equals(local.getSyncStatus())) {
                          facilityRepository.deleteFacility(local);
                        }
                      }
                      /* Sync facilities from cloud to local DB. */
                      if (onSuccessCallback != null) {
                        onSuccessCallback.run();
                      }
                    } catch (Exception e) {
                      MedDevLog.error("SyncManager", "Upsert/Delete failed", e);
                    }
                  }

                  Toast.makeText(context, "Facility sync successful", Toast.LENGTH_SHORT).show();
                } else {
                  MedDevLog.error("SyncManager",
                          "Facility sync failed: " +
                                  (response != null ? response.message() : "null response"));
                  Toast.makeText(context, "Facility sync failed", Toast.LENGTH_SHORT).show();
                }
              }

              @Override
              public void onFailure(Call<FacilitySyncResponse> call, Throwable t) {
                MedDevLog.error("SyncManager", "Facility sync error", t);
                Toast.makeText(context, "Facility sync error", Toast.LENGTH_SHORT).show();
              }
            });
  }

  /**
   * Sync procedures from cloud to local DB.
   */
  public void syncProcedure() {
    if (storageManagerProvider.get().getStorageMode() != IStorageManager.StorageMode.ONLINE) {
      return;
    }
    if (blockSync) {
      return;
    }
    if (!NetworkUtils.isNetworkAvailable(context)) {
      return;
    }

    String organizationId = PrefsManager.getOrganizationId(context);
    if (organizationId == null) {
      Toast.makeText(context, "Organization ID not found", Toast.LENGTH_SHORT).show();
      MedDevLog.error("SyncManager", "Organization ID not found (Toast shown)");
      return;
    }

    new Thread(() -> {
      try {
        java.util.List<com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity>
                allEncounters = encounterRepository
                .getAllEncounterDataList();
        java.util.List<com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity>
                pendingEncounters = new java.util.ArrayList<>();
        for (com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity enc : allEncounters) {
          if ("Pending".equalsIgnoreCase(enc.getSyncStatus())) {
            pendingEncounters.add(enc);
          }
        }

        if (pendingEncounters.isEmpty()) {
          android.os.Handler mainHandler =
                  new android.os.Handler(android.os.Looper.getMainLooper());
          mainHandler.post(
                  () -> {
                    Toast.makeText(context, "No pending encounters to sync", Toast.LENGTH_SHORT)
                            .show();
                    // No pending encounters
                  });
          return;
        }

        ProcedureSyncRequest request = new ProcedureSyncRequest();
        java.util.List<ProcedureSyncRequest.ProcedureData> addedList = new java.util.ArrayList<>();
        // Build file map in-line while constructing ProcedureSyncRequest data
        java.util.Map<Integer, java.io.File> fileMap = new java.util.HashMap<>();

        for (com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity enc : pendingEncounters) {
          ProcedureSyncRequest.ProcedureData data = new ProcedureSyncRequest.ProcedureData();
          // Use ISO8601 for request (previously accepted by server)
          data.startTime = Converters.longToIso8601String(
                  String.valueOf(enc.getStartTime() != null ? enc.getStartTime() : 0L));
          // Debug: also log cloud-style format for matching later
          String dbgCloudFmt = Converters.millisToCloudCentisecondFormat(
                  enc.getStartTime() != null ? enc.getStartTime() : 0L);
          // Preparing encounter data for sync
          data.trimLengthCm =
                  enc.getTrimLengthCm() != null ? enc.getTrimLengthCm().doubleValue() : 0.0;
          data.insertedCatheterCm =
                  enc.getInsertedCatheterCm() != null ? enc.getInsertedCatheterCm().doubleValue()
                          : 0.0;
          data.externalCatheterCm =
                  enc.getExternalCatheterCm() != null ? enc.getExternalCatheterCm().doubleValue()
                          : 0.0;
          data.hydroGuideConfirmed =
                  enc.getHydroGuideConfirmed() != null ? enc.getHydroGuideConfirmed() : false;
          data.state = enc.getState() != null ? enc.getState().name() : null;
          data.appSoftwareVersion = enc.getAppSoftwareVersion();
          data.controllerFirmwareVersion = enc.getControllerFirmwareVersion();
          data.organizationId = enc.getOrganizationId();
          data.facilityOrLocation = enc.getFacilityOrLocation();
          data.clinicianId = enc.getClinicianId();
          data.armCircumference = enc.getArmCircumference();
          data.veinSize = enc.getVeinSize();
          data.insertionVein = enc.getInsertionVein();
          data.patientSide = enc.getPatientSide();

          // Convert noOfLumens and catheterSize from String to Double
          try {
            String lumensStr = enc.getPatientNoOfLumens();
            data.Nooflumens =
                    (lumensStr != null && !lumensStr.isEmpty()) ? Double.parseDouble(lumensStr) :
                            null;
          } catch (NumberFormatException e) {
            Log.e("SyncManager", "Error parsing noOfLumens: " + enc.getPatientNoOfLumens(), e);
            data.Nooflumens = null;
          }

          try {
            String catheterStr = enc.getPatientCatheterSize();
            data.Cathetersize = (catheterStr != null && !catheterStr.isEmpty()) ?
                    Double.parseDouble(catheterStr)
                    : null;
          } catch (NumberFormatException e) {
            Log.e("SyncManager", "Error parsing catheterSize: " + enc.getPatientCatheterSize(), e);
            data.Cathetersize = null;
          }

          data.isUltrasoundUsed = enc.getIsUltrasoundUsed() != null ? enc.getIsUltrasoundUsed() : false;
          data.isUltrasoundCaptureSaved = enc.getIsUltrasoundCaptureSaved() != null ? enc.getIsUltrasoundCaptureSaved() : false;

          // Fetch captures for this encounter
          java.util.List<ProcedureCaptureEntity> captures = new java.util.ArrayList<>();
          for (ProcedureCaptureEntity cap : procedureCaptureRepository.getAllCaptures()) {
            if (cap.getEncounterId() != null && cap.getEncounterId().intValue() == enc.getId()) {
              captures.add(cap);
            }
          }
          java.util.List<ProcedureSyncRequest.ProcedureCapture> captureList =
                  new java.util.ArrayList<>();
          for (ProcedureCaptureEntity cap : captures) {
            ProcedureSyncRequest.ProcedureCapture capture =
                    new ProcedureSyncRequest.ProcedureCapture();
            // Required fields for sync
            try {
              capture.captureId = Integer.parseInt(cap.getCaptureId());
            } catch (Exception e) {
              MedDevLog.error("SyncManager", "Error parsing captureId: " + cap.getCaptureId(), e);
              capture.captureId = 0;
            }
            capture.isIntravascular = "true".equalsIgnoreCase(cap.getIsIntravascular());
            capture.shownInReport = "true".equalsIgnoreCase(cap.getShownInReport());
            try {
              capture.insertedLengthCm = Double.parseDouble(cap.getInsertedLengthCm());
            } catch (Exception e) {
              MedDevLog.error("SyncManager", "Error parsing insertedLengthCm: " + cap.getInsertedLengthCm(),
                      e);
              capture.insertedLengthCm = 0.0;
            }
            try {
              capture.exposedLengthCm = Double.parseDouble(cap.getExposedLengthCm());
            } catch (Exception e) {
              MedDevLog.error("SyncManager", "Error parsing exposedLengthCm: " + cap.getExposedLengthCm(), e);
              capture.exposedLengthCm = 0.0;
            }
            try {
              capture.upperBound = Double.parseDouble(cap.getUpperBound());
            } catch (Exception e) {
              MedDevLog.error("SyncManager", "Error parsing upperBound: " + cap.getUpperBound(), e);
              capture.upperBound = 0.0;
            }
            try {
              capture.lowerBound = Double.parseDouble(cap.getLowerBound());
            } catch (Exception e) {
              MedDevLog.error("SyncManager", "Error parsing lowerBound: " + cap.getLowerBound(), e);
              capture.lowerBound = 0.0;
            }
            try {
              capture.increment = Double.parseDouble(cap.getIncrement());
            } catch (Exception e) {
              MedDevLog.error("SyncManager", "Error parsing increment: " + cap.getIncrement(), e);
              capture.increment = 0.0;
            }
            // Optionally include captureData and markedPoints as before
            try {
              Object captureDataObj = cap.getCaptureData();
              if (captureDataObj instanceof String) {
                String str = (String) captureDataObj;
                if (str != null && str.trim().startsWith("[") && str.trim().endsWith("]")) {
                  capture.captureData = str;
                } else {
                  Object arr = new Gson().fromJson(str, Object.class);
                  capture.captureData = new Gson().toJson(arr);
                }
              } else {
                capture.captureData = new Gson().toJson(captureDataObj);
              }
            } catch (Exception e) {
              MedDevLog.error("SyncManager",
                      "Error serializing captureData for captureId: " + cap.getCaptureId(), e);
              capture.captureData = "[]";
            }
            try {
              Object markedPointsObj = cap.getMarkedPoints();
              if (markedPointsObj instanceof String) {
                String str = (String) markedPointsObj;
                if (str != null && str.trim().startsWith("[") && str.trim().endsWith("]")) {
                  capture.markedPoints = str;
                } else {
                  Object arr = new Gson().fromJson(str, Object.class);
                  capture.markedPoints = new Gson().toJson(arr);
                }
              } else {
                capture.markedPoints = new Gson().toJson(markedPointsObj);
              }
            } catch (Exception e) {
              MedDevLog.error("SyncManager",
                      "Error serializing markedPoints for captureId: " + cap.getCaptureId(), e);
              capture.markedPoints = "[]";
            }
            captureList.add(capture);
          }
          data.procedureCaptures = captureList;

          ProcedureSyncRequest.Patient patient = new ProcedureSyncRequest.Patient();
          String orgId = enc.getOrganizationId();
          if (orgId != null && orgId.matches("[0-9a-fA-F-]{36}")) {
            patient.organizationId = orgId;
          } else {
            patient.organizationId = null;
          }
          patient.patientName = enc.getPatientName();
          patient.patientMrn = enc.getPatientMrn();
          patient.patientDob = Converters.longToDateString(enc.getPatientDob());
          data.patient = patient;

          addedList.add(data);
          // Inline file detection mapped to the index of this ProcedureData
          try {
            String pathLocal = enc.getDataDirPath() + "/ProcedureReport.pdf";
            if (pathLocal != null) {
              java.io.File candidate = new java.io.File(pathLocal);
              java.io.File chosen = null;
              if (candidate.exists()) {
                if (candidate.isFile()) {
                  chosen = candidate;
                  /*
                   * } else if (candidate.isDirectory()) {
                   * java.io.File[] contents = candidate.listFiles();
                   * if (contents != null) {
                   * for (java.io.File f : contents) {
                   * if (f.isFile() && f.length() > 0) { chosen = f; break; }
                   * }
                   * }
                   * }
                   */
                }
              }
              if (chosen != null) {
                int idx = addedList.size() - 1; // index of just-added ProcedureData
                fileMap.put(idx, chosen);
              }
            }
          } catch (Exception ex) {
            MedDevLog.error("SyncManager", "Inline file detection failed: " + ex.getMessage());
          }
        }

        request.setAdded(addedList);

        // Log the outgoing procedureCaptures for each ProcedureData
        for (ProcedureSyncRequest.ProcedureData pdata : addedList) {
          if (pdata.procedureCaptures != null) {
            for (ProcedureSyncRequest.ProcedureCapture cap : pdata.procedureCaptures) {
            }
          }
        }
        String orgId = pendingEncounters.get(0).getOrganizationId();
        // Use multipart variant (unchanged response processing below)
        Call<ProcedureSyncResponse> call =
                procedureSyncRepository.syncProcedureMultipart(orgId, request, fileMap);
        call.enqueue(
                new Callback<ProcedureSyncResponse>() {
                  @Override
                  public void onResponse(
                          Call<ProcedureSyncResponse> call,
                          Response<ProcedureSyncResponse> response) {
                    android.os.Handler mainHandler =
                            new android.os.Handler(android.os.Looper.getMainLooper());
                    if (response != null && (response.code() == 401 || response.code() == 403)) {
                      if (unauthorizedListener != null) {
                        unauthorizedListener.onUnauthorized();
                      }
                      return;
                    }
                    if (response.isSuccessful() && response.body() != null &&
                            response.body().isSuccess()) {
                      new Thread(() -> {
                        int updatedCount = 0;
                        try {
                          java.util.List<com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity>
                                  allEncounters = encounterRepository
                                  .getAllEncounterDataList();
                          java.util.List<com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity>
                                  pendingLocals = new java.util.ArrayList<>();
                          for (com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity e : allEncounters) {
                            if ("Pending".equalsIgnoreCase(e.getSyncStatus()) &&
                                    e.getStartTime() != null
                                    && e.getStartTime() > 0) {
                              pendingLocals.add(e);
                            }
                          }
                          java.util.List<ProcedureSyncResponse.ProcedureData> cloudData =
                                  response.body().getData();
                          java.text.SimpleDateFormat isoParser = new java.text.SimpleDateFormat(
                                  "yyyy-MM-dd'T'HH:mm:ss.SSS");
                          java.text.SimpleDateFormat spaceParser =
                                  new java.text.SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SS");
                          if (cloudData != null) {
                            for (ProcedureSyncResponse.ProcedureData cloudProc : cloudData) {
                              String cloudStartRaw = cloudProc.startTime;
                              if (cloudStartRaw == null || cloudStartRaw.isEmpty()) {
                                continue;
                              }
                              long cloudEpoch;
                              try {
                                cloudEpoch = isoParser.parse(cloudStartRaw).getTime();
                              } catch (Exception pe1) {
                                try {
                                  cloudEpoch = spaceParser.parse(cloudStartRaw).getTime();
                                } catch (Exception pe2) {
                                  // Failed to parse cloud start time; skipping entry
                                  continue;
                                }
                              }
                              final long TOLERANCE_MS = 1500;
                              com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity
                                      best = null;
                              long bestDiff = Long.MAX_VALUE;
                              for (com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity local : pendingLocals) {
                                long diff = Math.abs(local.getStartTime() - cloudEpoch);
                                if (diff <= TOLERANCE_MS && diff < bestDiff) {
                                  bestDiff = diff;
                                  best = local;
                                }
                              }
                              if (best != null) {
                                best.setSyncStatus("Synced");
                                best.setCloudDbPrimaryKey(cloudProc.procedureId);
                                encounterRepository.updateEncounter(best);
                                updatedCount++;
                                pendingLocals.remove(best);
                                // Matched startTime and updated local encounter
                              } else {
                                // No local match within tolerance
                              }
                            }
                          }
                        } catch (Exception e) {
                          MedDevLog.error("SyncManager", "Error during match/update phase", e);
                        }
                        final int finalUpdated = updatedCount;
                        mainHandler.post(() -> {
                          if (finalUpdated > 0) {
                            Toast.makeText(context,
                                    "Encounter sync updated " + finalUpdated + " record(s)",
                                    Toast.LENGTH_SHORT).show();
                            // Updated encounters count
                          } else {
                            Toast.makeText(context, "Encounter sync success but no matches",
                                    Toast.LENGTH_SHORT).show();
                            // Success without matches
                          }
                        });
                      }).start();
                    } else {
                      String errMsg =
                              response.body() != null ? response.body().getErrorMessage() : null;
                      final String toastMsg = "Encounter sync failed code=" + response.code()
                              + (errMsg != null ? (" msg=" + errMsg) : "");
                      mainHandler.post(() -> {
                        Toast.makeText(context, toastMsg, Toast.LENGTH_SHORT).show();
                        // Sync failed toast shown
                      });
                    }
                  }

                  @Override
                  public void onFailure(
                          Call<ProcedureSyncResponse> call,
                          Throwable t) {
                    android.os.Handler mainHandler =
                            new android.os.Handler(android.os.Looper.getMainLooper());
                    mainHandler.post(() -> {
                      Toast.makeText(context, "Encounter sync error: " + t.getMessage(),
                              Toast.LENGTH_SHORT).show();
                      MedDevLog.error("SyncManager", "Encounter sync error: " + t.getMessage(), t);
                    });
                  }
                });
      } catch (Exception e) {
        MedDevLog.error("SyncManager", "Error preparing sync payload", e);
        android.os.Handler mainHandler = new android.os.Handler(android.os.Looper.getMainLooper());
        mainHandler
                .post(() -> {
                  Toast.makeText(context, "Error preparing sync: " + e.getMessage(),
                          Toast.LENGTH_SHORT).show();
                  MedDevLog.error("SyncManager", "Error preparing sync: " + e.getMessage(), e);
                });
      }
    }).start();
  }

  /**
   * Sync user settings from cloud to local database. Fetches the user's audio and waveform labeling
   * preferences from cloud and updates the local database to maintain local DB as source of truth.
   */
  public void syncUserSettings() {
    if (storageManagerProvider.get().getStorageMode() != IStorageManager.StorageMode.ONLINE) {
      Log.d("UserSettings Sync", "Skipping user settings sync - not in online mode");
      return;
    }
    if (blockSync) {
      Log.d("UserSettings Sync", "Skipping user settings sync - sync is blocked");
      return;
    }
    if (!NetworkUtils.isNetworkAvailable(context)) {
      Log.d("UserSettings Sync", "Skipping user settings sync - no network available");
      return;
    }

    String userId = PrefsManager.getUserId(context);
    if (userId == null || userId.isEmpty()) {
      MedDevLog.warn("UserSettings Sync", "User ID not found in preferences");
      return;
    }

    Log.d("UserSettings Sync", "Starting sync for userId: " + userId);

    // Get current user settings from local DB on background thread
    String userEmail = PrefsManager.getUserEmail(context);
    if (userEmail == null || userEmail.isEmpty()) {
      MedDevLog.warn("UserSettings Sync", "User email not found in preferences");
      return;
    }

    // Run database operations on background thread
    new Thread(() -> {
      try {
        UsersEntity localUser = usersRepository
                .getUserByEmail(userEmail);

        if (localUser == null) {
          MedDevLog.warn("UserSettings Sync", "User not found in local database");
          return;
        }

        // Create request with current settings
        UserSettingsModel.UserSettingsSyncRequest request =
                new UserSettingsModel.UserSettingsSyncRequest(
                        localUser.getIsInsertedLengthlabellingOn() != null ?
                                localUser.getIsInsertedLengthlabellingOn() : true,
                        localUser.getIsAudioOn() != null ? localUser.getIsAudioOn() : true);

        // Call API to sync settings using injected repository
        userSettingsSyncRepository.syncUserSettings(userId, request,
                new retrofit2.Callback<UserSettingsModel.UserSettingsSyncResponse>() {
                  @Override
                  public void onResponse(
                          retrofit2.Call<UserSettingsModel.UserSettingsSyncResponse> call,
                          retrofit2.Response<UserSettingsModel.UserSettingsSyncResponse> response) {
                    Log.d("UserSettings Sync",
                            "onResponse - Status Code: " + response.code() + ", isSuccessful: " +
                                    response.isSuccessful());

                    if (response.body() != null) {
                      Log.d("UserSettings Sync",
                              "onResponse - Response body exists. isSuccess: " +
                                      response.body().isSuccess());

                    }

                    if (response.isSuccessful() && response.body() != null &&
                            response.body().isSuccess()) {
                      UserSettingsModel.UserSettingsData data = response.body()
                              .getData();
                      if (data != null) {
                        // Update local database with cloud settings on background thread
                        new Thread(() -> {
                          localUser.setIsInsertedLengthlabellingOn(
                                  data.getIsInsertedLengthlabellingOn());
                          localUser.setIsAudioOn(data.getIsAudioOn());
                          usersRepository.update(localUser);
                          Log.d("UserSettings Sync",
                                  "User settings synced successfully - Inserted Length: " +
                                          data.getIsInsertedLengthlabellingOn() + ", Audio: " +
                                          data.getIsAudioOn());
                        }).start();
                      } else {
                        MedDevLog.warn("UserSettings Sync", "Response body data is null");
                      }
                    } else {
                      String errorMsg =
                              response.body() != null ? response.body().getErrorMessage() :
                                      "Unknown error";
                      MedDevLog.error("UserSettings Sync", "Failed to sync user settings: " + errorMsg +
                              " (HTTP " + response.code() + ", isSuccessful: " +
                              response.isSuccessful() +
                              ", bodySuccess: " +
                              (response.body() != null ? response.body().isSuccess() : "N/A") +
                              ")");
                    }
                  }

                  @Override
                  public void onFailure(
                          retrofit2.Call<UserSettingsModel.UserSettingsSyncResponse> call,
                          Throwable t) {
                    MedDevLog.error("UserSettings Sync", "Error syncing user settings", t);
                  }
                });
      } catch (Exception e) {
        MedDevLog.error("UserSettings Sync", "Error in syncUserSettings", e);
      }
    }).start();
  }

  /**
   * Call this to sync all modules at once (facility, procedures, etc).
   */
  public void syncAll() {
    if (storageManagerProvider.get().getStorageMode() != IStorageManager.StorageMode.ONLINE) {
      return;
    }
    if (blockSync) {
      return;
    }
    if (!NetworkUtils.isNetworkAvailable(context)) {
      return;
    }
    syncFacility();
    syncProcedure();
    syncUserSettings();
  }

  private OnUnauthorizedListener unauthorizedListener;

  public void setOnUnauthorizedListener(OnUnauthorizedListener listener) {
    this.unauthorizedListener = listener;
  }

  public interface OnUnauthorizedListener {
    void onUnauthorized();
  }

  public void getPreferences(){
            if (storageManagerProvider.get().getStorageMode() != IStorageManager.StorageMode.ONLINE) {
              return;
            }
            if (blockSync) {
              return;
            }
            if (!NetworkUtils.isNetworkAvailable(context)) {
              return;
            }

            String userId = PrefsManager.getUserId(context);
            if (userId == null || userId.isEmpty()) {
              return;
            }

            Log.d("UserSettings Sync", "Starting sync for userId: " + userId);

            // Get current user settings from local DB on background thread
            String userEmail = PrefsManager.getUserEmail(context);
            if (userEmail == null || userEmail.isEmpty()) {
              MedDevLog.warn("UserSettings Sync", "User email not found in preferences");
              return;
            }
            new Thread(() -> {
              // Call API to sync settings using injected repository
                UsersEntity localUser = usersRepository.getUserByEmail(userEmail);
                userSettingsSyncRepository.getUserSettings(userId,
                new retrofit2.Callback<UserSettingsModel.UserSettingsSyncResponse>() {
                  @Override
                  public void onResponse(
                          retrofit2.Call<UserSettingsModel.UserSettingsSyncResponse> call,
                          retrofit2.Response<UserSettingsModel.UserSettingsSyncResponse> response) {
                    Log.d("UserSettings Sync",
                            "onResponse - Status Code: " + response.code() + ", isSuccessful: " +
                                    response.isSuccessful());

                    if (response.body() != null) {
                      Log.d("UserSettings Sync",
                              "onResponse - Response body exists. isSuccess: " +
                                      response.body().isSuccess());

                    }

                    if (response.isSuccessful() && response.body() != null &&
                            response.body().isSuccess()) {
                      UserSettingsModel.UserSettingsData data = response.body()
                              .getData();
                      if (data != null) {
                        // Update local database with cloud settings on background thread
                        new Thread(() -> {
                          localUser.setIsInsertedLengthlabellingOn(data.getIsInsertedLengthlabellingOn());
                          localUser.setIsAudioOn(data.getIsAudioOn());
                          usersRepository.update(localUser);
                          Log.d("UserSettings Sync",
                                  "User settings synced successfully - Inserted Length: " +
                                          data.getIsInsertedLengthlabellingOn() + ", Audio: " +
                                          data.getIsAudioOn());
                        }).start();
                      } else {
                        MedDevLog.warn("UserSettings Sync", "Response body data is null");
                      }
                    } else {
                      String errorMsg =
                              response.body() != null ? response.body().getErrorMessage() :
                                      "Unknown error";
                      MedDevLog.error("UserSettings Sync", "Failed to sync user settings: " + errorMsg +
                              " (HTTP " + response.code() + ", isSuccessful: " +
                              response.isSuccessful() +
                              ", bodySuccess: " +
                              (response.body() != null ? response.body().isSuccess() : "N/A") +
                              ")");
                    }
                  }

                  @Override
                  public void onFailure(
                          retrofit2.Call<UserSettingsModel.UserSettingsSyncResponse> call,
                          Throwable t) {
                    MedDevLog.error("UserSettings Sync", "Error syncing user settings", t);
                  }
                });

            }).start();
  }
}
