package com.accessvascularinc.hydroguide.mma.ui;

import com.accessvascularinc.hydroguide.mma.serial.ControllerStatus;
import com.accessvascularinc.hydroguide.mma.serial.EcgSample;
import com.accessvascularinc.hydroguide.mma.serial.Heartbeat;
import com.accessvascularinc.hydroguide.mma.serial.ImpedanceSample;
import com.accessvascularinc.hydroguide.mma.serial.Packet;
import com.accessvascularinc.hydroguide.mma.serial.StartupStatus;

public interface ControllerListener {
  void onConnectionStatusChange(boolean isConnected);

  void onPacketArrival(Packet incPacket);

  void onCatheterImpedance(ImpedanceSample impedanceSample, String impedanceLogMsg);

  void onFilteredSamples(EcgSample sample, String sampleLogMsg);

  void onPatientHeartbeat(Heartbeat heartbeat, String heartbeatLogMsg);

  void onControllerStatus(ControllerStatus controllerStatus, String controllerLogMsg);

  void onStartupStatus(StartupStatus startupStatus, String startupStatusLogMsg);

  void onConnectionStatusTimeout();

  void onEcgStatusTimeout();
}
