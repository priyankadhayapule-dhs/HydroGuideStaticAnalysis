package com.accessvascularinc.hydroguide.mma.ui.input;

import android.content.Context;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.Filter;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.model.Facility;
import com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity;
import com.accessvascularinc.hydroguide.mma.model.SortMode;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class FacilitiesAutoCompleteAdapter extends ArrayAdapter<Facility> {
  private ArrayList<Facility> facilitiesList;
  private ArrayList<Facility> originalFacilities;
  private List<String> favorites;
  private ArrayFilter mFilter = null;

  public FacilitiesAutoCompleteAdapter(final Context context,
      final int resource,
      final int textViewResourceId,
      final Facility[] facilities,
      final String[] faves) {
    super(context, resource, textViewResourceId, facilities);
    setFacilities(facilities, faves);
  }

  // New constructor supporting FacilityEntity list
  public FacilitiesAutoCompleteAdapter(final Context context,
      final int resource,
      final int textViewResourceId,
      final List<FacilityEntity> facilityEntities,
      final String[] faves,
      final boolean fromEntities) {
    super(context, resource, textViewResourceId, convertEntities(facilityEntities));
    setFacilities(convertEntities(facilityEntities), faves);
  }

  @Override
  public int getCount() {
    return facilitiesList.size();
  }

  @Override
  public Facility getItem(final int position) {
    return facilitiesList.get(position);
  }

  public void updateFacilities(final Facility[] newFacilities, final String[] newFaves) {
    setFacilities(newFacilities, newFaves);
    this.notifyDataSetChanged();
  }

  // Overload: update using entities list
  public void updateFacilities(final List<FacilityEntity> newFacilityEntities,
      final String[] newFaves) {
    final Facility[] converted = convertEntities(newFacilityEntities);
    setFacilities(converted, newFaves);
    this.notifyDataSetChanged();
  }

  private void setFacilities(final Facility[] facilities, final String[] faves) {
    favorites = new ArrayList<>(Arrays.asList(faves));
    final ArrayList<Facility> sortedFacilities = new ArrayList<>(Arrays.asList(facilities));
    sortedFacilities.sort(new InputUtils.FacilitySortingComparator(faves, SortMode.Name));
    facilitiesList = new ArrayList<>(sortedFacilities);
    originalFacilities = new ArrayList<>(facilitiesList);
  }

  private static Facility[] convertEntities(final List<FacilityEntity> entities) {
    if (entities == null) {
      return new Facility[0];
    }
    final Facility[] out = new Facility[entities.size()];
    for (int i = 0; i < entities.size(); i++) {
      final FacilityEntity e = entities.get(i);
      final String id = (e.getFacilityId() != null) ? Long.toString(e.getFacilityId()) : "";
      final String name = e.getFacilityName() != null ? e.getFacilityName() : "";
      final String lastUsed = e.getDateLastUsed() != null ? e.getDateLastUsed() : "";
      out[i] = new Facility(id, name, lastUsed);
    }
    return out;
  }

  @NonNull
  @Override
  public View getView(final int position, final View cView, @NonNull final ViewGroup parent) {
    final LayoutInflater inflater = LayoutInflater.from(getContext());
    final View rowView = inflater.inflate(R.layout.facility_list_item, parent, false);
    final CheckBox checkbox = rowView.findViewById(R.id.facility_list_item_checkbox);
    final ImageView icon = rowView.findViewById(R.id.facility_list_item_icon);
    final TextView text = rowView.findViewById(R.id.facility_list_item_text);

    checkbox.setVisibility(View.GONE);

    final Facility facility = facilitiesList.get(position);
    final boolean isFavorite = favorites.contains(facility.getId());
    icon.setColorFilter(Color.WHITE, PorterDuff.Mode.MULTIPLY);
    icon.setVisibility(isFavorite ? View.VISIBLE : View.GONE);
    text.setText(facility.getFacilityName());

    if ((position % 2) != 0) {
      rowView.setBackgroundColor(ContextCompat.getColor(getContext(), R.color.av_alt_light_blue));
    } else {
      rowView.setBackgroundColor(ContextCompat.getColor(getContext(), R.color.av_blue));
    }

    return rowView;
  }

  @Override
  public Filter getFilter() {
    if (mFilter == null) {
      mFilter = new ArrayFilter();
    }
    return mFilter;
  }

  private class ArrayFilter extends Filter {
    private final Object lock = new Object();

    @Override
    protected FilterResults performFiltering(final CharSequence prefix) {
      final FilterResults results = new FilterResults();

      if (originalFacilities == null) {
        synchronized (lock) {
          originalFacilities = new ArrayList<>(facilitiesList);
        }
      }

      if (prefix == null || prefix.length() == 0) {
        synchronized (lock) {
          final List<Facility> list = new ArrayList<>(originalFacilities);
          results.values = list;
          results.count = list.size();
        }
      } else {
        final String prefixString = prefix.toString().toLowerCase();

        final ArrayList<Facility> values = originalFacilities;
        final int count = values.size();

        final List<Facility> newValues = new ArrayList<>(count);

        for (int i = 0; i < count; i++) {
          final Facility item = values.get(i);
          if (item.getFacilityName().toLowerCase().contains(prefixString)) {
            newValues.add(item);
          }
        }
        results.values = newValues;
        results.count = newValues.size();
      }
      return results;
    }

    @SuppressWarnings("unchecked")
    @Override
    protected void publishResults(final CharSequence constraint, final FilterResults results) {
      if (results.values != null) {
        facilitiesList = (ArrayList<Facility>) results.values;
      } else {
        facilitiesList = new ArrayList<>();
      }
      if (results.count > 0) {
        notifyDataSetChanged();
      } else {
        notifyDataSetInvalidated();
      }
    }
  }
}