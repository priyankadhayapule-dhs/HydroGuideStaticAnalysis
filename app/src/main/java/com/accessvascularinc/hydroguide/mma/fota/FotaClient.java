package com.accessvascularinc.hydroguide.mma.fota;

import android.os.Handler;
import android.os.Looper;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class FotaClient
{
    private static final String TAG = "Logs";
    private InputStream inputStream;
    private OutputStream outputStream;
    private int negotiatedMtu = FotaConstants.DEFAULT_MTU;
    private final InterfaceProtocol.PacketStream packetStream;
    private boolean verbose = false;
    public FotaClient() {
        this.packetStream = new InterfaceProtocol.PacketStream();
        //executorService = Executors.newSingleThreadScheduledExecutor();
    }

    public void setStreams(InputStream inputStream, OutputStream outputStream) {
        this.inputStream = inputStream;
        this.outputStream = outputStream;
    }
    public void setVerbose(boolean verbose) {
        this.verbose = verbose;
    }

    public int getNegotiatedMtu() {
        return negotiatedMtu;
    }

    public void clearBuffer() {
        packetStream.clear();
    }

    public void sendKeepalive() throws IOException
    {
        byte[] encoded = InterfaceProtocol.encodeFrame(InterfaceProtocol.PacketType.KEEP_ALIVE, InterfaceProtocol.PacketSubtype.COMMAND, new byte[0]);
        outputStream.write(encoded);
        outputStream.flush();
        if (verbose)
        {
            pushLogger("KEEPALIVE sent " + encoded.length + " bytes");
        }
    }

    public void sendPacket(byte[] payload, String label) throws IOException
    {
        byte[] encoded = InterfaceProtocol.encodeFrame(InterfaceProtocol.PacketType.DFU, InterfaceProtocol.PacketSubtype.COMMAND, payload);
        if (verbose)
        {
            pushLogger("TX (" + label + "): " + encoded.length + " bytes: " + bytesToHex(encoded));
        }
        outputStream.write(encoded);
        outputStream.flush();
    }

    public FotaStatusResponse waitForResponse(double timeout, double keepaliveInterval) throws FotaException, IOException
    {
        long startTime = System.currentTimeMillis();
        long lastKeepalive = System.currentTimeMillis();
        long timeoutMs = (long) (timeout * 1000);
        long keepaliveMs = (long) (keepaliveInterval * 100);
        packetStream.clear();
        byte[] readBuffer = new byte[256];

        if (timeout == FotaConstants.END_ACK_TIMEOUT_MS)
        {
            try
            {
                takePause(30000L);
            }
            catch (Exception e)
            {
                throw new RuntimeException(e);
            }
        }
        int consecutiveEofReads = 0;
        final int MAX_EOF_BEFORE_FAILURE = 20;
        int totalBytesRead = 0;

        while (System.currentTimeMillis() - startTime < timeoutMs)
        {
            long now = System.currentTimeMillis();
            if (now - lastKeepalive >= keepaliveMs)
            {
                try
                {
                    sendKeepalive();
                    lastKeepalive = now;
                }
                catch (IOException e)
                {
                    MedDevLog.info(TAG,"Failed to send keepalive: " + e.getMessage());
                    throw new FotaException("Connection lost - cannot send keepalive");
                }
            }

            int bytesRead = -1;
            try
            {
                int available = inputStream.available();
                if (available > 0)
                {
                    bytesRead = inputStream.read(readBuffer, 0, Math.min(available, readBuffer.length));
                    if(timeout == FotaConstants.END_ACK_TIMEOUT_MS)
                    {
                        MedDevLog.info(TAG,"Called For end session now available : "+bytesRead);
                    }
                }
                else
                {
                    bytesRead = inputStream.read(readBuffer, 0, readBuffer.length);
                    if(timeout == FotaConstants.END_ACK_TIMEOUT_MS)
                    {
                        MedDevLog.info(TAG,"Called For end session : "+bytesRead);
                    }
                }
            }
            catch (IOException e)
            {
                String msg = e.getMessage();
                if (msg != null && (msg.toLowerCase().contains("timeout") || msg.contains("ETIMEDOUT")))
                {
                    bytesRead = 0;
                }
                else
                {
                    MedDevLog.info(TAG,"Read error: " + e.getMessage());
                    throw e;
                }
            }

            if (bytesRead > 0)
            {
                consecutiveEofReads = 0;
                totalBytesRead += bytesRead;

                if (verbose)
                {
                    pushLogger("RX: " + bytesRead + " bytes (total: " + totalBytesRead + ")");
                }
                packetStream.append(readBuffer, 0, bytesRead);

                List<InterfaceProtocol.DecodedPacket> packets = packetStream.popPackets();
                for (InterfaceProtocol.DecodedPacket packet : packets)
                {
                    if (packet.valid && packet.type == InterfaceProtocol.PacketType.DFU)
                    {
                        FotaStatusResponse status = FotaStatusResponse.parse(packet.payload);
                        if (status != null)
                        {
                            if (verbose)
                            {
                                MedDevLog.info(TAG,"Status: " + status);
                            }
                            return status;
                        }
                    }
                    else if (verbose)
                    {
                        //MedDevLog.info(TAG,"Got packet: type=" + packet.type + ", valid=" + packet.valid + ", payload_len=" + packet.payload.length);
                    }
                }
            }
            else if (bytesRead == 0)
            {
                // No data read, reset EOF counter and sleep
                consecutiveEofReads = 0;
                try
                {
                    takePause(1000);
                }
                catch (Exception e)
                {
                    MedDevLog.info(TAG,"Interrupted while waiting for response");
                    throw new FotaException("Interrupted while waiting for response");
                }
            }
            else
            {
                consecutiveEofReads++;

                if (verbose && consecutiveEofReads % 5 == 0)
                {
                    pushLogger("Read returned -1 (count: " + consecutiveEofReads + ", total bytes read: " + totalBytesRead + ", Byte Reads = "+bytesRead);
                    sendKeepalive();
                }

                if (consecutiveEofReads >= MAX_EOF_BEFORE_FAILURE)
                {
                    try
                    {
                        sendKeepalive();
                        consecutiveEofReads = 0;
                        pushLogger("Keepalive succeeded after -1 reads, continuing...");
                    }
                    catch (IOException e)
                    {
                        pushLogger("Connection confirmed dead after " + consecutiveEofReads + " EOF reads and failed keepalive");
                        throw new FotaException("Connection lost (read " + totalBytesRead + " bytes before failure)");
                    }
                }
                try
                {
                    takePause(5);
                }
                catch (Exception e)
                {
                    throw new FotaException("Interrupted while waiting for response");
                }
            }
        }
        throw FotaException.timeout("RESPONSE (read " + totalBytesRead + " bytes)");
    }

    public FotaStatusResponse waitForResponse() throws FotaException, IOException
    {
        return waitForResponse(FotaConstants.RESPONSE_TIMEOUT_MS, FotaConstants.KEEPALIVE_INTERVAL_MS);
    }

    public FotaStatusResponse sendAndWait(byte[] payload, int expectedOp, String label, long timeoutMs) throws FotaException, IOException
    {
        sendPacket(payload, label);
        FotaStatusResponse status = waitForResponse(timeoutMs, FotaConstants.KEEPALIVE_INTERVAL_MS);
        if (status.getOpcode() != expectedOp && status.getOpcode() != FotaOpcode.QUERY.getValue())
        {
            throw FotaException.unexpectedOpcode(expectedOp, status.getOpcode());
        }
        if (!status.isSuccess())
        {
            FotaStatus dfuStatus = status.getStatus();
            if (dfuStatus != null)
            {
                throw FotaException.deviceError(label, dfuStatus);
            }
            else
            {
                //DFUException[END]: Device error: status_code=13
                throw new FotaException(label, "Device error: status_code=" + status.getStatusCode());
            }
        }
        return status;
    }

    public FotaStatusResponse sendAndWait(byte[] payload, int expectedOp, String label) throws FotaException, IOException {
        return sendAndWait(payload, expectedOp, label, FotaConstants.RESPONSE_TIMEOUT_MS);
    }

    public FotaStatusResponse waitReady(long timeoutMs) throws FotaException, IOException
    {
        long startTime = System.currentTimeMillis();
        while (System.currentTimeMillis() - startTime < timeoutMs) {
            byte[] queryPkt = FotaPacketFactory.query();
            sendPacket(queryPkt, "QUERY");
            try {
                FotaStatusResponse status = waitForResponse(FotaConstants.RESPONSE_TIMEOUT_MS, FotaConstants.KEEPALIVE_INTERVAL_MS);
                pushLogger("QUERY status: " + status);
                if (!status.isBusy())
                {
                    return status;
                }
            }
            catch (FotaException e)
            {
                e.printStackTrace();
                throw new FotaException("Timeout on query retry in waitRead(timeoutMs)");
            }
            try
            {
                takePause(200);
            }
            catch (Exception e)
            {
                e.printStackTrace();
                throw new FotaException("Interrupted while waiting for device ready");
            }
        }

        throw new FotaException("Device did not become ready (BUSY timeout)");
    }

    public FotaStatusResponse waitReady() throws FotaException, IOException
    {
        return waitReady(FotaConstants.READY_TIMEOUT_MS);
    }

    public FotaStatusResponse negotiateSession(int imageId, int maxMtu) throws FotaException, IOException
    {
        byte[] startPkt = FotaPacketFactory.start(imageId, maxMtu);
        FotaStatusResponse status = sendAndWait(startPkt, FotaOpcode.STATUS.getValue(), FotaPacketFactory.LABEL_START);
        // Extract negotiated MTU
        if (status.hasNegotiatedMtu())
        {
            negotiatedMtu = status.getNegotiatedMtu();
        }
        return status;
    }

    public FotaStatusResponse sendMetadata(int imageId, int firmwareSize, int firmwareCrc32) throws FotaException, IOException
    {
        byte[] preparePkt = FotaPacketFactory.prepare(imageId, firmwareSize, firmwareCrc32);
        FotaStatusResponse status = sendAndWait(preparePkt, FotaOpcode.STATUS.getValue(), FotaPacketFactory.LABEL_PREPARE);
        if (status.isBusy())
        {
            try
            {
                takePause(200);
            }
            catch (Exception e)
            {
                throw new FotaException("Interrupted while sending command PREPARE in sendMetadata()");
            }
            waitReady();
        }

        return status;
    }

    public void sendChunks(byte[] firmwareData, FotaProgressCallback progressCallback) throws FotaException, IOException
    {
        int firmwareSize = firmwareData.length;
        int numPackets = (firmwareSize + negotiatedMtu - 1) / negotiatedMtu;
        int bytesSent = 0;
        for (int seq = 0; seq < numPackets; seq++)
        {
            int offset = seq * negotiatedMtu;
            int chunkSize = Math.min(negotiatedMtu, firmwareSize - offset);
            byte[] chunk = new byte[chunkSize];
            System.arraycopy(firmwareData, offset, chunk, 0, chunkSize);
            byte[] dataPkt = FotaPacketFactory.data(seq, chunk);
            FotaException lastError = null;
            for (int attempt = 0; attempt < FotaConstants.MAX_RETRIES; attempt++)
            {
                try
                {
                    if (attempt > 0)
                    {
                        packetStream.clear();
                    }
                    sendPacket(dataPkt, "DATA seq=" + seq);
                    outputStream.flush();
                    waitForResponse();
                    lastError = null;
                    break;
                }
                catch (FotaException e)
                {
                    lastError = e;
                    pushLogger("DATA seq=" + seq + " failed (attempt " + (attempt + 1) + "/" + FotaConstants.MAX_RETRIES + "): " + e.getMessage());
                    packetStream.clear();
                    try
                    {
                        takePause(50 * (attempt + 1));
                    }
                    catch (Exception ie)
                    {
                        e.printStackTrace();
                        throw new FotaException("Interrupted while sending chunk retries");
                    }
                }
            }

            if (lastError != null)
            {
                throw lastError;
            }

            bytesSent += chunkSize;
            if (progressCallback != null)
            {
                progressCallback.onTransferProgress(bytesSent, firmwareSize);
            }

            try
            {
                takePause(5);
            }
            catch (Exception e)
            {
                e.printStackTrace();
            }
        }
    }

    public FotaStatusResponse verifyImage(int imageId) throws FotaException, IOException
    {
        byte[] verifyPkt = FotaPacketFactory.verify(imageId);
        return sendAndWait(verifyPkt, FotaOpcode.STATUS.getValue(),FotaPacketFactory.LABEL_VERIFY);
    }

    public FotaStatusResponse installImage(int imageId) throws FotaException, IOException
    {
        byte[] installPkt = FotaPacketFactory.install(imageId);
        return sendAndWait(installPkt, FotaOpcode.STATUS.getValue(),FotaPacketFactory.LABEL_INSTALL);
    }

    public FotaStatusResponse endSession(boolean waitForAck, long timeoutMs) throws FotaException, IOException
    {
        byte[] endPkt = FotaPacketFactory.end();
        if (!waitForAck)
        {
            MedDevLog.info(TAG,"Wait For Ack : "+waitForAck);
            sendPacket(endPkt, FotaPacketFactory.LABEL_END);
            return null;
        }
        return sendAndWait(endPkt, FotaOpcode.STATUS.getValue(), FotaPacketFactory.LABEL_END, timeoutMs);
    }

    public FotaStatusResponse endSession(boolean waitForAck) throws FotaException, IOException
    {
        return endSession(waitForAck, FotaConstants.END_ACK_TIMEOUT_MS);
    }

    private static String bytesToHex(byte[] bytes)
    {
        return bytesToHex(bytes, bytes.length);
    }

    private static String bytesToHex(byte[] bytes, int length)
    {
        StringBuilder sb = new StringBuilder();
        int maxLen = Math.min(length, 40);
        for (int i = 0; i < maxLen; i++)
        {
            sb.append(String.format("%02x", bytes[i] & 0xFF));
        }
        if (length > maxLen)
        {
            sb.append("...");
        }
        return sb.toString();
    }

    private void takePause(long millis) throws InterruptedException
    {
        /*final Handler handler = new Handler(Looper.getMainLooper());
        Runnable runnable = new Runnable() {
            @Override
            public void run()
            {
            }
        };
        handler.postDelayed(runnable, millis);*/
        final CountDownLatch latch = new CountDownLatch(1);
        final Handler handler = new Handler(Looper.getMainLooper());
        handler.postDelayed(() -> latch.countDown(), millis);
        latch.await(millis + 100, TimeUnit.MILLISECONDS);
    }

    private void pushLogger(String mesg)
    {
        /*MedDevLog.info(TAG,mesg);*/
    }

}