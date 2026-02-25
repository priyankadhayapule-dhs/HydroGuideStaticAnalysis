package com.accessvascularinc.hydroguide.mma.ui;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.serial.Button;
import com.accessvascularinc.hydroguide.mma.serial.ControllerStatus;
import com.accessvascularinc.hydroguide.mma.serial.EcgSample;
import com.accessvascularinc.hydroguide.mma.serial.FramedOutput;
import com.accessvascularinc.hydroguide.mma.serial.Heartbeat;
import com.accessvascularinc.hydroguide.mma.serial.Packet;
import com.accessvascularinc.hydroguide.mma.serial.PacketId;
import com.accessvascularinc.hydroguide.mma.serial.SerialFrameParser;
import com.accessvascularinc.hydroguide.mma.serial.SerialHelper;
import com.accessvascularinc.hydroguide.mma.serial.StartupStatus;
import com.accessvascularinc.hydroguide.mma.ui.input.ButtonInputListener;
import com.accessvascularinc.hydroguide.mma.ui.interfaces.IControllerCommunicationManager;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.ftdi.j2xx.D2xxManager;
import com.ftdi.j2xx.FT_Device;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;

public abstract class UsbControllerManager implements IControllerCommunicationManager {
  private static final int READ_LENGTH = 512;
  private static final int KEEP_ALIVE_PACKET_INTERVAL_MS = 1000;
  private static final int BAUD_RATE = 1000000;
  private static final byte STOP_BITS = D2xxManager.FT_STOP_BITS_1;
  private static final byte DATA_BITS = D2xxManager.FT_DATA_BITS_8;
  private static final byte PARITY = D2xxManager.FT_PARITY_NONE;
  private static final short FLOW_CONTROL = D2xxManager.FT_FLOW_NONE;
  private static final long CONTROLLER_STATUS_TIMEOUT = 5000;
  private static final long ECG_DATA_TIMEOUT = 500;
  private static final long USB_SIGNAL_TIMEOUT = 2000;
  private final Context context;
  private final SerialFrameParser sfp = new SerialFrameParser();
  private final List<ControllerListener> controllerListeners = new ArrayList<>();
  private final List<ButtonInputListener> buttonInputListeners = new ArrayList<>();
  private final D2xxManager ftdiD2xx;
  private final Scheduler statusTimeout;
  private final Scheduler ecgDataTimeout;
  private final Scheduler usbTimeout;
  private final Handler timeoutHandler = new Handler(Looper.getMainLooper());
  private boolean bReadThreadGoing = false;
  private int DevCount = -1;
  private int currentIndex = -1;
  int openIndex = 0; // 1 for Rev 1, 0 for Rev 2
  private boolean receiveFailed = false;
  private boolean sendingKeepAlive = false;
  private boolean controllerIsConnected = false;
  private FT_Device ftDev = null;
  @Nullable
  private Packet receiveBuffer = null;
  private int packetCounter = 0;
  private int timestampCounter = 0;
  private int receiveBufferPos = 0;
  private boolean timedOut = false;
  private boolean ecgMissing = false;
  private final Handler readHandler = new Handler(Looper.getMainLooper()) {
    @Override
    public void handleMessage(@NonNull final Message msg) {
      final byte[] rxData = msg.getData().getByteArray("data");
      if (rxData != null) {
        usbTimeout.circulateTimeout(false);
        deserializePacket(rxData);
      }
    }
  };
  private FramedOutput outputFramer = null;

  public UsbControllerManager(final Context mContext, final D2xxManager ftD2xx) {
    this.context = mContext;
    this.ftdiD2xx = ftD2xx;
    statusTimeout = new Scheduler(timeoutHandler, CONTROLLER_STATUS_TIMEOUT,
        this::connectionStatusTimeout);
    ecgDataTimeout = new Scheduler(timeoutHandler, ECG_DATA_TIMEOUT, this::ecgStatusTimeout);
    usbTimeout = new Scheduler(timeoutHandler, USB_SIGNAL_TIMEOUT, this::usbTimeout);

  }

  public void addControllerListener(final ControllerListener evtListener) {
    this.controllerListeners.add(evtListener);
  }

  public void connectionStatusTimeout() {
    timedOut = true;
    controllerListeners.forEach(ControllerListener::onConnectionStatusTimeout);
  }

  public void ecgStatusTimeout() {
    if (!checkConnection()) {
      return;
    }
    ecgMissing = true;
    controllerListeners.forEach(ControllerListener::onEcgStatusTimeout);
  }

  public void usbTimeout() {
    disconnectFunction();
  }

  public void addButtonInputListener(final ButtonInputListener evtListener) {
    this.buttonInputListeners.add(evtListener);
  }

  public void removeControllerListener(final ControllerListener evtListener) {
    this.controllerListeners.remove(evtListener);
  }

  public void removeButtonInputListener(final ButtonInputListener evtListener) {
    this.buttonInputListeners.remove(evtListener);
  }

  public boolean checkConnection() {
    return controllerIsConnected;
  }

  public boolean isConnectionTimedOut() {
    return timedOut;
  }

  public boolean isEcgMissing() {
    return ecgMissing;
  }

  public void connectController() {
    DevCount = 0;
    createDeviceList();
    if (DevCount > 0) {
      connectFunction();
      setConfig();
    }
  }

  public void notifyUSBDeviceAttach() {
    Log.d("ABC", "notifyUSBDeviceAttach: ");
    connectController();
  }

  public void notifyUSBDeviceDetach() {
    disconnectFunction();
  }

  private void createDeviceList() {
    final int tempDevCount = ftdiD2xx.createDeviceInfoList(context);

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
    MedDevLog.info("UsbControllerManager", "Controller disconnecting - Connection: USB");

    ecgMissing = ecgDataTimeout.circulateTimeout(true);
    usbTimeout.circulateTimeout(true);
    usbTimeout.cleanUp();
    ecgDataTimeout.cleanUp();

    DevCount = -1;
    currentIndex = -1;
    bReadThreadGoing = false;

    packetCounter = 0;
    timestampCounter = 0;
    sendingKeepAlive = false;

    try {
      Thread.sleep(50);
    } catch (final InterruptedException e) {
      MedDevLog.error("UsbControllerManager", "Error calling thread sleep", e);
    }

    if (ftDev != null) {
      synchronized (ftDev) {
        if (ftDev.isOpen()) {
          ftDev.close();
          outputFramer = null;
        }
      }
    }
    if (controllerIsConnected) {
      controllerIsConnected = false;
      controllerListeners.forEach((el) -> el.onConnectionStatusChange(false));
      Toast.makeText(context, "Device disconnected.", Toast.LENGTH_LONG).show();
    }
    timedOut = statusTimeout.circulateTimeout(true);
    statusTimeout.cleanUp();
  }

  public void connectFunction() {
    MedDevLog.info("Usb Controller Manager", String.format("Port index is : %s", openIndex));
    final int tmpPortNumber = openIndex + 1;
    // Get the number of connected FTDI devices

    var deviceCount = ftdiD2xx.createDeviceInfoList(context);
    MedDevLog.debug("Usb Controller Manager", "Number of FTDI devices found: " + deviceCount);

    if (deviceCount == 0) {
      MedDevLog.debug("Usb Controller Manager", "No devices connected.");
      return;
    }

    // Enumerate and print each device’s info
    for (int i = 0; i < deviceCount; i++) {
      var info = ftdiD2xx.getDeviceInfoListDetail(i);
      MedDevLog.debug("", "Device " + i + ": " + info.description + " | " + info.serialNumber);
    }
    if (currentIndex != openIndex) {
      if (null == ftDev) {
        // ftDev = ftdiD2xx.openByIndex(context, openIndex);
        ftDev = ftdiD2xx.openByDescription(context, "HydroGUIDE Controller Prototype A");
      } else {
        synchronized (ftDev) {
          // ftDev = ftdiD2xx.openByIndex(context, openIndex);
          ftDev = ftdiD2xx.openByDescription(context, "HydroGUIDE Controller Prototype A");
        }
      }
    } else {
      Toast.makeText(context, "Device port " + tmpPortNumber + " is already opened",
          Toast.LENGTH_LONG).show();
      return;
    }

    if (ftDev == null) {
      Toast.makeText(context, "open device port(" + tmpPortNumber + ") NG, ftDev == null",
          Toast.LENGTH_LONG).show();
      return;
    }

    if (ftDev.isOpen()) {
      currentIndex = openIndex;
      Toast.makeText(context, "open device port(" + tmpPortNumber + ") OK", Toast.LENGTH_SHORT)
          .show();

      if (!bReadThreadGoing) {
        final ReadThread read_thread = new ReadThread(readHandler);
        read_thread.start();
        bReadThreadGoing = true;
      }

      if (!sendingKeepAlive) {
        outputFramer = new FramedOutput();
        sendingKeepAlive = true;
        final KeepAliveThread keep_alive_thread = new KeepAliveThread();
        keep_alive_thread.start();
      }

      ftDev.purge((byte) (D2xxManager.FT_PURGE_TX | D2xxManager.FT_PURGE_RX));
      MedDevLog.info("UsbControllerManager", "Controller connected successfully - Connection: USB");
      controllerListeners.forEach((el) -> el.onConnectionStatusChange(true));
      controllerIsConnected = true;
    } else {
      Toast.makeText(context, "open device port(" + tmpPortNumber + ") NG", Toast.LENGTH_LONG)
          .show();
    }
    timedOut = statusTimeout.circulateTimeout(false);
    ecgMissing = ecgDataTimeout.circulateTimeout(false);
    usbTimeout.circulateTimeout(false);
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
    if (!ftDev.isOpen()) {
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

  private void sendMessage(final byte[] writeData) {
    synchronized (ftDev) {
      if (!ftDev.isOpen()) {
        MedDevLog.error("j2xx", "SendMessage: device not open");
        return;
      }
      ftDev.write(writeData, writeData.length);
    }
  }

  private void deserializePacket(final byte[] readData) {
    for (final byte by : readData) {
      final var dataBy = sfp.process(by);
      if (sfp.fail || sfp.stufffail) {
        receiveFailed = true;
        onFramingError(
            (sfp.stufffail ? SerialFrameParser.Status.ImproperStuffTrailByte : sfp.result), by);
        receiveBuffer = null;
      }
      switch (sfp.result) {
        case STX:
          receiveBuffer = new Packet();
          receiveFailed = false;
          break;

        case PktID:
          if (receiveBuffer != null) {
            receiveBuffer.packetType = dataBy;
          }
          break;

        case Timestamp_MSB:
          if (receiveBuffer != null) {
            receiveBuffer.timestamp |= (dataBy << 8);
          }
          break;

        case Timestamp_LSB:
          if (receiveBuffer != null) {
            receiveBuffer.timestamp |= (dataBy & 0xFF);
          }
          break;

        case PayloadLength:
          if (receiveBuffer != null) {
            receiveBuffer.payload = new byte[dataBy & 0xFF];
          }
          receiveBufferPos = 0;
          break;

        case Continue:
        case Payload:
          if (dataBy != null && receiveBuffer != null) {
            receiveBuffer.payload[receiveBufferPos] = dataBy;
            receiveBufferPos++;
          }
          break;

        case CRC_MSB:
          if (!receiveFailed && (receiveBuffer != null)) {
            onPacketArrival(receiveBuffer);
          }
          receiveBuffer = null;
          break;

        case MismatchedCRC:
          receiveBuffer = null;
          break;
      }
    }
  }

  protected void onFramingError(final SerialFrameParser.Status expected, final byte actualByte) {
    MedDevLog.error("onFramingError", String.format("%s : %s", expected.toString(), actualByte));
  }

  private void onPacketArrival(final Packet incPacket) {
    Log.w("Packet Arrival", String.valueOf(incPacket.packetType));
    final PacketId id = PacketId.valueOfByte(incPacket.packetType);
    //Log.w("Packet ", String.valueOf(id));
    ecgMissing = ecgDataTimeout.circulateTimeout(false);

    controllerListeners.forEach((el) -> el.onPacketArrival(incPacket));

    switch (Objects.requireNonNull(id)) {
      case EcgWaveform:
        final EcgSample ecgSample = new EcgSample(incPacket.timestamp, incPacket.payload);
        MedDevLog.info("ECG Data", String.format(
            "ECG packet received - Timestamp: %d ms, StartTime: %d ms, SamplingStatus: %s, Connection: USB",
            incPacket.timestamp, ecgSample.startTime_ms,
            ecgSample.samplingStatus != null ? ecgSample.samplingStatus.toString() : "N/A"));
        onFilteredSamples(incPacket.timestamp, ecgSample);
        //Log.w("EcgWaveform Timestamp", String.format("%s", ecgSample.startTime_ms));
        break;
      case RawAdcSamples:
        break;
      case PatientHeartbeat:
        final Heartbeat heartbeat = new Heartbeat(incPacket.payload);
        MedDevLog.info("Heartbeat Data", String.format(
            "Heartbeat received - Timestamp: %d ms, P-Wave Ext: %.4f V, P-Wave Int: %.4f V, Connection: USB",
            incPacket.timestamp, heartbeat.pWaveExternalAmplitude_V,
            heartbeat.pWaveInternalAmplitude_V));
        onPatientHeartbeat(incPacket.timestamp, heartbeat);
        break;
      case ButtonsState:
        final Button button = new Button(incPacket.payload);
        onButtonsStateChange(incPacket.timestamp, button);
        break;
      case StartUpStatus:
        final StartupStatus startupStatus = new StartupStatus(incPacket.payload);
        onStartupStatus(incPacket.timestamp, startupStatus);
        break;
      case ControllerStatus:
        final ControllerStatus controllerStatus = new ControllerStatus(incPacket.payload);
        MedDevLog.info("Controller Status", String.format(
            "Controller status received - Timestamp: %d ms, HeartRate: %.0f bpm, Battery: %.0f%%, Connection: USB",
            incPacket.timestamp, controllerStatus.heartRate_bpm,
            controllerStatus.batteryCharge_pct));
        onControllerStatus(incPacket.timestamp, controllerStatus);
        break;
      default:
        final int subTypeValue = incPacket.packetType & 0b11;
        MedDevLog.error("Unpacking Error", String.format("Unknown packet ID %s , type %s",
            incPacket.payload.length,
            SerialFrameParser.PacketSubId.valueOfByte((byte) subTypeValue)));
        break;
    }
  }

  public void onFilteredSamples(final long timestamp, final EcgSample sample) {
    final String sampleStr = String.format("Timestamp: %s ms, Start Time; %s, Sampling Status: %s, Samples: %s",
        timestamp, sample.startTime_ms,
        sample.samplingStatus != null ? sample.samplingStatus.toString() : R.string.N_A,
        Arrays.deepToString(sample.samples));
    // Log.i("onFilteredSamples", sampleStr);
    controllerListeners.forEach((el) -> el.onFilteredSamples(sample, sampleStr));
  }

  public void onPatientHeartbeat(final long timestamp, final Heartbeat heartbeat) {
    final String heartbeatStr = String.format("Timestamp: %s, P-Wave External Amp (V): %s, P-Wave Internal Amp (V): %s",
        timestamp, heartbeat.pWaveExternalAmplitude_V,
        heartbeat.pWaveInternalAmplitude_V);
    Log.i("onPatientHeartbeat", heartbeatStr);
    controllerListeners.forEach((el) -> el.onPatientHeartbeat(heartbeat, heartbeatStr));
  }

  public void onButtonsStateChange(final long timestamp, final Button newButton) {
    final String buttonStr = String.format(
        "Timestamp: %s, Up: %s, Down: %s, Left: %s, Right: %s, Select: %s, Capture: %s",
        timestamp,
        newButton.buttons.getButtonStateAtKey(Button.ButtonIndex.Up),
        newButton.buttons.getButtonStateAtKey(Button.ButtonIndex.Down),
        newButton.buttons.getButtonStateAtKey(Button.ButtonIndex.Left),
        newButton.buttons.getButtonStateAtKey(Button.ButtonIndex.Right),
        newButton.buttons.getButtonStateAtKey(Button.ButtonIndex.Select),
        newButton.buttons.getButtonStateAtKey(Button.ButtonIndex.Capture));
    MedDevLog.info("UsbControllerManager", "onButtonsStateChange: " + buttonStr);
    buttonInputListeners.forEach((el) -> el.onButtonsStateChange(newButton, buttonStr));
  }

  public void onStartupStatus(final long timestamp, final StartupStatus startupStatus) {
    final String startupStatusStr = String.format("Timestamp: %s, SFU Status: %s, App Status: %s", timestamp,
        startupStatus.sfuStatus, startupStatus.appStatus);
    Log.i("onStartupStatus", startupStatusStr);
    controllerListeners.forEach((el) -> el.onStartupStatus(startupStatus, startupStatusStr));
  }

  public void onControllerStatus(final long timestamp, final ControllerStatus controllerStatus) {
    final String controllerStr = String.format("Timestamp: %s, Heart Rate: %s, Charge Level: %s", timestamp,
        controllerStatus.heartRate_bpm, controllerStatus.batteryCharge_pct);
    Log.i("onControllerStatus", controllerStr);
    timedOut = statusTimeout.circulateTimeout(false);
    controllerListeners.forEach((el) -> el.onControllerStatus(controllerStatus, controllerStr));
  }

  @Override
  public void sendECGCommand(boolean started) {

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
            MedDevLog.error("UsbControllerManager", "Error calling thread sleep", e);
          }
        }
      }
    }
  }

  private class KeepAliveThread extends Thread {
    KeepAliveThread() {
      this.setPriority(Thread.MIN_PRIORITY);
    }

    @Override
    public void run() {
      while (sendingKeepAlive) {
        final int packetTimestamp = timestampCounter;
        final int sequenceNumber = packetCounter;
        final byte[] keepAlivePayload = SerialHelper.packInt(sequenceNumber);
        final byte[] txBytes = outputFramer.addFraming(
            PacketId.TabletKeepalive.getByteValue(), packetTimestamp, keepAlivePayload);
        // sendMessage(txBytes);

        timestampCounter++;
        packetCounter++;

        try {
          Thread.sleep(KEEP_ALIVE_PACKET_INTERVAL_MS);
        } catch (final InterruptedException e) {
          MedDevLog.error("UsbControllerManager", "Error calling thread sleep", e);
        }
      }
    }
  }
}