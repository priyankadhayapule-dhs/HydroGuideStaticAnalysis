package com.accessvascularinc.hydroguide.mma.repository;

import androidx.annotation.NonNull;

import com.accessvascularinc.hydroguide.mma.network.ApiService;
import com.accessvascularinc.hydroguide.mma.network.ProcedureSyncModel.ProcedureSyncRequest;
import com.accessvascularinc.hydroguide.mma.network.ProcedureSyncModel.ProcedureSyncResponse;
import okhttp3.MultipartBody;
import okhttp3.RequestBody;

import retrofit2.Call;

public class ProcedureSyncRepository {
  private final ApiService apiService;

  public ProcedureSyncRepository(@NonNull ApiService apiService) {
    this.apiService = apiService;
  }

  public Call<ProcedureSyncResponse> syncProcedure(String organizationId, ProcedureSyncRequest request) {
    return apiService.syncProcedure(organizationId, request);
  }

  // Build multipart/form-data parts from a ProcedureSyncRequest and optional file
  // map (index -> File)
  // Does not modify existing JSON sync behavior.
  public Call<ProcedureSyncResponse> syncProcedureMultipart(String organizationId,
      ProcedureSyncRequest request,
      java.util.Map<Integer, java.io.File> fileMap) {
    java.util.List<MultipartBody.Part> parts = new java.util.ArrayList<>();
    if (request != null && request.getAdded() != null) {
      java.util.List<ProcedureSyncRequest.ProcedureData> list = request.getAdded();
      for (int i = 0; i < list.size(); i++) {
        ProcedureSyncRequest.ProcedureData pd = list.get(i);
        String base = "Added[" + i + "]";
        // helper
        java.util.function.BiConsumer<String, String> add = (n, v) -> parts
            .add(MultipartBody.Part.createFormData(n, v == null ? "" : v));

        add.accept(base + ".StartTime", pd.startTime);
        add.accept(base + ".TrimLengthCm", String.valueOf(pd.trimLengthCm));
        add.accept(base + ".InsertedCatheterCm", String.valueOf(pd.insertedCatheterCm));
        add.accept(base + ".ExternalCatheterCm", String.valueOf(pd.externalCatheterCm));
        add.accept(base + ".HydroGuideConfirmed", String.valueOf(pd.hydroGuideConfirmed));
        add.accept(base + ".State", pd.state);
        add.accept(base + ".AppSoftwareVersion", pd.appSoftwareVersion);
        add.accept(base + ".ControllerFirmwareVersion", pd.controllerFirmwareVersion);
        add.accept(base + ".OrganizationId", pd.organizationId);
        add.accept(base + ".FacilityOrLocation", pd.facilityOrLocation);
        add.accept(base + ".ClinicianId", pd.clinicianId);
        add.accept(base + ".ArmCircumference", pd.armCircumference != null ? String.valueOf(pd.armCircumference) : "");
        add.accept(base + ".VeinSize", pd.veinSize != null ? String.valueOf(pd.veinSize) : "");
        add.accept(base + ".InsertionVein", pd.insertionVein);
        add.accept(base + ".PatientSide", pd.patientSide);
        add.accept(base + ".Nooflumens", pd.Nooflumens != null ? String.valueOf(pd.Nooflumens) : "");
        add.accept(base + ".Cathetersize", pd.Cathetersize != null ? String.valueOf(pd.Cathetersize) : "");
        add.accept(base + ".IsUltrasoundUsed", pd.isUltrasoundUsed != null ? String.valueOf(pd.isUltrasoundUsed) : "false");
        add.accept(base + ".IsUltrasoundCaptureSaved", pd.isUltrasoundCaptureSaved != null ? String.valueOf(pd.isUltrasoundCaptureSaved) : "false");

        if (pd.patient != null) {
          add.accept(base + ".Patient.OrganizationId", pd.patient.organizationId);
          add.accept(base + ".Patient.PatientName", pd.patient.patientName);
          add.accept(base + ".Patient.PatientMrn", pd.patient.patientMrn);
          add.accept(base + ".Patient.PatientDob", pd.patient.patientDob);
        }

        if (pd.procedureCaptures != null) {
          for (int j = 0; j < pd.procedureCaptures.size(); j++) {
            ProcedureSyncRequest.ProcedureCapture cap = pd.procedureCaptures.get(j);
            String cbase = base + ".ProcedureCaptures[" + j + "]";
            add.accept(cbase + ".CaptureId", String.valueOf(cap.captureId));
            add.accept(cbase + ".IsIntravascular", String.valueOf(cap.isIntravascular));
            add.accept(cbase + ".ShownInReport", String.valueOf(cap.shownInReport));
            add.accept(cbase + ".InsertedLengthCm", String.valueOf(cap.insertedLengthCm));
            add.accept(cbase + ".ExposedLengthCm", String.valueOf(cap.exposedLengthCm));
            add.accept(cbase + ".CaptureData", cap.captureData);
            add.accept(cbase + ".MarkedPoints", cap.markedPoints);
            add.accept(cbase + ".UpperBound", String.valueOf(cap.upperBound));
            add.accept(cbase + ".LowerBound", String.valueOf(cap.lowerBound));
            add.accept(cbase + ".Increment", String.valueOf(cap.increment));
          }
        }

        java.io.File f = fileMap != null ? fileMap.get(i) : null;
        if (f != null && f.exists() && f.isFile()) {
          RequestBody fb = RequestBody.create(f, okhttp3.MediaType.parse("application/octet-stream"));
          parts.add(MultipartBody.Part.createFormData(base + ".File", f.getName(), fb));
        }
      }
    }
    return apiService.syncProcedureMultipart(organizationId, parts);
  }
}
