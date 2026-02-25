package com.accessvascularinc.hydroguide.mma.ui;

import android.content.Context;
import android.graphics.Color;
import androidx.core.content.ContextCompat;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;
import com.accessvascularinc.hydroguide.mma.R;

public class CustomToast {
    public enum Type {
        SUCCESS, ERROR, WARNING
    }

    public static void show(Context context, String message, Type type) {
        LayoutInflater inflater = LayoutInflater.from(context);
        View layout = inflater.inflate(R.layout.custom_toast, null);
        TextView text = layout.findViewById(R.id.toastText);
        text.setText(message);
        switch (type) {
            case SUCCESS:
                layout.setBackgroundColor(ContextCompat.getColor(context, R.color.success_toaster));
                break;
            case ERROR:
                layout.setBackgroundColor(ContextCompat.getColor(context, R.color.error_toaster));
                break;
            case WARNING:
                layout.setBackgroundColor(ContextCompat.getColor(context, R.color.warning_toaster));
                break;
        }
        Toast toast = new Toast(context);
        toast.setGravity(Gravity.BOTTOM, 0, 350);
        toast.setDuration(Toast.LENGTH_SHORT);
        toast.setView(layout);
        toast.show();
    }
}
