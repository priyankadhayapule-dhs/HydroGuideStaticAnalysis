package com.accessvascularinc.hydroguide.mma.utils;

import android.util.Log;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

public class JsonExtractor {
  private static final String TAG = "JsonExtractor";

  public static final String DATE_TIME_FORMAT = "yyyy-MM-dd_HH-mm-ss";
  public static final String DATE_FORMAT_SLASH = "MM/dd/yyyy";
  public static final String DATE_FORMAT_DASH = "MM-dd-yyyy";
  public static final String START_TIME = "EEE MMM dd HH:mm:ss 'GMT'Z yyyy";

  public static String extractJsonObject(String jsonArrayString, String jsonObjectName) {
    try {
      JSONArray jsonArray = new JSONArray(jsonArrayString);
      String firstElement = jsonArray.getString(0); // first element is a JSON string
      JSONObject jsonObject = new JSONObject(firstElement);
      return jsonObject.getString(jsonObjectName);
    } catch (Exception e) {
      MedDevLog.error(TAG, "Error extracting json object", e);
      return "";
    }
  }

  public static String dateTimeStringToLong(String dateString) {
    try {
      SimpleDateFormat sdf = new SimpleDateFormat(DATE_TIME_FORMAT, Locale.getDefault());
      Date date = sdf.parse(dateString);
      return date != null ? String.valueOf(date.getTime()) : "";
    } catch (Exception e) {
      MedDevLog.error(TAG, "Error formatting date", e);
      return "";
    }
  }

  public static String dateStringToLong(String dateString) {
    try {
      Log.d("ABC", "dateStringToLong: " + dateString);
      SimpleDateFormat sdf;
      if (dateString.contains("-")) {
        sdf = new SimpleDateFormat(DATE_FORMAT_DASH, Locale.getDefault());
      } else {
        sdf = new SimpleDateFormat(DATE_FORMAT_SLASH, Locale.getDefault());
      }
      Date date = sdf.parse(dateString);
      Log.d("ABC", "date: " + date);
      return date != null ? String.valueOf(date.getTime()) : "";
    } catch (Exception e) {
      MedDevLog.error(TAG, "Error parsing date", e);
      return "";
    }
  }

  public static String dateStringToMillis(String dateString) {
    try {
      SimpleDateFormat sdf = new SimpleDateFormat(START_TIME, Locale.US);
      Date date = sdf.parse(dateString);
      return date != null ? String.valueOf(date.getTime()) : "0";
    } catch (Exception e) {
      MedDevLog.error(TAG, "Error parsing date", e);
      return "0";
    }
  }
  public static List<String> extractJsonValues(String jsonString) {
    List<String> extractedValues = new ArrayList<>();

    try {
      JSONArray jsonArray = new JSONArray(jsonString);

      for (int i = 0; i < jsonArray.length(); i++) {
        String element = jsonArray.getString(i);

        // --- Case 1: Nested JSON array (even empty ones) ---
        if (element.startsWith("[") && element.endsWith("]")) {
          JSONArray innerArray = new JSONArray(element);
          if (innerArray.length() == 0) {
            // Add empty array as "[]"
            extractedValues.add("[]");
          } else {
            for (int j = 0; j < innerArray.length(); j++) {
              extractedValues.add(innerArray.getString(j));
            }
          }
        }
        // --- Case 2: Nested JSON object ---
        else if (element.startsWith("{") && element.endsWith("}")) {
          JSONObject innerObject = new JSONObject(element);
          extractedValues.add(innerObject.toString());
        }
        // --- Case 3: Normal string value ---
        else {
          extractedValues.add(element);
        }
      }

    } catch (JSONException e) {
      MedDevLog.error(TAG, "Error extracting json value", e);
    }

    return extractedValues;
  }


}
