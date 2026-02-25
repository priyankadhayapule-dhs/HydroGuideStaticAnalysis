package com.accessvascularinc.hydroguide.mma.ui.interfaces;

import com.accessvascularinc.hydroguide.mma.ui.ControllerListener;
import com.accessvascularinc.hydroguide.mma.ui.input.ButtonInputListener;

public interface IControllerCommunicationManager {
  // --- Listener Management ---
  void addControllerListener(ControllerListener evtListener);

  void removeControllerListener(ControllerListener evtListener);

  void addButtonInputListener(ButtonInputListener evtListener);

  void removeButtonInputListener(ButtonInputListener evtListener);

  // --- Connection & Status ---
  boolean checkConnection();

  boolean isConnectionTimedOut();

  boolean isEcgMissing();

  void connectController();

  void connectController(String deviceName);

  void disconnectController();

  void notifyUSBDeviceAttach();

  void notifyUSBDeviceDetach();

  // --- Timeout & Event Handling ---
  void connectionStatusTimeout();

  void ecgStatusTimeout();

  void usbTimeout();

  // --- Data/Event Callbacks ---
    void onFilteredSamples(long timestamp, com.accessvascularinc.hydroguide.mma.serial.EcgSample sample);
    void onPatientHeartbeat(long timestamp, com.accessvascularinc.hydroguide.mma.serial.Heartbeat heartbeat);
    void onButtonsStateChange(long timestamp, com.accessvascularinc.hydroguide.mma.serial.Button newButton);
    void onStartupStatus(long timestamp, com.accessvascularinc.hydroguide.mma.serial.StartupStatus startupStatus);
    void onControllerStatus(long timestamp, com.accessvascularinc.hydroguide.mma.serial.ControllerStatus controllerStatus);
    void onCatheterImpedance(long timestamp, com.accessvascularinc.hydroguide.mma.serial.ImpedanceSample impedanceSample);


  void configureHardware(boolean isble);

  boolean isBleConnection();

  void sendECGCommand(boolean start);
}
