package com.accessvascularinc.hydroguide.mma.model;

public class MeasurementModel {
    
    private float startXPhysical;
    private float startYPhysical;
    private float endXPhysical;
    private float endYPhysical;
    

    private String measurementId;
    private long timestamp;
    private String notes;
    
    public MeasurementModel() {
        this.startXPhysical = 0f;
        this.startYPhysical = 0f;
        this.endXPhysical = 0f;
        this.endYPhysical = 0f;
        this.timestamp = System.currentTimeMillis();
    }
    
    public MeasurementModel(float startX, float startY, float endX, float endY) {
        this.startXPhysical = startX;
        this.startYPhysical = startY;
        this.endXPhysical = endX;
        this.endYPhysical = endY;
        this.timestamp = System.currentTimeMillis();
    }
    
    public float getStartXPhysical() {
        return startXPhysical;
    }
    
    public void setStartXPhysical(float startXPhysical) {
        this.startXPhysical = startXPhysical;
    }
    
    public float getStartYPhysical() {
        return startYPhysical;
    }
    
    public void setStartYPhysical(float startYPhysical) {
        this.startYPhysical = startYPhysical;
    }
    
    public float getEndXPhysical() {
        return endXPhysical;
    }
    
    public void setEndXPhysical(float endXPhysical) {
        this.endXPhysical = endXPhysical;
    }
    
    public float getEndYPhysical() {
        return endYPhysical;
    }
    
    public void setEndYPhysical(float endYPhysical) {
        this.endYPhysical = endYPhysical;
    }
    
    public String getMeasurementId() {
        return measurementId;
    }
    
    public void setMeasurementId(String measurementId) {
        this.measurementId = measurementId;
    }
    
    public long getTimestamp() {
        return timestamp;
    }
    
    public void setTimestamp(long timestamp) {
        this.timestamp = timestamp;
    }
    
    public String getNotes() {
        return notes;
    }
    
    public void setNotes(String notes) {
        this.notes = notes;
    }
    
    public float calculateDistanceMm() {
        float dx = endXPhysical - startXPhysical;
        float dy = endYPhysical - startYPhysical;
        return (float) Math.sqrt(dx * dx + dy * dy);
    }
    
    public float calculateDistanceCm() {
        return calculateDistanceMm() / 10.0f;
    }
    
    public String getFormattedDistance() {
        return String.format("%.2f cm", calculateDistanceCm());
    }
    
    public String getFormattedDistanceMm() {
        return String.format("%.1f mm", calculateDistanceMm());
    }
    
    public boolean isValid(float thresholdMm) {
        return calculateDistanceMm() > thresholdMm;
    }
    
    public boolean isValid() {
        return isValid(1.0f);
    }
    
    public MeasurementModel clone() {
        MeasurementModel copy = new MeasurementModel(
            startXPhysical, startYPhysical, 
            endXPhysical, endYPhysical
        );
        copy.setMeasurementId(measurementId);
        copy.setTimestamp(timestamp);
        copy.setNotes(notes);
        return copy;
    }
    
    @Override
    public String toString() {
        return String.format(
            "Measurement[id=%s, start=(%.2f, %.2f)mm, end=(%.2f, %.2f)mm, distance=%.2fmm]",
            measurementId != null ? measurementId : "null",
            startXPhysical, startYPhysical,
            endXPhysical, endYPhysical,
            calculateDistanceMm()
        );
    }
    
    public String toJsonString() {
        return String.format(
            "{\"startX\":%.4f,\"startY\":%.4f,\"endX\":%.4f,\"endY\":%.4f," +
            "\"timestamp\":%d,\"notes\":\"%s\",\"distanceMm\":%.4f}",
            startXPhysical, startYPhysical,
            endXPhysical, endYPhysical,
            timestamp,
            notes != null ? notes : "",
            calculateDistanceMm()
        );
    }
}
