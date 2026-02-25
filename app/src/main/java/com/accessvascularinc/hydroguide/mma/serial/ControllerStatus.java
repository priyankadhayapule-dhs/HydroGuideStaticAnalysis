package com.accessvascularinc.hydroguide.mma.serial;

import android.util.Log;

import com.accessvascularinc.hydroguide.mma.serial.flags.BitFlags;
import com.accessvascularinc.hydroguide.mma.serial.flags.ChargerStatusFlags;
import com.accessvascularinc.hydroguide.mma.serial.flags.ControllerStatusFlags;
import com.accessvascularinc.hydroguide.mma.serial.flags.EcgAnalysisStatusFlags;
import com.accessvascularinc.hydroguide.mma.serial.flags.FuelGaugeStatusFlags;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

public class ControllerStatus {
  public final int BYTE_LENGTH = 84;
  public int productID = 0;
  public int softwareVersion = 0;  // Encoded major (8 bits), minor (8 bits), patch (16 bits).
  public int softwareRevision = 0; // This will come from source control system.
  public int deviceUniqueId = 0;
  public int hardwareVersion = 0;
  public float heartRate_bpm = 0.0F;
  public float batteryCharge_pct = 0.0F;
  public float batteryTemperature_C = 0.0F;
  public float fuelGaugeTemperature_C = 0.0F;
  public float fuelGaugeVoltage_V = 0.0F;
  public float fuelGaugeChargingCurrent_A = 0.0F;
  public float systemVoltage_V = 0.0F;
  public float batteryVoltage_V = 0.0F;
  public float vProgVoltage_V = 0.0F;
  public float usbVBUSVoltage_V = 0.0F;
  public float usbCC1Voltage_V = 0.0F;
  public float usbCC2Voltage_V = 0.0F;
  public BitFlags<ControllerStatusFlags> controllerStatus = null;
  public BitFlags<EcgAnalysisStatusFlags> ecgAnalysisStatus = null;
  public BitFlags<FuelGaugeStatusFlags> fuelGaugeStatus = null;
  public BitFlags<ChargerStatusFlags> chargerStatus = null;

  public ControllerStatus(final byte[] payload) {
    if (payload.length == BYTE_LENGTH) {
      productID = SerialHelper.unpackInt(payload, 0);
      softwareVersion = SerialHelper.unpackInt(payload, 4);
      softwareRevision = SerialHelper.unpackInt(payload, 8);
      deviceUniqueId = SerialHelper.unpackInt(payload, 12);
      hardwareVersion = SerialHelper.unpackInt(payload, 16);
      heartRate_bpm = SerialHelper.unpackFloat(payload, 20);
      batteryCharge_pct = SerialHelper.unpackFloat(payload, 24);
      batteryTemperature_C = SerialHelper.unpackFloat(payload, 28);
      fuelGaugeTemperature_C = SerialHelper.unpackFloat(payload, 32);
      fuelGaugeVoltage_V = SerialHelper.unpackFloat(payload, 36);
      fuelGaugeChargingCurrent_A = SerialHelper.unpackFloat(payload, 40);
      systemVoltage_V = SerialHelper.unpackFloat(payload, 44);
      batteryVoltage_V = SerialHelper.unpackFloat(payload, 48);
      vProgVoltage_V = SerialHelper.unpackFloat(payload, 52);
      usbVBUSVoltage_V = SerialHelper.unpackFloat(payload, 56);
      usbCC1Voltage_V = SerialHelper.unpackFloat(payload, 60);
      usbCC2Voltage_V = SerialHelper.unpackFloat(payload, 64);
      controllerStatus = ControllerStatusFlags.valueOfInt(SerialHelper.unpackInt(payload, 68));
      ecgAnalysisStatus = EcgAnalysisStatusFlags.valueOfInt(SerialHelper.unpackInt(payload, 72));
      fuelGaugeStatus = FuelGaugeStatusFlags.valueOfInt(SerialHelper.unpackInt(payload, 76));
      // ignoring the 10th bit which is for controller internal usage.
      chargerStatus = ChargerStatusFlags.valueOfInt(SerialHelper.unpackInt(payload, 80) & 0x3FF);
    } else {
      MedDevLog.error("ControllerStatus Unpacking", String.format(
              "Length mismatch for ControllerStatus, got %s bytes, expected 84", payload.length));
    }
  }
}
