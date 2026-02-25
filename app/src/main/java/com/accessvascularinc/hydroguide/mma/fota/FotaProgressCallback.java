package com.accessvascularinc.hydroguide.mma.fota;

public interface FotaProgressCallback {

    void onPhaseStarted(String phase, String message);

    void onPhaseCompleted(String phase, String message);

    void onTransferProgress(int bytesSent, int totalBytes);

    void onSuccess(long elapsedTimeMs, float speedKBps);

    void onError(String phase, FotaException error);

    abstract class SimpleCallback implements FotaProgressCallback {
        @Override
        public void onPhaseStarted(String phase, String message) {
        }
        
        @Override
        public void onPhaseCompleted(String phase, String message) {
        }
        
        @Override
        public void onSuccess(long elapsedTimeMs, float speedKBps) {
        }
        
        @Override
        public void onError(String phase, FotaException error) {
        }
    }
}
