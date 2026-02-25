package com.accessvascularinc.hydroguide.mma.network.OfflineSyncPayload;

import com.google.gson.annotations.SerializedName;
import java.util.List;

/**
 * Offline Sync Request DTO
 * Matches backend OfflineSyncRequestDTO structure
 * Contains facilities and procedures to sync to cloud
 */
public class OfflineSyncRequestDTO {
    @SerializedName("Facilities")
    public List<OfflineSyncFacilityDTO> Facilities;

    @SerializedName("Procedures")
    public List<OfflineSyncProcedureDTO> Procedures;

    public OfflineSyncRequestDTO() {
    }

    public OfflineSyncRequestDTO(List<OfflineSyncFacilityDTO> facilities, List<OfflineSyncProcedureDTO> procedures) {
        this.Facilities = facilities;
        this.Procedures = procedures;
    }

    public List<OfflineSyncFacilityDTO> getFacilities() {
        return Facilities;
    }

    public void setFacilities(List<OfflineSyncFacilityDTO> facilities) {
        Facilities = facilities;
    }

    public List<OfflineSyncProcedureDTO> getProcedures() {
        return Procedures;
    }

    public void setProcedures(List<OfflineSyncProcedureDTO> procedures) {
        Procedures = procedures;
    }
}
