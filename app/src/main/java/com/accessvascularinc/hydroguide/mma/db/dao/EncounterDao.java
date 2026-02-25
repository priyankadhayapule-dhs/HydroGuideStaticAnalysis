package com.accessvascularinc.hydroguide.mma.db.dao;

import androidx.room.Dao;

import androidx.room.Delete;
import androidx.room.Insert;
import androidx.room.OnConflictStrategy;
import androidx.room.Query;

import com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity;

import java.util.List;
import androidx.room.Update;

@Dao
public interface EncounterDao {

  @Insert(onConflict = OnConflictStrategy.REPLACE)
  long insertEncounter(EncounterEntity encounter);

  @Query("SELECT * FROM encounters WHERE id = :id")
  EncounterEntity getEncounterById(int id);

  @Query("SELECT * FROM encounters")
  List<EncounterEntity> getAllEncounters();

  @Delete
  void deleteEncounter(EncounterEntity encounter);

  @Update
  void updateEncounter(EncounterEntity encounter);

  @Query("SELECT * FROM encounters WHERE dataDirPath = :datDir")
  List<EncounterEntity> getEncounterByDirPath(String datDir);

  @Query("SELECT COUNT(*) FROM encounters WHERE sync_status = 'Pending'")
  int getPendingEncounterCount();
}
