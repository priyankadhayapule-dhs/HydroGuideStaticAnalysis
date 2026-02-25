package com.echonous.usb;


/**
 * Enum to capture possible different USB device states
 */
public enum UsbDeviceState
{
    /** USB device is connected and is running normally */
    CONNECTED,
    /** USB device is connected and forced hibernate command has been sent. */
    CONNECTED_HIBERNATE_CALLED,
    /** USB device is in hibernation. */
    DISCONNECTED_HIBERNATE,
    /** USB device is disconnected. */
    DISCONNECTED,
    /** The device is in the process of powering off */
    POWERING_OFF,
    /** The device has been powered off */
    POWERED_OFF,
}
