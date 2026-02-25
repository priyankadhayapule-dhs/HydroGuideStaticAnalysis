package com.accessvascularinc.hydroguide.mma.adapters;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.Filter;
import android.widget.Filterable;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity;
import com.accessvascularinc.hydroguide.mma.model.SortMode;
import com.accessvascularinc.hydroguide.mma.utils.ScreenRoute;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.Locale;

public class DataRecordsAdapter<T> extends RecyclerView.Adapter<RecyclerView.ViewHolder>
        implements Filterable {
  private List<T> lstRecords;
  private List<T> lstFilteredRecords, lstActualCollection;
  private List<T> lstSelectedRecords = new ArrayList<>();
  public DataRecordsClickListener recordClickListener;
  private static final int VIEW_TYPE_HEADER = 0;
  private static final int VIEW_TYPE_ITEM = 1;
  private ScreenRoute routeSource;
  private int recordBackgroundColor;
  private LinearLayout.LayoutParams params;

  // Persisted sort state — auto-applied whenever lstFilteredRecords is rebuilt
  private SortMode currentSortMode = SortMode.None;
  private boolean currentSortByAlphabet = false;
  private boolean currentSortByDate = false;

  public DataRecordsAdapter(DataRecordsClickListener recordClickListener, List<T> lstRecords,
                            ScreenRoute routeSource) {
    this.recordClickListener = recordClickListener;
    this.lstRecords = lstRecords;
    this.lstFilteredRecords = new ArrayList<>(lstRecords);
    this.routeSource = routeSource;
  }

  @NonNull
  @Override
  public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
    if (viewType == VIEW_TYPE_HEADER) {
      View headerView = LayoutInflater.from(parent.getContext())
              .inflate(R.layout.admin_flow_table_headers, parent, false);
      switch (routeSource) {
        case CLINICIAN_RECORDS:
          headerView.findViewById(R.id.tblHeaderClinician).setVisibility(View.VISIBLE);
          headerView.findViewById(R.id.tblHeaderFacility).setVisibility(View.GONE);
          return new DataRecordsHeaderViewHolder(headerView);
        case FACILITY_RECORDS:
          headerView.findViewById(R.id.tblHeaderFacility).setVisibility(View.VISIBLE);
          headerView.findViewById(R.id.tblHeaderClinician).setVisibility(View.GONE);
          return new DataRecordsHeaderViewHolder(headerView);
      }
    } else {
      return new DataRecordsItemViewHolder(LayoutInflater.from(parent.getContext())
              .inflate(R.layout.row_table_records, parent, false));
    }
    return null;
  }

  @Override
  public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
    if (holder.getItemViewType() == VIEW_TYPE_HEADER) {
      DataRecordsHeaderViewHolder headerViewHolder = (DataRecordsHeaderViewHolder) holder;
    } else {
      DataRecordsItemViewHolder itemViewHolder = (DataRecordsItemViewHolder) holder;
      int dataPosition = itemViewHolder.getBindingAdapterPosition() - 1;
      if (dataPosition >= 0 &&
              dataPosition < lstFilteredRecords.size()) {
        prepareRecord(itemViewHolder, lstFilteredRecords.get(dataPosition));
      }
      recordBackgroundColor = (position % 2 != 0)
              ? ContextCompat.getColor(holder.itemView.getContext(), R.color.table_odd_rows)
              : ContextCompat.getColor(holder.itemView.getContext(), R.color.table_even_rows);
      itemViewHolder.itemView.setBackgroundColor(recordBackgroundColor);
      itemViewHolder.chkRecord.setBackgroundColor(recordBackgroundColor);
      itemViewHolder.chkRecord.setOnCheckedChangeListener(
              new CompoundButton.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(@NonNull CompoundButton buttonView,
                                             boolean isChecked) {
                  int checkedRecordPosition = holder.getBindingAdapterPosition() - 1;
                  if (isChecked) {
                    lstSelectedRecords.add(lstFilteredRecords.get(checkedRecordPosition));
                    changeSelectedRowBackground(itemViewHolder,
                            ContextCompat.getColor(holder.itemView.getContext(),
                                    R.color.table_selected_rows),
                            ContextCompat.getColor(holder.itemView.getContext(),
                                    R.color.av_darkest_blue));
                  } else {
                    lstSelectedRecords.remove(lstFilteredRecords.get(checkedRecordPosition));
                    int selectedRecordBackground = (holder.getBindingAdapterPosition() % 2 != 0)
                            ? ContextCompat.getColor(holder.itemView.getContext(),
                            R.color.table_odd_rows)
                            : ContextCompat.getColor(holder.itemView.getContext(),
                            R.color.table_even_rows);
                    changeSelectedRowBackground(itemViewHolder, selectedRecordBackground,
                            ContextCompat.getColor(holder.itemView.getContext(), R.color.white));
                  }
                }
              });
    }
  }

  private class DataRecordsHeaderViewHolder extends RecyclerView.ViewHolder {
    TextView tvHeaderClinicianName, tvHeaderEmailAddress, tvUserType;
    TextView tvHeaderFacilityName;

    public DataRecordsHeaderViewHolder(@NonNull final View view) {
      super(view);
      this.tvHeaderClinicianName = view.findViewById(R.id.tvHeaderClinicianName);
      this.tvHeaderEmailAddress = view.findViewById(R.id.tvHeaderEmailAddress);
      this.tvHeaderFacilityName = view.findViewById(R.id.tvHeaderFacilityName);
      this.tvUserType = view.findViewById(R.id.tvHeaderUserType);
    }
  }

  private class DataRecordsItemViewHolder extends RecyclerView.ViewHolder {
    CheckBox chkRecord;
    TextView tvName, tvEmailAddress, tvUserType;

    public DataRecordsItemViewHolder(@NonNull final View view) {
      super(view);
      this.chkRecord = view.findViewById(R.id.chkRecord);
      this.tvName = view.findViewById(R.id.tvName);
      this.tvEmailAddress = view.findViewById(R.id.tvEmailAddress);
      this.tvUserType = view.findViewById(R.id.tvUserType);
    }
  }

  public void prepareRecord(DataRecordsItemViewHolder holder, T dataRecords) {
    if (dataRecords instanceof FacilityEntity) {
      FacilityEntity facility = (FacilityEntity) dataRecords;
      holder.tvName.setText(facility.getFacilityName());
      holder.tvEmailAddress.setVisibility(View.GONE);
      holder.tvUserType.setVisibility(View.GONE);
      params = (LinearLayout.LayoutParams) holder.tvName.getLayoutParams();
      params.weight = 1f;
      params.width = 0;
      holder.tvName.setLayoutParams(params);
    } else if (dataRecords instanceof com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel) {
      com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel apiModel =
              (com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel) dataRecords;
      holder.tvName.setText(apiModel.getUserName());
      holder.tvEmailAddress.setText(apiModel.getUserEmail());
      if (apiModel.getRoles() != null && apiModel.getRoles().size() > 0)
        holder.tvUserType.setText(apiModel.getRoles().get(0).getRole());
      holder.tvEmailAddress.setVisibility(View.VISIBLE);
      holder.tvUserType.setVisibility(View.VISIBLE);
    }
    holder.itemView.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        recordClickListener.onDataRecordClick(holder.getBindingAdapterPosition() - 1);
      }
    });
  }

  public void changeSelectedRowBackground(DataRecordsItemViewHolder holder, int recordColor,
                                          int fontColor) {
    holder.itemView.setBackgroundColor(recordColor);
    holder.tvName.setTextColor(fontColor);
    holder.tvEmailAddress.setTextColor(fontColor);
  }

  public interface DataRecordsClickListener {
    void onDataRecordClick(int position);
  }

  @Override
  public int getItemViewType(final int position) {
    return (position == 0) ? VIEW_TYPE_HEADER : VIEW_TYPE_ITEM;
  }

  @Override
  public int getItemCount() {
    return lstFilteredRecords.size() + 1; // +1 for the header
  }

  @Override
  public Filter getFilter() {

    return new Filter() {
      @Override
      protected FilterResults performFiltering(CharSequence charSequence) {
        String query = charSequence.toString();
        if (query.isEmpty()) {
          lstFilteredRecords = new ArrayList<>(lstRecords);
        } else {
          List<T> searchResults = new ArrayList<>();
          for (T records : lstRecords) {
            if (records instanceof FacilityEntity) {
              FacilityEntity facilityInfo = (FacilityEntity) records;
              if (facilityInfo.getFacilityName().toLowerCase().contains(query.toLowerCase())) {
                searchResults.add(records);
              }
            } else if (records instanceof com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel) {
              com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel clinician =
                  (com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel) records;
              if (clinician.getUserName() != null && clinician.getUserName().toLowerCase().contains(query.toLowerCase())) {
                searchResults.add(records);
              }
            }
          }
          lstFilteredRecords = searchResults;
        }
        FilterResults filterResults = new FilterResults();
        filterResults.values = lstFilteredRecords;
        return filterResults;
      }

      @Override
      protected void publishResults(CharSequence charSequence, FilterResults filterResults) {
        lstFilteredRecords = (List<T>) filterResults.values;
        notifyDataSetChanged();
      }
    };
  }

  public void sortRecords(final SortMode mode, boolean statusSortByAlphabet,
                          boolean statusSortByDate) {
    // Persist sort state so it can be auto-applied after refreshList / filter
    this.currentSortMode = mode;
    this.currentSortByAlphabet = statusSortByAlphabet;
    this.currentSortByDate = statusSortByDate;

    // Ensure lstActualCollection is initialized with the original order
    if (lstActualCollection == null) {
      lstActualCollection = new ArrayList<>();
      if (lstRecords != null) {
        lstActualCollection.addAll(lstRecords);
      }
    }
    if (lstFilteredRecords == null) {
      lstFilteredRecords = new ArrayList<>();
    }

    switch (mode) {
      case Name, None:
        if (statusSortByAlphabet) {
          // Sort by name A-Z (ascending)
          Collections.sort(lstFilteredRecords, (a, b) -> {
            String nameA = getRecordName(a);
            String nameB = getRecordName(b);
            return nameA.compareToIgnoreCase(nameB);
          });
        } else {
          // Reset to original order
          lstFilteredRecords.clear();
          lstFilteredRecords.addAll(lstActualCollection);
        }
        break;
      case Date:
        if (statusSortByDate) {
          // Sort by date (createdOn) - newest first
            Collections.sort(lstFilteredRecords, (a, b) -> {
            Date dateA = parseRecordDate(getRecordDate(a));
            Date dateB = parseRecordDate(getRecordDate(b));
            if (dateA == null && dateB == null) return 0;
            if (dateA == null) return 1;
            if (dateB == null) return -1;
            return dateB.compareTo(dateA);
          });
        } else {
          // Reset to original order
          lstFilteredRecords.clear();
          lstFilteredRecords.addAll(lstActualCollection);
        }
        break;
    }
    notifyDataSetChanged();
  }

  /**
   * Extracts the name field from a record for sorting purposes.
   */
  private String getRecordName(T record) {
    if (record instanceof FacilityEntity) {
      String name = ((FacilityEntity) record).getFacilityName();
      return name != null ? name : "";
    } else if (record instanceof com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel) {
      String name = ((com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel) record).getUserName();
      return name != null ? name : "";
    }
    return "";
  }

  /**
   * Extracts the date field from a record for sorting purposes.
   */
  private String getRecordDate(T record) {
    if (record instanceof FacilityEntity) {
      String date = ((FacilityEntity) record).getCreatedOn();
      return date != null ? date : "";
    } else if (record instanceof com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel) {
      String date = ((com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel) record).getCreatedOn();
      return date != null ? date : "";
    }
    return "";
  }
   /**
   * Parses a date string into a Date object.
   * Tries multiple common formats and falls back to treating as a timestamp.
   */
  private static final String[] DATE_FORMATS = {
      "yyyy-MM-dd'T'HH:mm:ss.SSS'Z'",
      "yyyy-MM-dd'T'HH:mm:ss.SSS",
      "yyyy-MM-dd'T'HH:mm:ss",
      "yyyy-MM-dd HH:mm:ss.SS",
      "yyyy-MM-dd HH:mm:ss",
      "yyyy-MM-dd",
      "MM/dd/yyyy"
  };

  private Date parseRecordDate(String dateStr) {
    if (dateStr == null || dateStr.isEmpty()) return null;

    // Try parsing as a numeric timestamp first
    try {
      long timestamp = Long.parseLong(dateStr);
      return new Date(timestamp);
    } catch (NumberFormatException ignored) {}

    // Try common date formats
    for (String format : DATE_FORMATS) {
      try {
        SimpleDateFormat sdf = new SimpleDateFormat(format, Locale.US);
        sdf.setLenient(false);
        return sdf.parse(dateStr);
      } catch (ParseException ignored) {}
    }
    return null;
  }

  /**
   * Returns the item at the given position from the filtered/sorted list.
   * Use this instead of indexing the backing list when handling click events.
   */
  public T getFilteredItem(int position) {
    if (position >= 0 && position < lstFilteredRecords.size()) {
      return lstFilteredRecords.get(position);
    }
    return null;
  }

  public List<T> fetchActualCollection(List<T> lstData) {
    lstActualCollection = new ArrayList<>();
    if (lstData != null) {
      lstActualCollection.addAll(lstData);
    }
    return lstActualCollection;
  }

   /**
   * Synchronously refreshes the displayed list from the backing data list.
   * Automatically re-applies the current sort if one is active.
   */
  public void refreshList() {
    lstFilteredRecords = new ArrayList<>(lstRecords);
    if (currentSortByAlphabet || currentSortByDate) {
      applySortInternal();
    }
    notifyDataSetChanged();
  }

  /**
   * Internal sort helper — sorts lstFilteredRecords in-place based on persisted sort state.
   * Does NOT call notifyDataSetChanged (caller is responsible).
   */
  private void applySortInternal() {
    if (lstFilteredRecords == null || lstFilteredRecords.isEmpty()) return;

    switch (currentSortMode) {
      case Name:
      case None:
        if (currentSortByAlphabet) {
          Collections.sort(lstFilteredRecords, (a, b) -> {
            String nameA = getRecordName(a);
            String nameB = getRecordName(b);
            return nameA.compareToIgnoreCase(nameB);
          });
        }
        break;
      case Date:
        if (currentSortByDate) {
          Collections.sort(lstFilteredRecords, (a, b) -> {
            Date dateA = parseRecordDate(getRecordDate(a));
            Date dateB = parseRecordDate(getRecordDate(b));
            if (dateA == null && dateB == null) return 0;
            if (dateA == null) return 1;
            if (dateB == null) return -1;
            return dateB.compareTo(dateA);
          });
        }
        break;
    }
  }
}
