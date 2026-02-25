package com.accessvascularinc.hydroguide.mma.fota;

import android.util.Log;

import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import com.ftdi.j2xx.FT_Device;
import java.io.IOException;
import java.io.OutputStream;

public class FotaWriteStream extends OutputStream
{
  private String TAG = "FotaWriteStream";
  private final FT_Device device;
  private volatile boolean closed = false;

  public FotaWriteStream(FT_Device device)
  {
    this.device = device;
  }

  @Override
  public void write(int b) throws IOException
  {
    write(new byte[] { (byte) b }, 0, 1);
  }

  @Override
  public void write(byte[] b) throws IOException
  {
    write(b, 0, b.length);
  }

  @Override
  public void write(byte[] b, int off, int len) throws IOException
  {
    if (closed || device == null || !device.isOpen())
    {
      throw new IOException("Stream is closed");
    }

    byte[] data;
    if (off == 0 && len == b.length)
    {
      data = b;
    }
    else
    {
      data = new byte[len];
      System.arraycopy(b, off, data, 0, len);
    }
    try
    {
      synchronized (device)
      {
        int written = device.write(data, len);
        if (written != len)
        {
          throw new IOException("Write failed: wrote " + written + " of " + len + " bytes");
        }
      }
    }
    catch (Exception e)
    {
      MedDevLog.info(TAG,"Fotawrite Stream :  "+e.getMessage());
    }
  }


  @Override
  public void close() throws IOException
  {
    closed = true;
  }
}