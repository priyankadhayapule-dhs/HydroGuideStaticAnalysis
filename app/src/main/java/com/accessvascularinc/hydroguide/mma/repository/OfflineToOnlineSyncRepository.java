package com.accessvascularinc.hydroguide.mma.repository;

import android.content.Context;

import androidx.annotation.Nullable;

import com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity;
import com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity;
import com.accessvascularinc.hydroguide.mma.db.entities.ProcedureCaptureEntity;
import com.accessvascularinc.hydroguide.mma.db.repository.ProcedureCaptureRepository;
import com.accessvascularinc.hydroguide.mma.network.ApiService;
import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginRequest;
import com.accessvascularinc.hydroguide.mma.network.LoginModel.LoginResponse;
import com.accessvascularinc.hydroguide.mma.network.RetrofitClient;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

import javax.inject.Inject;

import dagger.hilt.android.qualifiers.ApplicationContext;

import java.util.List;
import java.util.Map;

/**
 * Repository for Offline → Online mode synchronization. Handles cloud
 * authentication and data sync
 * during mode migration.
 */
public class OfflineToOnlineSyncRepository {
  private final ApiService apiService;
  private final Context context;
  private final ProcedureCaptureRepository procedureCaptureRepository;

  public interface OfflineToOnlineSyncCallback {
    void onResult(boolean success, @Nullable String errorMessage);
  }

  public interface LoginCallback {
    void onResult(@Nullable LoginResponse response, @Nullable String errorMessage);
  }

  @Inject
  public OfflineToOnlineSyncRepository(
      @ApplicationContext Context context,
      ProcedureCaptureRepository procedureCaptureRepository) {
    this.context = context.getApplicationContext();
    this.apiService = RetrofitClient.getClient(context).create(ApiService.class);
    this.procedureCaptureRepository = procedureCaptureRepository;
  }

  /**
   * Authenticate user with cloud credentials
   */
  public void loginToCloud(String email, String password, LoginCallback callback) {
    MedDevLog.info("OfflineToOnlineSync", "Cloud authentication initiated for email: " + email);

    LoginRequest request = new LoginRequest(email, password);

    apiService.login(request).enqueue(new Callback<LoginResponse>() {
      @Override
      public void onResponse(Call<LoginResponse> call, Response<LoginResponse> response) {
        if (response.isSuccessful() && response.body() != null) {
          LoginResponse body = response.body();
          if (body.isSuccess()) {
            MedDevLog.audit("OfflineToOnlineSync", "Cloud authentication successful for email: " + email);
            callback.onResult(body, null);
          } else {
            String errorMsg = body.getErrorMessage() != null ? body.getErrorMessage()
                : (body.getAdditionalInfo() != null ? body.getAdditionalInfo()
                    : "Authentication failed. Please verify your email and password.");
            MedDevLog.warn("OfflineToOnlineSync", "Cloud authentication failed - " + errorMsg);
            callback.onResult(null, errorMsg);
          }
        } else {
          String errorMsg = "Cloud authentication failed. Please check your credentials.";
          MedDevLog.error("OfflineToOnlineSync", "HTTP error during authentication: " + response.code());

          if (response.errorBody() != null) {
            try {
              String errorBody = response.errorBody().string();
              com.google.gson.JsonObject jsonObject = com.google.gson.JsonParser.parseString(errorBody)
                  .getAsJsonObject();
              if (jsonObject.has("message")) {
                errorMsg = jsonObject.get("message").getAsString();
              }
            } catch (Exception e) {
              MedDevLog.error("OfflineToOnlineSync", "Error parsing authentication error response", e);
            }
          }
          callback.onResult(null, errorMsg);
        }
      }

      @Override
      public void onFailure(Call<LoginResponse> call, Throwable t) {
        String errorMsg = t.getMessage() != null ? t.getMessage() : "Network error during cloud authentication";
        MedDevLog.error("OfflineToOnlineSync", "Cloud authentication network failure: " + errorMsg, t);
        callback.onResult(null, errorMsg);
      }
    });
  }

  /**
   * Sync offline data (facilities and encounters) to cloud Backend will handle: -
   * User existence
   * check by email (createdbyEmail field) - User creation with appropriate roles
   * (Organization_Admin for facilities, Clinician for procedures) - Record
   * mapping to users -
   * Atomic insertion of all records - Duplicate detection by StartTime +
   * PatientId + ClinicianId +
   * DeviceId - Transaction rollback on error
   * <p>
   * API Contract: - Endpoint: POST /api/{organizationId}/procedure/offline-switch
   * - Content-Type:
   * multipart/form-data - Form Fields: "facilitiesJson" (JSON array),
   * "proceduresJson" (JSON array),
   * "files" (multipart file uploads) - Response: SyncResponseDTO with nested
   * data.message field
   * containing counts and errors
   *
   * @param organizationId Cloud organization ID
   * @param facilities     List of local facilities to sync
   * @param encounters     List of local encounters to sync
   * @param callback       Result callback
   */
  public void syncOfflineDataToCloud(
      String organizationId,
      java.util.List<FacilityEntity> facilities,
      java.util.List<EncounterEntity> encounters,
      String authToken,
      OfflineToOnlineSyncCallback callback) {

    try {
      MedDevLog.info("OfflineToOnlineSync", "Starting offline-to-online sync. Org: " + organizationId +
          ", Facilities: " + (facilities != null ? facilities.size() : 0) +
          ", Encounters: " + (encounters != null ? encounters.size() : 0));

      // Build facility JSON array (separate from procedures)
      java.util.List<com.google.gson.JsonObject> facilityJsonList = new java.util.ArrayList<>();

      for (FacilityEntity facility : facilities) {
        com.google.gson.JsonObject facilityJson = new com.google.gson.JsonObject();
        facilityJson.addProperty("facilityName", facility.getFacilityName());
        facilityJson.addProperty("organizationId", facility.getOrganizationId());
        facilityJson.addProperty("createdbyEmail", facility.getCreatedByEmail());
        facilityJson.addProperty("isActive", facility.isActive());

        // Add all config flags matching backend expectations
        facilityJson.addProperty("ispatientNameRequired", facility.isShowPatientName());
        facilityJson.addProperty("isinsertionVeinRequired", facility.isShowInsertionVein());
        facilityJson.addProperty("ispatientIDRequired", facility.isShowPatientId());
        facilityJson.addProperty("ispatientSideRequired", facility.isShowPatientSide());
        facilityJson.addProperty("ispatientDobRequired", facility.isShowPatientDob());
        facilityJson.addProperty("isarmCircumferenceRequired", facility.isShowArmCircumference());
        facilityJson.addProperty("isveinSizeRequired", facility.isShowVeinSize());
        facilityJson.addProperty("isreasonForInsertionRequired", facility.isShowReasonForInsertion());
        facilityJson.addProperty("isCatherSizeRequired", facility.isShowCatheterSize());
        facilityJson.addProperty("isNumberOfLumensRequired", facility.isShowNoOfLumens());

        facilityJsonList.add(facilityJson);
      }

      // Serialize facilities array to JSON string
      com.google.gson.Gson gson = new com.google.gson.Gson();
      String facilitiesJson = gson.toJson(facilityJsonList);
      MedDevLog.debug("OfflineToOnlineSync", "Facilities JSON serialized: " + facilityJsonList.size() + " facilities");

      // Build procedure JSON array with captures (following SyncManager pattern)
      java.util.List<com.google.gson.JsonObject> procedureJsonList = new java.util.ArrayList<>();
      // CRITICAL: Use LinkedHashMap to maintain file order matching procedure order
      // File[0] must correspond to Procedure[0], File[1] to Procedure[1], etc.
      java.util.Map<Integer, java.io.File> fileMap = new java.util.LinkedHashMap<>();

      MedDevLog.debug("OfflineToOnlineSync", "Starting procedure JSON serialization...");

      for (int idx = 0; idx < encounters.size(); idx++) {
        EncounterEntity encounter = encounters.get(idx);

        com.google.gson.JsonObject procedureJson = new com.google.gson.JsonObject();
        procedureJson.addProperty("startTime",
            com.accessvascularinc.hydroguide.mma.utils.Converters.longToIso8601String(
                String.valueOf(encounter.getStartTime() != null ? encounter.getStartTime() : 0L)));
        procedureJson.addProperty("organizationId", encounter.getOrganizationId());
        procedureJson.addProperty("facilityOrLocation", encounter.getFacilityOrLocation());
        procedureJson.addProperty("createdbyEmail", encounter.getConductedBy()); // For facility user creation
        procedureJson.addProperty("clinicianEmail", encounter.getConductedBy()); // Use conductedBy as clinician email
        procedureJson.addProperty("clinicianName", encounter.getConductedBy());

        // Add device and tablet IDs (convert from long to String)
        procedureJson.addProperty("deviceId",
            encounter.getDeviceId() != 0 ? String.valueOf(encounter.getDeviceId()) : "");
        procedureJson.addProperty("tabletId",
            encounter.getTabletId() != 0 ? String.valueOf(encounter.getTabletId()) : "");

        // Add procedure measurements
        procedureJson.addProperty("trimLengthCm",
            encounter.getTrimLengthCm() != null ? encounter.getTrimLengthCm() : 0.0);
        procedureJson.addProperty("insertedCatheterCm",
            encounter.getInsertedCatheterCm() != null ? encounter.getInsertedCatheterCm() : 0.0);
        procedureJson.addProperty("externalCatheterCm",
            encounter.getExternalCatheterCm() != null ? encounter.getExternalCatheterCm() : 0.0);

        // Add procedure status and firmware info
        procedureJson.addProperty("hydroGuideConfirmed",
            encounter.getHydroGuideConfirmed() != null ? encounter.getHydroGuideConfirmed() : false);
        procedureJson.addProperty("state", encounter.getState() != null ? encounter.getState().toString() : "PENDING");
        procedureJson.addProperty("appSoftwareVersion",
            encounter.getAppSoftwareVersion() != null ? encounter.getAppSoftwareVersion() : "");
        procedureJson.addProperty("controllerFirmwareVersion",
            encounter.getControllerFirmwareVersion() != null ? encounter.getControllerFirmwareVersion() : "");

        // Add patient body measurements
        procedureJson.addProperty("armCircumference",
            encounter.getArmCircumference() != null ? encounter.getArmCircumference() : 0.0);
        procedureJson.addProperty("veinSize", encounter.getVeinSize() != null ? encounter.getVeinSize() : 0.0);

        // Add insertion details
        procedureJson.addProperty("insertionVein",
            encounter.getInsertionVein() != null ? encounter.getInsertionVein() : "");
        procedureJson.addProperty("patientSide", encounter.getPatientSide() != null ? encounter.getPatientSide() : "");
        procedureJson.addProperty("nooflumens",
            encounter.getPatientNoOfLumens() != null ? encounter.getPatientNoOfLumens() : "");
        procedureJson.addProperty("cathetersize",
            encounter.getPatientCatheterSize() != null ? encounter.getPatientCatheterSize() : "");

        // Add ultrasound flags details
        procedureJson.addProperty("isUltrasoundUsed",
            encounter.getIsUltrasoundUsed() != null ? encounter.getIsUltrasoundUsed() : false);
        procedureJson.addProperty("isUltrasoundCaptureSaved",
            encounter.getIsUltrasoundCaptureSaved() != null ? encounter.getIsUltrasoundCaptureSaved() : false);

        // Build patient object
        com.google.gson.JsonObject patientJson = new com.google.gson.JsonObject();
        patientJson.addProperty("patientName", encounter.getPatientName());
        patientJson.addProperty("patientMrn", encounter.getPatientMrn());
        String patientDobFormatted = com.accessvascularinc.hydroguide.mma.utils.Converters.longToDateString(
            encounter.getPatientDob());
        patientJson.addProperty("patientDob", patientDobFormatted != null ? patientDobFormatted : "");
        procedureJson.add("patient", patientJson);

        // Fetch and build captures array (following SyncManager pattern)
        com.google.gson.JsonArray capturesArray = new com.google.gson.JsonArray();
        try {
          java.util.List<ProcedureCaptureEntity> allCaptures = procedureCaptureRepository.getAllCaptures();

          for (ProcedureCaptureEntity cap : allCaptures) {
            if (cap.getEncounterId() != null && cap.getEncounterId().intValue() == encounter.getId()) {
              com.google.gson.JsonObject captureObj = new com.google.gson.JsonObject();

              // Required fields for capture
              try {
                captureObj.addProperty("captureId", Integer.parseInt(cap.getCaptureId()));
              } catch (Exception e) {
                MedDevLog.warn("OfflineToOnlineSync",
                    "Error parsing captureId: " + cap.getCaptureId());
                captureObj.addProperty("captureId", 0);
              }

              captureObj.addProperty("isIntravascular",
                  "true".equalsIgnoreCase(cap.getIsIntravascular()));
              captureObj.addProperty("shownInReport",
                  "true".equalsIgnoreCase(cap.getShownInReport()));

              try {
                captureObj.addProperty("insertedLengthCm", Double.parseDouble(cap.getInsertedLengthCm()));
              } catch (Exception e) {
                captureObj.addProperty("insertedLengthCm", 0.0);
              }

              try {
                captureObj.addProperty("exposedLengthCm", Double.parseDouble(cap.getExposedLengthCm()));
              } catch (Exception e) {
                captureObj.addProperty("exposedLengthCm", 0.0);
              }

              try {
                captureObj.addProperty("upperBound", Double.parseDouble(cap.getUpperBound()));
              } catch (Exception e) {
                captureObj.addProperty("upperBound", 0.0);
              }

              try {
                captureObj.addProperty("lowerBound", Double.parseDouble(cap.getLowerBound()));
              } catch (Exception e) {
                captureObj.addProperty("lowerBound", 0.0);
              }

              try {
                captureObj.addProperty("increment", Double.parseDouble(cap.getIncrement()));
              } catch (Exception e) {
                captureObj.addProperty("increment", 0.0);
              }

              // Add capture data and marked points as JSON strings
              // Backend expects: "captureData": "[0.074, 0.091, ...]" (JSON string, not
              // array)
              try {
                Object captureDataObj = cap.getCaptureData();
                String captureDataJson = "";

                if (captureDataObj instanceof String) {
                  String str = (String) captureDataObj;
                  if (str != null && !str.isEmpty()) {
                    // If it's already a string, check if it needs to be converted
                    if (str.trim().startsWith("[") && str.trim().endsWith("]")) {
                      // Already in JSON array format, use as-is
                      captureDataJson = str;
                    } else {
                      // Try to parse and re-serialize as JSON
                      try {
                        Object parsed = gson.fromJson(str, Object.class);
                        captureDataJson = gson.toJson(parsed);
                      } catch (Exception ex) {
                        captureDataJson = str;
                      }
                    }
                  } else {
                    captureDataJson = "[]";
                  }
                } else if (captureDataObj instanceof java.util.List) {
                  // Convert List to JSON array string
                  captureDataJson = gson.toJson(captureDataObj);
                } else if (captureDataObj != null) {
                  // Convert any object to JSON
                  captureDataJson = gson.toJson(captureDataObj);
                } else {
                  captureDataJson = "[]";
                }

                // Add as string property so it becomes "captureData": "[...]" not
                // "captureData": [...]
                captureObj.addProperty("captureData", captureDataJson);
                MedDevLog.debug("OfflineToOnlineSync",
                    "captureData as JSON string: "
                        + (captureDataJson.length() > 50 ? captureDataJson.substring(0, 50) + "..." : captureDataJson));
              } catch (Exception e) {
                MedDevLog.warn("OfflineToOnlineSync",
                    "Error serializing captureData: " + e.getMessage());
                captureObj.addProperty("captureData", "[]");
              }

              // Add marked points as JSON string
              // Backend expects: "markedPoints": "[null, null, 0.24, ...]" (JSON string, not
              // array)
              try {
                Object markedPointsObj = cap.getMarkedPoints();
                String markedPointsJson = "";

                if (markedPointsObj instanceof String) {
                  String str = (String) markedPointsObj;
                  if (str != null && !str.isEmpty()) {
                    // If it's already a string, check if it needs to be converted
                    if (str.trim().startsWith("[") && str.trim().endsWith("]")) {
                      // Already in JSON array format, use as-is
                      markedPointsJson = str;
                    } else {
                      // Try to parse and re-serialize as JSON
                      try {
                        Object parsed = gson.fromJson(str, Object.class);
                        markedPointsJson = gson.toJson(parsed);
                      } catch (Exception ex) {
                        markedPointsJson = str;
                      }
                    }
                  } else {
                    markedPointsJson = "[]";
                  }
                } else if (markedPointsObj instanceof java.util.List) {
                  // Convert List to JSON array string
                  markedPointsJson = gson.toJson(markedPointsObj);
                } else if (markedPointsObj != null) {
                  // Convert any object to JSON
                  markedPointsJson = gson.toJson(markedPointsObj);
                } else {
                  markedPointsJson = "[]";
                }

                // Add as string property so it becomes "markedPoints": "[...]" not
                // "markedPoints": [...]
                captureObj.addProperty("markedPoints", markedPointsJson);
                MedDevLog.debug("OfflineToOnlineSync",
                    "markedPoints as JSON string: "
                        + (markedPointsJson.length() > 50 ? markedPointsJson.substring(0, 50) + "..."
                            : markedPointsJson));
              } catch (Exception e) {
                MedDevLog.warn("OfflineToOnlineSync",
                    "Error serializing markedPoints: " + e.getMessage());
                captureObj.addProperty("markedPoints", "[]");
              }

              capturesArray.add(captureObj);
            }
          }
        } catch (Exception e) {
          MedDevLog.error("OfflineToOnlineSync",
              "Error fetching captures for encounter " + encounter.getId(), e);
        }

        procedureJson.add("procedureCaptures", capturesArray);
        procedureJsonList.add(procedureJson);

        // Detect procedure report file (following SyncManager pattern)
        try {
          String pathLocal = encounter.getDataDirPath() + "/ProcedureReport.pdf";
          if (pathLocal != null) {
            java.io.File candidate = new java.io.File(pathLocal);
            if (candidate.exists() && candidate.isFile()) {
              fileMap.put(idx, candidate);
              MedDevLog.debug("OfflineToOnlineSync",
                  "Found procedure file at index " + idx + ": " + pathLocal);
            }
          }
        } catch (Exception ex) {
          MedDevLog.warn("OfflineToOnlineSync",
              "Inline file detection failed for encounter " + encounter.getId() + ": " +
                  ex.getMessage());
        }
      }

      // Serialize procedures array to JSON string
      // IMPORTANT: Even if empty, must send valid JSON array [] so backend doesn't
      // fail to parse
      String proceduresJson = gson.toJson(procedureJsonList);
      MedDevLog.debug("OfflineToOnlineSync", "Procedures JSON serialized: " + procedureJsonList.size() + " procedures");

      // Check if there is any data to sync
      // Allow facilities without procedures OR procedures without facilities
      // But reject if both are empty
      if ((facilityJsonList == null || facilityJsonList.isEmpty())
          && (procedureJsonList == null || procedureJsonList.isEmpty())) {
        MedDevLog.warn("OfflineToOnlineSync", "No facilities or procedures found for migration");
        android.os.Handler mainHandler = new android.os.Handler(android.os.Looper.getMainLooper());
        mainHandler.post(() -> {
          android.widget.Toast
              .makeText(context, "No data found for migration",
                  android.widget.Toast.LENGTH_SHORT)
              .show();
        });
        callback.onResult(false, "No facilities or procedures to sync");
        return;
      }

      // Log what will be synced
      MedDevLog.debug("OfflineToOnlineSync", "Syncing " + facilityJsonList.size() + " facilities, " +
          procedureJsonList.size() + " procedures");

      MedDevLog.info("OfflineToOnlineSync", "Data validation passed. Preparing API request...");

      // Build multipart file list for upload (following SyncManager pattern)
      // CRITICAL: Files MUST be in same order as procedures for backend validation
      java.util.List<okhttp3.MultipartBody.Part> fileParts = new java.util.ArrayList<>();

      MedDevLog.debug("OfflineToOnlineSync", "");
      MedDevLog.debug("OfflineToOnlineSync", "=== BUILDING FILE PARTS ===");
      if (!procedureJsonList.isEmpty()) {
        MedDevLog.debug("OfflineToOnlineSync", "  ⚠️ CRITICAL: Maintaining file order to match procedures");
      } else {
        MedDevLog.debug("OfflineToOnlineSync", "  (No procedures, files list will be empty)");
      }

      for (java.util.Map.Entry<Integer, java.io.File> entry : fileMap.entrySet()) {
        int procedureIndex = entry.getKey();
        java.io.File file = entry.getValue();

        MedDevLog.debug("OfflineToOnlineSync",
            "  [Procedure " + procedureIndex + "] File: " + file.getName()
                + " (" + file.length() + " bytes)");

        okhttp3.RequestBody fileBody = okhttp3.RequestBody.create(file,
            okhttp3.MediaType.parse("application/pdf"));
        okhttp3.MultipartBody.Part filePart = okhttp3.MultipartBody.Part.createFormData(
            "files",
            file.getName(),
            fileBody);
        fileParts.add(filePart);
      }

      MedDevLog.debug("OfflineToOnlineSync", "✓ Multipart file list built. Total files: " + fileParts.size());
      MedDevLog.debug("OfflineToOnlineSync",
          "✓ File order matches procedure order (LinkedHashMap preserves insertion order)");
      MedDevLog.debug("OfflineToOnlineSync", "");

      // VALIDATION: Each procedure MUST have a corresponding file
      if (procedureJsonList.size() != fileParts.size()) {
        MedDevLog.error("OfflineToOnlineSync", "File validation error: Procedures=" + procedureJsonList.size() +
            ", Files=" + fileParts.size() + ". Each procedure must have a corresponding file");

        android.os.Handler mainHandler = new android.os.Handler(android.os.Looper.getMainLooper());
        mainHandler.post(() -> {
          android.widget.Toast
              .makeText(context,
                  "Error: Each procedure must have a file. Procedures: " + procedureJsonList.size()
                      + ", Files: " + fileParts.size(),
                  android.widget.Toast.LENGTH_LONG)
              .show();
        });

        callback.onResult(false,
            "File validation error: Each procedure must have a corresponding file. "
                + "Expected " + procedureJsonList.size() + " files but found " + fileParts.size());
        return;
      }

      MedDevLog.debug("OfflineToOnlineSync", "File validation passed - " + procedureJsonList.size() +
          " procedures, " + fileParts.size() + " files");

      // Build Authorization header with Bearer token
      String authHeaderValue = "Bearer " + authToken;

      // Call cloud API endpoint with separate form fields and auth token
      // Backend expects: POST /api/{organizationId}/procedure/offline-switch
      // Form fields: facilitiesJson, proceduresJson, files
      // Header: Authorization: Bearer {token}
      MedDevLog.debug("OfflineToOnlineSync", "Sending multipart request to: /api/" + organizationId +
          "/procedure/offline-switch");

      MedDevLog.debug("OfflineToOnlineSync", "");

      // Build RequestBody objects for JSON fields
      MedDevLog.debug("OfflineToOnlineSync", "✓ Building MultipartBody.Part list:");

      okhttp3.RequestBody facilitiesJsonBody = okhttp3.RequestBody.create(
          okhttp3.MediaType.parse("application/json; charset=utf-8"),
          facilitiesJson);
      MedDevLog.debug("OfflineToOnlineSync",
          "  - facilitiesJsonBody created: " + facilitiesJsonBody.contentLength() + " bytes");

      okhttp3.RequestBody proceduresJsonBody = okhttp3.RequestBody.create(
          okhttp3.MediaType.parse("application/json; charset=utf-8"),
          proceduresJson);
      MedDevLog.debug("OfflineToOnlineSync",
          "  - proceduresJsonBody created: " + proceduresJsonBody.contentLength() + " bytes");

      // Send with separate named RequestBody fields and file parts list
      MedDevLog.debug("OfflineToOnlineSync",
          "  ✓ facilitiesJsonBody: " + facilitiesJsonBody.contentLength() + " bytes");
      MedDevLog.debug("OfflineToOnlineSync",
          "  ✓ proceduresJsonBody: " + proceduresJsonBody.contentLength() + " bytes");
      MedDevLog.debug("OfflineToOnlineSync",
          "  ✓ File parts: " + fileParts.size());

      MedDevLog.debug("OfflineToOnlineSync", "");
      MedDevLog.debug("OfflineToOnlineSync", "=== SENDING REQUEST ===");
      MedDevLog.debug("OfflineToOnlineSync",
          "Using @Part(\"facilitiesJson\") + @Part(\"proceduresJson\") + @Part(\"files\")");
      MedDevLog.debug("OfflineToOnlineSync", "");

      apiService
          .syncOfflineDataToCloud(organizationId, authHeaderValue, facilitiesJsonBody, proceduresJsonBody, fileParts)
          .enqueue(
              new retrofit2.Callback<com.accessvascularinc.hydroguide.mma.network.OfflineSyncPayload.SyncResponseDTO>() {
                @Override
                public void onResponse(
                    retrofit2.Call<com.accessvascularinc.hydroguide.mma.network.OfflineSyncPayload.SyncResponseDTO> call,
                    retrofit2.Response<com.accessvascularinc.hydroguide.mma.network.OfflineSyncPayload.SyncResponseDTO> response) {
                  MedDevLog.debug("OfflineToOnlineSync", "");
                  MedDevLog.debug("OfflineToOnlineSync", "=== API RESPONSE RECEIVED ===");
                  MedDevLog.debug("OfflineToOnlineSync", "  HTTP Status Code: " + response.code());
                  MedDevLog.debug("OfflineToOnlineSync", "  HTTP Status Message: " + response.message());
                  MedDevLog.debug("OfflineToOnlineSync", "  Is Successful (2xx): " + response.isSuccessful());
                  MedDevLog.debug("OfflineToOnlineSync", "  Body is null: " + (response.body() == null));
                  MedDevLog.debug("OfflineToOnlineSync", "");
                  MedDevLog.debug("OfflineToOnlineSync", "=== RESPONSE HEADERS ===");
                  for (String headerName : response.headers().names()) {
                    MedDevLog.debug("OfflineToOnlineSync",
                        "  " + headerName + ": " + response.headers().get(headerName));
                  }
                  MedDevLog.debug("OfflineToOnlineSync", "");

                  // CRITICAL: Check the 'success' flag in response body
                  // Backend now returns HTTP 200 always, but success flag indicates actual result
                  // success = true → Clear DB and switch mode to ONLINE
                  // success = false → Keep DB intact and mode as OFFLINE
                  if (response.body() != null) {
                    com.accessvascularinc.hydroguide.mma.network.OfflineSyncPayload.SyncResponseDTO responseDto = response
                        .body();

                    MedDevLog.debug("OfflineToOnlineSync", "=== RESPONSE BODY ===");
                    MedDevLog.debug("OfflineToOnlineSync", "  response.success: " + responseDto.isSuccess());
                    MedDevLog.debug("OfflineToOnlineSync", "  response.message: " + responseDto.getMessage());
                    MedDevLog.debug("OfflineToOnlineSync",
                        "  response.errorMessage: " + responseDto.getErrorMessage());
                    MedDevLog.debug("OfflineToOnlineSync",
                        "  response.additionalInfo: " + responseDto.getAdditionalInfo());
                    MedDevLog.debug("OfflineToOnlineSync",
                        "  response.facilitiesSynced: " + responseDto.getFacilitiesSynced());
                    MedDevLog.debug("OfflineToOnlineSync",
                        "  response.proceduresSynced: " + responseDto.getProceduresSynced());

                    if (responseDto.getData() != null) {
                      MedDevLog.debug("OfflineToOnlineSync", "  response.data: {");
                      MedDevLog.debug("OfflineToOnlineSync",
                          "    data.message: " + responseDto.getData().getMessage());
                      MedDevLog.debug("OfflineToOnlineSync", "  }");
                    } else {
                      MedDevLog.debug("OfflineToOnlineSync", "  response.data: null");
                    }
                    MedDevLog.debug("OfflineToOnlineSync", "");

                    // CHECK SUCCESS FLAG - NEW LOGIC
                    if (responseDto.isSuccess()) {
                      // Success = true → Proceed with database clearing and mode switch
                      String message = responseDto.getMessage();
                      int facilitiesSynced = responseDto.getFacilitiesSynced();
                      int proceduresSynced = responseDto.getProceduresSynced();

                      MedDevLog.info("OfflineToOnlineSync", "");
                      MedDevLog.info("OfflineToOnlineSync",
                          "✅ SYNC SUCCESSFUL (success: true) ✅");
                      MedDevLog.info("OfflineToOnlineSync",
                          "  HTTP Status: 200 OK");
                      MedDevLog.info("OfflineToOnlineSync",
                          "  Success Flag: TRUE");
                      MedDevLog.info("OfflineToOnlineSync",
                          "  Facilities synced: " + facilitiesSynced);
                      MedDevLog.info("OfflineToOnlineSync",
                          "  Procedures synced: " + proceduresSynced);
                      MedDevLog.info("OfflineToOnlineSync", "  Backend message: " + message);
                      MedDevLog.info("OfflineToOnlineSync", "  ✓ Database will be cleared");
                      MedDevLog.info("OfflineToOnlineSync", "  ✓ Mode will switch to ONLINE");
                      MedDevLog.debug("OfflineToOnlineSync", "✓ Invoking callback.onResult(true, null)...");
                      callback.onResult(true, null);
                      MedDevLog.debug("OfflineToOnlineSync", "✓ callback.onResult() invoked");
                    } else {
                      // Success = false → DO NOT clear database, keep mode as OFFLINE
                      String message = responseDto.getMessage();
                      String errorMessage = responseDto.getErrorMessage();
                      String additionalInfo = responseDto.getAdditionalInfo();

                      MedDevLog.warn("OfflineToOnlineSync", "");
                      MedDevLog.warn("OfflineToOnlineSync",
                          "❌ SYNC FAILED (success: false) ❌");
                      MedDevLog.warn("OfflineToOnlineSync",
                          "  HTTP Status: 200 OK");
                      MedDevLog.warn("OfflineToOnlineSync",
                          "  Success Flag: FALSE");
                      MedDevLog.warn("OfflineToOnlineSync",
                          "  Backend message: " + message);
                      if (errorMessage != null && !errorMessage.isEmpty()) {
                        MedDevLog.warn("OfflineToOnlineSync",
                            "  Error details: " + errorMessage);
                      }
                      if (additionalInfo != null && !additionalInfo.isEmpty()) {
                        MedDevLog.warn("OfflineToOnlineSync",
                            "  Additional info: " + additionalInfo);
                      }
                      MedDevLog.warn("OfflineToOnlineSync", "");
                      MedDevLog.warn("OfflineToOnlineSync", "  ⚠️ DATABASE WILL NOT BE CLEARED");
                      MedDevLog.warn("OfflineToOnlineSync", "  ⚠️ MODE WILL REMAIN OFFLINE");
                      MedDevLog.warn("OfflineToOnlineSync", "  ⚠️ Data retained for retry");
                      MedDevLog.warn("OfflineToOnlineSync", "");

                      // Build user-friendly error message
                      String errorMsg = message;
                      if (errorMsg == null || errorMsg.isEmpty()) {
                        errorMsg = "Sync failed: " + (additionalInfo != null && !additionalInfo.isEmpty()
                            ? additionalInfo
                            : "Please check your data and try again.");
                      }

                      MedDevLog.debug("OfflineToOnlineSync", "✓ Invoking callback.onResult(false, errorMsg)...");
                      callback.onResult(false, errorMsg);
                      MedDevLog.debug("OfflineToOnlineSync", "✓ callback.onResult() invoked");
                    }
                  } else {
                    // Response body is null - this shouldn't happen but handle it
                    MedDevLog.error("OfflineToOnlineSync", "");
                    MedDevLog.error("OfflineToOnlineSync", "❌ API RESPONSE ERROR ❌");
                    MedDevLog.error("OfflineToOnlineSync", "  Response body is null");
                    MedDevLog.error("OfflineToOnlineSync",
                        "  HTTP Status: " + response.code() + " " + response.message());
                    MedDevLog.error("OfflineToOnlineSync", "");
                    MedDevLog.error("OfflineToOnlineSync", "  ⚠️ DATABASE WILL NOT BE CLEARED");
                    MedDevLog.error("OfflineToOnlineSync", "  ⚠️ MODE WILL REMAIN OFFLINE");
                    MedDevLog.error("OfflineToOnlineSync", "");

                    String errorMsg = "Failed to parse server response. Database will NOT be cleared. Please retry.";

                    if (response.errorBody() != null) {
                      try {
                        String errorBody = response.errorBody().string();
                        MedDevLog.error("OfflineToOnlineSync", "  Error Response Body: " + errorBody);

                        com.google.gson.JsonObject jsonObject = com.google.gson.JsonParser
                            .parseString(errorBody)
                            .getAsJsonObject();
                        if (jsonObject.has("data")) {
                          com.google.gson.JsonObject data = jsonObject.getAsJsonObject("data");
                          if (data.has("message")) {
                            errorMsg = data.get("message").getAsString();
                          }
                        } else if (jsonObject.has("message")) {
                          errorMsg = jsonObject.get("message").getAsString();
                        }
                        MedDevLog.error("OfflineToOnlineSync", "  Parsed Error Message: " + errorMsg);
                      } catch (Exception e) {
                        MedDevLog.error("OfflineToOnlineSync",
                            "  Error parsing error body: " + e.getMessage(), e);
                      }
                    } else {
                      MedDevLog.error("OfflineToOnlineSync", "  Error Body: null (cannot read details)");
                    }

                    MedDevLog.error("OfflineToOnlineSync", "✗ Sync failed: " + errorMsg);
                    MedDevLog.debug("OfflineToOnlineSync", "✓ Invoking callback.onResult(false, errorMsg)...");
                    callback.onResult(false, errorMsg);
                    MedDevLog.debug("OfflineToOnlineSync", "✓ callback.onResult() invoked");
                  }
                }

                @Override
                public void onFailure(
                    retrofit2.Call<com.accessvascularinc.hydroguide.mma.network.OfflineSyncPayload.SyncResponseDTO> call,
                    Throwable t) {
                  MedDevLog.error("OfflineToOnlineSync", "");
                  MedDevLog.error("OfflineToOnlineSync", "❌ NETWORK FAILURE / REQUEST FAILED ❌");
                  MedDevLog.error("OfflineToOnlineSync", "  Exception Type: " + t.getClass().getSimpleName());
                  MedDevLog.error("OfflineToOnlineSync",
                      "  Exception Message: " + (t.getMessage() != null ? t.getMessage() : "null"));
                  ;
                  MedDevLog.error("OfflineToOnlineSync",
                      "  Call URL: " + (call != null && call.request() != null ? call.request().url() : "unknown"));
                  MedDevLog.error("OfflineToOnlineSync", "  Stack trace: ", t);
                  MedDevLog.error("OfflineToOnlineSync", "");
                  MedDevLog.error("OfflineToOnlineSync", "  Database will NOT be cleared");
                  MedDevLog.error("OfflineToOnlineSync", "  User should check network connectivity and retry");
                  MedDevLog.debug("OfflineToOnlineSync", "✓ Invoking callback.onResult(false, errorMessage)...");
                  callback.onResult(false,
                      "Network error: " +
                          (t.getMessage() != null ? t.getMessage() : "Unknown"));
                  MedDevLog.debug("OfflineToOnlineSync", "✓ callback.onResult() invoked");
                }
              });
    } catch (Exception e) {
      MedDevLog.error("OfflineToOnlineSync", "Exception during sync request building: " + e.getMessage(), e);
      callback.onResult(false, "Error building sync request: " + e.getMessage());
    }
  }
}
