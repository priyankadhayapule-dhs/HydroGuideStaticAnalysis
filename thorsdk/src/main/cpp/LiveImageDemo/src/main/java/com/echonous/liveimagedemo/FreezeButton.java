package com.echonous.liveimagedemo;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.ToggleButton;

//-------------------------------------------------------------------------
public class FreezeButton extends ToggleButton {
    private boolean mIgnoreChecked = false;

    public FreezeButton(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }
    
    public FreezeButton(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public FreezeButton(Context context, AttributeSet attrs) {
        super(context, attrs);
    }
    
    public FreezeButton(Context context) {
        super(context);
    }

    public void setSilentChecked(boolean checked) {
        mIgnoreChecked = true;
        setChecked(checked);
        mIgnoreChecked = false;
    }

    public boolean ignoreChecked() {
        return mIgnoreChecked;
    }
};

