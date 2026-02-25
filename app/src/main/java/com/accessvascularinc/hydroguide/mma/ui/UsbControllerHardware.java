package com.accessvascularinc.hydroguide.mma.ui;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.widget.Toast;
import androidx.annotation.NonNull;
import com.accessvascularinc.hydroguide.mma.fota.FotaReadStream;
import com.accessvascularinc.hydroguide.mma.fota.FotaWriteStream;
import com.accessvascularinc.hydroguide.mma.ui.interfaces.IControllerHardware;
import com.accessvascularinc.hydroguide.mma.ui.interfaces.IPacketReceiver;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import com.ftdi.j2xx.D2xxManager;
import com.ftdi.j2xx.FT_Device;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.function.Consumer;

public class UsbControllerHardware implements IControllerHardware {
  private static final String TAG = "UsbControllerHardware";
  private final Context context;
  private static final int READ_LENGTH = 512;
  private final D2xxManager ftdiD2xx;
  private boolean controllerIsConnected = false;
  private FT_Device ftDev = null;

  private static final int BAUD_RATE = 1000000;
  private static final byte STOP_BITS = D2xxManager.FT_STOP_BITS_1;
  private static final byte DATA_BITS = D2xxManager.FT_DATA_BITS_8;
  private static final byte PARITY = D2xxManager.FT_PARITY_NONE;
  private static final short FLOW_CONTROL = D2xxManager.FT_FLOW_NONE;
  private int DevCount = -1;
  private int currentIndex = -1;
  int openIndex = 0; // 1 for Rev 1, 0 for Rev 2
  private boolean bReadThreadGoing = false;
  private IPacketReceiver hardwareClient;

  private final Handler readHandler = new Handler(Looper.getMainLooper()) {
    @Override
    public void handleMessage(@NonNull final Message msg) {
      final byte[] rxData = msg.getData().getByteArray("data");
      if (rxData != null) {
        hardwareClient.getUsbTimeout().circulateTimeout(false);
        hardwareClient.deserializePacket(rxData);
      }
    }
  };

  public UsbControllerHardware(Context context, D2xxManager ftdiD2xx) {
    this.context = context;
    this.ftdiD2xx = ftdiD2xx;
  }

  @Override
  public void connectController(Consumer<Boolean> onResult) {
    MedDevLog.info(TAG, "connectController called");
    // Implement USB connect logic here
    DevCount = 0;
    createDeviceList();
    if (DevCount > 0) {
      connectFunction();
      setConfig();
      onResult.accept(controllerIsConnected);
    } else {
      onResult.accept(false);
    }
  }

  @Override
  public void connectController(Consumer<Boolean> onResult, String deviceName) {

  }

  @Override
  public boolean checkConnection() {
    // Implement USB connection check logic here
    return controllerIsConnected;
  }

  @Override
  public void disconnectController() {
    disconnectFunction();
  }

  @Override
  public void setPacketReceiver(IPacketReceiver client) {
    this.hardwareClient = client;
  }

  private void createDeviceList() {
    final int tempDevCount = ftdiD2xx.createDeviceInfoList(context);
    Utility.debugUsbDevices(TAG,context);
    if (tempDevCount > 0) {
      if (DevCount != tempDevCount) {
        DevCount = tempDevCount;
        updatePortNumberSelector();
      }
    } else {
      DevCount = -1;
      currentIndex = -1;
    }
  }

  private void disconnectFunction() {
    DevCount = -1;
    currentIndex = -1;
    bReadThreadGoing = false;

    if (ftDev != null) {
      synchronized (ftDev) {
        if (ftDev.isOpen()) {
          ftDev.close();
        }
      }
    }
    if (controllerIsConnected) {
      controllerIsConnected = false;
    }
  }

  private void connectFunction() {
    MedDevLog.info("Usb Controller Manager", String.format("Port index is : %s", openIndex));
    final int tmpPortNumber = openIndex + 1;

    // Get the number of connected FTDI devices

    var deviceCount = ftdiD2xx.createDeviceInfoList(context);
    MedDevLog.debug("Usb Controller Manager", "Number of FTDI devices found: " + deviceCount);

    String deviceDescription = "HydroGUIDE Controller Prototype";
    if (deviceCount == 0) {
      MedDevLog.debug("Usb Controller Manager", "No devices connected.");
      return;
    }
    if(deviceCount == 2){
      deviceDescription = "HydroGUIDE Controller Prototype A";
    }
    // Used below only during development time for board name checks
    //deviceDescription = Utility.HARDWARE_BOARD_NAME;

    // Enumerate and print each device’s info
    for (int i = 0; i < deviceCount; i++) {
      var info = ftdiD2xx.getDeviceInfoListDetail(i);
      MedDevLog.debug("Usb Controller Manager", "Device " + i
              + ": " + info.description + " | " + info.serialNumber);
    }

    if (currentIndex != openIndex) {
      MedDevLog.debug(TAG, "Opening device at index: " + openIndex);
      if (null == ftDev) {
        try {
          MedDevLog.debug(TAG, "ftDev is null, opening device.");
          // ftDev = ftdiD2xx.openByIndex(context, openIndex);
          ftDev = ftdiD2xx.openByDescription(context, deviceDescription);
        } catch (Exception e) {
          MedDevLog.error(TAG, "Error opening FT_Device", e);
          throw e;
        }
      } else {
        synchronized (ftDev) {
          try {
            MedDevLog.debug(TAG, "ftDev is not null, opening synchronized.");
            // ftDev = ftdiD2xx.openByIndex(context, openIndex);
            ftDev = ftdiD2xx.openByDescription(context, deviceDescription);
          } catch (Exception e) {
            MedDevLog.error(TAG, "Error opening FT_Device synchronized", e);
            throw e;
          }
        }
      }
    } else {
      String msg = "Device port " + tmpPortNumber + " is already opened.";
      MedDevLog.warn(TAG, msg);
      Toast.makeText(context, msg, Toast.LENGTH_LONG).show();
      return;
    }

    if (ftDev == null) {
      String msg = "open device port " + tmpPortNumber + " NG, ftDev == null";
      MedDevLog.warn(TAG, msg);
      Toast.makeText(context, msg, Toast.LENGTH_LONG).show();
      return;
    }

    if (ftDev.isOpen()) {
      currentIndex = openIndex;
      String msg = "open device port " + tmpPortNumber + " OK";
      MedDevLog.info(TAG, msg);
      Toast.makeText(context, msg, Toast.LENGTH_SHORT).show();

      if (!bReadThreadGoing) {
        final ReadThread read_thread = new ReadThread(readHandler);
        read_thread.start();
        bReadThreadGoing = true;
      }
      ftDev.purge((byte) (D2xxManager.FT_PURGE_TX | D2xxManager.FT_PURGE_RX));
      controllerIsConnected = true;
    } else {
      String msg = "open device port " + tmpPortNumber + " NG";
      MedDevLog.error(TAG, msg);
      Toast.makeText(context, msg, Toast.LENGTH_LONG).show();
      controllerIsConnected = false;
    }
  }

  private void updatePortNumberSelector() {
    if (DevCount == 2) {
      Toast.makeText(context, "2 port device attached", Toast.LENGTH_SHORT).show();
    } else if (DevCount == 4) {
      Toast.makeText(context, "4 port device attached", Toast.LENGTH_SHORT).show();
    } else {
      Toast.makeText(context, "1 port device attached", Toast.LENGTH_SHORT).show();
    }
  }

  private void setConfig() {
    if (ftDev == null || !ftDev.isOpen()) {
      MedDevLog.error("j2xx", "SetConfig: device not open");
      return;
    }

    // configure our port
    ftDev.setBitMode((byte) 0, D2xxManager.FT_BITMODE_RESET);
    ftDev.setBaudRate(BAUD_RATE);
    ftDev.setDataCharacteristics(DATA_BITS, STOP_BITS, PARITY);
    ftDev.setFlowControl(FLOW_CONTROL, (byte) 0x0b, (byte) 0x0d);
    ftDev.setLatencyTimer((byte) 1);

    Toast.makeText(context, "Config done", Toast.LENGTH_SHORT).show();
  }

  @Override
  public void sendMessage(final byte[] writeData) {
    // Defensive checks to avoid NPE when USB not yet connected/switching from BLE
    if (ftDev == null) {
      MedDevLog.warn("UsbControllerHardware", "sendMessage: FT_Device is null (USB not connected yet). Dropping packet.");
      return;
    }
    try {
      synchronized (ftDev) {
        if (!ftDev.isOpen()) {
          MedDevLog.warn("UsbControllerHardware", "sendMessage: device not open. Dropping packet.");
          return;
        }
        ftDev.write(writeData, writeData.length);
      }
    } catch (Exception e) {
      MedDevLog.error("UsbControllerHardware", "sendMessage: exception while writing", e);
    }
  }

  private class ReadThread extends Thread {
    final Handler mHandler;

    ReadThread(final Handler h) {
      mHandler = h;
      this.setPriority(Thread.MIN_PRIORITY);
    }

    @Override
    public void run() {
      final byte[] readData = new byte[READ_LENGTH];

      while (bReadThreadGoing) {
        boolean sleepNow = false;
        synchronized (ftDev) {
          // check the amount of available data
          int iAvailable = ftDev.getQueueStatus();
          if (iAvailable > 0) {
            if (iAvailable > READ_LENGTH) {
              iAvailable = READ_LENGTH;
            }

            // get the data
            final int byteCount = ftDev.read(readData, iAvailable);
            final byte[] newData = new byte[byteCount];
            System.arraycopy(readData, 0, newData, 0, byteCount);

            final Message msg = mHandler.obtainMessage();
            final Bundle bundle = new Bundle();
            bundle.putByteArray("data", newData);
            msg.setData(bundle);
            mHandler.sendMessage(msg);
          } else {
            sleepNow = true;
          }
        }
        if (sleepNow) {
          try {
            Thread.sleep(50);
          } catch (final InterruptedException e) {
            MedDevLog.error("UsbControllerHardware", "Error calling thread sleep", e);
          }
        }
      }
    }
  }

  public InputStream getFotaInputStream()
  {
    if (ftDev != null && ftDev.isOpen())
    {
      return new FotaReadStream(ftDev);
    }
    return null;
  }

  public OutputStream getFotaOutputStream()
  {
    if (ftDev != null && ftDev.isOpen())
    {
      return new FotaWriteStream(ftDev);
    }
    return null;
  }

  public void updateReads(boolean enable) {
    bReadThreadGoing = enable;
  }
}
