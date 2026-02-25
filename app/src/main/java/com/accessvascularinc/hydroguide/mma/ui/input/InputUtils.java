package com.accessvascularinc.hydroguide.mma.ui.input;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.graphics.Rect;
import android.text.InputFilter;
import android.text.Spanned;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.WindowManager;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.InputMethodManager;
import android.widget.ArrayAdapter;
import android.widget.Filter;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.db.entities.PatientFacilityInfo;
import com.accessvascularinc.hydroguide.mma.model.Facility;
import com.accessvascularinc.hydroguide.mma.model.SortMode;
import com.accessvascularinc.hydroguide.mma.serial.Button;
import com.accessvascularinc.hydroguide.mma.serial.ButtonState;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Date;
import java.util.Locale;
import java.util.Objects;
import java.util.TimeZone;

public class InputUtils {
  private static final int KEYBOARD_HEIGHT_PX_THRESHOLD = 200;
  public static final String SHORT_DATE_PATTERN = "MM/dd/yyyy";
  public static final String FILE_DATE_TIME_PATTERN = "yyyy-MM-dd_HH-mm-ss";

  public static long parseDateToMillis(final String dateStr) {
    final SimpleDateFormat dateFormat = new SimpleDateFormat(SHORT_DATE_PATTERN, Locale.US);
    dateFormat.setTimeZone(TimeZone.getTimeZone("UTC"));
    try {
      final Date date = dateFormat.parse(dateStr);
      if (date == null) {
        return 0;
      }
      return date.getTime();
    } catch (final ParseException e) {
      MedDevLog.error("InputUtils", "Error parsing date", e);
      return 0;
    }
  }

  public static String getDateStringFromMillis(final long dateMillis) {
    final SimpleDateFormat dateFormat = new SimpleDateFormat(SHORT_DATE_PATTERN, Locale.US);
    dateFormat.setTimeZone(TimeZone.getTimeZone("UTC"));
    return dateFormat.format(dateMillis);
  }

  public static void hideKeyboard(final Activity activity) {
    // Check if no view has focus:
    try {
      final View view = activity.getCurrentFocus();
      if (view != null) {
        final InputMethodManager inputManager =
                (InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE);
        inputManager.hideSoftInputFromWindow(view.getWindowToken(),
                InputMethodManager.HIDE_NOT_ALWAYS);
      }
    } catch (final Exception e) {
      MedDevLog.error("BaseHydroGuideFragment", "Error hiding keyboard", e);
    }
  }

  public static void simulateKeyInput(final Button.ButtonIndex btnIdx, final ButtonState btnState,
                                      final BaseInputConnection inputConnection) {
    int keyCode = KeyEvent.KEYCODE_UNKNOWN;
    switch (btnIdx) {
      case Up -> keyCode = KeyEvent.KEYCODE_DPAD_UP;
      case Down -> keyCode = KeyEvent.KEYCODE_DPAD_DOWN;
      case Left -> keyCode = KeyEvent.KEYCODE_DPAD_LEFT;
      case Right -> keyCode = KeyEvent.KEYCODE_DPAD_RIGHT;
      case Select -> keyCode = KeyEvent.KEYCODE_ENTER;
      default -> {
      }
    }

    switch (btnState) {
      case ButtonHeld -> {
        //Log.w("Controller Input - Brightness", String.format("Button Held: %s", buttonIdx));
        final KeyEvent keyEvt = new KeyEvent(KeyEvent.ACTION_DOWN, keyCode);
        inputConnection.sendKeyEvent(KeyEvent.changeFlags(keyEvt, KeyEvent.FLAG_LONG_PRESS));
      }
      case PressTransition -> {
        //Log.w("Controller Input - Brightness", String.format("Button Pressed: %s", buttonIdx));
        inputConnection.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, keyCode));
      }
      case ReleaseTransition -> {
//        Log.w("Controller Input - Brightness", String.format("Button Released: %s", btnIdx));
        inputConnection.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, keyCode));
      }
      default -> {
      }
    }
  }

  public static ViewTreeObserver.OnGlobalLayoutListener getCloseKeyboardHandler(
          final Activity activity, final View root, final Dialog closeKbBtn) {
    return new ViewTreeObserver.OnGlobalLayoutListener() {
      private Dialog cKbBtn = closeKbBtn;

      @Override
      public void onGlobalLayout() {
        final Rect r = new Rect();
        root.getWindowVisibleDisplayFrame(r);

        final int heightDiff = root.getRootView().getHeight() - (r.bottom - r.top);
        final boolean keyboardIsVisible = (heightDiff > KEYBOARD_HEIGHT_PX_THRESHOLD);

        if (keyboardIsVisible) {
          if (cKbBtn == null) {
            final View dialogView =
                    activity.getLayoutInflater().inflate(R.layout.close_keyboard_layout, null);
            dialogView.setOnClickListener(view -> hideKeyboard(activity));

            cKbBtn = new AlertDialog.Builder(root.getContext()).setView(dialogView).create();
            cKbBtn.getWindow().addFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);
            cKbBtn.getWindow().setDimAmount(0);
            cKbBtn.getWindow().setGravity(Gravity.BOTTOM | Gravity.END);
            final WindowManager.LayoutParams params = cKbBtn.getWindow().getAttributes();
            params.y = -30;
            cKbBtn.getWindow().setAttributes(params);
            cKbBtn.show();
            cKbBtn.getWindow().setLayout(
                    (int) activity.getResources().getDimension(R.dimen.button_close_keyboard),
                    ViewGroup.LayoutParams.WRAP_CONTENT);
          } else {
            cKbBtn.show();
          }
        } else {
          if (cKbBtn != null) {
            cKbBtn.dismiss();
          }
        }
      }
    };
  }

  public static class FacilitySortingComparator implements Comparator<Facility> {
    private final String[] favoriteIds;
    private final SortMode sortMode;

    public FacilitySortingComparator(final String[] faveIds, final SortMode mode) {
      favoriteIds = faveIds.clone();
      sortMode = mode;
    }

    public int compare(final Facility facilityA, final Facility facilityB) {
      // Put favorite facilities first
      final int faveCompare = isFavorite(facilityB).compareTo(isFavorite(facilityA));
      int nameCompare = -1;
      int dateCompare = -1;

      switch (sortMode) {
        case Name -> {
          nameCompare = facilityA.getFacilityName().compareTo(facilityB.getFacilityName());
          return (faveCompare == 0) ? nameCompare : faveCompare;
        }
        case Date -> {
          dateCompare = facilityB.getDateLastUsed().compareTo(facilityA.getDateLastUsed());
          return (faveCompare == 0) ? dateCompare : faveCompare;
        }
        default -> {
          return faveCompare;
        }
      }
    }

    private Boolean isFavorite(final Facility facility) {
      return Arrays.stream(favoriteIds).anyMatch(x -> (Objects.equals(x, facility.getId())));
    }
  }

  public static class RecordsSortingComparator implements Comparator<PatientFacilityInfo> {
    private final SortMode sortingMode;

    public RecordsSortingComparator(final SortMode mode) {
      sortingMode = mode;
    }

    public int compare(final PatientFacilityInfo recA, final PatientFacilityInfo recB) {
      int nameCompare = -1;
      int dateCompare = -1;

      switch (sortingMode) {
        case Name -> {
          nameCompare =
                  recA.patientName.compareTo(recB.patientName);
          return nameCompare;
        }
        default -> {
          dateCompare = recB.createdOn.compareTo(recA.createdOn);
          return dateCompare;
        }
      }
    }
  }

  public static class DecimalDigitsInputFilter implements InputFilter {
    private final int mMaxIntegerDigitsLength;
    private final int mMaxDigitsAfterLength;

    public DecimalDigitsInputFilter(final int maxDigitsBeforeDot, final int maxDigitsAfterDot) {
      mMaxIntegerDigitsLength = maxDigitsBeforeDot;
      mMaxDigitsAfterLength = maxDigitsAfterDot;
    }

    @Nullable
    @Override
    public CharSequence filter(final CharSequence source, final int start, final int end,
                               final Spanned dest, final int dStart, final int dEnd) {
      final String allText = getAllText(source, dest, dStart);
      final String onlyDigitsText = getOnlyDigitsPart(allText);

      if (allText.isEmpty()) {
        return null;
      } else {
        final double enteredValue;
        try {
          enteredValue = Double.parseDouble(onlyDigitsText);
        } catch (final NumberFormatException e) {
          return "";
        }
        return checkMaxValueRule(enteredValue, onlyDigitsText);
      }
    }

    private CharSequence checkMaxValueRule(final double enteredValue, final String onlyDigitsText) {
      return handleInputRules(onlyDigitsText);
    }

    private CharSequence handleInputRules(final String onlyDigitsText) {
      if (isDecimalDigit(onlyDigitsText)) {
        return checkRuleForDecimalDigits(onlyDigitsText);
      } else {
        return checkRuleForIntegerDigits(onlyDigitsText.length());
      }
    }

    private boolean isDecimalDigit(final String onlyDigitsText) {
      return onlyDigitsText.contains(".");
    }

    @Nullable
    private CharSequence checkRuleForDecimalDigits(final String onlyDigitsPart) {
      final String afterDotPart = onlyDigitsPart.substring(onlyDigitsPart.indexOf("."),
              onlyDigitsPart.length() - 1);
      if (afterDotPart.length() > mMaxDigitsAfterLength) {
        return "";
      }
      return null;
    }

    @Nullable
    private CharSequence checkRuleForIntegerDigits(final int allTextLength) {
      if (allTextLength > mMaxIntegerDigitsLength) {
        return "";
      }
      return null;
    }

    private String getOnlyDigitsPart(final String text) {
      return text.replaceAll("[^0-9?!.]", "");
    }

    private String getAllText(final CharSequence source, final Spanned dest, final int dStart) {
      String allText = "";
      if (!dest.toString().isEmpty()) {
        if (source.toString().isEmpty()) {
          allText = deleteCharAtIndex(dest, dStart);
        } else {
          allText = new StringBuilder(dest).insert(dStart, source).toString();
        }
      }
      return allText;
    }

    private String deleteCharAtIndex(final Spanned dest, final int dStart) {
      final StringBuilder builder = new StringBuilder(dest);
      builder.deleteCharAt(dStart);
      return builder.toString();
    }
  }

  public static final class NonFilterArrayAdapter<T> extends ArrayAdapter<T> {

    public NonFilterArrayAdapter(@NonNull final Context context, final int resource,
                                 @NonNull final T[] objects) {
      super(context, resource, objects);
    }

    @NonNull
    @Override
    public Filter getFilter() {
      return new NonFilterArrayAdapter.NonFilter();
    }

    private static class NonFilter extends Filter {
      @Nullable
      @Override
      protected FilterResults performFiltering(final CharSequence constraint) {
        return null;
      }

      @Override
      protected void publishResults(final CharSequence constraint, final FilterResults results) {

      }
    }
  }
}
