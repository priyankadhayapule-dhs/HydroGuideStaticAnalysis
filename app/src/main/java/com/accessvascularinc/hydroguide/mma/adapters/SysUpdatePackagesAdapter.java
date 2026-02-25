package com.accessvascularinc.hydroguide.mma.adapters;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.utils.Utility;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class SysUpdatePackagesAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder>
{
  private Context context;
  private List<File> lstItems = new ArrayList();
  private DataRecordsAdapter.DataRecordsClickListener recordsClickListener;

  public SysUpdatePackagesAdapter(Context context, List<File> lstItems,DataRecordsAdapter.DataRecordsClickListener recordsClickListener)
  {
    this.context = context;
    this.lstItems = lstItems;
    this.recordsClickListener = recordsClickListener;
  }

  @NonNull
  @Override
  public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
      return new SysUpdatePackagesAdapter.SysUpdatePackageViewHolder(LayoutInflater.from(parent.getContext()).inflate(R.layout.row_sys_updates_packages, parent, false));
  }

  @Override
  public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position)
  {
    SysUpdatePackageViewHolder itemViewHolder = (SysUpdatePackageViewHolder)holder;
    itemViewHolder.imgPackageInstallIcon.setImageResource(lstItems.get(holder.getBindingAdapterPosition()).getName().endsWith(Utility.PKG_FILE_EXTENSION_APP) ? R.drawable.ic_package : R.drawable.icon_pkg_install_controller);
    itemViewHolder.buildName.setText(lstItems.get(holder.getBindingAdapterPosition()).getName());
    if (lstItems.get(holder.getBindingAdapterPosition()).getName().endsWith(Utility.PKG_FILE_EXTENSION_CTRL))
    {
      itemViewHolder.btnInstall.setText(context.getResources().getString(R.string.extract));
    }
    else
    {
      itemViewHolder.btnInstall.setText(context.getResources().getString(R.string.install));
    }
    itemViewHolder.btnInstall.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        recordsClickListener.onDataRecordClick(position);
      }
    });
  }

  @Override
  public int getItemCount()
  {
    return lstItems.size();
  }

  private class SysUpdatePackageViewHolder extends RecyclerView.ViewHolder
  {
    ImageView imgPackageInstallIcon;
    TextView buildName;
    Button btnInstall;
    public SysUpdatePackageViewHolder(@NonNull final View view)
    {
      super(view);
      this.imgPackageInstallIcon = view.findViewById(R.id.imgPackageInstallIcon);
      this.buildName = view.findViewById(R.id.buildName);
      this.btnInstall = view.findViewById(R.id.btnInstall);
    }
  }
}