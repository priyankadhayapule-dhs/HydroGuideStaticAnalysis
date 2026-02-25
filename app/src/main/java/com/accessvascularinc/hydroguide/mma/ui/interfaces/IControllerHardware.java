package com.accessvascularinc.hydroguide.mma.ui.interfaces;

import java.util.function.Consumer;

public interface IControllerHardware {
  void connectController(Consumer<Boolean> onResult);

  void connectController(Consumer<Boolean> onResult, String deviceName);

  boolean checkConnection();

  void sendMessage(final byte[] writeData);

  void disconnectController();
  
  void setPacketReceiver(IPacketReceiver client);
}
