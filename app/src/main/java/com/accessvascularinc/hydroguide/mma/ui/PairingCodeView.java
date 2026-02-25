package com.accessvascularinc.hydroguide.mma.ui;

import android.content.Context;
import android.graphics.Typeface;
import android.util.AttributeSet;
import android.view.Gravity;
import android.widget.Button;
import android.widget.LinearLayout;

import androidx.annotation.Nullable;

public class PairingCodeView extends LinearLayout {

  public PairingCodeView(Context context, String[] directions) {
    super(context);
    init(context, directions);
  }

  public PairingCodeView(Context context, @Nullable AttributeSet attrs) {
    super(context, attrs);
  }

  private void init(Context context, String[] directions) {
    setOrientation(HORIZONTAL);
    setGravity(Gravity.CENTER);
    int buttonSize = (int) (64 * getResources().getDisplayMetrics().density); // 48dp
    //int margin = (int) (8 * getResources().getDisplayMetrics().density); // 8dp
    for (String dir : directions) {
      Button btn = new Button(context);
      switch (dir.trim()) {
        case "UP":
          btn.setText("↑");
          break;
        case "RIGHT":
          btn.setText("→");
          break;
        case "DOWN":
          btn.setText("↓");
          break;
        case "LEFT":
          btn.setText("←");
          break;
        default:
          btn.setText("?");
      }
      btn.setEnabled(false); // Display only
      btn.setTextSize(32);
      btn.setTypeface(null, Typeface.BOLD); // Make text bold

      // Set fixed size and margin for each button
      LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(buttonSize, buttonSize);
      //params.setMargins(margin, margin, margin, margin);
      btn.setLayoutParams(params);

      addView(btn);
    }
  }
}