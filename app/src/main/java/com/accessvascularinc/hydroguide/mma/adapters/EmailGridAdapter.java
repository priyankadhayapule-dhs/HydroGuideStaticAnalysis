package com.accessvascularinc.hydroguide.mma.adapters;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.TextView;

import java.util.List;

public class EmailGridAdapter extends BaseAdapter {
  private final Context context;
  private final List<String> emails;
  private int selectedPosition = -1;
  private OnEmailSelectedListener listener;

  public interface OnEmailSelectedListener {
    void onEmailSelected(String email, int position);
  }

  public EmailGridAdapter(Context context, List<String> emails) {
    this.context = context;
    this.emails = emails;
  }

  public void setOnEmailSelectedListener(OnEmailSelectedListener listener) {
    this.listener = listener;
  }

  @Override
  public int getCount() {
    return emails.size();
  }

  @Override
  public Object getItem(int position) {
    return emails.get(position);
  }

  @Override
  public long getItemId(int position) {
    return position;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent) {
    // Create horizontal container with radio button on left and text on right
    LinearLayout container = new LinearLayout(context);
    container.setOrientation(LinearLayout.HORIZONTAL);
    container.setLayoutParams(new ViewGroup.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT));
    container.setPadding(8, 8, 8, 8);
    container.setGravity(android.view.Gravity.CENTER_VERTICAL);

    // Create RadioButton
    RadioButton radioButton = new RadioButton(context);
    radioButton.setChecked(position == selectedPosition);
    // Set radio button color to white
    ColorStateList colorStateList = ColorStateList.valueOf(Color.WHITE);
    radioButton.setButtonTintList(colorStateList);
    radioButton.setEnabled(false); // Disable to prevent direct interaction
    LinearLayout.LayoutParams radioParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.WRAP_CONTENT,
            LinearLayout.LayoutParams.WRAP_CONTENT);
    radioParams.setMarginEnd(8);
    radioButton.setLayoutParams(radioParams);

    // Create TextView for email
    TextView emailText = new TextView(context);
    emailText.setText(emails.get(position));
    emailText.setTextColor(Color.WHITE);
    emailText.setTextSize(20);
    emailText.setMaxLines(2);
    emailText.setEllipsize(android.text.TextUtils.TruncateAt.END);
    LinearLayout.LayoutParams textParams = new LinearLayout.LayoutParams(
            0,
            LinearLayout.LayoutParams.WRAP_CONTENT,
            1);
    emailText.setLayoutParams(textParams);

    container.addView(radioButton);
    container.addView(emailText);

    // Handle click on the entire cell
    final int pos = position;
    container.setOnClickListener(v -> {
      int previousPosition = selectedPosition;
      selectedPosition = pos;
      notifyDataSetChanged();
      if (listener != null) {
        listener.onEmailSelected(emails.get(pos), pos);
      }
    });

    return container;
  }
}
