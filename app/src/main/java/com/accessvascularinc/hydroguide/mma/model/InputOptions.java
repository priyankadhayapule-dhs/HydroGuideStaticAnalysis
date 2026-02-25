package com.accessvascularinc.hydroguide.mma.model;

/**
 * Collection of lists used by UI elements that offer various options for the user to choose from.
 */
public class InputOptions {
  /**
   * A mock list of fake hospital names for testing the dropdown input for facility selection.
   */
  public static final String[] MOCK_FACILITIES = {
          "Some Hospital", "That Other Hospital",
          "The Second Hospital", "The Third Hospital",
          "The Fourth Hospital", "The Fifth Hospital",
          "The Sixth Hospital", "The Seventh Hospital",
          "The Eighth Hospital", "The Ninth Hospital",
          "The Tenth Hospital", "The Eleventy Hospital",
          "The Dozen Hospital", "The Unlucky Hospital",
          "The Fourth Part 2 Hospital", "The Second Hospital Again",
          "The 6 Hospital", "The Lucky Hospital",
          "The Infinity Hospital", "The Hospital but 19",
  };

  /**
   * A list of possible veins to use for the procedure.
   */
  public static final String[] VEIN_OPTIONS = {
          "Basilic",
          "Brachial",
          "Cephalic",
          "Other"
  };

  //"Left Basilic", "Right Basilic",
  //          "Left Brachial", "Right Brachial",
  //          "Left Cephalic", "Right Cephalic",
  //          "Other"

  public static final String[] SIDE_OPTIONS = {
          "Left", "Right"
  };

  /**
   * A list of possible genders to use for the procedure.
   */
  public static final String[] GENDER_OPTIONS = {
          "Undisclosed", "Male", "Female"
  };
}
