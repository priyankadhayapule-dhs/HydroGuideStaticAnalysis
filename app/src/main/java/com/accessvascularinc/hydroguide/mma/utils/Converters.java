package com.accessvascularinc.hydroguide.mma.utils;

import androidx.room.TypeConverter;

import com.accessvascularinc.hydroguide.mma.model.Capture;
import com.accessvascularinc.hydroguide.mma.model.PatientData;
import com.google.gson.ExclusionStrategy;
import com.google.gson.FieldAttributes;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.reflect.TypeToken;

import java.lang.reflect.Modifier;
import java.lang.reflect.Type;

public class Converters {

  /**
   * Converts a long (timestamp) to an ISO 8601 formatted string
   * (yyyy-MM-dd'T'HH:mm:ss.SSS).
   * Returns null if input is null or empty.
   */
  public static String longToIso8601String(String longValue) {
    if (longValue == null || longValue.isEmpty())
      return null;
    try {
      long millis = Long.parseLong(longValue);
      java.text.SimpleDateFormat sdf = new java.text.SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS");
      java.util.Date date = new java.util.Date(millis);
      return sdf.format(date);
    } catch (Exception e) {
      return null;
    }
  }

  /**
   * Cloud expects startTime like: yyyy-MM-dd HH:mm:ss.SS (centiseconds, space
   * separator).
   * This converts epoch millis to that pattern by truncating milliseconds to 2
   * digits.
   */
  public static String millisToCloudCentisecondFormat(Long millis) {
    if (millis == null || millis <= 0)
      return null;
    try {
      java.util.Date date = new java.util.Date(millis);
      java.text.SimpleDateFormat base = new java.text.SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
      String prefix = base.format(date);
      int ms = (int) (millis % 1000); // 0-999
      int centis = ms / 10; // truncate to 2 digits (0-99)
      return prefix + "." + String.format(java.util.Locale.US, "%02d", centis);
    } catch (Exception e) {
      return null;
    }
  }

  /**
   * Converts a long (timestamp) to a yyyy-MM-dd formatted string.
   * Returns null if input is null or empty.
   */
  public static String longToDateString(String longValue) {
    if (longValue == null || longValue.isEmpty())
      return null;
    try {
      long millis = Long.parseLong(longValue);
      java.text.SimpleDateFormat sdf = new java.text.SimpleDateFormat("yyyy-MM-dd");
      java.util.Date date = new java.util.Date(millis);
      return sdf.format(date);
    } catch (Exception e) {
      return null;
    }
  }

  private static final Gson gson = new GsonBuilder().excludeFieldsWithModifiers(Modifier.STATIC, Modifier.TRANSIENT)
      .addSerializationExclusionStrategy(new ExclusionStrategy() {
        @Override
        public boolean shouldSkipClass(Class<?> clazz) {
          // 🔹 Skip AndroidPlot classes and your FadeFormatter hierarchy
          String name = clazz.getName();
          return name.startsWith("com.androidplot.") ||
              name.startsWith("com.accessvascularinc.hydroguide.mma.ui.plot") ||
              name.contains("Formatter") || name.contains("Renderer");
        }

        @Override
        public boolean shouldSkipField(FieldAttributes f) {
          // Optionally skip nested fields named "pointLabeler" directly
          return f.getName().equals("pointLabeler");
        }
      }).serializeNulls().create();

  // ---- PatientData ----
  @TypeConverter
  public static String fromPatientData(PatientData patient) {
    return patient == null ? null : gson.toJson(patient);
  }

  @TypeConverter
  public static PatientData toPatientData(String patientJson) {
    return patientJson == null ? null : gson.fromJson(patientJson, PatientData.class);
  }

  // ---- Capture ----
  @TypeConverter
  public static String fromCapture(Capture capture) {
    return capture == null ? null : gson.toJson(capture);
  }

  @TypeConverter
  public static Capture toCapture(String captureJson) {
    return captureJson == null ? null : gson.fromJson(captureJson, Capture.class);
  }

  // ---- Capture[] ----
  @TypeConverter
  public static String fromCaptureArray(Capture[] captures) {
    return captures == null ? null : gson.toJson(captures);
  }

  @TypeConverter
  public static Capture[] toCaptureArray(String capturesJson) {
    if (capturesJson == null) {
      return null;
    }
    Type type = new TypeToken<Capture[]>() {
    }.getType();
    return gson.fromJson(capturesJson, type);
  }
}
