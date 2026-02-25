package com.accessvascularinc.hydroguide.mma.ui;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.pdf.PdfDocument;
import android.graphics.pdf.PdfRenderer;
import android.media.MediaScannerConnection;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.model.Capture;
import com.accessvascularinc.hydroguide.mma.model.DataFiles;
import com.accessvascularinc.hydroguide.mma.model.DisplayedEntryFields;
import com.accessvascularinc.hydroguide.mma.model.EncounterData;
import com.accessvascularinc.hydroguide.mma.model.UserPreferences;
import com.accessvascularinc.hydroguide.mma.model.UserProfile;
import com.accessvascularinc.hydroguide.mma.ui.input.InputUtils;
import com.accessvascularinc.hydroguide.mma.ui.plot.FadeFormatter;
import com.accessvascularinc.hydroguide.mma.ui.plot.PlotStyle;
import com.accessvascularinc.hydroguide.mma.ui.plot.PlotUtils;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.androidplot.xy.XYPlot;

import org.json.JSONException;

import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.OptionalDouble;

public class ReportPdf {
  /** Dimension For US Letter Size Paper (1 inch = 72 points) **/
  private static final int PAGE_WIDTH_PT = 612; // 8.5 Inch
  private static final int PAGE_HEIGHT_PT = 792; // 11 Inch
  private static final int PAGE_WIDTH_PX = 2720;
  private static final int PAGE_HEIGHT_PX = 3520;
  private static final float PAGE_DIMEN_SCALE = (float) PAGE_WIDTH_PT / PAGE_WIDTH_PX;
  private static final int CAPTURES_PER_ROW = 2;
  private static final int CAPTURE_ROWS_PER_PAGE = 3;
  private static final int FIRST_PAGE_INTRAV_CAPTURES = 3;
  private static final int FIRST_PAGE_INTRAV_CAPTURES_ULTRASOUND = 1;
  private static final int CAPTURES_PER_PAGE = CAPTURES_PER_ROW * CAPTURE_ROWS_PER_PAGE;

  public static void getPdfReport(final Context context,
      final EncounterData procedure,DisplayedEntryFields entryFields) throws IOException {
    final PdfDocument document = new PdfDocument();
    final PdfDocument.PageInfo pageInfo = new PdfDocument.PageInfo.Builder(PAGE_WIDTH_PT, PAGE_HEIGHT_PT, 1).create();
    final PdfDocument.Page page = document.startPage(pageInfo);
    final Canvas pageCanvas = page.getCanvas();
    final LinearLayout parentView = getParentView(context);

    parentView.addView(getProcedureDataView(parentView, procedure,entryFields));

    // Check if ultrasound captures exist - use filtered paths for report
    String[] ultrasoundPaths = procedure.getUltrasoundPathsForReport();
    boolean hasUltrasound = ultrasoundPaths != null && ultrasoundPaths.length > 0;

    // Filter intravascular captures to only include those marked to show in report
    final List<Capture> iCaptures = new ArrayList<>();
    for (Capture capture : procedure.getIntravCaptures()) {
      if (capture.getShowInReport()) {
        iCaptures.add(capture);
      }
    }

    // Get external capture if it should be shown
    final Capture externalCapture = (procedure.getExternalCapture() != null &&
        procedure.getExternalCapture().getShowInReport())
            ? procedure.getExternalCapture()
            : null;

    int completedCaptures = 0;

    if (hasUltrasound) {
      // Add ultrasound captures on first page
      for (String ultrasoundPath : ultrasoundPaths) {
        parentView.addView(getUltrasoundCaptureView(parentView, ultrasoundPath));
      }
      // only add one row
      parentView.addView(
          getTopCapturesView(parentView, externalCapture,
              (iCaptures.toArray().length == 0) ? null : iCaptures.get(0)));

      pageCanvas.drawBitmap(getBitmapFromView(parentView), 0.0f, 0.0f, null);
      pageCanvas.scale(PAGE_DIMEN_SCALE, PAGE_DIMEN_SCALE);
      parentView.draw(pageCanvas);
      document.finishPage(page);
      parentView.removeAllViews();

      completedCaptures = FIRST_PAGE_INTRAV_CAPTURES_ULTRASOUND;
    } else {
      // No ultrasound, start ECG captures on first page as normal
      parentView.addView(
          getTopCapturesView(parentView, externalCapture,
              (iCaptures.toArray().length == 0) ? null : iCaptures.get(0)));

      // There is enough to show 2 extra intravascular captures on the first page.
      if (iCaptures.size() > 1) {
        final LinearLayout capturesRow = getExtraCapturesRowView(context);
        for (int i = 1; i <= 2 && i < iCaptures.size(); i++) {
          capturesRow.addView(getCaptureView(capturesRow, iCaptures.get(i)));
        }
        parentView.addView(capturesRow);
      }

      pageCanvas.drawBitmap(getBitmapFromView(parentView), 0.0f, 0.0f, null);
      pageCanvas.scale(PAGE_DIMEN_SCALE, PAGE_DIMEN_SCALE);
      parentView.draw(pageCanvas);
      document.finishPage(page);
      parentView.removeAllViews();

      completedCaptures = FIRST_PAGE_INTRAV_CAPTURES;
    }

    // Add more pages as needed for remaining intravascular captures.
    if (iCaptures.size() > FIRST_PAGE_INTRAV_CAPTURES
        || (hasUltrasound && iCaptures.size() > FIRST_PAGE_INTRAV_CAPTURES_ULTRASOUND)) {
      // Find total page required for the pdf.
      int remainingCaptures = iCaptures.size() - completedCaptures;
      final int totalAddtlPage = (int) Math.ceil((double) remainingCaptures / CAPTURES_PER_PAGE);
      var pageNum = 0;

      // If the total item is less than pageCapacity then we take the item size as new
      // capacity.

      // We paginated the list and add them to parentView.
      while (pageNum < totalAddtlPage) {
        remainingCaptures = iCaptures.size() - completedCaptures;
        final int pageCapacity = Math.min(CAPTURES_PER_PAGE, remainingCaptures);
        parentView.addView(getMarginSpace(context));
        ++pageNum;
        List<Capture> curPageCaptures = iCaptures.subList(completedCaptures, completedCaptures + pageCapacity);
        if (pageCapacity < CAPTURES_PER_PAGE) {
          curPageCaptures = iCaptures.subList(completedCaptures, iCaptures.size());
        }

        int addtlCaptures = 0;

        for (int row = 0; row < CAPTURE_ROWS_PER_PAGE; row++) {
          final LinearLayout capturesRow = getExtraCapturesRowView(context);
          for (int col = 0; col < CAPTURES_PER_ROW; col++) {
            // int index = col + (addtlCaptures); //off-by-one
            completedCaptures++;
            if (addtlCaptures >= curPageCaptures.size()) {
              continue;
            }
            capturesRow.addView(getCaptureView(capturesRow, curPageCaptures.get(addtlCaptures)));
            addtlCaptures++;
          }
          parentView.addView(capturesRow);
        }

        // After adding items to view we render the new page and add it to pdf.
        final PdfDocument.Page dynamicPage = document.startPage(pageInfo);
        final Canvas c = dynamicPage.getCanvas();
        c.drawBitmap(getBitmapFromView(parentView), 0.0f, 0.0f, null);
        c.scale(PAGE_DIMEN_SCALE, PAGE_DIMEN_SCALE);
        parentView.draw(c);
        document.finishPage(dynamicPage);
        parentView.removeAllViews();
      }
    }
    // Must handle procedure.getDataDirPath() = "";

    Path targetLoc = null;

    if (procedure.getDataDirPath() == null) {
      // Init new directory
      final File proceduresDir = context.getExternalFilesDir(DataFiles.PROCEDURES_DIR);
      String filename = null;
      if (procedure.getStartTime() == null) {
        filename = (new SimpleDateFormat(InputUtils.FILE_DATE_TIME_PATTERN, Locale.US)).format(
            new Date());
        // No official start-time, cannot give the procedure one
      } else {
        filename = procedure.getStartTimeText();
      }

      final File procedureDir = new File(proceduresDir, filename);

      if ((proceduresDir != null) && !proceduresDir.exists()) {
        final boolean dirCreated = proceduresDir.mkdir();
        if (dirCreated) {
          Toast.makeText(context, "Procedures directory created successfully.",
              Toast.LENGTH_SHORT).show();
        } else {
          Toast.makeText(context, "Procedures directory creation failed. File not saved.",
              Toast.LENGTH_SHORT).show();
          return;
        }
      }

      procedureDir.mkdir();

      final File procedureFile = new File(procedureDir, DataFiles.PROCEDURE_DATA);
      procedureFile.createNewFile();
      procedure.setDataDirPath(procedureDir.getAbsolutePath());

      try (final FileWriter writer = new FileWriter(procedureFile, StandardCharsets.UTF_8)) {
        final String[] data = procedure.getProcedureDataText();

        for (final String str : data) {
          Log.d("ABC", "3str: " + str);
          writer.write(str);
          Log.d("ABC", "lineSeparator: " + System.lineSeparator());
          writer.write(System.lineSeparator());
          writer.flush();
        }

        MediaScannerConnection.scanFile(context, new String[] { procedureFile.getAbsolutePath() },
            new String[] {}, null);
      } catch (final IOException | JSONException e) {
        MedDevLog.error("ReportPdf", "Error writing procedure file", e);
        Toast.makeText(context, e.getMessage(), Toast.LENGTH_SHORT).show();
        throw new RuntimeException(e);
      }
      targetLoc = procedureDir.toPath();

    } else {
      targetLoc = Paths.get(procedure.getDataDirPath());
    }

    final File pdfFile = new File(targetLoc.toFile(), DataFiles.REPORT_PDF);
    // pdfFile.createNewFile();
    final FileOutputStream fos = new FileOutputStream(pdfFile);
    document.writeTo(fos);
    document.close();
    fos.close();
    MedDevLog.audit("PDF Generation", "Procedure Report Created");
  }

  private static LinearLayout getParentView(final Context context) {
    final LinearLayout parentView = new LinearLayout(context);
    final ViewGroup.LayoutParams params = new ViewGroup.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT,
        ViewGroup.LayoutParams.MATCH_PARENT);
    parentView.setLayoutParams(params);
    parentView.setOrientation(LinearLayout.VERTICAL);
    return parentView;
  }

  private static View getProcedureDataView(final ViewGroup parent, final EncounterData procedure,DisplayedEntryFields entryFields) {
    final TextView tvSwVer, tvFwVer, tvDate, tvPatientName, tvId, tvDob, tvClinicianName, tvInsVein,
        tvPatientSide, tvTrimLength, tvExtLength, tvTipConf, tvState, tvNotes, tvCatheterSize, tvNoOfLumens,
            tvVeinSize,tvArmCircumference,tvReasonForInsertion;
    final View view = LayoutInflater.from(parent.getContext())
        .inflate(R.layout.procedure_report_layout, null);

    tvSwVer = view.findViewById(R.id.software_version);
    tvFwVer = view.findViewById(R.id.firmware_version);
    tvDate = view.findViewById(R.id.report_insertion_date);
    tvPatientName = view.findViewById(R.id.report_patient_name);
    tvId = view.findViewById(R.id.report_patient_id);
    tvDob = view.findViewById(R.id.report_patient_dob);
    tvClinicianName = view.findViewById(R.id.report_clinician_name);
    tvInsVein = view.findViewById(R.id.report_insertion_vein);
    tvPatientSide = view.findViewById(R.id.report_insertion_side);
    tvTrimLength = view.findViewById(R.id.report_trim_length);
    tvExtLength = view.findViewById(R.id.report_ext_length);
    tvTipConf = view.findViewById(R.id.report_tip_location);
    tvState = view.findViewById(R.id.report_completion_sts);
    tvNotes = view.findViewById(R.id.notes);
    tvCatheterSize = view.findViewById(R.id.report_catheter_size);
    tvNoOfLumens = view.findViewById(R.id.report_no_of_lumens);
    tvVeinSize = view.findViewById(R.id.tvVeinSize);
    tvArmCircumference = view.findViewById(R.id.tvArmCircumference);
    tvReasonForInsertion = view.findViewById(R.id.tvReasonForInsertion);

    tvSwVer.setText(procedure.getAppSoftwareVersion());
    tvFwVer.setText(procedure.getControllerFirmwareVersion());
    tvDate.setText(procedure.getStartTimeShortText());
    tvPatientName.setText(procedure.getPatient().getPatientName());
    tvId.setText(procedure.getPatient().getPatientId());
    tvDob.setText(procedure.getPatient().getPatientDob());
    tvClinicianName.setText(procedure.getPatient().getPatientInserter());
    tvInsVein.setText(procedure.getPatient().getPatientInsertionVein());
    tvPatientSide.setText(procedure.getPatient().getPatientInsertionSide());
    switch (procedure.getState()) {
      case CompletedSuccessful -> tvState.setText("Concluded Sucessfully");
      case ConcludedIncomplete -> tvState.setText("Abandoned Procedure");
      case CompletedUnsuccessful -> tvState.setText("Concluded Unsucessfully");
      default -> tvState.setText("Inconclusive");
    }
    if (procedure.getHydroGuideConfirmed() == null) {
      tvTipConf.setText("Tip location verification method unknown");
    } else {
      final String confirmTxt = procedure.getHydroGuideConfirmed() ? "CONFIRMED" : "NOT CONFIRMED";
      tvTipConf.setText("Tip location " + confirmTxt + " using HydroGUIDE");
    }
    final OptionalDouble trimLength = procedure.getTrimLengthCm();
    tvTrimLength.setText(
        trimLength.isPresent() ? String.format("%.2f cm", trimLength.getAsDouble()) : "");
    final OptionalDouble extLength = procedure.getExternalCatheterCm();
    tvExtLength.setText(
        extLength.isPresent() ? String.format("%.2f cm", extLength.getAsDouble()) : "");
    tvNotes.setText(procedure.getPatient().getPatientNotes());

    // Set catheter size
    String catheterSize = procedure.getPatient().getPatientCatheterSize();
    tvCatheterSize.setText((catheterSize != null && !catheterSize.isEmpty()) ? catheterSize : "N/A");

    // Set number of lumens
    String noOfLumens = procedure.getPatient().getPatientNoOfLumens();
    tvNoOfLumens.setText((noOfLumens != null && !noOfLumens.isEmpty()) ? noOfLumens : "N/A");

    final OptionalDouble veinLength = procedure.getPatient().getPatientVeinSize();
    tvVeinSize.setText(veinLength.isPresent() ? String.format("%.2f mm", veinLength.getAsDouble()) : "");

    final OptionalDouble circumferenceLength = procedure.getPatient().getPatientArmCircumference();
    tvArmCircumference.setText(circumferenceLength.isPresent() ? String.format("%.2f cm", circumferenceLength.getAsDouble()) : "");

    String reasonForInsertion = procedure.getPatient().getPatientReasonInsertion();
    tvReasonForInsertion.setText((reasonForInsertion != null && !reasonForInsertion.isEmpty()) ? reasonForInsertion : "N/A");

    ReportPdf.renderReportFields(view.findViewById(R.id.report_patient_name_grp),entryFields.isShowName());
    ReportPdf.renderReportFields(view.findViewById(R.id.report_patient_id_grp),entryFields.isShowId());
    ReportPdf.renderReportFields(view.findViewById(R.id.report_patient_dob_grp),entryFields.isShowDob());
    ReportPdf.renderReportFields(view.findViewById(R.id.lnrReasonForInsertion),entryFields.isShowReasonForInsertion());
    ReportPdf.renderReportFields(view.findViewById(R.id.report_insertion_vein_grp),entryFields.isShowInsertionVein());
    ReportPdf.renderReportFields(view.findViewById(R.id.report_insertion_side_grp),entryFields.isShowPatientSide());
    ReportPdf.renderReportFields(view.findViewById(R.id.lnrArmCircumference),entryFields.isShowArmCircumference());
    ReportPdf.renderReportFields(view.findViewById(R.id.lnrVeinSizeContainer),entryFields.isShowVeinSize());
    ReportPdf.renderReportFields(view.findViewById(R.id.report_no_of_lumens_grp),entryFields.isShowNoOfLumens());
    ReportPdf.renderReportFields(view.findViewById(R.id.report_catheter_size_grp),entryFields.isShowCatheterSize());
    return view;
  }

  private static View getTopCapturesView(final ViewGroup parent, final Capture extCapture,
      final Capture intravCapture) {
    final View view = LayoutInflater.from(parent.getContext())
        .inflate(R.layout.report_top_captures_layout, null);

    // Only show external capture if it's selected (extCapture != null means it's
    // selected)
    if (extCapture != null && extCapture.getCaptureId() != -1) {
      createCaptureView(view.findViewById(R.id.report_external_capture), extCapture);
      view.findViewById(R.id.report_external_capture).setVisibility(View.VISIBLE);
    } else {
      view.findViewById(R.id.report_external_capture).setVisibility(View.GONE);
    }

    // Only show intravascular capture if it's selected
    if (intravCapture != null && intravCapture.getCaptureId() != -1) {
      createCaptureView(view.findViewById(R.id.report_top_intrav_capture), intravCapture);
      view.findViewById(R.id.report_top_intrav_capture).setVisibility(View.VISIBLE);
    } else {
      view.findViewById(R.id.report_top_intrav_capture).setVisibility(View.GONE);
    }

    // Hide additional intravascular title if no intravascular capture shown
    if (intravCapture == null || intravCapture.getCaptureId() == -1) {
      view.findViewById(R.id.report_additional_intrav_title).setVisibility(View.GONE);
    }

    return view;
  }

  private static View getUltrasoundCaptureView(final ViewGroup parent, final String imagePath) {
    final Context context = parent.getContext();
    final Resources resources = context.getResources();
    final int pageMargin = (int) resources.getDimension(R.dimen.PDF_PAGE_MARGIN);

    final LinearLayout container = new LinearLayout(context);
    container.setOrientation(LinearLayout.VERTICAL);
    container.setLayoutParams(new ViewGroup.LayoutParams(
        PAGE_WIDTH_PX,
        ViewGroup.LayoutParams.WRAP_CONTENT));
    container.setPadding(pageMargin, 0, pageMargin, 0);

    // Add title
    final TextView titleView = new TextView(context);
    titleView.setText(R.string.ultrasound_capture_label);
    titleView.setTextColor(Color.BLACK);
    titleView.setTextSize(20);
    titleView.setTypeface(null, Typeface.BOLD);
    titleView.setGravity(Gravity.CENTER);
    titleView.setPadding(0, 20, 0, 10);
    container.addView(titleView);

    // Add image
    final ImageView imageView = new ImageView(context);
    final Bitmap bitmap = android.graphics.BitmapFactory.decodeFile(imagePath);
    if (bitmap != null) {
      // Crop empty pixels before displaying
      final Bitmap croppedBitmap = cropEmptyPixels(bitmap);
      imageView.setImageBitmap(croppedBitmap);
      imageView.setScaleType(ImageView.ScaleType.FIT_CENTER);
      imageView.setAdjustViewBounds(true);

      final LinearLayout.LayoutParams imageParams = new LinearLayout.LayoutParams(
          ViewGroup.LayoutParams.MATCH_PARENT,
          ViewGroup.LayoutParams.WRAP_CONTENT);
      imageParams.setMargins(0, 0, 0, 20);
      imageView.setLayoutParams(imageParams);
      container.addView(imageView);
      
      // Recycle original bitmap if it's different from cropped
      if (croppedBitmap != bitmap) {
        bitmap.recycle();
      }
    }

    return container;
  }

  /**
   * Crops empty (transparent or near-white) pixels from the edges of a bitmap.
   * 
   * @param bitmap The original bitmap
   * @return A new cropped bitmap, or the original if no cropping is needed
   */
  private static Bitmap cropEmptyPixels(final Bitmap bitmap) {
    if (bitmap == null) {
      return null;
    }

    final int width = bitmap.getWidth();
    final int height = bitmap.getHeight();
    
    int top = 0;
    int bottom = height;
    int left = 0;
    int right = width;

    // Find top boundary
    topLoop:
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        if (!isEmptyPixel(bitmap.getPixel(x, y))) {
          top = y;
          break topLoop;
        }
      }
    }

    // Find bottom boundary
    bottomLoop:
    for (int y = height - 1; y >= top; y--) {
      for (int x = 0; x < width; x++) {
        if (!isEmptyPixel(bitmap.getPixel(x, y))) {
          bottom = y + 1;
          break bottomLoop;
        }
      }
    }

    // Find left boundary
    leftLoop:
    for (int x = 0; x < width; x++) {
      for (int y = top; y < bottom; y++) {
        if (!isEmptyPixel(bitmap.getPixel(x, y))) {
          left = x;
          break leftLoop;
        }
      }
    }

    // Find right boundary
    rightLoop:
    for (int x = width - 1; x >= left; x--) {
      for (int y = top; y < bottom; y++) {
        if (!isEmptyPixel(bitmap.getPixel(x, y))) {
          right = x + 1;
          break rightLoop;
        }
      }
    }

    // Check if cropping is needed
    if (top == 0 && left == 0 && bottom == height && right == width) {
      return bitmap; // No cropping needed
    }

    // Ensure valid dimensions
    if (right <= left || bottom <= top) {
      return bitmap; // Invalid crop bounds, return original
    }

    // Create cropped bitmap
    return Bitmap.createBitmap(bitmap, left, top, right - left, bottom - top);
  }

  /**
   * Determines if a pixel is considered "empty" (transparent or near-white).
   * 
   * @param pixel The pixel color value
   * @return true if the pixel is empty, false otherwise
   */
  private static boolean isEmptyPixel(final int pixel) {
    final int alpha = Color.alpha(pixel);
    final int red = Color.red(pixel);
    final int green = Color.green(pixel);
    final int blue = Color.blue(pixel);

    // Consider pixel empty if it's transparent or very light (near white)
    final boolean isTransparent = alpha < 10;
    final boolean isNearWhite = red > 245 && green > 245 && blue > 245;
    
    return isTransparent || isNearWhite;
  }

  private static LinearLayout getExtraCapturesRowView(final Context context) {
    final LinearLayout parentView = new LinearLayout(context);
    final ViewGroup.LayoutParams params = new ViewGroup.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT,
        ViewGroup.LayoutParams.WRAP_CONTENT);
    final Resources resources = context.getResources();
    final int pageMargin = (int) resources.getDimension(R.dimen.PDF_PAGE_MARGIN);
    final int rowMargin = (int) resources.getDimension(R.dimen.pdf_captures_row_vertical_margin);

    parentView.setLayoutParams(params);
    parentView.setPadding(pageMargin, rowMargin, pageMargin, rowMargin);
    parentView.setGravity(Gravity.CENTER_HORIZONTAL);
    parentView.setOrientation(LinearLayout.HORIZONTAL);
    return parentView;
  }

  private static View getCaptureView(final ViewGroup parent, final Capture capture) {
    final View capView = LayoutInflater.from(parent.getContext())
        .inflate(R.layout.report_capture_simple_black, null);
    final Resources resources = parent.getContext().getResources();
    final ViewGroup.MarginLayoutParams params = new ViewGroup.MarginLayoutParams(
        (int) resources.getDimension(R.dimen.pdf_capture_width),
        (int) resources.getDimension(R.dimen.pdf_capture_height));
    params.setMarginStart((int) resources.getDimension(R.dimen.pdf_top_captures_horizontal_margin));
    params.setMarginEnd((int) resources.getDimension(R.dimen.pdf_top_captures_horizontal_margin));
    capView.setLayoutParams(params);
    createCaptureView(capView, capture);
    return capView;
  }

  private static void createCaptureView(final View captureView, final Capture capture) {
    final TextView tvCapId, tvInsertedLength, tvExposedLength;
    final ImageView ivCaptureIcon;
    final XYPlot captureGraph;

    tvCapId = captureView.findViewById(R.id.report_capture_id);
    ivCaptureIcon = captureView.findViewById(R.id.report_capture_icon);
    tvInsertedLength = captureView.findViewById(R.id.report_capture_inserted_length);
    tvExposedLength = captureView.findViewById(R.id.report_capture_exposed_length);
    captureGraph = captureView.findViewById(R.id.report_capture_graph);

    if (capture == null || capture.getCaptureId() == -1) {
      return;
    }

    tvCapId.setText(
        capture.getIsIntravascular() ? String.format("%s", capture.getCaptureId()) : "");
    ivCaptureIcon.setImageResource(
        capture.getIsIntravascular() ? R.drawable.logo_capture_internal : R.drawable.logo_external);
    final String newInsText = String.format(Locale.US, "%.1f", capture.getInsertedLengthCm());
    final String newExtText = String.format(Locale.US, "%.1f", capture.getExposedLengthCm());
    tvInsertedLength.setText(String.format("%s cm", newInsText));
    tvExposedLength.setText(String.format("%s cm", newExtText));

    // Pass in formatter for black and white version, with adjusted styling
    final FadeFormatter oldFormat = capture.getFormatter();
    capture.setFormatter(PlotUtils.getFormatter(captureView.getContext(), PlotStyle.Report, 0));
    PlotUtils.drawCaptureGraph(captureGraph, capture);
    PlotUtils.stylePlot(captureGraph);
    capture.setFormatter(oldFormat);
  }

  private static LinearLayout getMarginSpace(final Context context) {
    final LinearLayout parentView = new LinearLayout(context);
    final ViewGroup.LayoutParams params = new ViewGroup.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT,
        ViewGroup.LayoutParams.WRAP_CONTENT);
    final Resources resources = context.getResources();
    parentView.setLayoutParams(params);
    parentView.setPadding(0, (int) resources.getDimension(R.dimen.PDF_PAGE_MARGIN), 0, 0);
    return parentView;
  }

  private static Bitmap getBitmapFromView(final View view) {
    final int width = View.MeasureSpec.makeMeasureSpec(PAGE_WIDTH_PX, View.MeasureSpec.EXACTLY);
    final int height = View.MeasureSpec.makeMeasureSpec(PAGE_HEIGHT_PX, View.MeasureSpec.EXACTLY);
    view.measure(width, height);
    final int reqWidth = view.getMeasuredWidth();
    final int reqHeight = view.getMeasuredHeight();

    final Bitmap bitmap = Bitmap.createBitmap(reqWidth, reqHeight, Bitmap.Config.ARGB_8888);
    final Canvas bitmapCanvas = new Canvas(bitmap);
    bitmapCanvas.drawColor(Color.WHITE);
    view.layout(view.getLeft(), view.getTop(), view.getRight(), view.getBottom());
    return bitmap;
  }

  public static List<Bitmap> getBitmapsFromPdf(final File pdfFile) {
    final List<Bitmap> pdfBitmaps;
    try {
      final ParcelFileDescriptor fileDescriptor = ParcelFileDescriptor.open(pdfFile,
          ParcelFileDescriptor.MODE_READ_ONLY);
      final PdfRenderer pdfRenderer = new PdfRenderer(fileDescriptor);
      final int pageCount = pdfRenderer.getPageCount();
      pdfBitmaps = new ArrayList<>(pageCount);

      for (int pageIdx = 0; pageIdx < pageCount; pageIdx++) {
        final PdfRenderer.Page rendererPage = pdfRenderer.openPage(pageIdx);
        final int rendererPageWidth = rendererPage.getWidth();
        final int rendererPageHeight = rendererPage.getHeight();
        final Bitmap bitmap = Bitmap.createBitmap(PAGE_WIDTH_PX, PAGE_HEIGHT_PX, Bitmap.Config.ARGB_8888);
        rendererPage.render(bitmap, null, null, PdfRenderer.Page.RENDER_MODE_FOR_DISPLAY);
        pdfBitmaps.add(bitmap);
        rendererPage.close();
      }

      pdfRenderer.close();
      fileDescriptor.close();

    } catch (final IOException e) {
      throw new RuntimeException(e);
    }
    return pdfBitmaps;
  }

  private static void renderReportFields(View view,boolean isVisible)
  {
    if(isVisible)
    {
      view.setVisibility(View.VISIBLE);
    }
    else
    {
      view.setVisibility(View.INVISIBLE);
    }
  }
}
