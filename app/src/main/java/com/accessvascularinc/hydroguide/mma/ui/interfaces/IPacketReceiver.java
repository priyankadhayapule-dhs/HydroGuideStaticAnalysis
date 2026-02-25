package com.accessvascularinc.hydroguide.mma.ui.interfaces;

import com.accessvascularinc.hydroguide.mma.ui.Scheduler;

public interface IPacketReceiver {
  void deserializePacket(final byte[] readData);
  Scheduler getUsbTimeout();
}
