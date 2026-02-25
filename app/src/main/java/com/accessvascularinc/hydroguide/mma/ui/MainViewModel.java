package com.accessvascularinc.hydroguide.mma.ui;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import com.accessvascularinc.hydroguide.mma.model.ChartConfig;
import com.accessvascularinc.hydroguide.mma.model.ControllerState;
import com.accessvascularinc.hydroguide.mma.model.EncounterData;
import com.accessvascularinc.hydroguide.mma.model.MonitorData;
import com.accessvascularinc.hydroguide.mma.model.ProfileState;
import com.accessvascularinc.hydroguide.mma.model.TabletState;
import com.accessvascularinc.hydroguide.mma.serial.Button;
import com.accessvascularinc.hydroguide.mma.ui.interfaces.IControllerCommunicationManager;

/**
 * The main view model used to share data across all the fragments in the app.
 */
public class MainViewModel extends ViewModel {

  public enum BleConnectionState {
    NOTCONNECTED,
    CONNECTED,
    CONNECTING,
    FAILED
  }
  private final TabletState tabletState = new TabletState();
  private final ControllerState controllerState = new ControllerState();
  private final ChartConfig chartConfig = new ChartConfig();
  private EncounterData encounterData = new EncounterData();
  private final MonitorData monitorData = new MonitorData();
  private final ProfileState profileState = new ProfileState();
  private EncounterData procedureReport = null;
  private UsbControllerManager usbControllerManager = null;
  private ControllerManager controllerManager = null;
  private boolean isBleCommunication = false;
  // Persist which tab (local vs cloud) was last selected in RecordsFragment.
  private boolean recordsIsCloudMode = false;

  private boolean IsMockCycleStarted = false;
  private MutableLiveData<BleConnectionState> bleConnectionLiveData = new MutableLiveData<>(BleConnectionState.NOTCONNECTED);
  private MutableLiveData <Button.ButtonIndex> buttonIndex = new MutableLiveData<>();

  /**
   * Gets the state of the tablet.
   *
   * @return the observable instance of the tablet state.
   */
  public TabletState getTabletState() {
    return tabletState;
  }

  /**
   * Gets the state of the ECG controller.
   *
   * @return the observable instance of the controller state.
   */
  public ControllerState getControllerState() {
    return controllerState;
  }

  /**
   * Gets settings or configuration of charts in the procedure screen.
   *
   * @return the observable instance of the chart config.
   */
  public ChartConfig getChartConfig() {
    return chartConfig;
  }

  /**
   * Gets the state of the current procedure.
   *
   * @return the observable instance of the encounter data.
   */
  public EncounterData getEncounterData() {
    return encounterData;
  }

  public void setEncounterData(final EncounterData record) {
    encounterData = record;
  }

  /**
   * Gets the procedure selected for generating or viewing a PDF report..
   *
   * @return the observable instance of the encounter data.
   */
  public EncounterData getProcedureReport() {
    return procedureReport;
  }

  public void setProcedureReport(final EncounterData report) {
    this.procedureReport = report;
  }

  /**
   * Gets state of monitored data during a procedure.
   *
   * @return the observable instance of the monitor data.
   */
  public MonitorData getMonitorData() {
    return monitorData;
  }

  /**
   * Gets the controller manager that handles communication with the ECG controller over USB.
   *
   * @return the controller manager for communicating with the ECG controller.
   */
  public IControllerCommunicationManager getControllerCommunicationManager() {
    return controllerManager;    
  }



  /**
   * Injects controller manager needed to communicate with the controller over USB.
   *
   * @param newUsbControllerManager the controller manager needed to communicated with the
   *                                controller.
   */
  public void injectUsbControllerManager(final UsbControllerManager newUsbControllerManager) {
    this.usbControllerManager = newUsbControllerManager;
  }

  public void injectControllerManager(final ControllerManager newcontrollerManager)
  {
    this.controllerManager = newcontrollerManager;
  }

  public ProfileState getProfileState() {
    return profileState;
  }

  // Records tab persistence
  public boolean getRecordsIsCloudMode() {
    return recordsIsCloudMode;
  }

  public void setRecordsIsCloudMode(boolean cloud) {
    this.recordsIsCloudMode = cloud;
  }

  public boolean getMockCycle() {
    return IsMockCycleStarted;
  }

  public void setMockCycle(boolean mockvalue) {
    this.IsMockCycleStarted = mockvalue;
  }

  public LiveData<BleConnectionState> getBleConnectionLiveData() {
    return bleConnectionLiveData;
  }

  public void setBleConnection(BleConnectionState state) {
    bleConnectionLiveData.postValue(state);
  }

  public LiveData<Button.ButtonIndex> getButtonIndex() {
    return buttonIndex;
  }

  public void setButtonIndex(Button.ButtonIndex value) {
    buttonIndex.postValue(value);
  }
}