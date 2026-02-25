package com.accessvascularinc.hydroguide.mma.ui;

import android.app.Dialog;
import android.content.Context;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;

import com.accessvascularinc.hydroguide.mma.R;

public class ConfirmDialog {
    public interface ConfirmCallback {
        void onResult(boolean confirmed);
    }

    public static Dialog show(Context context, String message, ConfirmCallback callback) {
        Dialog dialog = new Dialog(context);
        View view = LayoutInflater.from(context).inflate(R.layout.confirm_dialog, null);
        TextView msgView = view.findViewById(R.id.dialog_message);
        Button yesBtn = view.findViewById(R.id.yes_button);
        Button noBtn = view.findViewById(R.id.no_button);
        msgView.setText(message);
        yesBtn.setBackgroundColor(0xFF333333);
        yesBtn.setTextColor(0xFFFFFFFF);
        yesBtn.setBackgroundTintList(null);
        noBtn.setBackgroundColor(0xFF333333);
        noBtn.setTextColor(0xFFFFFFFF);
        noBtn.setBackgroundTintList(null);
        yesBtn.setOnClickListener(v -> {
            callback.onResult(true);
            dialog.dismiss();
        });
        noBtn.setOnClickListener(v -> {
            callback.onResult(false);
            dialog.dismiss();
        });
        dialog.setContentView(view);
        dialog.setCancelable(false);
        dialog.show();
        return dialog;
    }

    public static Dialog show(Context context, String message, String yesButtonText, String noButtonText, ConfirmCallback callback) {
        Dialog dialog = new Dialog(context);
        View view = LayoutInflater.from(context).inflate(R.layout.confirm_dialog, null);
        TextView msgView = view.findViewById(R.id.dialog_message);
        Button yesBtn = view.findViewById(R.id.yes_button);
        Button noBtn = view.findViewById(R.id.no_button);
        msgView.setText(message);
        yesBtn.setText(yesButtonText);
        noBtn.setText(noButtonText);
        yesBtn.setBackgroundColor(0xFF333333);
        yesBtn.setTextColor(0xFFFFFFFF);
        yesBtn.setBackgroundTintList(null);
        noBtn.setBackgroundColor(0xFF333333);
        noBtn.setTextColor(0xFFFFFFFF);
        noBtn.setBackgroundTintList(null);
        yesBtn.setOnClickListener(v -> {
            callback.onResult(true);
            dialog.dismiss();
        });
        noBtn.setOnClickListener(v -> {
            callback.onResult(false);
            dialog.dismiss();
        });
        dialog.setContentView(view);
        dialog.setCancelable(false);
        dialog.show();
        return dialog;
    }

    public static Dialog singleActionPopup(Context context, String message,String buttonCaption, ConfirmCallback callback,double popupWidth,double popupHeight) {
        Dialog dialog = new Dialog(context);
        View view = LayoutInflater.from(context).inflate(R.layout.confirm_dialog, null);
        TextView msgView = view.findViewById(R.id.dialog_message);
        Button yesBtn = view.findViewById(R.id.yes_button);
        Button noBtn = view.findViewById(R.id.no_button);
        msgView.setText(message);
        yesBtn.setBackgroundColor(0xFF333333);
        yesBtn.setTextColor(0xFFFFFFFF);
        yesBtn.setBackgroundTintList(null);
        noBtn.setBackgroundColor(0xFF333333);
        noBtn.setTextColor(0xFFFFFFFF);
        noBtn.setBackgroundTintList(null);
        yesBtn.setText(buttonCaption);
        yesBtn.setOnClickListener(v -> {
            callback.onResult(true);
            dialog.dismiss();
        });
        noBtn.setOnClickListener(v -> {
            callback.onResult(false);
            dialog.dismiss();
        });
        noBtn.setVisibility(View.GONE);
        dialog.setContentView(view);
        dialog.setCancelable(false);
        //dialog.show();

        DisplayMetrics displayMetrics = new DisplayMetrics();
        dialog.getWindow().getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams();
        layoutParams.copyFrom(dialog.getWindow().getAttributes());
        layoutParams.width = (int) (displayMetrics.widthPixels * 0.7f);
        //layoutParams.height = (int) (displayMetrics.heightPixels * 0.3f);
        dialog.getWindow().setAttributes(layoutParams);

        return dialog;
    }

    public static Dialog patientFilterPopup(Dialog dialog,Context context)
    {
        dialog.setContentView(LayoutInflater.from(context).inflate(R.layout.popup_filter_patient_records, null));
        setupPopupDisplayMetrics(dialog);
        return dialog;
    }

    public static Dialog dateRangePopup(Context context) {
        Dialog dialog = new Dialog(context);
        View view = LayoutInflater.from(context).inflate(R.layout.popup_dates_selector, null);

        Button btnCancel = view.findViewById(R.id.btnCancel);
        btnCancel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dialog.dismiss();
            }
        });
        dialog.setContentView(view);
        setupPopupDisplayMetrics(dialog);
        return dialog;
    }

    private static void setupPopupDisplayMetrics(Dialog dialog)
    {
        dialog.setCancelable(false);
        DisplayMetrics displayMetrics = new DisplayMetrics();
        dialog.getWindow().getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams();
        layoutParams.copyFrom(dialog.getWindow().getAttributes());
        layoutParams.width = (int) (displayMetrics.widthPixels * 0.7f);
        //layoutParams.height = (int) (displayMetrics.heightPixels * 0.3f);
        dialog.getWindow().setAttributes(layoutParams);
    }
}
