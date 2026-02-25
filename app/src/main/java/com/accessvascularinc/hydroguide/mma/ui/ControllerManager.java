package com.accessvascularinc.hydroguide.mma.ui;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.Toast;

import androidx.annotation.Nullable;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.fota.FotaClient;
import com.accessvascularinc.hydroguide.mma.serial.Button;
import com.accessvascularinc.hydroguide.mma.serial.ControllerStatus;
import com.accessvascularinc.hydroguide.mma.serial.EcgSample;
import com.accessvascularinc.hydroguide.mma.serial.FramedOutput;
import com.accessvascularinc.hydroguide.mma.serial.Heartbeat;
import com.accessvascularinc.hydroguide.mma.serial.ImpedanceSample;
import com.accessvascularinc.hydroguide.mma.serial.Packet;
import com.accessvascularinc.hydroguide.mma.serial.PacketId;
import com.accessvascularinc.hydroguide.mma.serial.SerialFrameParser;
import com.accessvascularinc.hydroguide.mma.serial.SerialHelper;
import com.accessvascularinc.hydroguide.mma.serial.StartupStatus;
import com.accessvascularinc.hydroguide.mma.ui.input.ButtonInputListener;
import com.accessvascularinc.hydroguide.mma.ui.interfaces.IControllerCommunicationManager;
import com.accessvascularinc.hydroguide.mma.ui.interfaces.IControllerHardware;
import com.accessvascularinc.hydroguide.mma.ui.interfaces.IPacketReceiver;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.ftdi.j2xx.D2xxManager;

import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class ControllerManager implements IControllerCommunicationManager, IPacketReceiver {
  private String TAG = "Logs";
  @Nullable
  private IControllerHardware Hardware = null;
  private static final int KEEP_ALIVE_PACKET_INTERVAL_MS = 1000;
  private final Context context;
  private final SerialFrameParser sfp = new SerialFrameParser();
  private final List<ControllerListener> controllerListeners = new ArrayList<>();
  private final List<ButtonInputListener> buttonInputListeners = new ArrayList<>();
  private final Handler timeoutHandler = new Handler(Looper.getMainLooper());

  private boolean receiveFailed = false;
  private boolean sendingKeepAlive = false;
  // private boolean controllerIsConnected = false;

  @Nullable
  private Packet receiveBuffer = null;
  private int packetCounter = 0;
  private int timestampCounter = 0;
  private int receiveBufferPos = 0;
  private boolean timedOut = false;
  private boolean ecgMissing = false;

  private FramedOutput outputFramer = null;
  private final Scheduler statusTimeout;
  private final Scheduler ecgDataTimeout;

  public Scheduler usbTimeout;

  private static final long CONTROLLER_STATUS_TIMEOUT = 5000;
  private static final long ECG_DATA_TIMEOUT = 1000; // Changed from 500 to 1000 per Vipul 20260212 MG
  private static final long USB_SIGNAL_TIMEOUT = 2000;
  private static D2xxManager ftD2xx;
  private boolean isBle = false;
  private boolean isControllerCommandDisabled = false;
  private final ExecutorService ecgCommandExecutor = Executors.newSingleThreadExecutor();

  public ControllerManager(final Context mContext) throws D2xxManager.D2xxException {
    context = mContext;
    ftD2xx = D2xxManager.getInstance(mContext);
    // Specify a non-default VID and PID combination to match if required.
    if (!ftD2xx.setVIDPID(0x0403, 0xada1)) {
      Log.i("ftd2xx-java", "setVIDPID Error");
    }
    statusTimeout =
            new Scheduler(timeoutHandler, CONTROLLER_STATUS_TIMEOUT, this::connectionStatusTimeout);
    ecgDataTimeout = new Scheduler(timeoutHandler, ECG_DATA_TIMEOUT, this::ecgStatusTimeout);
    usbTimeout = new Scheduler(timeoutHandler, USB_SIGNAL_TIMEOUT, this::usbTimeout);
    // Hardware = new UsbControllerHardware(mContext, ftD2xx);
    // Hardware = new BLEControllerHardware(mContext);
    // Hardware.setPacketReceiver(this);
  }

  public void configureHardware(boolean _isble) {
    try {
      if (Hardware != null) {
        Hardware.disconnectController();
        Hardware = null;
      }
    } catch (Exception ignored) {

    }
    if (_isble) {
      Hardware = new BLEControllerHardware(context);
      if (Hardware instanceof BLEControllerHardware) {
        ((BLEControllerHardware) Hardware).setOnDisconnectRequested(this::disconnectController);
      }
    } else {
      Hardware = new UsbControllerHardware(context, ftD2xx);
    }
    Hardware.setPacketReceiver(this);
    isBle = _isble;
  }

  public void addControllerListener(final ControllerListener evtListener) {
    this.controllerListeners.add(evtListener);
  }

  public void connectionStatusTimeout() {
    timedOut = true;
    // Post to UI thread to avoid CalledFromWrongThreadException
    timeoutHandler.post(() -> {
      controllerListeners.forEach(ControllerListener::onConnectionStatusTimeout);
    });
  }

  public void ecgStatusTimeout() {
    if (!checkConnection()) {
      return;
    }
    ecgMissing = true;
    // Post to UI thread to avoid CalledFromWrongThreadException
    timeoutHandler.post(() -> {
      controllerListeners.forEach(ControllerListener::onEcgStatusTimeout);
    });
  }

  public void usbTimeout() {
    MedDevLog.info("UsbControllerManager", "usbTimeout() -> ecgDataTimeout(true)");
    ecgMissing = ecgDataTimeout.circulateTimeout(true);
    // TODO : Check when this happens
    // Hardware.disconnectController();
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
    if (Hardware == null) {
      return false;
    }
    return Hardware.checkConnection();
  }

  public boolean isConnectionTimedOut() {
    return timedOut;
  }

  public boolean isEcgMissing() {
    return ecgMissing;
  }

  public void connectController() {
    connectFunction();
  }

  public void connectController(String deviceName) {
    connectFunction(deviceName);
  }

  @Override
  public void disconnectController() {
    disconnectFunction();
  }
  private void connectFunction() {
    Hardware.connectController(success -> {
      initializeConnection(success);
    });

  }
  private void initializeConnection(Boolean success) {
    if (success) {
      if (Hardware.checkConnection()) {
        Log.d("ControllerManager", "connectFunction: ");
        MedDevLog.info("Controller Connection", String.format(
            "Controller connected successfully - Connection: %s",
            isBle ? "Bluetooth" : "USB"));
        if (!sendingKeepAlive) {
          outputFramer = new FramedOutput();
          sendingKeepAlive = true;
          final KeepAliveThread keep_alive_thread = new KeepAliveThread();
          keep_alive_thread.start();
        }
        // Post to UI thread to avoid CalledFromWrongThreadException
        timeoutHandler.post(() -> {
          controllerListeners.forEach((el) -> el.onConnectionStatusChange(true));
        });
      } else {
        Log.d("Controller", "connectFunction: checkConnection Failed");
        MedDevLog.info("Controller Connection", String.format(
            "Controller connection failed (checkConnection) - Connection: %s",
            isBle ? "Bluetooth" : "USB"));
        Toast.makeText(context, "Failed to connect", Toast.LENGTH_LONG).show();
        // Post to UI thread to avoid CalledFromWrongThreadException
        timeoutHandler.post(() -> {
          controllerListeners.forEach((el) -> el.onConnectionStatusChange(false));
        });
      }
      timedOut = statusTimeout.circulateTimeout(false);
      ecgMissing = ecgDataTimeout.circulateTimeout(false);
      usbTimeout.circulateTimeout(false);
    } else {
      Log.d("ControllerManager", "connectFunction: Failed");
      MedDevLog.info("Controller Connection", String.format(
          "Controller connection failed - Connection: %s",
          isBle ? "Bluetooth" : "USB"));
      Toast.makeText(context, "Failed to connect", Toast.LENGTH_LONG).show();
      // Post to UI thread to avoid CalledFromWrongThreadException
      timeoutHandler.post(() -> {
        controllerListeners.forEach((el) -> el.onConnectionStatusChange(false));
      });
    }
  }

  private void connectFunction(String deviceName) {
    Hardware.connectController(success -> {
      initializeConnection(success);
    }, deviceName);
  }

  private void disconnectFunction() {
    MedDevLog.info("Controller Connection", String.format(
        "Controller disconnecting - Connection: %s",
        isBle ? "Bluetooth" : "USB"));
    usbTimeout.circulateTimeout(true);
    usbTimeout.cleanUp();
    ecgDataTimeout.cleanUp();

    packetCounter = 0;
    timestampCounter = 0;
    sendingKeepAlive = false;

    try {
      Thread.sleep(50);
    } catch (final InterruptedException e) {
      MedDevLog.error("ControllerManager", "Error calling thread sleep", e);
    }
    Log.d("disconnectFunction", "disconnectFunction: Checking connection");
    if (Hardware.checkConnection()) {
        Log.d("disconnectFunction", "disconnectFunction: Scheduling listener callbacks");
        // If already on main thread, call immediately; else post at front of queue
        if (Looper.myLooper() == Looper.getMainLooper()) {
          Log.d("disconnectFunction", "disconnectFunction: Calling listeners (direct)");
          controllerListeners.forEach((el) -> el.onConnectionStatusChange(false));
          Toast.makeText(context, "Device disconnected.", Toast.LENGTH_LONG).show();
        } else {
          timeoutHandler.postAtFrontOfQueue(() -> {
            Log.d("disconnectFunction", "disconnectFunction: Calling listeners (posted)");
            controllerListeners.forEach((el) -> el.onConnectionStatusChange(false));
          });
          timeoutHandler.post(() -> Toast.makeText(context, "Device disconnected.", Toast.LENGTH_LONG).show());
        }      
    }
    disconnectBluetooth();
    Hardware.disconnectController();
    outputFramer = null;
    timedOut = statusTimeout.circulateTimeout(true);
    statusTimeout.cleanUp();
  }

  public void notifyUSBDeviceAttach() {
    // connectController();
    // Hardware.connectController(success -> {
    // });
  }

  public void notifyUSBDeviceDetach() {
    try {
      if (!isBle && checkConnection()) {
        MedDevLog.info("ControllerManager", "USB device detached; disconnecting controller");
        disconnectFunction();
      }
    } catch (Exception e) {
      MedDevLog.error("ControllerManager", "notifyUSBDeviceDetach: disconnect failed", e);
    }
  }

  // In ControllerCommunicationManager.java (or the relevant interface/class)
  public boolean isBleConnection() {
    // Return true if BLE, false if USB
    return isBle; // or your actual logic/field
  }

  public void disconnectBluetooth() {
    if (isControllerCommandDisabled) {
      return;
    }
    if (Hardware == null || !Hardware.checkConnection()) {
      MedDevLog.warn("ControllerManager", "sendECGCommand : Hardware not connected");
      return;
    }

    if (Hardware instanceof BLEControllerHardware) {
      final int packetTimestamp = timestampCounter;
      final byte commandValue = (byte) (0x01);
      final byte[] disconnectPayload = {commandValue};
      final byte[] txBytes =
              outputFramer.addFraming(PacketId.DisconnectBluetooth.getByteValue(), packetTimestamp,
                      disconnectPayload);
      // sendMessage(txBytes);
      Log.d("ControllerManager", "sendDisconnectBluetooth: sending message");
      try {
        Hardware.sendMessage(txBytes);
      } catch (Exception e) {
        MedDevLog.error("ControllerManager", "sendDisconnectBluetooth: exception sending Disconnect Command", e);
      }
      timestampCounter++;
      packetCounter++;

      try {
        Thread.sleep(KEEP_ALIVE_PACKET_INTERVAL_MS);
      } catch (final InterruptedException e) {
        MedDevLog.error("ControllerManager", "Error calling thread sleep", e);
      }
    }

  }

  @Override
  public void sendECGCommand(boolean start) {
    if (isControllerCommandDisabled) {
      return;
    }
    // If hardware is missing or not connected, stop keepalive gracefully
    if (Hardware == null || !Hardware.checkConnection()) {
      MedDevLog.warn("ControllerManager", "sendECGCommand : Hardware not connected");
      return;
    }
    MedDevLog.info("ECG Command", String.format(
        "%s command queued for controller - Connection: %s",
        start ? "START ECG" : "STOP ECG", isBle ? "Bluetooth" : "USB"));

    // Execute on a dedicated background thread so the Thread.sleep calls
    // don't block the main/UI thread (which was causing the ECG data timeout
    // scheduler and Handler callbacks to be delayed, creating timing issues).
    // This is safe: Hardware.sendMessage() and outputFramer.addFraming() are
    // already called from the background KeepAliveThread.
    ecgCommandExecutor.execute(() -> {
      try {
        // Delay before sending ECG command — the controller hardware needs
        // time to be ready after connection establishment.
        // TODO : Applying temporary fix, immediate call is failing in case of reconnection
        try {
          Thread.sleep(1000);
        } catch (final InterruptedException e) {
          MedDevLog.error("ControllerManager", "Error calling thread sleep in sendECGCommand", e);
          return;
        }

        // Re-check connection after sleep (may have disconnected during wait)
        if (Hardware == null || !Hardware.checkConnection() || outputFramer == null) {
          MedDevLog.warn("ControllerManager", "sendECGCommand: Hardware disconnected during delay");
          return;
        }

        final int packetTimestamp = timestampCounter;
        final byte commandValue = (byte) (start ? 0x01 : 0x00);
        final byte[] startEcgPayload = {commandValue};
        final byte[] txBytes =
                outputFramer.addFraming(PacketId.StartEcg.getByteValue(), packetTimestamp,
                        startEcgPayload);
        Log.d("ControllerManager", "sendECGCommand: sending message");
        try {
          Hardware.sendMessage(txBytes);
        } catch (Exception e) {
          MedDevLog.error("ControllerManager", "sendECGCommand: exception sending ECG Command", e);
        }
        timestampCounter++;
        packetCounter++;
        // Reset ECG timeout AFTER sending start command
        if (start) {
          ecgMissing = ecgDataTimeout.circulateTimeout(false);
          MedDevLog.info("ControllerManager", "ECG timeout reset after sending start command");
        }

        try {
          Thread.sleep(KEEP_ALIVE_PACKET_INTERVAL_MS);
        } catch (final InterruptedException e) {
          MedDevLog.error("ControllerManager", "Error calling thread sleep", e);
        }
      } catch (Exception e) {
        MedDevLog.error("ControllerManager", "sendECGCommand: unexpected error on background thread", e);
      }
    });
  }

  public void deserializePacket(final byte[] readData) {
    MedDevLog.info(TAG," ***** In deserializePacket - Controller Manager *****"+readData.length+" Val = "+ Arrays.toString(readData));
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

  @Override
  public Scheduler getUsbTimeout() {
    return usbTimeout;
  }

  protected void onFramingError(final SerialFrameParser.Status expected, final byte actualByte) {
    MedDevLog.error("onFramingError", String.format("%s : %s", expected.toString(), actualByte));
  }

  private void onPacketArrival(final Packet incPacket) {
    Log.d("Packet Arrival", String.valueOf(incPacket.packetType));
    final PacketId id = PacketId.valueOfByte(incPacket.packetType);
    if( id == null ) {
      MedDevLog.error("onPacketArrival", String.format("Unknown Packet ID: %s", incPacket.packetType));
      return;
    }
    ecgMissing = ecgDataTimeout.circulateTimeout(false);

    // Post to UI thread to avoid CalledFromWrongThreadException
    timeoutHandler.post(() -> {
      controllerListeners.forEach((el) -> el.onPacketArrival(incPacket));
    });
    switch (Objects.requireNonNull(id)) {
      case EcgWaveform:
        final EcgSample ecgSample = new EcgSample(incPacket.timestamp, incPacket.payload);
        MedDevLog.info("ECG Data", String.format(
            "ECG packet received - Timestamp: %d ms, StartTime: %d ms, SamplingStatus: %s, Connection: %s",
            incPacket.timestamp, ecgSample.startTime_ms,
            ecgSample.samplingStatus != null ? ecgSample.samplingStatus.toString() : "N/A",
            isBle ? "Bluetooth" : "USB"));
        onFilteredSamples(incPacket.timestamp, ecgSample);
        //Log.w("EcgWaveform Timestamp", String.format("%s", ecgSample.startTime_ms));
        MedDevLog.debug("EcgWaveform", String.format("%s", ecgSample.startTime_ms));
        break;
      case RawAdcSamples:
        //Log.d("ControllerManager", "onPacketArrival: RawAdcSamples " + incPacket.packetType);
        break;
      case PatientHeartbeat:
        final Heartbeat heartbeat = new Heartbeat(incPacket.payload);
        MedDevLog.info("Heartbeat Data", String.format(
            "Heartbeat received - Timestamp: %d ms, P-Wave Ext: %.4f V, P-Wave Int: %.4f V, Connection: %s",
            incPacket.timestamp, heartbeat.pWaveExternalAmplitude_V,
            heartbeat.pWaveInternalAmplitude_V, isBle ? "Bluetooth" : "USB"));
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
            "Controller status received - Timestamp: %d ms, HeartRate: %.0f bpm, Battery: %.0f%%, Connection: %s",
            incPacket.timestamp, controllerStatus.heartRate_bpm,
            controllerStatus.batteryCharge_pct, isBle ? "Bluetooth" : "USB"));
        onControllerStatus(incPacket.timestamp, controllerStatus);
        break;
      case BiozData:
        try {
          StringBuilder hexString = new StringBuilder();
          for (byte b : incPacket.payload) {
            hexString.append(String.format("%02X ", b));
          }
          //Log.d("BiozData", "onPacketArrival: timestamp=" + incPacket.timestamp + ", length=" + incPacket.payload.length + ", payload=" + hexString.toString().trim());

          if (incPacket.payload.length >= 16) {
            // Extract uint32 values (little-endian) from payload
            long ts = ((incPacket.payload[0] & 0xFF) << 0) |
                    ((incPacket.payload[1] & 0xFF) << 8) |
                    ((incPacket.payload[2] & 0xFF) << 16) |
                    ((incPacket.payload[3] & 0xFF) << 24);
            long imp1 = ((incPacket.payload[4] & 0xFF) << 0) |
                    ((incPacket.payload[5] & 0xFF) << 8) |
                    ((incPacket.payload[6] & 0xFF) << 16) |
                    ((incPacket.payload[7] & 0xFF) << 24);
            long imp2 = ((incPacket.payload[8] & 0xFF) << 0) |
                    ((incPacket.payload[9] & 0xFF) << 8) |
                    ((incPacket.payload[10] & 0xFF) << 16) |
                    ((incPacket.payload[11] & 0xFF) << 24);
            long imp3 = ((incPacket.payload[12] & 0xFF) << 0) |
                    ((incPacket.payload[13] & 0xFF) << 8) |
                    ((incPacket.payload[14] & 0xFF) << 16) |
                    ((incPacket.payload[15] & 0xFF) << 24);

            // Convert to float Ohms (divide by 1000)
//            float imp1_ohms = imp1 / 1000.0f;
//            float imp2_ohms = imp2 / 1000.0f;
//            float imp3_ohms = imp3 / 1000.0f;
//            float timestamp_ms = ts;
//            MedDevLog.debug("BiozData", String.format("onPacketArrival: timestamp=%d (0x%04X), imp1=%d, imp2=%d, imp3=%d",
//                    incPacket.timestamp, ts, imp1, imp2, imp3));

            final ImpedanceSample impedanceSample =
                    new ImpedanceSample(ts, imp1, imp2, imp3);// This uses kohms
            MedDevLog.info("BioZ Data", String.format(
                "BioZ packet received - Timestamp: %d ms, BioZ_Timestamp: %d ms, Imp1: %d, Imp2: %d, Imp3: %d, Connection: %s",
                incPacket.timestamp, ts, imp1, imp2, imp3, isBle ? "Bluetooth" : "USB"));
            onCatheterImpedance(incPacket.timestamp, impedanceSample);
          }
        } catch (Exception e) {
          MedDevLog.error("BiozData", "onPacketArrival: exception parsing BiozData", e);
        }
        break;
      default:
        final int subTypeValue = incPacket.packetType & 0b11;
        MedDevLog.error("Unpacking Error",
                String.format("Unknown packet ID %s , type %s", incPacket.payload.length,
                        SerialFrameParser.PacketSubId.valueOfByte((byte) subTypeValue)));
        break;
    }
  }

  public void onFilteredSamples(final long timestamp, final EcgSample sample) {
    final String sampleStr =
            String.format("Timestamp: %s ms, Start Time; %s, Sampling Status: %s, Samples: %s",
                    timestamp, sample.startTime_ms,
                    sample.samplingStatus != null ? sample.samplingStatus.toString() : R.string.N_A,
                    Arrays.deepToString(sample.samples));
    Log.i("onFilteredSamples", sampleStr);
    // Post to UI thread to avoid CalledFromWrongThreadException
    timeoutHandler.post(() -> {
      controllerListeners.forEach((el) -> el.onFilteredSamples(sample, sampleStr));
    });
  }

  public void onCatheterImpedance(final long timestamp, final ImpedanceSample impedanceSample) {
    final String sampleStr =
            String.format("Impedance timestamp + Sample Received - Timestamp: %s ms, Start Time; %s, Sampling Status: %s, Samples: %s",
                    timestamp, impedanceSample.timestamp_ms,
                    R.string.N_A,
                    "N/A");
    Log.i("onCatheterImpedance", sampleStr);
    // Post to UI thread to avoid CalledFromWrongThreadException
    timeoutHandler.post(() -> {
      try {
        controllerListeners.forEach((el) -> el.onCatheterImpedance(impedanceSample, sampleStr));
      } catch (Exception e) {
        MedDevLog.error("onCatheterImpedance", "Exception in onCatheterImpedance listener", e);
      }
    });
  }

  public void onPatientHeartbeat(final long timestamp, final Heartbeat heartbeat) {
    final String heartbeatStr =
            String.format("Timestamp: %s, P-Wave External Amp (V): %s, P-Wave Internal Amp (V): %s",
                    timestamp, heartbeat.pWaveExternalAmplitude_V,
                    heartbeat.pWaveInternalAmplitude_V);
    Log.i("onPatientHeartbeat", heartbeatStr);
    // Post to UI thread to avoid CalledFromWrongThreadException
    timeoutHandler.post(() -> {
      controllerListeners.forEach((el) -> el.onPatientHeartbeat(heartbeat, heartbeatStr));
    });
  }

  public void onButtonsStateChange(final long timestamp, final Button newButton) {
    final String buttonStr = String.format(
            "Timestamp: %s, Up: %s, Down: %s, Left: %s, Right: %s, Select: %s, Capture: %s",
            timestamp, newButton.buttons.getButtonStateAtKey(Button.ButtonIndex.Up),
            newButton.buttons.getButtonStateAtKey(Button.ButtonIndex.Down),
            newButton.buttons.getButtonStateAtKey(Button.ButtonIndex.Left),
            newButton.buttons.getButtonStateAtKey(Button.ButtonIndex.Right),
            newButton.buttons.getButtonStateAtKey(Button.ButtonIndex.Select),
            newButton.buttons.getButtonStateAtKey(Button.ButtonIndex.Capture));
    MedDevLog.info("ControllerManager", "onButtonsStateChange: " + buttonStr);
    // Post to UI thread to avoid CalledFromWrongThreadException
    timeoutHandler.post(() -> {
      buttonInputListeners.forEach((el) -> el.onButtonsStateChange(newButton, buttonStr));
    });
  }

  public void onStartupStatus(final long timestamp, final StartupStatus startupStatus) {
    final String startupStatusStr =
            String.format("Timestamp: %s, SFU Status: %s, App Status: %s", timestamp,
                    startupStatus.sfuStatus, startupStatus.appStatus);
    Log.i("onStartupStatus", startupStatusStr);
    // Post to UI thread to avoid CalledFromWrongThreadException
    timeoutHandler.post(() -> {
      controllerListeners.forEach((el) -> el.onStartupStatus(startupStatus, startupStatusStr));
    });
  }

  public void onControllerStatus(final long timestamp, final ControllerStatus controllerStatus) {
    final String controllerStr =
            String.format("Timestamp: %s, Heart Rate: %s, Charge Level: %s", timestamp,
                    controllerStatus.heartRate_bpm, controllerStatus.batteryCharge_pct);
    // Log.i("onControllerStatus", controllerStr);
    timedOut = statusTimeout.circulateTimeout(false);
    // Post to UI thread to avoid CalledFromWrongThreadException
    timeoutHandler.post(() -> {
      controllerListeners.forEach((el) -> el.onControllerStatus(controllerStatus, controllerStr));
    });
  }

  private class KeepAliveThread extends Thread {
    KeepAliveThread() {
      this.setPriority(Thread.MIN_PRIORITY);
    }

    @Override
    public void run() {
      while (sendingKeepAlive) {
        // If hardware is missing or not connected, stop keepalive gracefully
        if (Hardware == null || !Hardware.checkConnection()) {
          MedDevLog.warn("ControllerManager", "KeepAliveThread: Hardware not connected");
          sendingKeepAlive = false;
          break;
        }
        final int packetTimestamp = timestampCounter;
        final int sequenceNumber = packetCounter;
        final byte[] keepAlivePayload = SerialHelper.packInt(sequenceNumber);
        final byte[] txBytes =
                outputFramer.addFraming(PacketId.TabletKeepalive.getByteValue(), packetTimestamp,
                        keepAlivePayload);
        // sendMessage(txBytes);
        Log.d("ControllerManager", "run: sending message");
        try {
          if (!isBle) {
            Hardware.sendMessage(txBytes);
          }
        } catch (Exception e) {
          MedDevLog.error("ControllerManager", "KeepAliveThread: exception sending keepalive", e);
        }
        timestampCounter++;
        packetCounter++;

        try {
          Thread.sleep(KEEP_ALIVE_PACKET_INTERVAL_MS);
        } catch (final InterruptedException e) {
          MedDevLog.error("ControllerManager", "Error calling thread sleep", e);
        }
      }
    }
  }
  public FotaClient createFotaClient()
  {
    if (Hardware == null || !Hardware.checkConnection())
    {
      MedDevLog.info(TAG,"Fota client hardware null or unable to connect");
      return null;
    }
    if (Hardware instanceof UsbControllerHardware)
    {
      MedDevLog.info(TAG,"Fota Client in hardware is UsbControllerHardware");
      UsbControllerHardware usbHardware = (UsbControllerHardware) Hardware;
      InputStream inputStream = usbHardware.getFotaInputStream();
      OutputStream outputStream = usbHardware.getFotaOutputStream();
      if (inputStream != null && outputStream != null)
      {
        MedDevLog.info(TAG,"hardware instance input/output stream not null");
        FotaClient fclient = new FotaClient();
        fclient.setStreams(inputStream,outputStream);
        return fclient;
      }
    }
    return null;
  }

  public void allowAccess(boolean isAllowed)
  {
    if (Hardware == null || !Hardware.checkConnection())
    {
      return;
    }
    if (Hardware instanceof UsbControllerHardware)
    {
      UsbControllerHardware usbHardware = (UsbControllerHardware) Hardware;
      usbHardware.updateReads(isAllowed);
    }
  }
}