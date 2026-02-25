package com.accessvascularinc.hydroguide.mma.utils;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.os.Build;
import android.os.Process;
import android.util.Log;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.time.Instant;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

/**
 * MedDevLog
 * -
 * PURPOSE:
 * Centralized logging utility for IEC 62304 compliant medical device software.
 * -
 * REGULATORY INTENT:
 * - Provides diagnostic and official audit logging
 * - Supports FDA-expected audit trail requirements
 * - Separates runtime event logging from static environment metadata
 * -
 * IMPORTANT:
 * - Audit logs are append-only
 * - Logging failures must never affect clinical operation
 * - Do NOT log PHI/PII unless explicitly approved by risk management
 */
public final class MedDevLog {

  private static final String TAG = "MedDevLog";

  private static final String APP_LOG_FILE = "app.log";
  private static final String AUDIT_LOG_FILE = "audit.log";

  private static final long MAX_LOG_SIZE_BYTES = 5 * 1024 * 1024; // 5 MB

  private static File appLogFile;
  private static File auditLogFile;

  private static String appVersion = "UNKNOWN";

  private static final Object FILE_LOCK = new Object();

  private static final SimpleDateFormat TIMESTAMP_FORMAT;
  private static String loggedInUserId = "NA";

  static {
    TIMESTAMP_FORMAT = new SimpleDateFormat(
            "yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", Locale.US
    );
    TIMESTAMP_FORMAT.setTimeZone(TimeZone.getTimeZone("UTC"));
  }

  private MedDevLog() {
    // Prevent instantiation
  }

  /**
   * Initializes the logging subsystem.
   * -
   * MUST be called once during application startup.
   * Writes a startup header entry to both application and audit logs.
   * -
   * @param context Application context for file storage access.
   */
  public static void init(final Context context) {
    try {
      final File logDir = new File(context.getExternalFilesDir(null), "logs");
      if (!logDir.exists()) {
        //noinspection ResultOfMethodCallIgnored
        if(logDir.mkdirs()) {
          Log.d(TAG, "Log directory created at: " + logDir.getAbsolutePath());
        }
        else {
          Log.e(TAG, "Log directory NOT created at: " + logDir.getAbsolutePath());
        }
      }

      appLogFile = new File(logDir, APP_LOG_FILE);
      auditLogFile = new File(logDir, AUDIT_LOG_FILE);

      final PackageInfo info = context.getPackageManager()
              .getPackageInfo(context.getPackageName(), 0);
      appVersion = info.versionName;

      writeStartupHeader();

    } catch (final Exception e) {
      Log.e(TAG, "Logger initialization failed", e);
    }
  }

    /* ============================================================
       Diagnostic / Application Logging
       ============================================================ */

  /**
   * @param tag Category for log message
   * @param message Log message content
   */
  public static void debug(final String tag, final String message) {
    Log.d(tag, message);
    log(Log.DEBUG, tag, message, null, false);
  }

  /**
   * @param tag Category for log message
   * @param message Log message content
   */
  public static void info(final String tag, final String message) {
    Log.i(tag, message);
    log(Log.INFO, tag, message, null, false);
  }

  /**
   * @param tag Category for log message
   * @param message Log message content
   */
  public static void warn(final String tag, final String message) {
    Log.w(tag, message);
    log(Log.WARN, tag, message, null, false);
  }

  /**
   * @param tag Category for log message
   * @param message Log message content
   */
  public static void error(final String tag, final String message) {
    Log.e(tag, message);
    log(Log.ERROR, tag, message, null, false);
  }

  /**
   * @param tag Category for log message
   * @param message Log message content
   * @param t Throwable associated with the warning
   */
  public static void error(final String tag, final String message, final Throwable t) {
    Log.e(tag, message, t);
    log(Log.ERROR, tag, message, t, false);
  }

    /* ============================================================
       Official Audit Logging
       ============================================================ */

  /**
   * Writes an official audit trail entry.
   * -
   * USE FOR:
   * - User authentication events
   * - Authorization failures
   * - Configuration changes
   * - Clinical workflow milestones
   * - Software updates
   * -
   * @param tag Category for log message
   * @param message Log message content
   */
  public static void audit(final String tag, final String message) {
    log(Log.INFO, tag, message, null, true);
    // Also, write audit messages to general log file
    log(Log.INFO, tag, message, null, false);
  }

    /* ============================================================
       Internal implementation
       ============================================================ */

  private static void log(
          final int level,
          final String tag,
          final String message,
          final Throwable throwable,
          final boolean audit
  ) {
    try {
      // Android Logcat (developer visibility)
      if (throwable != null) {
        Log.println(level, tag,
                message + "\n" + Log.getStackTraceString(throwable));
      } else {
        Log.println(level, tag, message);
      }

      // File-based logging (regulatory evidence)
      writeRuntimeEntry(level, tag, message, throwable, audit);

    } catch (final Exception e) {
      Log.e(TAG, "Logging subsystem failure", e);
    }
  }

  /**
   * Writes static environment metadata once at application startup.
   * -
   * This entry provides traceability without repeating static fields
   * on every log message.
   */
  private static void writeStartupHeader() {
    final String timestamp = TIMESTAMP_FORMAT.format(Date.from(Instant.now()));

    final String header =
            timestamp + " | STARTUP | " +
                    "APP_VER=" + appVersion + " | " +
                    "DEVICE=" + Build.MODEL + " | " +
                    "OS=Android " + Build.VERSION.RELEASE +
                    " (SDK " + Build.VERSION.SDK_INT + ")";

    synchronized (FILE_LOCK) {
      writeRawLine(appLogFile, header);
      writeRawLine(auditLogFile, header);
    }

    Log.i(TAG, "Startup header written");
  }

  private static void writeRuntimeEntry(
          final int level,
          final String tag,
          final String message,
          final Throwable throwable,
          final boolean audit
  ) {
    if (appLogFile == null || auditLogFile == null) {
      return;
    }

    synchronized (FILE_LOCK) {
      final File target = audit ? auditLogFile : appLogFile;

      rotateIfNeeded(target);

      try (final BufferedWriter writer = new BufferedWriter(
              new FileWriter(target, true))) {

        writer.write(formatRuntimeEntry(
                level, tag, message, throwable, audit));
        writer.newLine();

      } catch (final IOException e) {
        // Never impact clinical function
        Log.e(TAG, "writeRuntimeEntry failed", e);
      }
    }
  }

  private static void writeRawLine(final File file, final String line) {
    try (final BufferedWriter writer = new BufferedWriter(
            new FileWriter(file, true))) {

      writer.write(line);
      writer.newLine();

    } catch (final IOException e) {
      // Defensive: logging must never crash the app
      Log.e(TAG, "writeRawLine failed", e);
    }
  }

  private static void rotateIfNeeded(final File file) {
    if (file.exists() && file.length() > MAX_LOG_SIZE_BYTES) {
      // Delete file and start again when it exceeds size threshold
      file.delete();

      // In the future, we can rotate log files and keep them, once we have
      // a strategy for managing storage/removal over time.
      //final File rotated = new File(
      //        file.getParent(),
      //        file.getName() + "." + System.currentTimeMillis()
      //);
      //noinspection ResultOfMethodCallIgnored
      //file.renameTo(rotated);
    }
  }

  private static String formatRuntimeEntry(
          final int level,
          final String tag,
          final String message,
          final Throwable throwable,
          final boolean audit
  ) {
    final String timestamp = TIMESTAMP_FORMAT.format(new Date());

    final StringBuilder sb = new StringBuilder();
    sb.append(timestamp).append(" | ");
    sb.append(levelToString(level)).append(" | ");
    sb.append("PID=").append(Process.myPid()).append(" | ");
    sb.append("TID=").append(Thread.currentThread().getId()).append(" | ");
    if(audit) {
      sb.append("UID=").append(loggedInUserId).append(" | ");
    }
    sb.append(tag).append(" | ");
    sb.append(message);

    if (throwable != null) {
      sb.append(" | EX=").append(Log.getStackTraceString(throwable));
    }

    return sb.toString();
  }

  private static String levelToString(final int level) {
    return switch (level) {
      case Log.DEBUG -> "DEBUG";
      case Log.INFO -> "INFO";
      case Log.WARN -> "WARN";
      case Log.ERROR -> "ERROR";
      default -> "UNKNOWN";
    };
  }

  public static void setLoggedInUserId(String userId) {
    loggedInUserId = userId;
  }
}
