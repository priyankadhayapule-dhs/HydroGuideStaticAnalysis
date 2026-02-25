package com.accessvascularinc.hydroguide.mma.fota;

import android.util.Log;
import com.ftdi.j2xx.FT_Device;
import java.io.IOException;
import java.io.InputStream;

public class FotaReadStream extends InputStream
{
  private static final String TAG = "Logs";
  private final FT_Device device;
  private int readTimeoutMs = 2000;
  private volatile boolean closed = false;

  public FotaReadStream(FT_Device device)
  {
    this.device = device;
  }

  public void setReadTimeout(int timeoutMs)
  {
    this.readTimeoutMs = timeoutMs;
  }

  @Override
  public int read() throws IOException
  {
    byte[] buffer = new byte[1];
    int count = read(buffer, 0, 1);
    return count > 0 ? (buffer[0] & 0xFF) : -1;
  }

  @Override
  public int read(byte[] b) throws IOException
  {
    return read(b, 0, b.length);
  }

  public int read(byte[] b, int off, int len) throws IOException
  {
    if (closed || device == null || !device.isOpen())
    {
      throw new IOException("Stream is closed");
    }
    long startTime = System.currentTimeMillis();
    try
    {
      synchronized (device)
      {
        while (System.currentTimeMillis() - startTime < readTimeoutMs)
        {
          int available = device.getQueueStatus();
          if (available > 0)
          {
            int toRead = Math.min(available, len);
            byte[] temp = new byte[toRead];
            int count = device.read(temp, toRead);
            if (count > 0)
            {
              System.arraycopy(temp, 0, b, off, count);
              return count;
            }
          }
        }
      }
    }
    catch (Exception e)
    {
      Log.d("Logs", "Fota Read Stream Err", e);
    }

    return -1; // Timeout
  }

  @Override
  public int available() throws IOException
  {
    if (closed || device == null || !device.isOpen())
    {
      return 0;
    }
    return device.getQueueStatus();
  }

  @Override
  public void close() throws IOException
  {
    closed = true;
  }

}