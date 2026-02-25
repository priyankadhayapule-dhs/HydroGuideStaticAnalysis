package com.echonous.hardware.ultrasound;

public interface AutoControlListener
{
    void onAutoOrgan(int organId);
    void onAutoDepth(int depthIndex);
    void onAutoGain(int gainVal);
}