package com.accessvascularinc.hydroguide.mma.ui.input;

import android.graphics.Color;
import android.graphics.PorterDuff;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.model.Facility;

public class FacilityListItemViewHolder extends RecyclerView.ViewHolder {
  public final CheckBox cbSelectFacility;
  private final TextView tvFacilityName;
  private final ImageView ivFavoriteIcon;
  private int rowColor = Color.TRANSPARENT;

  public FacilityListItemViewHolder(@NonNull final View view) {
    super(view);
    cbSelectFacility = itemView.findViewById(R.id.facility_list_item_checkbox);
    ivFavoriteIcon = itemView.findViewById(R.id.facility_list_item_icon);
    this.tvFacilityName = itemView.findViewById(R.id.facility_list_item_text);
  }

  @NonNull
  public static FacilityListItemViewHolder create(@NonNull final ViewGroup parent) {
    return new FacilityListItemViewHolder(LayoutInflater.from(parent.getContext())
            .inflate(R.layout.facility_list_item, parent, false));
  }

  public void setFacility(final Facility facility, final boolean isFavorite) {
    tvFacilityName.setText(facility.getFacilityName());
    ivFavoriteIcon.setVisibility(isFavorite ? View.VISIBLE : View.GONE);
  }

  public void setFacilityChecked(final boolean isChecked) {
    cbSelectFacility.setChecked(isChecked);
    toggleRowColors();
  }

  public void setRowColor(final int newRowColor) {
    this.rowColor = newRowColor;
    itemView.setBackgroundColor(newRowColor);
  }

  public void toggleRowColors() {
    int bgColor = rowColor;
    int textColor = ContextCompat.getColor(itemView.getContext(), R.color.white);

    if (cbSelectFacility.isChecked()) {
      bgColor = ContextCompat.getColor(itemView.getContext(), R.color.av_gray);
      textColor = ContextCompat.getColor(itemView.getContext(), R.color.av_darkest_blue);
    }

    itemView.setBackgroundColor(bgColor);
    ivFavoriteIcon.setColorFilter(textColor, PorterDuff.Mode.MULTIPLY);
    tvFacilityName.setTextColor(textColor);
  }
}