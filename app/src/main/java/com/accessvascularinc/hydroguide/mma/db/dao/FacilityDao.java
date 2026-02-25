package com.accessvascularinc.hydroguide.mma.db.dao;


import androidx.room.Dao;
import androidx.room.Delete;
import androidx.room.Insert;
import androidx.room.OnConflictStrategy;
import androidx.room.Query;
import androidx.room.Update;

import com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity;

import java.util.List;

@Dao
public interface FacilityDao {

  @Insert(onConflict = OnConflictStrategy.REPLACE)
  long insertFacility(FacilityEntity facility);

  @Update
  void updateFacility(FacilityEntity facility);

  @Delete
  void deleteFacility(FacilityEntity facility);

  @Query("SELECT * FROM facilities WHERE facilityId = :id")
  FacilityEntity getFacilityById(long id);

  @Query("SELECT * FROM facilities WHERE organization_id = :orgId")
  List<FacilityEntity> getFacilitiesByOrganization(String orgId);

  // Include facilities where action_type is NULL or not 'Deleted'
  @Query("SELECT * FROM facilities WHERE action_type IS NULL OR action_type != 'Deleted'")
  List<FacilityEntity> getAllFacilities();
}
