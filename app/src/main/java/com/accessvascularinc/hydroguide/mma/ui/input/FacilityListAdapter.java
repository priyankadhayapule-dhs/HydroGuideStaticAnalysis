package com.accessvascularinc.hydroguide.mma.ui.input;

import android.graphics.Color;
import android.support.annotation.NonNull;
import android.view.ViewGroup;

import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.model.Facility;
import com.accessvascularinc.hydroguide.mma.model.SortMode;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.stream.IntStream;

public class FacilityListAdapter extends RecyclerView.Adapter<FacilityListItemViewHolder> {
  private final ArrayList<Facility> selectedFacilities = new ArrayList<>();
  private ArrayList<Facility> facilities = new ArrayList<>();
  private ArrayList<Facility> originalFacilities = new ArrayList<>();
  private String[] favoriteIds = new String[0];

  public FacilityListAdapter(final ArrayList<Facility> list, final String[] favIds) {
    facilities = new ArrayList<>(list);
    originalFacilities = new ArrayList<>(facilities);
    favoriteIds = favIds.clone();
  }

  @NonNull
  @Override
  public FacilityListItemViewHolder onCreateViewHolder(@NonNull final ViewGroup parent,
                                                       final int position) {
    return FacilityListItemViewHolder.create(parent);
  }
  public void setFacilities(List<Facility> newFacilities) {
    this.facilities.clear();
    this.facilities.addAll(newFacilities);
  }
  @Override
  public void onBindViewHolder(@NonNull final FacilityListItemViewHolder vh, final int position) {
    final Facility facility = facilities.get(position);
    final boolean isFav = Arrays.stream(favoriteIds).anyMatch(x -> (Objects.equals(x,
            facility.getId())));
    vh.setFacility(facility, isFav);

    // Alternate row colors for better readability.
    if ((position % 2) != 0) {
      vh.setRowColor(ContextCompat.getColor(vh.itemView.getContext(), R.color.av_alt_light_blue));
    } else {
      vh.setRowColor(Color.TRANSPARENT);
    }

    vh.setFacilityChecked(selectedFacilities.contains(facility));

    vh.itemView.setOnClickListener(view -> vh.cbSelectFacility.toggle());
    vh.cbSelectFacility.setOnCheckedChangeListener((compoundButton, isChecked) -> {
      final Facility selectedFacility = facilities.get(vh.getBindingAdapterPosition());
      if (isChecked) {
        if (!selectedFacilities.contains(selectedFacility)) {
          selectedFacilities.add(selectedFacility);
        }
      } else {
        selectedFacilities.remove(facilities.get(vh.getBindingAdapterPosition()));
      }

      vh.toggleRowColors();
    });
  }

  @Override
  public int getItemCount() {
    return facilities.size();
  }

  public void removeFacilityById(final String id) {
    final int index = getIdxFromId(id);
    facilities.remove(index);
    this.notifyItemRemoved(index);
  }

  public void updateSavedFacilities(final Facility[] newFacilities) {
    facilities = new ArrayList<>(Arrays.asList(newFacilities));
    originalFacilities = new ArrayList<>(facilities);
    this.notifyDataSetChanged();
  }

  public void updateFavorites(final String[] newFavorites) {
    favoriteIds = newFavorites.clone();
    this.notifyDataSetChanged();
  }

  public void selectAllFacilities() {
    selectedFacilities.clear();
    selectedFacilities.addAll(facilities);
    this.notifyDataSetChanged();

  }

  public void deselectAllFacilities() {
    selectedFacilities.clear();
    this.notifyDataSetChanged();
  }

  public Facility[] getSelectedFacilities() {
    return selectedFacilities.toArray(new Facility[0]);
  }

  public void sortFacilities(final SortMode sortMode) {
    facilities.sort(new InputUtils.FacilitySortingComparator(favoriteIds, sortMode));
    this.notifyDataSetChanged();
  }

  private int getIdxFromId(final String id) {
    return IntStream.range(0, facilities.size())
            .filter(i -> (Objects.equals(id, facilities.get(i).getId())))
            .findFirst() // first occurrence
            .orElse(-1); // No element found
  }
}