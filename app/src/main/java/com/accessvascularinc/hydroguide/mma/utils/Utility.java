package com.accessvascularinc.hydroguide.mma.utils;

import android.app.Activity;
import android.app.Dialog;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.text.InputFilter;
import android.text.Spanned;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.ui.ConfirmDialog;
import com.accessvascularinc.hydroguide.mma.ui.input.InputUtils;
import com.google.android.material.datepicker.MaterialDatePicker;
import java.io.File;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Random;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

public class Utility {
  public static final String TAG = "Logs";
  public static final String DB_NAME = "HydroGuide.db";
  public static final String PKG_BUILDS_FOLDER_NAME = "AviBuilds";
  public static final String BACKUP_FILE_PREFIX = "HyG_Backup_";
  public static final String FILE_NAME_STAMP_FORMAT = "yyyyMMdd_HHmmss";

  // Default organization ID used when one is not available from prefs or backend.
  public static final String DEFAULT_ORGANIZATION_ID = "2c0676b3-0b4b-44fe-92fa-56109814d486";
  public static final String PROCEDURE_EXPORTS_FILE_NAME = "ProcedureExports.zip";

  // Navigation screen identifiers
  public static final String FROM_SCREEN_PATIENT_INPUT = "patient_input";
  public static final String FROM_SCREEN_SETUP = "setup";
  public static final String FROM_SCREEN_BLUETOOTH_PAIRING_PATTERN = "bluetooth_pairing_pattern";
  public static final String FROM_SCREEN_CONNECT_ULTRASOUND = "connect_ultrasound";
  public static final String FROM_SCREEN_HOME = "home";
  public static final String PKG_FILE_EXTENSION_APP = ".apk";
  public static final String PKG_FILE_EXTENSION_CTRL = ".zip";
  public static final String HARDWARE_BOARD_NAME = "TTL232R-3V3";
  public static final int MIN_BATTERY_LEVEL_THRESHOLD = 20;
  public static final String KEY_CRASH_TYPE = "crash_type";
  public static final String KEY_CRASH_LOGS = "crash_logs";
  public static final String KEY_CRASH_TRACE = "crash_trace";

  public static void showConfirmMessage(Context context) {
    ConfirmDialog.show(context, context.getResources().getString(R.string.dashboard_exit_message),
        confirmed -> {
          if (confirmed) {
            ((Activity) context).finish();
          }
        });
  }

  public static String fetchDate() {
    SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd");
    Calendar calendar = Calendar.getInstance();
    try {
      calendar.setTime(sdf.parse(sdf.format(new Date())));
    } catch (ParseException e) {
      throw new RuntimeException(e);
    }
    calendar.add(Calendar.DATE, 1);
    return sdf.format(calendar.getTime());
  }

  public static String getOrganizationId() {
    return DEFAULT_ORGANIZATION_ID;
  }

  public static final String DATE_PATTERN_TYPE_1 = "MM/dd/yyyy";
  public static final String DATE_PATTERN_TYPE_2 = "yyyy-MM-dd";

  /**
   * Generates a random 8-character password containing at least one uppercase,
   * one lowercase, one
   * digit, and one special character. Shared by screens that need to
   * auto-generate strong
   * passwords.
   */
  public static String generateRandomPassword() {
    final String UPPERCASE = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    final String LOWERCASE = "abcdefghijklmnopqrstuvwxyz";
    final String DIGITS = "0123456789";
    final String SPECIAL_CHARS = "!@#$%^&*";
    final String ALL_CHARS = UPPERCASE + LOWERCASE + DIGITS + SPECIAL_CHARS;

    StringBuilder password = new StringBuilder();
    Random random = new Random();

    // Ensure at least one character from each category
    password.append(UPPERCASE.charAt(random.nextInt(UPPERCASE.length())));
    password.append(LOWERCASE.charAt(random.nextInt(LOWERCASE.length())));
    password.append(DIGITS.charAt(random.nextInt(DIGITS.length())));
    password.append(SPECIAL_CHARS.charAt(random.nextInt(SPECIAL_CHARS.length())));

    // Fill remaining characters randomly from all characters
    for (int i = 4; i < 8; i++) {
      password.append(ALL_CHARS.charAt(random.nextInt(ALL_CHARS.length())));
    }

    // Shuffle characters for better randomness
    char[] chars = password.toString().toCharArray();
    for (int i = chars.length - 1; i > 0; i--) {
      int randomIndex = random.nextInt(i + 1);
      char temp = chars[i];
      chars[i] = chars[randomIndex];
      chars[randomIndex] = temp;
    }

    return new String(chars);
  }

  public static boolean isUsbStoragePlugged(Context context) {
    StorageManager storageManager = (StorageManager) context.getSystemService(Context.STORAGE_SERVICE);
    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N) {
      List<StorageVolume> volumes = storageManager.getStorageVolumes();
      for (StorageVolume volume : volumes) {
        if (volume.isRemovable() && volume.getState().equals(Environment.MEDIA_MOUNTED) && !volume.getDirectory().toPath().getName(0).toString().equals("storage"))
        {
            return true;
        }
      }
    }
    return false;
  }

  public static List<File> getLocalPackageBuilds(Context context) {
    List<File> lstBuildFiles = new ArrayList<>();
    File filBuildsFolder = new File(context.getExternalFilesDir(null), Utility.PKG_BUILDS_FOLDER_NAME);
    if (filBuildsFolder.exists()) {
      List<File> supportedExtensionFiles = Arrays.asList(filBuildsFolder.listFiles());
      lstBuildFiles.addAll(supportedExtensionFiles.stream().filter(name -> name.getName().endsWith(PKG_FILE_EXTENSION_APP) || name.getName().endsWith(PKG_FILE_EXTENSION_CTRL)).collect(Collectors.toList()));
    }
    return lstBuildFiles;
  }

  public static boolean deletePackageBuilds(Context context) {
    File filBuildsFolder = new File(context.getExternalFilesDir(null), Utility.PKG_BUILDS_FOLDER_NAME);
    if (filBuildsFolder.exists()) {
      if (filBuildsFolder.listFiles().length > 0) {
        File toBeDeletedFiles[] = filBuildsFolder.listFiles();
        for (File fil : toBeDeletedFiles) {
          fil.delete();
        }
      }
      return filBuildsFolder.delete();
    }
    return false;
  }

  /**
   * Validates username according to specific rules:
   * - Not null or empty
   * - Max 10 characters
   * - No spaces allowed
   * - Only alphanumeric and underscore characters (a-z, A-Z, 0-9, _)
   *
   * @param username The username to validate
   * @return ValidationResult containing isValid flag and error message
   */
  public static ValidationResult validateUsername(String username) {
    if (username == null || username.trim().isEmpty()) {
      return new ValidationResult(false, "USERNAME_IS_REQUIRED");
    }

    if (username.length() > 10) {
      return new ValidationResult(false, "USERNAME_EXCEEDS_MAX_LENGTH");
    }

    if (username.contains(" ")) {
      return new ValidationResult(false, "USERNAME_CONTAINS_SPACES");
    }

    // Allow only alphanumeric and underscore: a-z, A-Z, 0-9, _
    if (!Pattern.matches("^[a-zA-Z0-9_]+$", username)) {
      return new ValidationResult(false, "USERNAME_INVALID_FORMAT");
    }

    return new ValidationResult(true, "");
  }

  /**
   * Helper class to return validation results with error messages
   */
  public static class ValidationResult {
    private final boolean isValid;
    private final String errorMessage;

    public ValidationResult(boolean isValid, String errorMessage) {
      this.isValid = isValid;
      this.errorMessage = errorMessage;
    }

    public boolean isValid() {
      return isValid;
    }

    public String getErrorMessage() {
      return errorMessage;
    }
  }

  public static MaterialDatePicker<Long> createDatePicker(MaterialDatePicker<Long> datePicker,String title,String selectionValue)
  {
    if(datePicker != null && datePicker.getDialog() != null && !datePicker.getDialog().isShowing())
    {
      return datePicker;
    }
    else if(datePicker != null && datePicker.getDialog() != null && datePicker.getDialog().isShowing())
    {
      datePicker.dismiss();
    }
    else
    {
      datePicker = MaterialDatePicker.Builder.datePicker()
              .setTitleText(title)
              .setSelection(!selectionValue.isEmpty() ? InputUtils.parseDateToMillis(selectionValue) : null)
              .setInputMode(MaterialDatePicker.INPUT_MODE_TEXT)
              .build();
    }
    return datePicker;
  }

  public static InputFilter allowOnlyAlphanumeric()
  {
    return new InputFilter()
    {
      private final Pattern pattern = Pattern.compile("^[a-zA-Z0-9]+$");
      @Override
      public CharSequence filter(CharSequence source, int start, int end, Spanned dest, int dstart, int dend)
      {
        if (source.length() > 0 && !pattern.matcher(source).matches())
        {
          return "";
        }
        return null;
      }
    };
  }

  public static Date getDate(String dateProvided,String patternType)
  {
    SimpleDateFormat inputFormat = new SimpleDateFormat(patternType);
    try
    {
      return inputFormat.parse(dateProvided.trim());
    }
    catch (ParseException e)
    {
      e.printStackTrace();
    }
    return null;
  }

  public static String formatDate(String createdOn) {

    try {
      if (createdOn == null) {
        return "";
      }
      if(createdOn.contains("/") || createdOn.contains("-"))
      {
        SimpleDateFormat datePatternOne = new SimpleDateFormat(Utility.DATE_PATTERN_TYPE_1);
        SimpleDateFormat datePatternTwo = new SimpleDateFormat(Utility.DATE_PATTERN_TYPE_2);
        return datePatternTwo.format(datePatternOne.parse(createdOn));
      }
      else if(createdOn.matches("\\d+"))
      {
          return new SimpleDateFormat(Utility.DATE_PATTERN_TYPE_2).format(new Date(Long.parseLong(createdOn)));
      }
      final SimpleDateFormat dateFormat = new SimpleDateFormat(InputUtils.SHORT_DATE_PATTERN, Locale.US);
      return dateFormat.format(new Date(Long.parseLong(createdOn)));
    } catch (Exception e) {
      return "";
    }
  }

  public static String[] splitDate(String textProvided,String splitChar)
  {
    if(textProvided.contains("|"))
    {
      return textProvided.split("\\|");
    }
    return null;
  }

  public static void debugUsbDevices(String TAG,Context context)
  {
    final String ACTION_USB_PERMISSION = "com.accessvascularinc.hydroguide.USB_PERMISSION";
    UsbManager usbManager = (UsbManager)context.getSystemService(Context.USB_SERVICE);
    if (usbManager == null)
    {
      MedDevLog.info(TAG,"UsbManager is null - USB Host not supported?");
      return;
    }
    HashMap<String, UsbDevice> deviceList = usbManager.getDeviceList();
    MedDevLog.info(TAG,"=== Android USB Devices Found: " + deviceList.size() + " ===");
    if (deviceList.isEmpty())
    {
      MedDevLog.info(TAG,"No USB devices detected by Android!");
      return;
    }
    for (var entry : deviceList.entrySet())
    {
      UsbDevice device = entry.getValue();
      MedDevLog.info(TAG,"Device: " + device.getDeviceName());
      MedDevLog.info(TAG,"  VID: 0x" + Integer.toHexString(device.getVendorId()) + " (" + device.getVendorId() + ")");
      MedDevLog.info(TAG,"  PID: 0x" + Integer.toHexString(device.getProductId()) + " (" + device.getProductId() + ")");
      MedDevLog.info(TAG,"  Manufacturer: " + device.getManufacturerName());
      MedDevLog.info(TAG,"  Product: " + device.getProductName());
      MedDevLog.info(TAG,"  Has Permission: " + usbManager.hasPermission(device));
      if (!usbManager.hasPermission(device))
      {
        MedDevLog.info(TAG,"  Requesting USB permission...");
        int flags = Build.VERSION.SDK_INT >= Build.VERSION_CODES.S ? PendingIntent.FLAG_IMMUTABLE : 0;
        PendingIntent permissionIntent = PendingIntent.getBroadcast(context, 0, new Intent(ACTION_USB_PERMISSION), flags);
        usbManager.requestPermission(device, permissionIntent);
      }
    }
  }

  public static String getRemovableVolumeState(Context context)
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
      StorageManager sm = (StorageManager)context.getSystemService(Context.STORAGE_SERVICE);
      for (StorageVolume v : sm.getStorageVolumes()) {
        if (v.isRemovable()) return v.getState();
      }
    }
    return Environment.getExternalStorageState();
  }

  /**
   * Validates if a PIN is in the correct format (4-6 digits)
   */
  public static boolean isValidPin(String pin) {
    if (pin == null || pin.isEmpty()) {
      return false;
    }
    return pin.matches("^\\d{4,6}$");
  }

  public static String generateErrorId()
  {
    Random random = new Random();
    long randomNumber = random.nextLong() % 10000000000L;
    if (randomNumber < 0)
    {
      randomNumber *= -1;
    }
    return String.format("%010d", randomNumber);
  }
}
