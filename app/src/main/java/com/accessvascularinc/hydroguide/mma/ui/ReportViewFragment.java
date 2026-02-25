package com.accessvascularinc.hydroguide.mma.ui;

import android.graphics.Bitmap;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.activity.OnBackPressedCallback;
import androidx.databinding.DataBindingUtil;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.Navigation;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentReportViewBinding;
import com.accessvascularinc.hydroguide.mma.model.Capture;
import com.accessvascularinc.hydroguide.mma.model.DataFiles;
import com.accessvascularinc.hydroguide.mma.model.EncounterData;
import com.accessvascularinc.hydroguide.mma.model.SoftwareUpdateSource;
import com.accessvascularinc.hydroguide.mma.ui.settings.BrightnessChangeFragment;
import com.accessvascularinc.hydroguide.mma.ui.settings.VolumeChangeFragment;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import java.io.File;
import java.io.IOException;
import java.util.List;

public class ReportViewFragment extends BaseHydroGuideFragment {
  private PageViewListAdapter pageViewListAdapter = null;
  private final View.OnClickListener homeBtn_OnClickListener = v -> {
    if (syncManager != null) {
      syncManager.setBlockSync(false);
    }
    syncManager.syncAll();
    viewmodel.setProcedureReport(null);
    Navigation.findNavController(requireView()).navigate(R.id.nav_report_view_to_records);
  };
  private final View.OnClickListener backBtn_OnClickListener = v -> {
    viewmodel.setProcedureReport(null);
    Navigation.findNavController(requireView()).popBackStack();
  };

  private final View.OnClickListener volumeBtn_OnClickListener = v -> {
    VolumeChangeFragment bottomSheet = new VolumeChangeFragment();
    bottomSheet.show(getChildFragmentManager(), "TEST");
  };
  private final View.OnClickListener brightnessBtn_OnClickListener = v -> {
    BrightnessChangeFragment bottomSheet = new BrightnessChangeFragment();
    bottomSheet.show(getChildFragmentManager(), "TEST");
  };

  public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container,
                           final Bundle savedInstanceState) {
    final FragmentReportViewBinding binding =
            DataBindingUtil.inflate(inflater, R.layout.fragment_report_view, container, false);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    setupVolumeAndBrightnessListeners();
    final EncounterData record = viewmodel.getProcedureReport();

    MedDevLog.audit("Report", "Viewing report for encounter: " + record.getStartTime());
    logPrinter(record);

    final double scale_factor = (1.0 / 1048576.0); // Byte -> MegaByte
    final int mb_available =
            (int) ((viewmodel.getTabletState().getFreeSpaceBytes()) * scale_factor);
    final int mb_init = (int) ((viewmodel.getTabletState().getTotalSpaceBytes()) * scale_factor);
    final TextView spaceRemaining = binding.freeByteValue;
    spaceRemaining.setText(mb_available + " MB / " + mb_init + " MB");

    binding.setProcedureReport(record);
    binding.setControllerState(viewmodel.getControllerState());
    binding.setTabletState(viewmodel.getTabletState());
    binding.reportViewTopBar.setTabletState(viewmodel.getTabletState());

    // Hide satisfaction text and checkboxes if this is a cloud report (use MainViewModel flag)
    if (viewmodel != null && viewmodel.getRecordsIsCloudMode()) {
      if (binding.reportSatisfactionLabel != null) binding.reportSatisfactionLabel.setVisibility(View.GONE);
      if (binding.reportSatisfactionRadioGroup != null) binding.reportSatisfactionRadioGroup.setVisibility(View.GONE);
    }

    final RecyclerView pdfViewer = binding.reportViewer;

    final String recordPath = record.getDataDirPath() + File.separator + DataFiles.REPORT_PDF;    
    final File reportPdfFile = new File(recordPath);

    // Generate a report PDF if it does not already exist.
    if (!reportPdfFile.isFile() || !reportPdfFile.exists()) {
      try {
        if(viewmodel != null)
        {
         if(viewmodel.getProfileState() != null)
         {
           if(viewmodel.getProfileState().getSelectedProfile() != null)
           {
             if(viewmodel.getProfileState().getSelectedProfile().getUserPreferences() != null)
             {
               if(viewmodel.getProfileState().getSelectedProfile().getUserPreferences().getDisplayedEntryFields() != null)
               {
                 ReportPdf.getPdfReport(requireContext(), record,viewmodel.getProfileState().getSelectedProfile().getUserPreferences().getDisplayedEntryFields());
               }
             }
           }
         }
        }
      } catch (final IOException e) {
        throw new RuntimeException(e);
      }
    }

    triggerSync();

    pdfViewer.setLayoutManager(
            new LinearLayoutManager(requireContext(), LinearLayoutManager.VERTICAL, false));
    pageViewListAdapter = new PageViewListAdapter(ReportPdf.getBitmapsFromPdf(reportPdfFile));
    pdfViewer.setAdapter(pageViewListAdapter);

    tvBattery = binding.controllerBatteryLevelPct;
    ivBattery = binding.remoteBatteryLevelIndicator;
    ivUsbIcon = binding.usbIcon;
    ivTabletBattery = binding.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.tabletBatteryLevelPct;

    binding.navToHomeScreen.setOnClickListener(homeBtn_OnClickListener);
    binding.backButtonReview.setOnClickListener(backBtn_OnClickListener);
    binding.brightnessBtn.setOnClickListener(brightnessBtn_OnClickListener);
    binding.volumeBtn.setOnClickListener(volumeBtn_OnClickListener);

    final Bundle args = getArguments();
    if (args != null)
    {
      if(args.getBoolean(getResources().getString(R.string.report_isSubmitted)))
      {
       binding.home.setVisibility(View.INVISIBLE);
      }
      else
      {
        binding.home.setVisibility(View.VISIBLE);
      }
    }

    return binding.getRoot();
  }

  private void logPrinter(EncounterData record) {
    Log.d("ABC", "2getStartTime: "+record.getStartTime());
    Log.d("ABC", "getState: "+record.getState());
    Log.d("ABC", "getPatient: "+record.getPatient());
    Log.d("ABC", "getExternalCapture: "+record.getExternalCapture());
    Log.d("ABC", "getIntravCaptures: "+record.getIntravCaptures());
    Log.d("ABC", "getTrimLengthCm: "+record.getTrimLengthCm());
    Log.d("ABC", "getInsertedCatheterCm: "+ record.getInsertedCatheterCm());
    Log.d("ABC", "getExternalCatheterCm: "+ record.getExternalCatheterCm());
    Log.d("ABC", "getHydroGuideConfirmed: "+ record.getHydroGuideConfirmed());
    Log.d("ABC", "getAppSoftwareVersion: "+ record.getAppSoftwareVersion());
    Log.d("ABC", "getControllerFirmwareVersion: "+ record.getControllerFirmwareVersion());
    Log.d("ABC", "getProcedureStarted: "+ record.getProcedureStarted());
    Log.d("ABC", "getDataDirPath: "+ record.getDataDirPath());

  }

  private static final class PageViewListAdapter extends RecyclerView.Adapter<PageViewHolder> {
    private final List<Bitmap> pdfBitmaps;

    private PageViewListAdapter(final List<Bitmap> bitmaps) {
      pdfBitmaps = bitmaps;
    }

    @NonNull
    @Override
    public PageViewHolder onCreateViewHolder(@NonNull final ViewGroup parent, final int position) {
      return PageViewHolder.create(parent);
    }

    @Override
    public void onBindViewHolder(@NonNull final PageViewHolder viewHolder, final int position) {
      final Bitmap bitmap = pdfBitmaps.get(position);
      viewHolder.mImg.setImageBitmap(bitmap);
    }

    @Override
    public int getItemCount() {
      return pdfBitmaps.size();
    }
  }

  private static final class PageViewHolder extends RecyclerView.ViewHolder {
    public ImageView mImg;

    private PageViewHolder(final View pageView) {
      super(pageView);
      mImg = pageView.findViewById(R.id.pdf_page_image);
    }

    @NonNull
    public static PageViewHolder create(@NonNull final ViewGroup parent) {
      return new PageViewHolder(LayoutInflater.from(parent.getContext())
              .inflate(R.layout.pdf_viewer_page, parent, false));
    }
  }
}
