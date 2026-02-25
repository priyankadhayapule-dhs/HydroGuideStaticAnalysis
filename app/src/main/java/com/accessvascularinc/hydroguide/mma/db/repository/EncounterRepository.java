package com.accessvascularinc.hydroguide.mma.db.repository;

import android.util.Log;

import com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase;
import com.accessvascularinc.hydroguide.mma.db.dao.EncounterDao;
import com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity;
import com.accessvascularinc.hydroguide.mma.model.EncounterData;

import java.util.List;
import java.util.OptionalDouble;
import java.util.UUID;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

public class EncounterRepository {
  private final EncounterDao encounterDao;
  private final ExecutorService executor;

  public EncounterRepository(HydroGuideDatabase db) {
    this.encounterDao = db.encounterDao();
    this.executor = Executors.newSingleThreadExecutor(); // You can use a larger pool if needed
  }

  public void insertEncounterData(EncounterData encounterData) {
    // Convert EncounterData → EncounterEntity using Gson or a mapper
    final EncounterEntity entity = new EncounterEntity();
    entity.setUuid(UUID.randomUUID().toString());
    entity.setStartTime(System.currentTimeMillis());
    entity.setEndTime(null);
    entity.setState(encounterData.getState());
    entity.setPatient(encounterData.getPatient()); // Stored as object (via converter)
    entity.setExternalCapture(encounterData.getExternalCapture());
    entity.setIntravCaptures(encounterData.getIntravCaptures());
    entity.setTrimLengthCm(getValueOrDefault(encounterData.getTrimLengthCm()));
    entity.setInsertedCatheterCm(getValueOrDefault(encounterData.getInsertedCatheterCm()));
    entity.setExternalCatheterCm(getValueOrDefault(encounterData.getExternalCatheterCm()));
    entity.setHydroGuideConfirmed(encounterData.getHydroGuideConfirmed());
    entity.setAppSoftwareVersion(encounterData.getAppSoftwareVersion());
    entity.setControllerFirmwareVersion(encounterData.getControllerFirmwareVersion());
    entity.setProcedureStarted(encounterData.getProcedureStarted());
    entity.setDataDirPath(encounterData.getDataDirPath());
    entity.setActionType("Added");
    entity.setSyncStatus("Pending");
    entity.setCloudDbPrimaryKey(null);
    entity.setOrganizationId(encounterData.getOrganizationId());
    entity.setFacilityId(encounterData.getFacilityId());
    entity.setFacilityOrLocation(encounterData.getFacilityOrLocation());
    entity.setClinicianId(encounterData.getClinicianId());
    entity.setPatientId(encounterData.getPatientId());
    entity.setDeviceId(encounterData.getDeviceId());
    entity.setTabletId(encounterData.getTabletId());

    // Extract patient data fields
    if (encounterData.getPatient() != null) {
      entity.setPatientNoOfLumens(encounterData.getPatient().getPatientNoOfLumens());
      entity.setPatientCatheterSize(encounterData.getPatient().getPatientCatheterSize());
    }

    entity.setIsUltrasoundUsed(encounterData.getIsUltrasoundUsed());
    entity.setIsUltrasoundCaptureSaved(encounterData.getIsUltrasoundCaptureSaved());

    // Run this off the main thread
    Executors.newSingleThreadExecutor().execute(() -> {
      encounterDao.insertEncounter(entity);
      Log.d("ABC", "insertEncounterData size: " + encounterDao.getAllEncounters().size());
    });
  }

  public List<EncounterEntity> getAllEncounterDataList() throws ExecutionException,
      InterruptedException {
    Callable<List<EncounterEntity>> task = () -> encounterDao.getAllEncounters();
    Future<List<EncounterEntity>> future = executor.submit(task);
    return future.get();
  }

  /**
   * Get count of encounters with "Pending" sync status.
   * Must be called from background thread or via executor.
   */
  public int getPendingEncounterCount() throws ExecutionException, InterruptedException {
    Callable<Integer> task = () -> encounterDao.getPendingEncounterCount();
    Future<Integer> future = executor.submit(task);
    return future.get();
  }

  public static double getValueOrDefault(OptionalDouble optional) {
    return optional != null && optional.isPresent() ? optional.getAsDouble() : 0.0;
  }

  public void updateEncounter(EncounterEntity encounter) {
    executor.execute(() -> {
      encounterDao.updateEncounter(encounter);
    });
  }
  public void deleteEncounterByid(int id) {
    executor.execute(() -> {
      List<EncounterEntity> allEncounters = encounterDao.getAllEncounters();
      for (EncounterEntity entity : allEncounters) {
        if (id == entity.getId()) {
          encounterDao.deleteEncounter(entity);
          Log.d("EncounterRepository", "Deleted encounter with UUID: " + id);
          break;
        }
      }
    });
  }

  public void deleteEncounter(EncounterEntity encounter) {
    executor.execute(() -> {
      encounterDao.deleteEncounter(encounter);
    });
  }

}
