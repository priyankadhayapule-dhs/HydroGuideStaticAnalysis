package com.accessvascularinc.hydroguide.mma.adapters;

import android.content.Context;
import android.graphics.Color;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.Filter;
import android.widget.Filterable;
import android.widget.TableRow;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.model.EncounterTableRow;
import com.accessvascularinc.hydroguide.mma.model.SortMode;
import com.accessvascularinc.hydroguide.mma.ui.CustomToast;
import com.accessvascularinc.hydroguide.mma.ui.input.OnItemClickListener;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import com.accessvascularinc.hydroguide.mma.utils.SortComparators;
import com.accessvascularinc.hydroguide.mma.utils.Utility;

public class CloudRecordsAdapter extends RecyclerView.Adapter<CloudRecordsAdapter.CloudRecordVH> implements Filterable
{
    private ArrayList<EncounterTableRow> rows,lstActualCollection;
    private final OnItemClickListener listener;
    private final List<EncounterTableRow> selectedRows = new ArrayList<>();
    private SortComparators sortComparators = new SortComparators();
    private int firstColumWidth = 0;
    private AdapterFilter filter;
    private Context context;


    public CloudRecordsAdapter(ArrayList<EncounterTableRow> rows, OnItemClickListener listener)
    {
        this.rows = rows;
        this.lstActualCollection = new ArrayList<>(rows);
        this.listener = listener;
    }

    @NonNull
    @Override
    public CloudRecordVH onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        context = parent.getContext();
        TableRow tr = new TableRow(parent.getContext());
        tr.setLayoutParams(
                new TableRow.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT));
        tr.setBackgroundColor(Color.TRANSPARENT);
        return new CloudRecordVH(tr);
    }

    @Override
    public void onBindViewHolder(@NonNull CloudRecordVH holder, int position) {
        holder.bind(rows.get(position), position, listener);
    }

    public void update(ArrayList<EncounterTableRow> newRows) {
        this.rows = new ArrayList<>(newRows);
        this.lstActualCollection = new ArrayList<>(newRows);
        if(filter != null)
        {
            filter.setOriginalList(new ArrayList<>(newRows));
        }
        notifyDataSetChanged();
    }

    public List<EncounterTableRow> getSelectedRows() {
        return Collections.unmodifiableList(selectedRows);
    }

    public void sortRecords(SortMode mode) {
        if (mode == SortMode.None) {
            notifyDataSetChanged();
            return;
        }
        try {
            if (mode == SortMode.Name) {
                rows.sort((a, b) -> String.valueOf(a.patientName)
                        .compareToIgnoreCase(String.valueOf(b.patientName)));
            } else if (mode == SortMode.Date) {
                rows.sort((a, b) -> Long.compare(parseLongSafe(b.createdOn),
                        parseLongSafe(a.createdOn))); // recent first
            }
        } catch (Exception e) {
          MedDevLog.error("CloudSort", "Sorting error", e);
        }
        notifyDataSetChanged();
    }

    private long parseLongSafe(String v) {
        try {
            return Long.parseLong(v);
        } catch (Exception e) {
            return 0L;
        }
    }

    public class CloudRecordVH extends RecyclerView.ViewHolder {
        private final TableRow row;

        public CloudRecordVH(@NonNull View itemView) {
            super(itemView);
            this.row = (TableRow) itemView;
        }

        public void bind(EncounterTableRow r, int position, OnItemClickListener listener) {
            Context ctx = row.getContext();
            row.removeAllViews();
            // First column is a selection checkbox to match local layout
            List<String> vals = Arrays.asList(
                    null, // placeholder for checkbox
                    r.patientName,
                    r.patientId != null ? r.patientId : "",
                    r.facilityName,
                    Utility.formatDate(r.createdOn),
                    r.createdBy);
            int[] gravities = { Gravity.START | Gravity.CENTER_VERTICAL, Gravity.START | Gravity.CENTER_VERTICAL,
                    Gravity.START | Gravity.CENTER_VERTICAL, Gravity.CENTER,
                    Gravity.START | Gravity.CENTER_VERTICAL };
            int cbW = ctx.getResources().getDimensionPixelSize(R.dimen.records_column_checkbox_width);
            int nameW = ctx.getResources().getDimensionPixelSize(R.dimen.records_column_name_width);
            int idW = ctx.getResources().getDimensionPixelSize(R.dimen.records_column_id_width);
            int facilityBase = ctx.getResources().getDimensionPixelSize(R.dimen.records_column_facility_width);
            int dobW = ctx.getResources().getDimensionPixelSize(R.dimen.records_column_dob_width);
            int syncW = ctx.getResources().getDimensionPixelSize(R.dimen.records_column_sync_status_width);
            int extraCloud = ctx.getResources().getDimensionPixelSize(R.dimen.records_cloud_extra_width);
            int facilityW = facilityBase + (dobW / 2) + (syncW / 2);
            int dateW = ctx.getResources().getDimensionPixelSize(R.dimen.records_column_date_width);
            int doneByBase = ctx.getResources().getDimensionPixelSize(R.dimen.records_column_done_by_width);
            int doneByW = doneByBase + (dobW / 2) + (syncW / 2) + extraCloud;
            int[] widths = { cbW, nameW+firstColumWidth, idW, facilityW, dateW, doneByW };
            int rowHeight = ctx.getResources().getDimensionPixelSize(R.dimen.records_table_row_height);
            int pad = (int) ctx.getResources().getDimension(R.dimen.records_table_cell_padding);
            for (int i = 0; i < vals.size(); i++) {
                if (i == 0) { // checkbox cell
                    CheckBox cb = new CheckBox(ctx);
                    TableRow.LayoutParams lp = new TableRow.LayoutParams(widths[i], rowHeight);
                    cb.setLayoutParams(lp);
                    cb.setBackgroundColor(ContextCompat.getColor(ctx, R.color.av_darkest_blue));
                    cb.setButtonDrawable(R.drawable.checkbox_background);
                    cb.setOnCheckedChangeListener((button, checked) -> {
                        if (checked) {
                            if (!selectedRows.contains(r)) {
                                selectedRows.add(r);
                            }
                        } else {
                            selectedRows.remove(r);
                        }
                        applyRowSelectionVisual(r, position);
                    });
                    row.addView(cb);
                    continue;
                }
                TextView tv = new TextView(ctx);
                TableRow.LayoutParams lp = new TableRow.LayoutParams(widths[i], rowHeight);
                tv.setLayoutParams(lp);
                tv.setPadding(pad, 0, pad, 0);
                tv.setGravity(gravities[Math.min(i - 1, gravities.length - 1)]);
                tv.setBackground(ContextCompat.getDrawable(ctx, R.drawable.thin_white_border));
                tv.setText(vals.get(i) != null ? vals.get(i) : "");
                tv.setTextColor(ContextCompat.getColor(ctx, R.color.white));
                row.addView(tv);
            }
            if ((position % 2) != 0) {
                row.setBackgroundColor(ContextCompat.getColor(ctx, R.color.av_alt_light_blue));
            } else {
                row.setBackgroundColor(Color.TRANSPARENT);
            }
            applyRowSelectionVisual(r, position);
                // Add row click listener to trigger OnItemClickListener
                row.setOnClickListener(v -> {
                    if (listener != null) {
                        listener.onItemClickListener(getBindingAdapterPosition());
                    }
                });
        }

        private void applyRowSelectionVisual(EncounterTableRow r, int position) {
            Context ctx = row.getContext();
            boolean selected = selectedRows.contains(r);
            int baseAlt = ContextCompat.getColor(ctx, R.color.av_alt_light_blue);
            int normal = Color.TRANSPARENT;
            int selectedBg = ContextCompat.getColor(ctx, R.color.av_gray);
            int selectedText = ContextCompat.getColor(ctx, R.color.av_darkest_blue);
            int normalText = ContextCompat.getColor(ctx, R.color.white);

            int bg = ((position % 2) != 0) ? baseAlt : normal;
            if (selected) {
                bg = selectedBg;
            }
            row.setBackgroundColor(bg);
            for (int i = 0; i < row.getChildCount(); i++) {
                View child = row.getChildAt(i);
                if (child instanceof TextView) {
                    ((TextView) child).setTextColor(selected ? selectedText : normalText);
                }
            }
        }

    }

    public void sortFields(SortMode mode, TextView fieldName)
    {
        if(selectedRows.isEmpty())
        {
            sortComparators.sortFields(mode,fieldName,rows,this);
        }
        else
        {
            sortComparators.sortFields(mode,fieldName,selectedRows,this);
        }
    }

    public SortComparators getSortingInstance()
    {
        return (sortComparators == null) ? (sortComparators = new SortComparators()) : sortComparators;
    }

    @Override
    public int getItemCount()
    {
        return rows.size();
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

    public void setFilteredRows(ArrayList<EncounterTableRow> filteredRows)
    {
        this.rows = filteredRows;
        notifyDataSetChanged();
        if(context != null && this.rows.isEmpty())
        {
            CustomToast.show(context,context.getResources().getString(R.string.mesg_no_records), CustomToast.Type.ERROR);
        }
    }

    public void clearFilter()
    {
        this.rows = new ArrayList<>(lstActualCollection);
        if(filter != null)
        {
            filter.setOriginalList(new ArrayList<>(lstActualCollection));
        }
        notifyDataSetChanged();
    }
}
