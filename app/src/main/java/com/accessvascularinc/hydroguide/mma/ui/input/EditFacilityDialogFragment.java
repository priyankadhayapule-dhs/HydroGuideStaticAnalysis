package com.accessvascularinc.hydroguide.mma.ui.input;

import android.app.Dialog;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.EditText;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.databinding.DataBindingUtil;
import androidx.fragment.app.DialogFragment;
import androidx.lifecycle.ViewModelProvider;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.EditFacilityLayoutBinding;
import com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase;
import com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity;
import com.accessvascularinc.hydroguide.mma.db.repository.FacilityRepository;
import com.accessvascularinc.hydroguide.mma.model.Facility;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.DbHelper;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.Objects;
import java.util.UUID;
import java.util.function.Consumer;
import java.util.stream.IntStream;

public class EditFacilityDialogFragment extends DialogFragment {
  private MainViewModel viewmodel = null;
  private EditText etFacilityName = null;
  private Facility facility = null;
  private Consumer<Facility> onNewFacilityRbl = null;
  private boolean isNewFacility = false;
  private HydroGuideDatabase db;

  private final View.OnClickListener confirmBtn_OnClickListener = view -> confirmFacility();

  // Empty constructor will result in the "Add New Facility" version of this
  // dialog.
  public EditFacilityDialogFragment() {
    this.facility = getNewFacilityWithId();
    this.isNewFacility = true;
  }

  public EditFacilityDialogFragment(final Consumer<Facility> onNewFacility) {
    this.facility = getNewFacilityWithId();
    this.onNewFacilityRbl = onNewFacility;
    this.isNewFacility = true;
  }

  public EditFacilityDialogFragment(final Facility newFacility) {
    this.facility = newFacility;
    this.isNewFacility = false;
  }

  @Override
  public View onCreateView(@NonNull final LayoutInflater inflater,
      @Nullable final ViewGroup container,
      @Nullable final Bundle savedInstanceState) {
    db = HydroGuideDatabase.buildDatabase(requireContext());
    final EditFacilityLayoutBinding binding = DataBindingUtil.inflate(inflater, R.layout.edit_facility_layout,
        container, false);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    Log.d("ABC", "7: ");
    binding.setEncounterData(viewmodel.getEncounterData());

    etFacilityName = binding.facilityNameInput;
    etFacilityName.setText(facility.getFacilityName());

    binding.title.setText(
        isNewFacility ? R.string.title_add_facility : R.string.title_edit_facility);

    binding.confirmBtn.setOnClickListener(confirmBtn_OnClickListener);
    binding.cancelButton.setOnClickListener(view -> dismiss());

    return binding.getRoot();
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(final Bundle savedInstanceState) {
    final Dialog dialog = super.onCreateDialog(savedInstanceState);
    dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
    Objects.requireNonNull(dialog.getWindow()).setBackgroundDrawable(
        ContextCompat.getDrawable(requireContext(), R.drawable.capture_dialog));
    return dialog;
  }

  private void confirmFacility() {
    // Don't allow saving if text is empty.
    if (!etFacilityName.getText().toString().isEmpty()) {
      final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireActivity());
      builder.setMessage(getString(R.string.pref_action_warning_text));
      builder.setNegativeButton(getString(R.string.pref_action_warning_no), (dialog, id) -> {
      });
      builder.setPositiveButton(getString(R.string.pref_action_warning_yes), (dialog, id) -> {
        facility.setFacilityName(etFacilityName.getText().toString());
        FacilityRepository facilityDetailsRepository = new FacilityRepository(
            db.facilityDao());
        if (isNewFacility) {
          FacilityEntity facilityDetails = new FacilityEntity(
              facility.getFacilityName(), // facilityName
              "", // organizationId
              facility.getDateLastUsed(), // dateLastUsed
              0, // createdBy (Integer, placeholder)
              new SimpleDateFormat(InputUtils.FILE_DATE_TIME_PATTERN, Locale.US).format(new Date()), // createdOn
              true, // isActive
              false, // showPatientName
              false, // showInsertionVein
              false, // showPatientId
              false, // showPatientSide
              false, // showPatientDob
              false, // showArmCircumference
              false, // showVeinSize
              false, // showReasonForInsertion
              false, // showNoOfLumens
              false, // showCatheterSize
              "test@maildrop.cc" // createdByEmail
          );
          DbHelper.insertFacilityData(facilityDetailsRepository, facilityDetails);
          insertFacility(facility);
          MedDevLog.audit("Facility", "Facility created: " + facility.getFacilityName());
        } else {
          DbHelper.updateFacilityInDatabase(facility, facilityDetailsRepository);
          updateFacility(facility);
          MedDevLog.audit("Facility", "Facility modified: " + facility.getFacilityName());
        }
        dismiss();
      });
      builder.show();
    }
  }

  private Facility getNewFacilityWithId() {
    final String newId = UUID.randomUUID().toString();
    return new Facility(newId, "", "");
  }

  private void updateFacility(final Facility curFacility) {
    final List<Facility> facilities = new ArrayList<>(Arrays.asList(viewmodel.getProfileState().getFacilityList()));
    final int facilityIdx = IntStream.range(0, facilities.size())
        .filter(i -> (Objects.equals(curFacility.getId(), facilities.get(i).getId())))
        .findFirst().orElse(-1);
    if (facilityIdx >= 0) {
      facilities.set(facilityIdx, curFacility);
      viewmodel.getProfileState().setFacilityList(facilities.toArray(new Facility[0]));
    }
  }

  private void insertFacility(final Facility newFacility) {
    final List<Facility> facilities = new ArrayList<>(Arrays.asList(viewmodel.getProfileState().getFacilityList()));
    facilities.add(newFacility);
    viewmodel.getProfileState().setFacilityList(facilities.toArray(new Facility[0]));

    if (onNewFacilityRbl != null) {
      onNewFacilityRbl.accept(newFacility);
    }
  }
}
