package com.accessvascularinc.hydroguide.mma.db.dao;

import androidx.room.Dao;
import androidx.room.Delete;
import androidx.room.Insert;
import androidx.room.OnConflictStrategy;
import androidx.room.Query;
import androidx.room.Update;

import com.accessvascularinc.hydroguide.mma.db.entities.ProcedureCaptureEntity;

import java.util.List;

@Dao
public interface ProcedureCaptureDao {

  @Insert(onConflict = OnConflictStrategy.REPLACE)
  long insertCapture(ProcedureCaptureEntity capture);

  @Update
  void updateCapture(ProcedureCaptureEntity capture);

  @Delete
  void deleteCapture(ProcedureCaptureEntity capture);

  @Query("SELECT * FROM procedure_captures WHERE capture_local_id = :id")
  ProcedureCaptureEntity getCaptureById(long id);

  @Query("SELECT * FROM procedure_captures WHERE procedure_id = :procedureId")
  List<ProcedureCaptureEntity> getCapturesByProcedure(long procedureId);

  @Query("SELECT * FROM procedure_captures")
  List<ProcedureCaptureEntity> getAllCaptures();
}
