package com.accessvascularinc.hydroguide.mma.adapters;

import android.widget.Filter;
import androidx.recyclerview.widget.RecyclerView;
import com.accessvascularinc.hydroguide.mma.model.EncounterTableRow;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.SortComparators;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import org.json.JSONException;
import org.json.JSONObject;
import java.util.ArrayList;

public class AdapterFilter extends Filter
{
  private final RecyclerView.Adapter adapter;
  private ArrayList<EncounterTableRow> originalList;
  private ArrayList<EncounterTableRow> filteredList;

  public AdapterFilter(RecyclerView.Adapter adapter, ArrayList<EncounterTableRow> originalList)
  {
    this.adapter = adapter;
    this.originalList = originalList;
    this.filteredList = new ArrayList<>();
  }

  @Override
  protected FilterResults performFiltering(CharSequence charSequence)
  {
    filteredList.clear();
    String query = charSequence.toString();
    String filterByName = "",filterById = "",filterByFacility = "",filterByDoneBy = "",captureDob = "",captureInsertionDate = "";
    String filterByDob[] = null, filterByInsertionDate[] = null;
    try
    {
      JSONObject queryJson = new JSONObject(query);
      filterByName = queryJson.getString("filteredName").toLowerCase();
      filterById = queryJson.getString("filteredId").toLowerCase();
      filterByFacility = queryJson.getString("filteredFacility").toLowerCase();
      filterByDoneBy = queryJson.getString("filteredInserter").toLowerCase();
      captureDob = queryJson.getString("filteredDob");
      captureInsertionDate = queryJson.getString("filteredInsertionDate");
      filterByDob = Utility.splitDate(captureDob,"|");
      filterByInsertionDate = Utility.splitDate(captureInsertionDate,"|");
    }
    catch (JSONException e)
    {
      MedDevLog.error(getClass().getSimpleName(),""+e.getMessage());
    }
    if (filterByName.trim().isEmpty() && filterById.trim().isEmpty() && filterByFacility.trim().isEmpty() && filterByDoneBy.trim().isEmpty() && captureDob.trim().isEmpty() && captureInsertionDate.trim().isEmpty())
    {
      filteredList.addAll(originalList);
    }
    else
    {
      ArrayList<EncounterTableRow> searchResults = new ArrayList<>();
      for (EncounterTableRow records : originalList)
      {
        boolean matches = true;

        if (!filterByName.trim().isEmpty())
        {
          matches = matches && records.patientName.trim().toLowerCase().contains(filterByName);
        }
        if (!filterById.trim().isEmpty()) {
          matches = matches && records.patientId != null && records.patientId.trim().toLowerCase().contains(filterById);
        }
        if (!filterByFacility.trim().isEmpty()) {
          matches = matches && records.facilityName.trim().toLowerCase().contains(filterByFacility);
        }
        if (!filterByDoneBy.trim().isEmpty()) {
          matches = matches && records.createdBy.trim().toLowerCase().contains(filterByDoneBy);
        }
        if(filterByDob != null && filterByDob.length == 2)
        {
          String patientDob = Utility.formatDate(records.patientDob.trim());
          String startDate = filterByDob[0].trim();
          String endDate = filterByDob[1].trim();
          if(Utility.getDate(patientDob,Utility.DATE_PATTERN_TYPE_2) != null && Utility.getDate(startDate,Utility.DATE_PATTERN_TYPE_2) != null && Utility.getDate(endDate,Utility.DATE_PATTERN_TYPE_2) != null)
          {
            matches = matches && SortComparators.isDateBetween(Utility.getDate(patientDob,Utility.DATE_PATTERN_TYPE_2),Utility.getDate(startDate,Utility.DATE_PATTERN_TYPE_2),Utility.getDate(endDate,Utility.DATE_PATTERN_TYPE_2));
          }
        }
        if(filterByInsertionDate != null && filterByInsertionDate.length == 2)
        {
          String insertionDate = Utility.formatDate(records.createdOn.trim());
          String startDate = filterByInsertionDate[0].trim();
          String endDate = filterByInsertionDate[1].trim();
          if(Utility.getDate(insertionDate,Utility.DATE_PATTERN_TYPE_2) != null && Utility.getDate(startDate,Utility.DATE_PATTERN_TYPE_2) != null && Utility.getDate(endDate,Utility.DATE_PATTERN_TYPE_2) != null)
          {
            matches = matches && SortComparators.isDateBetween(Utility.getDate(insertionDate,Utility.DATE_PATTERN_TYPE_2),Utility.getDate(startDate,Utility.DATE_PATTERN_TYPE_2),Utility.getDate(endDate,Utility.DATE_PATTERN_TYPE_2));
          }
        }
        if (matches)
        {
          searchResults.add(records);
        }
      }
      filteredList.addAll(searchResults);
    }
    FilterResults filterResults = new FilterResults();
    filterResults.values = filteredList;
    return filterResults;
  }

  @SuppressWarnings("unchecked")
  @Override
  protected void publishResults(CharSequence constraint, FilterResults filterResults)
  {
    if(adapter != null && adapter instanceof CloudRecordsAdapter)
    {
      ((CloudRecordsAdapter) adapter).setFilteredRows(new ArrayList<>((ArrayList<EncounterTableRow>) filterResults.values));
    }
    else if(adapter != null && adapter instanceof RecordsAdapter)
    {
      ((RecordsAdapter) adapter).setFilteredRows((ArrayList<EncounterTableRow>) filterResults.values);
    }
  }

  public void setOriginalList(ArrayList<EncounterTableRow> newList)
  {
    this.originalList = newList;
  }
}
