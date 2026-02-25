package com.echonous.hardware.ultrasound;

public class UsbProbeData {
    private int vid;
    private int pid;
    private int probeType;

    public int getVid() {
        return vid;
    }

    public void setVid(int vid) {
        this.vid = vid;
    }

    public int getPid() {
        return pid;
    }

    public void setPid(int pid) {
        this.pid = pid;
    }

    public void setProbeType(int probeType) {
        this.probeType = probeType;
    }

    public int getProbeType() {
        return probeType;
    }
}
