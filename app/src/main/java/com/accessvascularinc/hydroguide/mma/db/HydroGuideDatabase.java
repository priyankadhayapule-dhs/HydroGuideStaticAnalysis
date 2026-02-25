package com.accessvascularinc.hydroguide.mma.db;

import android.content.Context;

import androidx.room.Database;
import androidx.room.Room;
import androidx.room.RoomDatabase;
import androidx.room.TypeConverters;
import androidx.room.migration.Migration;
import androidx.sqlite.db.SupportSQLiteDatabase;

import com.accessvascularinc.hydroguide.mma.db.dao.EncounterDao;
import com.accessvascularinc.hydroguide.mma.db.dao.FacilityDao;
import com.accessvascularinc.hydroguide.mma.db.dao.ProcedureCaptureDao;
import com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity;
import com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity;
import com.accessvascularinc.hydroguide.mma.db.entities.ProcedureCaptureEntity;
import com.accessvascularinc.hydroguide.mma.db.entities.UsersEntity;
import com.accessvascularinc.hydroguide.mma.db.dao.UsersDao;
import com.accessvascularinc.hydroguide.mma.utils.Converters;

/**
 * Database class with all the tables in Room database.
 */
@Database(entities = { FacilityEntity.class,
    ProcedureCaptureEntity.class,
    EncounterEntity.class, UsersEntity.class }, version = 2, exportSchema = false)
@TypeConverters({ Converters.class })
public abstract class HydroGuideDatabase extends RoomDatabase {

  private static final String DB_NAME = "HydroGuide.db";

  public static final Migration MIGRATION_1_2 = new Migration(1, 2) {
    @Override
    public void migrate(SupportSQLiteDatabase database) {
      database.execSQL("ALTER TABLE users ADD COLUMN createOn TEXT");
    }
  };

  /**
   * Creates a new HydroGuideDatabase instance with all migrations applied.
   * Use this instead of calling Room.databaseBuilder directly to ensure
   * all migrations are always included.
   *
   * @param context Application or activity context
   * @return A fully configured HydroGuideDatabase instance
   */
  public static HydroGuideDatabase buildDatabase(Context context) {
    return Room.databaseBuilder(context, HydroGuideDatabase.class, DB_NAME)
        .addMigrations(MIGRATION_1_2)
        .build();
  }
  /**
   * @return Users details
   */
  public abstract UsersDao usersDao();

  /**
   * @return Facilities details
   */

  public abstract FacilityDao facilityDao();

  /**
   * @return Procedure details
   */

  /**
   * @return Procedure captured details
   */

  public abstract ProcedureCaptureDao procedureCaptureDao();

  public abstract EncounterDao encounterDao();
}
