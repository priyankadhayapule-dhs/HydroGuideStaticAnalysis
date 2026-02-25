package com.accessvascularinc.hydroguide.mma.exceptions;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import androidx.activity.OnBackPressedCallback;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentActivity;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.ui.MainActivity;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.Utility;

public class ErrorDispatcherActivity extends FragmentActivity
{
  private TextView tvErrorType,tvErrorReferenceId,tvStackTraceText;
  private Button btnOk;

  @Override
  protected void onCreate(@Nullable final Bundle bundle)
  {
    super.onCreate(bundle);
    setContentView(R.layout.fragment_error_reporter);
    tvErrorType = findViewById(R.id.tvErrorType);
    tvErrorReferenceId = findViewById(R.id.tvErrorReferenceId);
    tvStackTraceText = findViewById(R.id.tvStackTraceText);
    btnOk = findViewById(R.id.btnOk);
    Intent readCrashIntent = getIntent();
    if(readCrashIntent != null)
    {
      if(readCrashIntent.hasExtra(Utility.KEY_CRASH_LOGS))
      {
        String uuid = Utility.generateErrorId();
        tvErrorType.setText(readCrashIntent.getStringExtra(Utility.KEY_CRASH_TYPE));
        tvErrorReferenceId.setText(uuid);
        tvStackTraceText.setText(readCrashIntent.getStringExtra(Utility.KEY_CRASH_LOGS));
        Throwable ex = readCrashIntent.getSerializableExtra(Utility.KEY_CRASH_TRACE,Throwable.class);
        MedDevLog.error(uuid,"",ex);
      }
    }
    btnOk.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        restartApp();
      }
    });
    getOnBackPressedDispatcher().addCallback(handleBackPress);
  }

  private void restartApp()
  {
    Intent restartIntent = new Intent(ErrorDispatcherActivity.this, MainActivity.class);
    restartIntent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
    restartIntent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);
    restartIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    startActivity(restartIntent);
  }

  private OnBackPressedCallback handleBackPress = new OnBackPressedCallback(true)
  {
    @Override
    public void handleOnBackPressed()
    {
    }
  };
}