package com.accessvascularinc.hydroguide.mma.adapters;

import android.app.Activity;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.fragment.app.FragmentActivity;

import com.accessvascularinc.hydroguide.mma.model.DashboardGridItems;
import java.util.ArrayList;
import java.util.List;
import com.accessvascularinc.hydroguide.mma.R;

public class DashboardGridAdapter extends ArrayAdapter {
  private Context context;
  private int layoutResourceId;
  private List<DashboardGridItems> lstItems = new ArrayList();

  public DashboardGridAdapter(Context context, int layoutResourceId, List<DashboardGridItems> lstItems) {
    super(context, layoutResourceId, lstItems);
    this.layoutResourceId = layoutResourceId;
    this.context = context;
    this.lstItems = lstItems;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent) {
    View row = convertView;
    ViewHolder holder = null;
    boolean isViewRecycled = row != null;

    if (row == null) {
      LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
      row = inflater.inflate(layoutResourceId, parent, false);
      holder = new ViewHolder();
      holder.imgDashboardIcon = (ImageView) row.findViewById(R.id.imgDashboardIcon);
      holder.tvCaption = (TextView) row.findViewById(R.id.tvCaption);
      row.setTag(holder);
    } else {
      holder = (ViewHolder) row.getTag();
    }

    DashboardGridItems item = lstItems.get(position);
    holder.imgDashboardIcon.setImageResource(item.getIcon());
    holder.tvCaption.setText(item.getCaption());

    // Set alpha based on enabled/disabled state
    if (!item.isEnabled()) {
      holder.imgDashboardIcon.setAlpha(0.5f);
    } else {
      holder.imgDashboardIcon.setAlpha(1.0f);
    }
    return row;
  }

  static class ViewHolder {
    ImageView imgDashboardIcon;
    TextView tvCaption;
  }
}