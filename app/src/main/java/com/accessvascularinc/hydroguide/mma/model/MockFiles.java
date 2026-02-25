package com.accessvascularinc.hydroguide.mma.model;

/**
 * Files referencing mock data for live plotting of ECG (electrocardiogram) behavior and the
 * location of each data-stream's respective p-wave peaks
 */
public class MockFiles {

  /**
   * The data for default ECG waves
   */
  public static final String ECG_NORMAL = "ecg_bpm70.dat";

  /**
   * The p-wave peaks located in the normal ECG data
   */
  public static final String PEAKS_NORMAL = "peaks_bpm70.dat";

  /**
   * The ECG data with slightly increased p-wave amplitudes
   */
  public static final String ECG_SLIGHT_INCREASED = "ecg_bpm70_pscale26_pspread_09.dat";

  /**
   * The p-wave peaks located in the ECG data with slightly increased p-wave amplitudes
   */
  public static final String PEAKS_SLIGHT_INCREASED = "peaks_bpm70_pscale26_pspread_09.dat";

  /**
   * The ECG data with greatly increased p-wave amplitudes
   */
  public static final String ECG_GREATLY_INCREASED = "ecg_bpm70_pscale36_pspread_09.dat";

  /**
   * The p-wave peaks located in the ECG data with greatly increased p-wave amplitudes
   */
  public static final String PEAKS_GREATLY_INCREASED = "peaks_bpm70_pscale36_pspread_09.dat";

  /**
   * The ECG data with maximally increased p-wave amplitudes
   */
  public static final String ECG_MAX = "ecg_bpm70_pscale10_pspread_06.dat";

  /**
   * The p-wave peaks located in the ECG data with greatly increased p-wave amplitudes
   */
  public static final String PEAKS_MAX = "peaks_bpm70_pscale10_pspread_06.dat";

  /**
   * The default ECG data with slightly defected p-waves
   */
  public static final String ECG_SLIGHT_DEFLECT =
          "ecg_slight_deflection_bpm70_pscale36_pspread_09.dat";

  /**
   * The default ECG data with fully defected p-waves
   */
  public static final String ECG_DEFLECT = "ecg_deflection_bpm70_pscale36_pspread_09.dat";
  /**
   * The p-wave peaks located in the ECG data with full p-wave deflections
   */
  public static final String PEAKS_DEFLECT = "peaks_deflection_bpm70_pscale36_pspread_09.dat";

}
