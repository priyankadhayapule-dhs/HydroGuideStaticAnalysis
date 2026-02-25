package com.accessvascularinc.hydroguide.mma.adapters;

import android.content.Context;
import android.graphics.Color;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.Filter;
import android.widget.Filterable;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.model.EncounterData;
import com.accessvascularinc.hydroguide.mma.model.EncounterTableRow;
import com.accessvascularinc.hydroguide.mma.model.SortMode;
import com.accessvascularinc.hydroguide.mma.ui.CustomToast;
import com.accessvascularinc.hydroguide.mma.ui.input.OnItemClickListener;
import com.accessvascularinc.hydroguide.mma.ui.input.InputUtils;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.stream.IntStream;
import com.accessvascularinc.hydroguide.mma.utils.SortComparators;

public class RecordsAdapter extends RecyclerView.Adapter<RecordsAdapter.RecordItemViewHolder> implements Filterable
{
    private final List<EncounterTableRow> selectedRecords = new ArrayList<>();
    private final OnItemClickListener adapterItemClickListener;
    private ArrayList<EncounterTableRow> records;
    private ArrayList<EncounterTableRow> lstActualCollection;
    private ArrayList<EncounterData> records2;
    private boolean isOfflineMode;
    private SortComparators sortComparators = new SortComparators();
    private AdapterFilter filter;
    private Context context;
    public RecordsAdapter(final ArrayList<EncounterTableRow> dataSet,
            final OnItemClickListener listener,
            ArrayList<EncounterData> savedRecords) {
        this(dataSet, listener, savedRecords, false);
    }

    public RecordsAdapter(final ArrayList<EncounterTableRow> dataSet,
            final OnItemClickListener listener,
            ArrayList<EncounterData> savedRecords,
            boolean isOfflineMode) {
        records = dataSet;
        records2 = savedRecords;
        this.lstActualCollection = new ArrayList<>(records);
        this.adapterItemClickListener = listener;
        this.isOfflineMode = isOfflineMode;
    }

    @NonNull
    @Override
    public RecordItemViewHolder onCreateViewHolder(@NonNull final ViewGroup parent,
            final int position) {
        context = parent.getContext();
        return RecordItemViewHolder.create(parent);
    }

    @Override
    public void onBindViewHolder(@NonNull final RecordItemViewHolder vh, final int position) {
        final EncounterTableRow record = records.get(position);
        vh.setRecord(record);
        vh.setIsOfflineMode(isOfflineMode);

        if(selectedRecords.isEmpty())
        {
            vh.cbSelectRecord.setChecked(false);
        }
        // Alternate row colors for better readability.
        if ((position % 2) != 0) {
            vh.setRowColor(ContextCompat.getColor(vh.itemView.getContext(), R.color.av_alt_light_blue));
        } else {
            vh.setRowColor(Color.TRANSPARENT);
        }

        vh.itemView.setOnClickListener(
                view -> adapterItemClickListener.onItemClickListener(vh.getBindingAdapterPosition()));
        // Update checkbox state based on selection
        vh.cbSelectRecord.setOnCheckedChangeListener(null); // Remove listener temporarily
        vh.cbSelectRecord.setChecked(selectedRecords.contains(record));
        vh.toggleRowColors(); // Apply selection colors when checkbox state is set
        vh.cbSelectRecord.setOnCheckedChangeListener((compoundButton, isChecked) -> {
            // Do something on checkbox click
            if (isChecked) {
                if (!selectedRecords.contains(records.get(vh.getBindingAdapterPosition()))) {
                    selectedRecords.add(records.get(vh.getBindingAdapterPosition()));
                }
            } else {
                selectedRecords.remove(records.get(vh.getBindingAdapterPosition()));
            }

            vh.toggleRowColors();
            // Notify fragment to update UI
            if (adapterItemClickListener instanceof com.accessvascularinc.hydroguide.mma.ui.RecordsFragment) {
                ((com.accessvascularinc.hydroguide.mma.ui.RecordsFragment) adapterItemClickListener).updateSelectionUI();
            }
        });
    }

    @Override
    public int getItemCount() {
        return records.size();
    }

    public List<EncounterTableRow> getSelectedRecords() {
        return Collections.unmodifiableList(selectedRecords);
    }

    public void clearSelection() {
        selectedRecords.clear();
        notifyDataSetChanged();
    }

    /**
     * Select or deselect all records
     * @param selectAll true to select all, false to deselect all
     */
    public void selectAll(boolean selectAll) {
        selectedRecords.clear();
        if (selectAll) {
            selectedRecords.addAll(records);
        }
        notifyDataSetChanged();
    }

    public EncounterTableRow getRecordAtIdx2(final int index) {
        return records.get(index);
    }

    public EncounterData getRecordAtIdx(final int index,
            com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase db) {
        return transformDBToEncounterData(index, db);
    }

    private EncounterData transformDBToEncounterData(int index,
            com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase db) {
        ExecutorService executor = Executors.newSingleThreadExecutor();

        try {
            Future<EncounterData> future = executor.submit(() -> {
                com.accessvascularinc.hydroguide.mma.db.repository.EncounterRepository encounterRepository = new com.accessvascularinc.hydroguide.mma.db.repository.EncounterRepository(
                        db);

                try {
                    List<com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity> listEntity = encounterRepository
                            .getAllEncounterDataList();

                    if (listEntity == null || listEntity.isEmpty() || index >= listEntity.size()) {
                      MedDevLog.warn("ABC", "No data found or invalid index: " + index);
                        return null;
                    }

                    com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity encounterEntity = listEntity
                            .get(index);
                    EncounterData encounterData = encounterEntity.toEncounterData();
                    Log.d("ABC", "Fetched EncounterData: " + encounterData);
                    return encounterData;

                } catch (ExecutionException | InterruptedException e) {
                  MedDevLog.error("ABC", "Error fetching encounter data", e);
                    return null;
                }
            });
            return future.get();

        } catch (Exception e) {
            MedDevLog.error("ABC", "Unexpected error in transformDBToEncounterData", e);
            return null;
        }
    }

    public void removeProcedureByStartTime(final Date procedureStartTime) {
        final int index = getIdxFromStartTime(procedureStartTime);
        final ArrayList<EncounterTableRow> resizedRecords = new ArrayList<>(
                Collections.nCopies(records.size() - 1, null));
        System.arraycopy(records.toArray(), 0, resizedRecords.toArray(), 0, index);
        System.arraycopy(records.toArray(), index + 1, resizedRecords.toArray(), index,
                (records.size() - index) - 1);
        records = resizedRecords;
        this.notifyItemRemoved(index);
    }

    public void updateSavedProcedures(final ArrayList<EncounterTableRow> newRecords) {
        records = (ArrayList<EncounterTableRow>) newRecords.clone();
        originalOrder = new ArrayList<>(records); // Reset original order on data refresh
        // Remove any selected records that no longer exist in the updated records list
        selectedRecords.retainAll(records);
        this.notifyDataSetChanged();
    }

    public int getIdxFromStartTime(final Date procedureStartTime) {
        return IntStream.range(0, records.size())
                .filter(i -> (procedureStartTime == getDate(records.get(i).createdOn)))
                .findFirst()
                .orElse(-1);
    }

    private Date getDate(String createdOn) {
        try {
            final long millis = Long.parseLong(createdOn);
            return new Date(millis);
        } catch (Exception e) {
            return null;
        }
    }

    private ArrayList<EncounterTableRow> originalOrder;

    public void sortRecords(final SortMode mode) {
        // Preserve original order on first sort call
        if (originalOrder == null) {
            originalOrder = new ArrayList<>(records);
        }

        switch (mode) {
            case Name:
                // Sort by patient name A-Z (ascending)
                Collections.sort(records, (a, b) -> {
                    String nameA = a.patientName != null ? a.patientName : "";
                    String nameB = b.patientName != null ? b.patientName : "";
                    return nameA.compareToIgnoreCase(nameB);
                });
                break;
            case Date:
                // Sort by date - newest first (descending)
                Collections.sort(records, (a, b) -> {
                    long dateA = parseLongSafe(a.createdOn);
                    long dateB = parseLongSafe(b.createdOn);
                    return Long.compare(dateB, dateA);
                });
                break;
            case None:
            default:
                // Reset to original order
                records.clear();
                records.addAll(originalOrder);
                break;
        }
        this.notifyDataSetChanged();
    }

    private long parseLongSafe(String value) {
        try {
            return Long.parseLong(value);
        } catch (Exception e) {
            return 0L;
        }
    }

    public static class RecordItemViewHolder extends RecyclerView.ViewHolder {
        public final CheckBox cbSelectRecord;
        private final TextView tvName, tvId, tvDob, tvDate, tvDoneBy, tvFacility, tvSyncStatus;
        private int rowColor = Color.TRANSPARENT;
        private boolean isOfflineMode;

        private RecordItemViewHolder(@NonNull final View view) {
            super(view);
            cbSelectRecord = itemView.findViewById(R.id.record_item_checkbox);
            tvName = itemView.findViewById(R.id.record_item_name);
            tvId = itemView.findViewById(R.id.record_item_id);
            tvDob = itemView.findViewById(R.id.record_item_dob);
            tvDate = itemView.findViewById(R.id.record_item_date);
            tvDoneBy = itemView.findViewById(R.id.record_item_done_by);
            tvFacility = itemView.findViewById(R.id.record_item_facility);
            tvSyncStatus = itemView.findViewById(R.id.record_item_sync_status);
            // Use custom checkbox drawable with visible tick mark
            if (cbSelectRecord != null) {
                cbSelectRecord.setButtonDrawable(R.drawable.custom_checkbox_selector);
                cbSelectRecord.setButtonTintList(null);
            }
        }

        @NonNull
        public static RecordItemViewHolder create(@NonNull final ViewGroup parent) {
            return new RecordItemViewHolder(LayoutInflater.from(parent.getContext())
                    .inflate(R.layout.records_table_item, parent, false));
        }

        public void setIsOfflineMode(boolean isOffline) {
            this.isOfflineMode = isOffline;
            // Hide Sync Status column in offline mode
            if (isOffline && tvSyncStatus != null) {
                tvSyncStatus.setVisibility(View.GONE);
            }
        }

        public void setRecord(final EncounterTableRow record) {
            tvName.setText(record.patientName != null ? record.patientName : "");
            tvId.setText(record.patientId != null ? record.patientId.toString() : "");

            final SimpleDateFormat dateFormat = new SimpleDateFormat(InputUtils.SHORT_DATE_PATTERN, Locale.US);
            Log.d("ABC", "record.createdBy: " + record.createdBy);
            Log.d("ABC", "record.createdOn: " + record.createdOn);
            try {
                tvDate.setText(
                        record.createdOn != null ? dateFormat.format(new Date(Long.parseLong(record.createdOn))) : "");
            } catch (Exception e) {
                tvDate.setText("");
            }
            tvDob.setText(record.patientDob != null ? record.patientDob : "");
            tvDoneBy.setText(record.createdBy != null ? record.createdBy : "");
            tvFacility.setText(record.facilityName != null ? record.facilityName : "");
            tvSyncStatus.setText(record.syncStatus != null ? record.syncStatus : "");
        }

        private void setRowColor(final int newRowColor) {
            this.rowColor = newRowColor;
            tvName.setBackgroundColor(newRowColor);
            tvId.setBackgroundColor(newRowColor);
            tvDob.setBackgroundColor(newRowColor);
            tvDate.setBackgroundColor(newRowColor);
            tvDoneBy.setBackgroundColor(newRowColor);
            tvFacility.setBackgroundColor(newRowColor);
            tvSyncStatus.setBackgroundColor(newRowColor);
        }

        private void toggleRowColors() {
            int bgColor = rowColor;
            int textColor = ContextCompat.getColor(itemView.getContext(), R.color.white);

            if (cbSelectRecord.isChecked()) {
                bgColor = ContextCompat.getColor(itemView.getContext(), R.color.av_gray);
                textColor = ContextCompat.getColor(itemView.getContext(), R.color.av_darkest_blue);
            }
            tvName.setBackgroundColor(bgColor);
            tvId.setBackgroundColor(bgColor);
            tvDob.setBackgroundColor(bgColor);
            tvDate.setBackgroundColor(bgColor);
            tvDoneBy.setBackgroundColor(bgColor);
            tvFacility.setBackgroundColor(bgColor);
            tvSyncStatus.setBackgroundColor(bgColor);

            tvName.setTextColor(textColor);
            tvId.setTextColor(textColor);
            tvDob.setTextColor(textColor);
            tvDate.setTextColor(textColor);
            tvDoneBy.setTextColor(textColor);
            tvFacility.setTextColor(textColor);
            tvSyncStatus.setTextColor(textColor);
        }
    }

    public void sortFields(SortMode mode, TextView fieldName)
    {
        if(selectedRecords.isEmpty())
        {
            sortComparators.sortFields(mode,fieldName,records,this);
        }
        else
        {
            sortComparators.sortFields(mode,fieldName,selectedRecords,this);
        }
    }

    public SortComparators getSortingInstance()
    {
        return (sortComparators == null) ? (sortComparators = new SortComparators()) : sortComparators;
    }

    public void update(ArrayList<EncounterTableRow> updatedRows) {
        this.records = new ArrayList<>(updatedRows);
        this.lstActualCollection = new ArrayList<>(updatedRows);
        if(filter != null)
        {
            filter.setOriginalList(new ArrayList<>(updatedRows));
        }
        notifyDataSetChanged();
    }

    @Override
    public Filter getFilter()
    {
        if (filter == null)
        {
            filter = new AdapterFilter(this, lstActualCollection);
        }
        return filter;
    }

    public ArrayList<EncounterTableRow> fetchActualCollection()
    {
        return lstActualCollection;
    }

    public void setFilteredRows(ArrayList<EncounterTableRow> filteredRows) {
        this.records = filteredRows;
        notifyDataSetChanged();
        if(context != null && this.records.isEmpty())
        {
            CustomToast.show(context,context.getResources().getString(R.string.mesg_no_records), CustomToast.Type.ERROR);
        }
    }

    public void clearFilter()
    {
        this.records = new ArrayList<>(lstActualCollection);
        if(filter != null)
        {
            filter.setOriginalList(new ArrayList<>(lstActualCollection));
        }
        notifyDataSetChanged();
    }

}
