package com.accessvascularinc.hydroguide.mma.ultrasound;

public class RoiConstraints {

    private Float maxRoiAzimuthSpan;
    private Float minRoiAzimuthSpan;
    private Float minRoiAxialSpan;
    private Float maxRoiAxialSpan;
    private Float minRoiAxialStart;

    public RoiConstraints(){
        // Empty constructor
    }

    public Float getMaxRoiAzimuthSpan() {
        return maxRoiAzimuthSpan;
    }

    public RoiConstraints setMaxRoiAzimuthSpan(Float maxRoiAzimuthSpan) {
        this.maxRoiAzimuthSpan = maxRoiAzimuthSpan;
        return this;
    }

    public Float getMinRoiAzimuthSpan() {
        return minRoiAzimuthSpan;
    }

    public RoiConstraints setMinRoiAzimuthSpan(Float minRoiAzimuthSpan) {
        this.minRoiAzimuthSpan = minRoiAzimuthSpan;
        return this;
    }

    public Float getMinRoiAxialSpan() {
        return minRoiAxialSpan;
    }

    public RoiConstraints setMinRoiAxialSpan(Float minRoiAxialSpan) {
        this.minRoiAxialSpan = minRoiAxialSpan;
        return this;
    }

    public Float getMaxRoiAxialSpan() {
        return maxRoiAxialSpan;
    }

    public RoiConstraints setMaxRoiAxialSpan(Float maxRoiAxialSpan) {
        this.maxRoiAxialSpan = maxRoiAxialSpan;
        return this;
    }

    public Float getMinRoiAxialStart() {
        return minRoiAxialStart;
    }

    public RoiConstraints setMinRoiAxialStart(Float minRoiAxialStart) {
        this.minRoiAxialStart = minRoiAxialStart;
        return this;
    }
}
