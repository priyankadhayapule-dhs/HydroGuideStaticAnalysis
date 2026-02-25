package com.accessvascularinc.hydroguide.mma.utils;

import android.util.Log;
import android.view.View;
import android.widget.TextView;

import androidx.recyclerview.widget.RecyclerView;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.model.EncounterTableRow;
import com.accessvascularinc.hydroguide.mma.model.SortMode;

import org.w3c.dom.Text;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.Locale;

public class SortComparators
{
  private SortMode currentSortMode = SortMode.None;
  private boolean isAscending = false;
  public static final int CLOUD_COLUM_PATIENT_NAME = 1001;
  public static final int CLOUD_COLUM_PATIENT_ID = 1002;
  public static final int CLOUD_COLUM_FACILITY = 1003;
  public static final int CLOUD_COLUM_DATE = 1004;
  public static final int CLOUD_COLUM_DONE_BY = 1005;

  public static int compareDobStrings(String dob1, String dob2) {
    if (dob1 == null || dob1.isEmpty()) return -1;
    if (dob2 == null || dob2.isEmpty()) return 1;

    try {
      SimpleDateFormat sdf = new SimpleDateFormat("MM/dd/yyyy", Locale.US);
      Date d1 = sdf.parse(dob1);
      Date d2 = sdf.parse(dob2);
      if (d1 != null && d2 != null) {
        return d1.compareTo(d2);
      }
    } catch (Exception e) {
      // Fall back to string comparison
    }
    return dob1.compareToIgnoreCase(dob2);
  }

  public static long parseLongSafe(String v)
  {
    try {
      return v != null ? Long.parseLong(v) : 0L;
    } catch (Exception e) {
      return 0L;
    }
  }

  public static boolean isDateBetween(Date dateToCheck, Date startDate, Date endDate)
  {
    return dateToCheck.getTime() >= startDate.getTime() && dateToCheck.getTime() <= endDate.getTime();
  }

  public void sortFields(SortMode mode, TextView fieldName, List<EncounterTableRow> records, RecyclerView.Adapter adapter)
  {
    if (mode == SortMode.None)
    {
      adapter.notifyDataSetChanged();
      return;
    }
    if (currentSortMode == mode)
    {
      isAscending = !isAscending;
    }
    else
    {
      currentSortMode = mode;
      isAscending = true;
    }
    switch(fieldName.getId())
    {
      //Patient Name
      case CLOUD_COLUM_PATIENT_NAME:
      case R.id.tvPatientNameHeader:
        records.sort((a, b) ->
        {
          String patientNameOne = a.patientName != null ? a.patientName : "";
          String patientNameTwo = b.patientName != null ? b.patientName : "";
          int result = patientNameOne.compareToIgnoreCase(patientNameTwo);
          return isAscending ? result : -result;
        });
        break;

      //Patient ID
      case CLOUD_COLUM_PATIENT_ID:
      case R.id.tvPatientIdHeader:
        records.sort((a, b) -> {
          String patientIdOne = a.patientId != null ? a.patientId : "";
          String patientIdTwo = b.patientId != null ? b.patientId : "";
          int result = patientIdOne.compareToIgnoreCase(patientIdTwo);
          return isAscending ? result : -result;
        });
        break;

      //Patient Dob
      case R.id.tvPatientDobHeader:
        records.sort((a, b) -> {
          int result = SortComparators.compareDobStrings(a.patientDob, b.patientDob);
          return isAscending ? result : -result;
        });
        break;

      case CLOUD_COLUM_DATE:
      case R.id.tvInsertionDateHeader:
        records.sort((a, b) -> {
          int result = SortComparators.compareDobStrings(a.createdOn, b.createdOn);
          return isAscending ? result : -result;
        });
        break;

      //Patient Clinician Name
      case CLOUD_COLUM_DONE_BY:
      case R.id.tvDoneByHeader:
        records.sort((a, b) -> {
          String createdByOne = a.createdBy != null ? a.createdBy : "";
          String createdByTwo = b.createdBy != null ? b.createdBy : "";
          int result = createdByOne.compareToIgnoreCase(createdByTwo);
          return isAscending ? result : -result;
        });
        break;

      //Patient Facility Name
      case CLOUD_COLUM_FACILITY:
      case R.id.tvFacilityHeader:
        records.sort((a, b) -> {
          String facilityNameOne = a.facilityName != null ? a.facilityName : "";
          String facilityNameTwo = b.facilityName != null ? b.facilityName : "";
          int result = facilityNameOne.compareToIgnoreCase(facilityNameTwo);
          return isAscending ? result : -result;
        });
        break;
    }
    toggleSortIndicatorIcon((TextView)fieldName,isSortAscending());
    adapter.notifyDataSetChanged();
  }

  public SortMode getCurrentSortMode()
  {
    return currentSortMode;
  }

  public boolean isSortAscending()
  {
    return isAscending;
  }

  public void toggleSortIndicatorIcon(TextView fieldName, boolean enableSorting)
  {
    fieldName.setCompoundDrawablesWithIntrinsicBounds(0, 0,R.drawable.icon_sort, 0);
  }
}
