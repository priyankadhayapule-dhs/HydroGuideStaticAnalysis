package com.accessvascularinc.hydroguide.mma.network.FacilitySyncModel;

import java.util.List;
import java.util.UUID;
import java.util.Date;

public class FacilitySyncRequest {
    public List<FacilityDto> Added;
    public List<FacilityDto> Updated;
    public List<FacilityDto> Deleted;

    public static class FacilityDto {
        public Long FacilityId;
        public String FacilityName;
        public java.util.UUID OrganizationId;
        public java.util.Date DateLastUsed;
        public java.util.UUID CreatedBy;
        public java.util.Date CreatedOn;
        public Boolean IsActive;
        public Boolean IspatientNameRequired;
        public Boolean IsinsertionVeinRequired;
        public Boolean IspatientIDRequired;
        public Boolean IspatientSideRequired;
        public Boolean IspatientDobRequired;
        public Boolean IsarmCircumferenceRequired;
        public Boolean IsveinSizeRequired;
        public Boolean IsreasonForInsertionRequired;
        public Boolean IsCatherSizeRequired;
        public Boolean IsNumberOfLumensRequired;
    }
}