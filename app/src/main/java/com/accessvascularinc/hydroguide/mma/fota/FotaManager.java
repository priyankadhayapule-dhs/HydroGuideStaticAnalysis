package com.accessvascularinc.hydroguide.mma.fota;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.TextView;

import com.accessvascularinc.hydroguide.mma.R;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.zip.CRC32;

public class FotaManager {
    
    private static final String TAG = "FotaManager";
    
    private final FotaClient client;
    private final ExecutorService executor;
    private final Handler mainHandler;
    private volatile boolean cancelled = false;
    private final String PHASE_INIT = "INIT";
    private final String PHASE_NEGOTIATE = "NEGOTIATE";
    private final String PHASE_PREPARE = "PREPARE";
    private final String PHASE_TRANSFER = "TRANSFER";
    private final String PHASE_VERIFY = "VERIFY";
    private final String PHASE_INSTALL = "INSTALL";
    private final String PHASE_FINALIZE = "FINALIZE";

    public FotaManager(FotaClient client) {
        this.client = client;
        this.executor = Executors.newSingleThreadExecutor();
        this.mainHandler = new Handler(Looper.getMainLooper());
    }

    public void setVerbose(boolean verbose) {
        client.setVerbose(verbose);
    }

    public void cancel() {
        cancelled = true;
    }

    public boolean isCancelled() {
        return cancelled;
    }

    public FotaClient getClient() {
        return client;
    }

    public static int calculateCrc32(byte[] data) {
        CRC32 crc = new CRC32();
        crc.update(data);
        return (int) (crc.getValue() & 0xFFFFFFFFL);
    }

    public boolean transferSync(Context context, byte[] firmwareData, int imageId, FotaProgressCallback callback)
            throws FotaException, IOException {
        
        cancelled = false;
        long startTime = System.currentTimeMillis();
        int firmwareSize = firmwareData.length;
        int firmwareCrc32 = calculateCrc32(firmwareData);
        
        String currentPhase = PHASE_INIT;
        
        try {

            currentPhase = PHASE_NEGOTIATE;
            notifyPhaseStarted(callback, currentPhase, context.getResources().getString(R.string.fota_phase_mesg_negotiate));
            checkCancelled();
            
            FotaStatusResponse status = client.negotiateSession(imageId, FotaConstants.MAX_MTU);
            notifyPhaseCompleted(callback, currentPhase, "Negotiated MTU: " + client.getNegotiatedMtu() + " bytes");
            

            currentPhase = PHASE_PREPARE;
            notifyPhaseStarted(callback, currentPhase, context.getResources().getString(R.string.fota_phase_mesg_prepare_device));
            checkCancelled();
            
            client.sendMetadata(imageId, firmwareSize, firmwareCrc32);
            notifyPhaseCompleted(callback, currentPhase,context.getResources().getString(R.string.fota_phase_mesg_device_prepared));
            

            currentPhase = PHASE_TRANSFER;
            notifyPhaseStarted(callback, currentPhase,context.getResources().getString(R.string.fota_phase_mesg_transfer_firmware));
            
            client.sendChunks(firmwareData, new FotaProgressCallback() {
                @Override
                public void onPhaseStarted(String phase, String message) {}
                
                @Override
                public void onPhaseCompleted(String phase, String message) {}
                
                @Override
                public void onTransferProgress(int bytesSent, int totalBytes) {
                    if (cancelled) {
                        return;
                    }
                    notifyTransferProgress(callback, bytesSent, totalBytes);
                }
                
                @Override
                public void onSuccess(long elapsedTimeMs, float speedKBps) {}
                
                @Override
                public void onError(String phase, FotaException error) {}
            });
            
            checkCancelled();
            notifyPhaseCompleted(callback, currentPhase,context.getResources().getString(R.string.fota_phase_mesg_transfer_complete));
            

            currentPhase = PHASE_VERIFY;
            notifyPhaseStarted(callback, currentPhase, context.getResources().getString(R.string.fota_phase_mesg_verify_firmware));
            checkCancelled();
            
            client.verifyImage(imageId);
            notifyPhaseCompleted(callback, currentPhase,context.getResources().getString(R.string.fota_phase_mesg_verify_success));
            

            currentPhase = PHASE_INSTALL;
            notifyPhaseStarted(callback, currentPhase, context.getResources().getString(R.string.fota_phase_mesg_install_firmware));
            checkCancelled();
            
            client.installImage(imageId);
            notifyPhaseCompleted(callback, currentPhase,context.getResources().getString(R.string.fota_phase_mesg_installation_started));
            

            currentPhase = PHASE_FINALIZE;
            notifyPhaseStarted(callback, currentPhase, context.getResources().getString(R.string.fota_phase_mesg_finalize_firmware));
            
            boolean acknowledged = false;
            try {
                FotaStatusResponse ackStatus = client.endSession(true, FotaConstants.END_ACK_TIMEOUT_MS);
                if (ackStatus != null) {
                    acknowledged = true;
                }
            } catch (FotaException e) {
                e.printStackTrace();
            }
            
            notifyPhaseCompleted(callback, currentPhase, acknowledged ? context.getResources().getString(R.string.fota_phase_mesg_firmware_state_complete_device_ack) : context.getResources().getString(R.string.fota_phase_mesg_firmware_state_complete_pending_device_cnf));
            

            long elapsedMs = System.currentTimeMillis() - startTime;
            float speedKBps = (firmwareSize / 1024.0f) / (elapsedMs / 1000.0f);
            
            notifySuccess(callback, elapsedMs, speedKBps);
            
            return true;
            
        } catch (FotaException e) {
            notifyError(callback, currentPhase, e);
            throw e;
        } catch (IOException e) {
            FotaException dfuError = new FotaException(currentPhase, "I/O error: " + e.getMessage(), e);
            notifyError(callback, currentPhase, dfuError);
            throw e;
        }
    }

    public void transferAsync(Context context,byte[] firmwareData, int imageId, FotaProgressCallback callback) {
        executor.execute(() -> {
            try {
                transferSync(context,firmwareData, imageId, wrapForMainThread(callback));
            } catch (FotaException | IOException e) {
                Log.e(TAG, "DFU transfer failed", e);
                // Error already notified via callback
            }
        });
    }

    public void shutdown() {
        executor.shutdown();
    }
    
    private void checkCancelled() throws FotaException {
        if (cancelled) {
            throw new FotaException("DFU operation cancelled by user");
        }
    }
    
    private void notifyPhaseStarted(FotaProgressCallback callback, String phase, String message) {
        if (callback != null) {
            callback.onPhaseStarted(phase, message);
        }
    }
    
    private void notifyPhaseCompleted(FotaProgressCallback callback, String phase, String message) {
        if (callback != null) {
            callback.onPhaseCompleted(phase, message);
        }
    }
    
    private void notifyTransferProgress(FotaProgressCallback callback, int bytesSent, int totalBytes) {
        if (callback != null) {
            callback.onTransferProgress(bytesSent, totalBytes);
        }
    }
    
    private void notifySuccess(FotaProgressCallback callback, long elapsedTimeMs, float speedKBps) {
        if (callback != null) {
            callback.onSuccess(elapsedTimeMs, speedKBps);
        }
    }
    
    private void notifyError(FotaProgressCallback callback, String phase, FotaException error) {
        if (callback != null) {
            callback.onError(phase, error);
        }
    }

    private FotaProgressCallback wrapForMainThread(FotaProgressCallback callback) {
        if (callback == null) {
            return null;
        }
        
        return new FotaProgressCallback() {
            @Override
            public void onPhaseStarted(String phase, String message) {
                mainHandler.post(() -> callback.onPhaseStarted(phase, message));
            }
            
            @Override
            public void onPhaseCompleted(String phase, String message) {
                mainHandler.post(() -> callback.onPhaseCompleted(phase, message));
            }
            
            @Override
            public void onTransferProgress(int bytesSent, int totalBytes) {
                mainHandler.post(() -> callback.onTransferProgress(bytesSent, totalBytes));
            }
            
            @Override
            public void onSuccess(long elapsedTimeMs, float speedKBps) {
                mainHandler.post(() -> callback.onSuccess(elapsedTimeMs, speedKBps));
            }
            
            @Override
            public void onError(String phase, FotaException error) {
                mainHandler.post(() -> callback.onError(phase, error));
            }
        };
    }

    public interface DFUResultCallback
    {
        void onResult(boolean success, FotaException error);
    }

    public static byte[] loadFirmware(File file) throws IOException
    {
        try (FileInputStream fis = new FileInputStream(file))
        {
            return readAllBytes(fis);
        }
    }

    private static byte[] readAllBytes(InputStream is) throws IOException
    {
        ByteArrayOutputStream buffer = new ByteArrayOutputStream();
        byte[] data = new byte[4096];
        int bytesRead;
        while ((bytesRead = is.read(data)) != -1)
        {
            buffer.write(data, 0, bytesRead);
        }
        return buffer.toByteArray();
    }
}
