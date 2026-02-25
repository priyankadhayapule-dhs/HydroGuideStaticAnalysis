package com.accessvascularinc.hydroguide.mma.model;

import android.content.Context;
import android.media.MediaScannerConnection;
import android.util.Log;

import com.accessvascularinc.hydroguide.mma.utils.DbHelper;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import org.jetbrains.annotations.NonNls;
import org.json.JSONArray;
import org.json.JSONException;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Locale;
import java.util.OptionalDouble;

/**
 * Collection of file names used for saving procedure data to the tablet's internal storage. Some
 * helper methods related to reading or writing data files are also here.
 */
public class DataFiles {
  /**
   * This is the name of the directory where profiles with user preferences are stored.
   */
  public static final @NonNls String PROFILES_DIR = "profiles";
  /**
   * This is the name of the directory where data of procedures done are stored.
   */
  public static final @NonNls String PROCEDURES_DIR = "procedures";
  /**
   * This file stores selectable facilities to pick from for procedures.
   */
  public static final String FACILITIES_DATA = "facilitiesData.txt";
  /**
   * This file stores the data recorded in the patient input or report summary screens, as well as
   * any captures taken during the procedure.
   */
  public static final String PROCEDURE_DATA = "procedureData.txt";

  public static final String REPORT_PDF = "ProcedureReport.pdf";
  /**
   * This file is a log of every incoming ECG data packets from the controller.
   */
  public static final String SERIAL_ECG_DATA = "serialEcgData.txt";
  /**
   * This file is a log of every incoming heartbeat data packet from the controller.
   */
  public static final String SERIAL_HEARTBEAT_DATA = "serialHeartbeatData.txt";
  /**
   * This file is a log of every incoming controller status data packet from the controller.
   */
  public static final String SERIAL_CONTROLLER_STATUS_DATA = "serialControllerStatusData.txt";

  /**
   * This file is a log of every incoming button state data packets from the controller.
   */
  public static final String SERIAL_BUTTON_STATE_DATA = "serialButtonStateData.txt";

  /**
   * A helper method to parse an array not in JSON format that is inside a string. Ex: "[0, 1, 2]"
   *
   * @param arrayString the string containing the array.
   *
   * @return an array of strings that can be converted to other data types as needed.
   */
  public static String[] getArrayFromString(final String arrayString) {
    final String str = arrayString.replaceAll("[\\[\\] ]", "");
    if (str.isEmpty()) {
      return new String[0];
    }
    return str.split(",");
  }

  /**
   * A helper method to parse an OptionalDouble from a string.
   *
   * @param doubleString the string containing a double.
   *
   * @return an optional double parsed from the string, which can be empty.
   */
  public static OptionalDouble parseOptionalDouble(final String doubleString) {
    if (doubleString == null || doubleString.isEmpty()) {
      return OptionalDouble.empty();
    }
    try {
      return OptionalDouble.of(Double.parseDouble(doubleString));
    } catch (NumberFormatException e) {
      return OptionalDouble.empty();
    }
  }

  /**
   * A helper method to clean a string of invalid characters to be used as a file name.
   *
   * @param filenameString a string to clean.
   *
   * @return a valid string that can be used as a filename.
   */
  public static String cleanFileNameString(final String filenameString) {
    return filenameString.replaceAll("[\\[\\] ().,]", "");
  }

  public static UserProfile getProfileFromFile(final File profileFile) {
    try {
      final BufferedReader br = new BufferedReader(
              new InputStreamReader(new FileInputStream(profileFile), StandardCharsets.UTF_8));
      final UserProfile userProfile = new UserProfile(br.readLine(), br.readLine(), br.readLine());

      br.close();
      return userProfile;
    } catch (final IOException | JSONException e) {
      throw new RuntimeException(e);
    }
  }

  public static void writeArrayToFile(final File file, final String[] data,
                                      final Context context) throws IOException {
    final FileWriter writer = new FileWriter(file, StandardCharsets.UTF_8);

    for (final String str : data) {
      Log.d("ABC", "writeArrayToFile: " + str);
      writer.write(str);
      Log.d("ABC", "lineSeparator: " + System.lineSeparator());
      writer.write(System.lineSeparator());
      writer.flush();
    }
    writer.close();

    MediaScannerConnection.scanFile(context, new String[] {file.getAbsolutePath()},
            new String[] {}, null);
  }

  public static void writeJsonDataToFile(final File file, final String dataStr,
                                         final Context context) throws IOException {
    final FileWriter writer = new FileWriter(file, StandardCharsets.UTF_8);
    Log.d("ABC", "writeJsonDataToFile: " + dataStr);
    writer.write(dataStr);
    writer.flush();
    writer.close();

    MediaScannerConnection.scanFile(context, new String[] {file.getAbsolutePath()},
            new String[] {}, null);
  }

  public static void writeFacilitiesToFile(final String dataStr, final Context context) {
    final File facilitiesFile = new File(context.getApplicationContext().getExternalFilesDir(null),
            DataFiles.FACILITIES_DATA);
    String alertMsg;

    try {
      if (facilitiesFile.exists() && facilitiesFile.isFile()) {
        writeJsonDataToFile(facilitiesFile, dataStr, context);
        alertMsg = facilitiesFile.getName() + " was updated successfully";
      } else {
        alertMsg = facilitiesFile.getName() + " is missing. Failed to save facility data.";
      }
    } catch (final Exception e) {
      MedDevLog.error("DataFiles", "Error saving facility data", e);
      alertMsg = e.getMessage();
    }
    MedDevLog.info("Facilities File", alertMsg);
  }

  public static EncounterData[] getSavedRecords(final Context context) {
    final File storageDir = context.getExternalFilesDir(DataFiles.PROCEDURES_DIR);
    final String[] dirListOriginal = storageDir.list();
    String[] dirList = java.util.Arrays.stream(dirListOriginal)
            .filter(name -> name != null && !name.equalsIgnoreCase("temp"))
            .toArray(String[]::new);
    // Log.i("Records", Arrays.toString(dirList));

    EncounterData[] savedRecords = new EncounterData[0];
    if (dirList != null) {
      savedRecords = new EncounterData[dirList.length];
      for (int i = 0; i < savedRecords.length; i++) {
        savedRecords[i] = DataFiles.getRecordFromStorage(new File(storageDir, dirList[i]));
      }
    }
    return savedRecords;
  }

  public static EncounterData getRecordFromStorage(final File procedureDir) {
    try {
      final BufferedReader recordBr = new BufferedReader(
              new InputStreamReader(
                      new FileInputStream(new File(procedureDir, DataFiles.PROCEDURE_DATA)),
                      StandardCharsets.UTF_8));

      final EncounterData record = new EncounterData();
      final SimpleDateFormat formatter =
              new SimpleDateFormat("EEE MMM dd HH:mm:ss z yyyy", Locale.US);

      String startTimeLine = recordBr.readLine();
      if (startTimeLine != null) {
        record.setStartTime(formatter.parse(startTimeLine));
      }

      record.setTrimLengthCm(DataFiles.parseOptionalDouble(recordBr.readLine()));
      record.setInsertedCatheterCm(DataFiles.parseOptionalDouble(recordBr.readLine()));
      record.setExternalCatheterCm(DataFiles.parseOptionalDouble(recordBr.readLine()));

      String patientLine = recordBr.readLine();
      if (patientLine != null) {
        record.setPatient(new PatientData(patientLine));
      }

      String hydroGuideLine = recordBr.readLine();
      if (hydroGuideLine != null) {
        record.setHydroGuideConfirmed(Boolean.parseBoolean(hydroGuideLine));
      }

      String captureLine = recordBr.readLine();
      if (captureLine != null) {
        record.setExternalCapture(new Capture(captureLine));
        if (record.getExternalCapture().getCaptureId() == -1) {
          record.setExternalCapture(null);
        }
      }

      record.setDataDirPath(procedureDir.getAbsolutePath());

      String intravCapsLine = recordBr.readLine();
      final JSONArray jsonIntravCaps =
              (intravCapsLine != null) ? new JSONArray(intravCapsLine) : new JSONArray();
      final Capture[] intravCaps = new Capture[jsonIntravCaps.length()];
      for (int i = 0; i < intravCaps.length; i++) {
        intravCaps[i] = new Capture(jsonIntravCaps.getString(i));
      }

      record.setIntravCaptures(intravCaps);

      String recState = recordBr.readLine();
      if (recState == null) {
        recState = "CompletedSuccessful";
      }
      Log.i("Debugging", recState);

      switch (recState) {
        case "CompletedSuccessful" -> {
          record.setState(EncounterState.CompletedSuccessful);
        }
        case "CompletedUnsuccessful" -> {
          record.setState(EncounterState.CompletedUnsuccessful);
        }
        case "ConcludedIncomplete" -> {
          record.setState(EncounterState.ConcludedIncomplete);
        }
        default -> {
          record.setState(EncounterState.Active);
        }
      }

      final String appSwVer = recordBr.readLine();
      if (appSwVer != null) {
        record.setAppSoftwareVersion(appSwVer);
      }
      final String ctrlFwVer = recordBr.readLine();
      if (ctrlFwVer != null) {
        record.setControllerFirmwareVersion(ctrlFwVer);
      }
      
      // Read ultrasound capture paths if available
      final String ultrasoundCapturesLine = recordBr.readLine();
      if (ultrasoundCapturesLine != null && !ultrasoundCapturesLine.isEmpty()) {
        try {
          final JSONArray jsonUltrasoundPaths = new JSONArray(ultrasoundCapturesLine);
          final String[] ultrasoundPaths = new String[jsonUltrasoundPaths.length()];
          for (int i = 0; i < ultrasoundPaths.length; i++) {
            ultrasoundPaths[i] = jsonUltrasoundPaths.getString(i);
          }
          record.setUltrasoundCapturePaths(ultrasoundPaths);
        } catch (JSONException e) {
          MedDevLog.error("DataFiles", "Error reading ultrasound capture paths, using empty array", e);
          record.setUltrasoundCapturePaths(new String[0]);
        }
      }

      recordBr.close();
      return record;
    } catch (final IOException | JSONException | ParseException e) {
      throw new RuntimeException(e);
    }
  }
}
