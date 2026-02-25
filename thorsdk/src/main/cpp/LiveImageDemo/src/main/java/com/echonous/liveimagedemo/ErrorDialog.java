package com.echonous.liveimagedemo;

import android.app.Activity;
import android.app.Dialog;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

import com.echonous.hardware.ultrasound.ThorError;
import com.echonous.hardware.ultrasound.DauException;

public class ErrorDialog extends Dialog implements android.view.View.OnClickListener {
    private static String TAG = "ErrorDialog";

    private Activity mActivity;
    private Button mClose;
    private ThorError mThorError;
    private boolean mDoExit = false;
    private TextView mErrorCodeCtrl;
    private TextView mErrorMsgCtrl;

    public ErrorDialog(Activity activity) {
        super(activity);

        mActivity = activity;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        setContentView(R.layout.error_dialog);
        mClose = (Button) findViewById(R.id.close_button);
        mClose.setOnClickListener(this);

        mErrorCodeCtrl = (TextView) findViewById(R.id.error_code);
        if (null == mErrorCodeCtrl) {
            LogUtils.d(TAG, "onCreate: mErrorCodeCtrl is null");
            return;
        }
        else {
            String  hexCode = Integer.toHexString(mThorError.getCode()).toUpperCase();
            mErrorCodeCtrl.setText("Error Code: 0x" + hexCode);
        }

        mErrorMsgCtrl = (TextView) findViewById(R.id.error_message);
        if (null == mErrorMsgCtrl) {
            LogUtils.d(TAG, "onCreate: mErrorMsgCtrl is null");
            return;
        }
        else {
            mErrorMsgCtrl.setText(mThorError.getDescription());
        }
    }

    @Override
    public void onClick(View v) {
        if (v.getId() == R.id.close_button) {
            dismiss();
            if (mDoExit) {
                mActivity.finish();
            }
        }
    }

    public void setError(ThorError error, boolean doExit) {
        mThorError = error;
        mDoExit = doExit;
    }

    public void setException(DauException exception, boolean doExit) {
        mThorError = exception.getError();
        mDoExit = doExit;
    }
}
