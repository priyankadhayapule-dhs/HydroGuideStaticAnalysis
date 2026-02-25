package com.accessvascularinc.hydroguide.mma.utils;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase;
import com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity;
import com.accessvascularinc.hydroguide.mma.db.entities.ProcedureCaptureEntity;
import com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity;
import com.accessvascularinc.hydroguide.mma.db.repository.*;
import com.accessvascularinc.hydroguide.mma.model.EncounterState;
import com.accessvascularinc.hydroguide.mma.model.Facility;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;
import com.accessvascularinc.hydroguide.mma.ui.input.FacilityListAdapter;

import org.json.JSONArray;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.OptionalDouble;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;

public class DbHelper {
  private static final String TAG = "DbHelper";

  private static final ExecutorService ioExecutor = Executors.newSingleThreadExecutor();

  public static final String ID = "id";
  public static final String FACILITY_NAME = "facilityName";
  public static final String DATE_LAST_USED = "dateLastUsed";

  public static void insertDataInDb(android.content.Context context,
      String[] data,
      com.accessvascularinc.hydroguide.mma.model.PatientData patientData,
      FacilityRepository facilityRepository,
      boolean isActive,
      ProcedureCaptureRepository procedureCaptureRepository,
      String dataDirPath) {
    try {
      JSONArray jsonArray = new JSONArray(data[4]);

      // Use PrefsManager to get organizationId
      String organizationId = com.accessvascularinc.hydroguide.mma.utils.PrefsManager.getOrganizationId(context);
      FacilityEntity facilityEntity = new FacilityEntity(
          JsonExtractor.extractJsonObject(data[4], FACILITY_NAME), // facilityName
          JsonExtractor.extractJsonObject(data[4], ID), // organizationId
          JsonExtractor.dateStringToLong(JsonExtractor.extractJsonObject(data[4], DATE_LAST_USED)), // dateLastUsed
          0, // createdBy (Integer, placeholder)
          getCurrentFormattedDate(), // createdOn
          isActive, // isActive
          false, // showPatientName
          false, // showInsertionVein
          false, // showPatientId
          false, // showPatientSide
          false, // showPatientDob
          false, // showArmCircumference
          false, // showVeinSize
          false, // showReasonForInsertion
          false, // showNoOfLumens
          false, // showCatheterSize
          "" // createdByEmail
      );

      Log.d("ABC", "facilityEntity: " + facilityEntity);

      ioExecutor.submit(() -> {
        try {

          FacilityEntity lastFacility = null;
          try {
            if (patientData.getPatientFacility() != null &&
                patientData.getPatientFacility().getId() != null) {
              // Facility model stores id as String; parse to Long for repository.
              long selectedFacilityId = Long.parseLong(patientData.getPatientFacility().getId());
              lastFacility = facilityRepository.getFacilityById(selectedFacilityId);
            }
          } catch (NumberFormatException e) {
            MedDevLog.error(TAG, "Invalid facility id in PatientData: " +
                patientData.getPatientFacility().getId());
          }

          // Fallback: if not found in DB, use the facilityEntity parsed from the incoming
          // data
          // (and insert it if it's new).
          if (lastFacility == null && facilityEntity != null) {
            try {
              facilityRepository.addFacility(facilityEntity);
              lastFacility = facilityRepository.getFacilityById(facilityEntity.getFacilityId());
            } catch (ExecutionException | InterruptedException ex) {
              MedDevLog.error(TAG, "Failed to insert or retrieve fallback facility", ex);
            }
          }

          // Fetch userId from PrefsManager as String (GUID)
          String clinicianId = null;
          try {
            clinicianId = com.accessvascularinc.hydroguide.mma.utils.PrefsManager.getLoginResponse(
                context).getData()
                .getUserId();
          } catch (Exception e) {
            clinicianId = null;
          }

          String conductedBy = null;
          com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse loginResponse = com.accessvascularinc.hydroguide.mma.utils.PrefsManager
              .getLoginResponse(context);
          if (loginResponse != null && loginResponse.getData() != null) {
            conductedBy = loginResponse.getData().getEmail();
          }

          // --- Inserting EncounterEntity synchronously and retrieving its ID ---
          HydroGuideDatabase db = new com.accessvascularinc.hydroguide.mma.db.di.DatabaseModule()
              .provideDatabase(context.getApplicationContext());
          com.accessvascularinc.hydroguide.mma.db.dao.EncounterDao encounterDao = db.encounterDao();
          EncounterEntity encounterEntity = new EncounterEntity();
          encounterEntity.setFacilityId(
              lastFacility != null ? lastFacility.getFacilityId() : null);
          encounterEntity.setOrganizationId(
              lastFacility != null ? lastFacility.getOrganizationId() : null);
          encounterEntity.setFacilityOrLocation(
              lastFacility != null ? lastFacility.getFacilityName() : null);
          encounterEntity.setClinicianId(clinicianId);
          encounterEntity.setConductedBy(conductedBy);
          encounterEntity.setDeviceId(0L);
          encounterEntity.setTabletId(0L);
          OptionalDouble trimOpt = parseOptionalDouble(data[1]);
          encounterEntity.setTrimLengthCm(trimOpt.isPresent() ? trimOpt.getAsDouble() : null);
          OptionalDouble insertedOpt = parseOptionalDouble(data[2]);
          encounterEntity.setInsertedCatheterCm(
              insertedOpt.isPresent() ? insertedOpt.getAsDouble() : null);
          OptionalDouble externalOpt = parseOptionalDouble(data[3]);
          encounterEntity.setExternalCatheterCm(
              externalOpt.isPresent() ? externalOpt.getAsDouble() : null);
          Boolean hydroGuideConfirmed = parseNullableBoolean(data[5]);
          encounterEntity.setHydroGuideConfirmed(
              hydroGuideConfirmed != null ? hydroGuideConfirmed : Boolean.FALSE);
          encounterEntity.setAppSoftwareVersion(data[9]);
          encounterEntity.setState(EncounterState.valueOf(data[8]));
          encounterEntity.setPatient(patientData);
          encounterEntity.setDataDirPath(dataDirPath);
          encounterEntity.setActionType("Added");
          encounterEntity.setSyncStatus("Pending");
          encounterEntity.setCloudDbPrimaryKey(null);
          
          // Set ultrasound flags from data array (indices 12 and 13)
          if (data.length > 12) {
            Boolean isUltrasoundUsed = parseNullableBoolean(data[12]);
            encounterEntity.setIsUltrasoundUsed(isUltrasoundUsed != null ? isUltrasoundUsed : false);
            Log.d("DbHelper", "Set isUltrasoundUsed: " + encounterEntity.getIsUltrasoundUsed());
          }
          if (data.length > 13) {
            Boolean isUltrasoundCaptureSaved = parseNullableBoolean(data[13]);
            encounterEntity.setIsUltrasoundCaptureSaved(isUltrasoundCaptureSaved != null ? isUltrasoundCaptureSaved : false);
            Log.d("DbHelper", "Set isUltrasoundCaptureSaved: " + encounterEntity.getIsUltrasoundCaptureSaved());
          }

          // Log patient data before insertion
          Log.d("DbHelper_Insert", "=== ENCOUNTER DATA BEFORE INSERTION ===");
          Log.d("DbHelper_Insert", "Patient No. of Lumens: '" + patientData.getPatientNoOfLumens() + "'");
          Log.d("DbHelper_Insert", "Patient Catheter Size: '" + patientData.getPatientCatheterSize() + "'");
          Log.d("DbHelper_Insert", "Patient Name: '" + patientData.getPatientName() + "'");
          encounterEntity.setStartTime(System.currentTimeMillis());
          try {
            encounterEntity.setPatientName(jsonArray.getString(1));
            encounterEntity.setPatientMrn(jsonArray.getString(2));
            encounterEntity.setPatientDob(JsonExtractor.dateStringToLong(jsonArray.getString(3)));
          } catch (org.json.JSONException e) {
            MedDevLog.error(TAG,
                "Error parsing patientName, patientMrn, or patientDob from jsonArray", e);
            encounterEntity.setPatientName(null);
            encounterEntity.setPatientMrn(null);
            encounterEntity.setPatientDob(null);
          }

          // Set arm_circumference, insertion_vein, patient_side, vein_size from jsonArray
          // These are stored in data[4] which is a JSON array at indices 5-8
          try {
            OptionalDouble armCircOpt = parseOptionalDouble(jsonArray.getString(5));
            Log.d("DbHelper", "armCircumference: " + (armCircOpt.isPresent() ? armCircOpt.getAsDouble() : "empty"));
            encounterEntity.setArmCircumference(armCircOpt.isPresent() ? armCircOpt.getAsDouble() : null);

            String insertionVein = jsonArray.getString(6);
            Log.d("DbHelper", "insertionVein: " + insertionVein);
            encounterEntity.setInsertionVein(insertionVein);

            String patientSide = jsonArray.getString(7);
            Log.d("DbHelper", "patientSide: " + patientSide);
            encounterEntity.setPatientSide(patientSide);

            OptionalDouble veinSizeOpt = parseOptionalDouble(jsonArray.getString(8));
            Log.d("DbHelper", "veinSize: " + (veinSizeOpt.isPresent() ? veinSizeOpt.getAsDouble() : "empty"));
            encounterEntity.setVeinSize(veinSizeOpt.isPresent() ? veinSizeOpt.getAsDouble() : null);
          } catch (org.json.JSONException e) {
            MedDevLog.error(TAG,
                "Error parsing arm_circumference, insertion_vein, patient_side, or vein_size from jsonArray", e);
          }

          // Extract and set No. of Lumens and Catheter Size from patientData
          if (patientData != null) {
            String noOfLumens = patientData.getPatientNoOfLumens();
            if (noOfLumens != null && !noOfLumens.isEmpty()) {
              encounterEntity.setPatientNoOfLumens(noOfLumens);
              Log.d("DbHelper", "Set patientNoOfLumens from PatientData: '" + noOfLumens + "'");
            }

            String catheterSize = patientData.getPatientCatheterSize();
            if (catheterSize != null && !catheterSize.isEmpty()) {
              encounterEntity.setPatientCatheterSize(catheterSize);
              Log.d("DbHelper", "Set patientCatheterSize from PatientData: '" + catheterSize + "'");
            }
          }

          // Check if an encounter with the same dataDirPath already exists (prevents duplicate entries)
          EncounterEntity existingEncounterWithSameTime = null;
          try {
            List<EncounterEntity> existingEncounters = encounterDao.getEncounterByDirPath(dataDirPath);
            if (existingEncounters != null && !existingEncounters.isEmpty()) {
              for (EncounterEntity existing : existingEncounters) {
                String existingDataDir = existing.getDataDirPath();
                if (dataDirPath != null && dataDirPath.equals(existingDataDir)) {
                  existingEncounterWithSameTime = existing;
                  Log.w("DbHelper", "Encounter with dataDirPath '" + dataDirPath + "' already exists (id=" + existing.getId() + "). Updating existing record.");
                  break;
                }
              }
            }
          } catch (Exception e) {
            MedDevLog.error(TAG, "Error checking for duplicate encounter by dataDirPath", e);
          }

          long encounterRowId = -1;
          EncounterEntity insertedEncounter = null;

          if (existingEncounterWithSameTime == null) {
            // No duplicate found - insert new record
            encounterRowId = encounterDao.insertEncounter(encounterEntity);
            Log.d(TAG, "Inserted EncounterEntity with rowId: " + encounterRowId);

            // Log data after insertion
            Log.d("DbHelper_Insert", "=== ENCOUNTER DATA AFTER INSERTION ===");
            Log.d("DbHelper_Insert", "EncounterEntity No. of Lumens: '" + encounterEntity.getPatientNoOfLumens() + "'");
            Log.d("DbHelper_Insert", "EncounterEntity Catheter Size: '" + encounterEntity.getPatientCatheterSize() + "'");
            Log.d("DbHelper_Insert", "EncounterEntity Row ID: " + encounterRowId);

            // Now fetch the inserted EncounterEntity to get its auto-generated id
            List<EncounterEntity> allEncounters = encounterDao.getAllEncounters();
            if (allEncounters != null && !allEncounters.isEmpty()) {
              insertedEncounter = allEncounters.get(allEncounters.size() - 1);
            }
          } else {
            // Duplicate found - update existing record with new data
            Log.w("DbHelper", "Updating existing encounter with startTime " + encounterEntity.getStartTime());
            
            // Copy new data to existing encounter
            existingEncounterWithSameTime.setFacilityId(encounterEntity.getFacilityId());
            existingEncounterWithSameTime.setOrganizationId(encounterEntity.getOrganizationId());
            existingEncounterWithSameTime.setFacilityOrLocation(encounterEntity.getFacilityOrLocation());
            existingEncounterWithSameTime.setClinicianId(encounterEntity.getClinicianId());
            existingEncounterWithSameTime.setConductedBy(encounterEntity.getConductedBy());
            existingEncounterWithSameTime.setTrimLengthCm(encounterEntity.getTrimLengthCm());
            existingEncounterWithSameTime.setInsertedCatheterCm(encounterEntity.getInsertedCatheterCm());
            existingEncounterWithSameTime.setExternalCatheterCm(encounterEntity.getExternalCatheterCm());
            existingEncounterWithSameTime.setHydroGuideConfirmed(encounterEntity.getHydroGuideConfirmed());
            existingEncounterWithSameTime.setAppSoftwareVersion(encounterEntity.getAppSoftwareVersion());
            existingEncounterWithSameTime.setState(encounterEntity.getState());
            existingEncounterWithSameTime.setPatient(encounterEntity.getPatient());
            existingEncounterWithSameTime.setDataDirPath(encounterEntity.getDataDirPath());
            existingEncounterWithSameTime.setPatientName(encounterEntity.getPatientName());
            existingEncounterWithSameTime.setPatientMrn(encounterEntity.getPatientMrn());
            existingEncounterWithSameTime.setPatientDob(encounterEntity.getPatientDob());
            existingEncounterWithSameTime.setArmCircumference(encounterEntity.getArmCircumference());
            existingEncounterWithSameTime.setInsertionVein(encounterEntity.getInsertionVein());
            existingEncounterWithSameTime.setPatientSide(encounterEntity.getPatientSide());
            existingEncounterWithSameTime.setVeinSize(encounterEntity.getVeinSize());
            existingEncounterWithSameTime.setPatientNoOfLumens(encounterEntity.getPatientNoOfLumens());
            existingEncounterWithSameTime.setPatientCatheterSize(encounterEntity.getPatientCatheterSize());
            existingEncounterWithSameTime.setIsUltrasoundUsed(encounterEntity.getIsUltrasoundUsed());
            existingEncounterWithSameTime.setIsUltrasoundCaptureSaved(encounterEntity.getIsUltrasoundCaptureSaved());
            
            try {
              encounterDao.updateEncounter(existingEncounterWithSameTime);
              Log.d("DbHelper_Insert", "=== ENCOUNTER DATA AFTER UPDATE ===");
              Log.d("DbHelper_Insert", "EncounterEntity No. of Lumens: '" + existingEncounterWithSameTime.getPatientNoOfLumens() + "'");
              Log.d("DbHelper_Insert", "EncounterEntity Catheter Size: '" + existingEncounterWithSameTime.getPatientCatheterSize() + "'");
              Log.d("DbHelper_Insert", "EncounterEntity ID: " + existingEncounterWithSameTime.getId());
              insertedEncounter = existingEncounterWithSameTime;
            } catch (Exception e) {
              MedDevLog.error(TAG, "Error updating existing encounter", e);
            }
          }

          Long encounterId = insertedEncounter != null ? (long) insertedEncounter.getId() : null;
          Log.d("DbHelper", "Using encounterId for ProcedureCaptureEntity: " + encounterId);

          // Log retrieved encounter data
          if (insertedEncounter != null) {
            Log.d("DbHelper_Insert", "=== RETRIEVED ENCOUNTER DATA ===");
            Log.d("DbHelper_Insert", "Retrieved No. of Lumens: '" + insertedEncounter.getPatientNoOfLumens() + "'");
            Log.d("DbHelper_Insert", "Retrieved Catheter Size: '" + insertedEncounter.getPatientCatheterSize() + "'");
          }

          // Use a proper JSON parser for dataSix

          List<List<String>> dataSeven = parseResponse(data[7]);

          // encounterId is now guaranteed to be the correct, just-inserted value
          // Only insert procedure captures if the encounter was successfully inserted
          if (encounterId != null) {
            try {
              org.json.JSONArray arrSix = new org.json.JSONArray(data[6]);
              ProcedureCaptureEntity procedureCaptureEntity = new ProcedureCaptureEntity(
                  encounterId,
                  arrSix.optString(0, null), // captureId
                  arrSix.optString(1, null), // isIntravascular
                  arrSix.optString(2, null), // shownInReport
                  arrSix.optString(3, null), // insertedLengthCm
                  arrSix.optString(4, null), // exposedLengthCm
                  arrSix.optString(5, null), // captureData
                  arrSix.optString(6, null), // markedPoints
                  arrSix.optString(7, null), // upperBound
                  arrSix.optString(8, null), // lowerBound
                  arrSix.optString(9, null) // increment
              );
              Log.d("DbHelper",
                  "Created ProcedureCaptureEntity (arrSix): procedureId=" +
                      procedureCaptureEntity.getProcedureId()
                      + ", encounterId=" + procedureCaptureEntity.getEncounterId() +
                      ", captureId="
                      + procedureCaptureEntity.getCaptureId());
              procedureCaptureRepository.addCapture(procedureCaptureEntity);
            } catch (org.json.JSONException e) {
              e.printStackTrace();
            }

            int test = 0;
            for (List<String> item : dataSeven) {
              Log.d("ABC", "pos::" + test + " :: dataSeven: " + item);
              test++;

              ProcedureCaptureEntity captureEntity = new ProcedureCaptureEntity(
                  encounterId,
                  fetchElementAtPosition(item, 0),
                  fetchElementAtPosition(item, 1),
                  fetchElementAtPosition(item, 2),
                  fetchElementAtPosition(item, 3),
                  fetchElementAtPosition(item, 4),
                  fetchElementAtPosition(item, 5),
                  fetchElementAtPosition(item, 6),
                  fetchElementAtPosition(item, 7),
                  fetchElementAtPosition(item, 8),
                  fetchElementAtPosition(item, 9));

              Log.d("DbHelper",
                  "Created ProcedureCaptureEntity (dataSeven): procedureId=" +
                      captureEntity.getProcedureId()
                      + ", encounterId=" + captureEntity.getEncounterId() + ", captureId="
                      + captureEntity.getCaptureId());
              procedureCaptureRepository.addCapture(captureEntity);
            }
          } else {
            Log.w("DbHelper", "Skipping procedure capture insertion - encounter was not inserted (duplicate or error).");
          }

          showDBData(facilityRepository,
              procedureCaptureRepository);

        } catch (ExecutionException | InterruptedException e) {
          e.printStackTrace();
        }
      });

    } catch (Exception e) {
      e.printStackTrace();
    }
  }

  public static void showDBData(FacilityRepository facilityRepository,
      ProcedureCaptureRepository procedureCaptureRepository) {
    ioExecutor.submit(() -> {
      try {
        List<FacilityEntity> facilities = facilityRepository.getAllFacilities();
        List<ProcedureCaptureEntity> procedureCaptures = procedureCaptureRepository.getAllCaptures();

        for (FacilityEntity f : facilities) {
          Log.v("ABC", "DB_CHECK facilityId: " + f.getFacilityId() + ", facilityName: " +
              f.getFacilityName() + ", organizationId: " + f.getOrganizationId() +
              ", createdOn: " + f.getCreatedOn() + ", isActive: " + f.isActive() +
              ", getCreatedBy: " + f.getCreatedBy());
        }

        for (ProcedureCaptureEntity pc : procedureCaptures) {
          Log.i("ABC", "DB_CHECK procedureCapture: " + pc.getProcedureId() + ", captureId: " +
              pc.getCaptureId() + ", increment: " + pc.getIncrement() + ", lowerBound: " +
              pc.getLowerBound() + ", upperBound: " + pc.getUpperBound() +
              ", captureLocalId: " + pc.getCaptureLocalId() + ", exposedLengthCm: " +
              pc.getExposedLengthCm() + ", insertedLengthCm: " + pc.getInsertedLengthCm() +
              ", isIntravascular: " + pc.getIsIntravascular() + ", shownInReport: " +
              pc.getShownInReport() + ", captureData: " + pc.getCaptureData());
        }

      } catch (ExecutionException | InterruptedException e) {
        e.printStackTrace();
      }
    });
  }

  public static String getCurrentFormattedDate() {
    SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd_HH-mm-ss", Locale.US);
    return sdf.format(new Date());
  }

  public static String fetchElementAtPosition(List<String> data, int position) {
    if (position >= 0 && position < data.size()) {
      return data.get(position);
    }
    return null;
  }

  /**
   * Safely parse a String into an OptionalDouble. Returns OptionalDouble.empty()
   * if the input is
   * null, blank, or cannot be parsed.
   */
  private static OptionalDouble parseOptionalDouble(String raw) {
    if (raw == null) {
      return OptionalDouble.empty();
    }
    String trimmed = raw.trim();
    if (trimmed.isEmpty()) {
      return OptionalDouble.empty();
    }
    try {
      return OptionalDouble.of(Double.parseDouble(trimmed));
    } catch (NumberFormatException e) {
      Log.w("DbHelper", "parseOptionalDouble: invalid number '" + raw + "'", e);
      return OptionalDouble.empty();
    }
  }

  /**
   * Parse a nullable Boolean from a String. Returns null if input is null/blank.
   * Accepts
   * case-insensitive "true" or "false"; anything else returns null.
   */
  private static Boolean parseNullableBoolean(String raw) {
    if (raw == null) {
      return null;
    }
    String trimmed = raw.trim();
    if (trimmed.isEmpty()) {
      return null;
    }
    if (trimmed.equalsIgnoreCase("true")) {
      return Boolean.TRUE;
    }
    if (trimmed.equalsIgnoreCase("false")) {
      return Boolean.FALSE;
    }
    return null; // Unknown value -> treat as not yet set
  }

  public static List<String> parseResponseToList(String response) {
    String cleaned = response.replaceAll("^\\[|\\]$", "");
    String[] parts = cleaned.split(",");
    List<String> list = new ArrayList<>();
    for (String s : parts) {
      list.add(s.trim().replace("\"", ""));
    }
    return list;
  }

  public static List<List<String>> parseResponse(String response) {
    List<List<String>> result = new ArrayList<>();
    try {
      JSONArray outer = new JSONArray(response);
      for (int i = 0; i < outer.length(); i++) {
        String innerString = outer.getString(i);
        JSONArray innerArray = new JSONArray(innerString);
        List<String> innerList = new ArrayList<>();
        for (int j = 0; j < innerArray.length(); j++) {
          innerList.add(innerArray.getString(j));
        }
        result.add(innerList);
      }
    } catch (Exception e) {
      e.printStackTrace();
    }
    return result;
  }

  public static void insertFacilityData(FacilityRepository facilityRepository,
      FacilityEntity facilityDetails) {
    ioExecutor.submit(() -> {
      try {
        facilityRepository.addFacility(facilityDetails);
      } catch (ExecutionException e) {
        throw new RuntimeException(e);
      } catch (InterruptedException e) {
        throw new RuntimeException(e);
      }
      try {
        List<FacilityEntity> facilityDetailsList = facilityRepository.getAllFacilities();
        for (FacilityEntity pc : facilityDetailsList) {
          Log.i("ABC", "DB_CHECK getFacilityId: " + pc.getFacilityId() + ", getFacilityName: " +
              pc.getFacilityName() + ", getDateLastUsed: " + pc.getDateLastUsed());
        }
      } catch (ExecutionException | InterruptedException e) {
        e.printStackTrace();
      }

    });
  }

  public static Facility[] getFacilityName(FacilityRepository facilityRepository) {

    Future<Facility[]> future = ioExecutor.submit(() -> {
      List<FacilityEntity> facilityDetailsList = facilityRepository.getAllFacilities();
      Facility[] facilities = new Facility[facilityDetailsList.size()];

      for (int i = 0; i < facilityDetailsList.size(); i++) {
        FacilityEntity fd = facilityDetailsList.get(i);
        // Assuming your Facility class has a constructor or setters for name/id/date
        facilities[i] = new Facility(fd.getFacilityId().toString(), fd.getFacilityName(),
            fd.getDateLastUsed());
      }
      return facilities;
    });

    try {
      return future.get(); // Wait for background thread result
    } catch (ExecutionException | InterruptedException e) {
      e.printStackTrace();
      return new Facility[0];
    }
  }

  public static List<FacilityEntity> getAllFacilities(FacilityRepository facilityRepository) {

    Future<List<FacilityEntity>> future = ioExecutor.submit(() -> {
      List<FacilityEntity> facilityDetailsList = facilityRepository.getAllFacilities();
      return facilityDetailsList;
    });

    try {
      return future.get(); // Wait for background thread result
    } catch (ExecutionException | InterruptedException e) {
      e.printStackTrace();
      return new ArrayList<FacilityEntity>();
    }
  }

  public static void deleteFacility(Facility[] selectedFacilities,
      FacilityRepository facilityRepository,
      MainViewModel viewmodel,
      FacilityListAdapter facilityListAdapter) {
    final List<Facility> facilities = new ArrayList<>(Arrays.asList(getFacilityName(facilityRepository)));
    ioExecutor.submit(() -> {
      try {
        for (final Facility rmFacility : selectedFacilities) {
          facilities.remove(rmFacility);
          final FacilityEntity facilityDetails = facilityRepository.getFacilityById(Long.parseLong(rmFacility.getId()));
          facilityRepository.deleteFacility(facilityDetails);
        }
      } catch (ExecutionException | InterruptedException e) {
        e.printStackTrace();
      }

    });
    new Handler(Looper.getMainLooper()).post(() -> {
      Facility[] updatedFacilities = DbHelper.getFacilityName(facilityRepository);
      facilityListAdapter.setFacilities(new ArrayList<>(Arrays.asList(updatedFacilities)));
      facilityListAdapter.notifyDataSetChanged();
    });

  }

  public static void updateFacilityInDatabase(Facility updatedFacility,
      FacilityRepository facilityRepository) {
    ioExecutor.submit(() -> {
      try {
        FacilityEntity facilityDetails = facilityRepository.getFacilityById(Long.parseLong(updatedFacility.getId()));
        if (facilityDetails != null) {
          facilityDetails.setFacilityName(updatedFacility.getFacilityName());
          facilityRepository.updateFacility(facilityDetails);
        }

      } catch (ExecutionException | InterruptedException e) {
        e.printStackTrace();
      }
    });
  }

  public static void deleteFacility(FacilityRepository facilityRepository,
      FacilityEntity existingFacility) {
    ioExecutor.submit(() -> {
      try {
        final FacilityEntity facilityEntity = facilityRepository.getFacilityById(existingFacility.getFacilityId());
        facilityEntity.setSyncStatus("Pending");
        facilityEntity.setActionType("Deleted");
        facilityRepository.updateFacility(facilityEntity);
        // facilityRepository.deleteFacility(facilityEntity);
      } catch (ExecutionException | InterruptedException e) {
        e.printStackTrace();
      }
    });
  }

  public static void createFacility(FacilityRepository facilityRepository,
      FacilityEntity newFacility) {
    ioExecutor.submit(() -> {
      try {
        newFacility.setSyncStatus("Pending");
        newFacility.setActionType("Added");
        facilityRepository.addFacility(newFacility);
      } catch (ExecutionException e) {
        throw new RuntimeException(e);
      } catch (InterruptedException e) {
        throw new RuntimeException(e);
      }
    });
  }

  public static void updateFacility(FacilityRepository facilityRepository,
      FacilityEntity existingFacility) {
    ioExecutor.submit(() -> {
      try {
        FacilityEntity facilityEntity = facilityRepository.getFacilityById(existingFacility.getFacilityId());
        facilityEntity.setSyncStatus("Pending");
        facilityEntity.setActionType("Updated");
        if (facilityEntity != null) {
          facilityEntity.setFacilityName(existingFacility.getFacilityName());
          facilityEntity.setShowPatientName(existingFacility.isShowPatientName());
          facilityEntity.setShowPatientId(existingFacility.isShowPatientId());
          facilityEntity.setShowPatientDob(existingFacility.isShowPatientDob());
          facilityEntity.setShowArmCircumference(existingFacility.isShowArmCircumference());
          facilityEntity.setShowVeinSize(existingFacility.isShowVeinSize());
          facilityEntity.setShowReasonForInsertion(existingFacility.isShowReasonForInsertion());
          facilityEntity.setShowNoOfLumens(existingFacility.isShowNoOfLumens());
          facilityEntity.setShowCatheterSize(existingFacility.isShowCatheterSize());
          facilityEntity.setShowInsertionVein(existingFacility.isShowInsertionVein());
          facilityEntity.setShowPatientSide(existingFacility.isShowPatientSide());
          facilityRepository.updateFacility(facilityEntity);
        }
      } catch (ExecutionException | InterruptedException e) {
        MedDevLog.error(TAG, "Error updating facility", e);
      }
    });
  }

  public static List<FacilityEntity> fetchAllFacility(FacilityRepository facilityRepository) {
    Future<List<FacilityEntity>> future = ioExecutor.submit(() -> {
      try {
        return facilityRepository.getAllFacilities();
      } catch (Exception e) {
        MedDevLog.error(TAG, "Error fetching all facilities", e);
        return new ArrayList<>();
      }
    });
    try {
      return future.get();
    } catch (ExecutionException | InterruptedException e) {
      MedDevLog.error(TAG, "Error waiting background thread", e);
      return new ArrayList<>();
    }
  }
}
