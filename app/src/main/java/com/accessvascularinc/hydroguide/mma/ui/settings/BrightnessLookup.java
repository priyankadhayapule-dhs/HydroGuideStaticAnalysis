package com.accessvascularinc.hydroguide.mma.ui.settings;

import android.os.Build;
import android.util.Log;

public class BrightnessLookup {
  // HLG constants
  private static final float R = 0.5f;
  private static final float A = 0.17883277f;
  private static final float B = 0.28466892f;
  private static final float C = 0.55991073f;

  private static final int GAMMA_SPACE_MIN = 0;
  private static final int GAMMA_SPACE_MAX = 65535;
  private static final float MIN_BRIGHTNESS = 0;
  private static final float MAX_BRIGHTNESS = 255;
  private static final int MIN_BRIGHTNESS_FLOOR_HLG = 1; // Minimum for HLG devices
  private static final int MIN_BRIGHTNESS_FLOOR_LINEAR = 1; // Minimum for linear devices
  
  // Device configuration - set to true for HLG support, false for linear
  // TODO: Configure this based on device model or settings
  private static boolean useHLG = isHLGDevice();

  /**
   * Determines if the current device supports HLG brightness curve.
   * Customize this method based on your device models.
   * 
   * @return true if device supports HLG, false for linear scaling
   */
  private static boolean isHLGDevice() {
    // Example: Check device manufacturer/model
    // Adjust these conditions based on your actual device lineup
    String manufacturer = Build.MANUFACTURER.toLowerCase();
    String model = Build.MODEL.toLowerCase();
    Log.d("isHLGDevice", "isHLGDevice: " + manufacturer);
    // Add your HLG-capable devices here
    // For now, defaulting to linear for Unitech tablets
    if (manufacturer.contains("unitech")) {
      return true; // Unitech uses linear
    }
    
    // Default to HLG for other devices
    return false;
  }

  public static float getBrightnessPercentage(final float value) {
    if (useHLG) {
      return getBrightnessPercentageHLG(value);
    } else {
      return getBrightnessPercentageLinear(value);
    }
  }

  public static int getBrightnessValue(final float val) {
    if (useHLG) {
      return getBrightnessValueHLG(val);
    } else {
      return getBrightnessValueLinear(val);
    }
  }

  // ============= HLG Implementation =============
  
  private static float getBrightnessPercentageHLG(final float value) {
    // Clamp to minimum floor
    final float clampedValue = Math.max(value, MIN_BRIGHTNESS_FLOOR_HLG);
    
    // For some reason, HLG normalizes to the range [0, 12] rather than [0, 1]
    // Use MIN_BRIGHTNESS_FLOOR_HLG as the base to match output range
    final float normalizedVal = norm(MIN_BRIGHTNESS_FLOOR_HLG, MAX_BRIGHTNESS, clampedValue) * 12;
    final float ret;

    if (normalizedVal <= 1.0f) {
      ret = ((float) Math.sqrt(normalizedVal)) * R;
    } else {
      ret = A * ((float) Math.log(normalizedVal - B)) + C;
    }

    final int gammaValue = Math.round(lerp(GAMMA_SPACE_MIN, GAMMA_SPACE_MAX, ret));

    if (gammaValue > GAMMA_SPACE_MAX) {
      return 1.0f;
    }
    if (gammaValue < GAMMA_SPACE_MIN) {
      return 0.0f;
    }

    return (float) (gammaValue - GAMMA_SPACE_MIN) / (GAMMA_SPACE_MAX - GAMMA_SPACE_MIN);
  }

  

  // ============= Linear Implementation =============
  
  private static float getBrightnessPercentageLinear(final float value) {
    // Linear interpolation from MIN_BRIGHTNESS_FLOOR_LINEAR to MAX_BRIGHTNESS
    if (value <= MIN_BRIGHTNESS_FLOOR_LINEAR) {
      return 0.0f;
    }
    if (value >= MAX_BRIGHTNESS) {
      return 1.0f;
    }
    
    return (value - MIN_BRIGHTNESS_FLOOR_LINEAR) / (float) (MAX_BRIGHTNESS - MIN_BRIGHTNESS_FLOOR_LINEAR);
  }

  private static int getBrightnessValueLinear(final float val) {
    // Linear interpolation from MIN_BRIGHTNESS_FLOOR_LINEAR to MAX_BRIGHTNESS
    final float clampedVal = Math.max(0.0f, Math.min(1.0f, val));
    final int brightness = Math.round(MIN_BRIGHTNESS_FLOOR_LINEAR + (clampedVal * (MAX_BRIGHTNESS - MIN_BRIGHTNESS_FLOOR_LINEAR)));
    
    return brightness;
  }

  // ============= Helper Methods =============

  private static int getBrightnessValueHLG(final float val) {
    final float gammaValue = (val * (GAMMA_SPACE_MAX - GAMMA_SPACE_MIN)) + GAMMA_SPACE_MIN;
    final float normalizedVal = norm(GAMMA_SPACE_MIN, GAMMA_SPACE_MAX, gammaValue);
    final float ret;
    if (normalizedVal <= R) {
      ret = sq(normalizedVal / R);
    } else {
      ret = ((float) Math.exp((normalizedVal - C) / A)) + B;
    }

    // HLG is normalized to the range [0, 12], so we need to re-normalize to the range [0, 1]
    // Map to [MIN_BRIGHTNESS_FLOOR_HLG, MAX_BRIGHTNESS] range
    final int brightness = Math.round(lerp(MIN_BRIGHTNESS_FLOOR_HLG, MAX_BRIGHTNESS, ret / 12));
    
    return brightness;
  }

  private static float lerp(final float start, final float stop, final float amount) {
    return start + (stop - start) * amount;
  }

  private static float norm(final float start, final float stop, final float value) {
    return (value - start) / (stop - start);
  }

  private static float sq(final float v) {
    return v * v;
  }
}
