package com.echonous.kLink;


import static com.echonous.kLink.KLinkConstants.KLINK_CMD_FLASH_ROW;
import static com.echonous.kLink.KLinkConstants.KLINK_REQUEST;
import static com.echonous.kLink.KLinkConstants.KLINK_REQUEST_TYPE_OUT;
import static com.echonous.kLink.KLinkConstants.KLINK_TRANSACTION_TIMEOUT_MS;
import android.content.Context;
import android.hardware.usb.UsbDeviceConnection;

import com.echonous.util.FileHelper;
import com.echonous.util.LogAreas;
import com.echonous.util.LogUtils;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

/**
 * Class that manages the INTEL hex format hub firmware image in the assets
 * that is used to check and update the firmware image running on the hub.
 *
 * The hub contains a bootloader and two separate firmware images. The hex image
 * contains all three of these, but we only try to update the inactive firmware
 * image.
 */
public class KLinkFirmwareImage
{

    //==========================================================================
    // Public Section
    //==========================================================================

    /**
     * Enum value used to identify each of the firmware images that can be loaded on the hub
     */
    enum Firmware
    {
        /** The first firmware image */
        FIRMWARE1,
        /** The second firmware image */
        FIRMWARE2,
    }

    //==========================================================================
    // Constructors and object creation

    /**
     * Constructor
     *
     * @param aContext The context the application is running in to access assets.
     */
    public KLinkFirmwareImage(Context aContext)
    {
        mContext = aContext;
    }

    //==========================================================================
    // Information functions

    /**
     * Gets whether a valid firmware image has been loaded
     */
    public boolean isValid()
    {
        return mValid;
    }

    /**
     * Gets the major version of the loaded firmware image
     */
    public short getMajorVer()
    {
        return mMajorVer;
    }

    /**
     * Gets the minor version of the loaded firmware image
     */
    public short getMinorVer()
    {
        return mMinorVer;
    }

    /**
     * Gets the revision of the loaded firmware image
     */
    public short getRev()
    {
        return mRevision;
    }

    /**
     * Creates a string that represents the full software version loaded.
     */
    public String getVersionString()
    {
        StringBuilder builder = new StringBuilder();
        builder.append(mMajorVer).append('.').append(mMinorVer).append('.').append(mRevision);
        return builder.toString();
    }

    //==========================================================================
    // Firmware loading and flashing functions

    /**
     * Loads a firmware image into memory ready for version comparision and flashing
     *
     * @param aFileName The assets file name of the firmware hex file to load
     *
     * @return Whether a valid firmware image was successfully loaded
     */
    public boolean loadFirmwareImage(String aFileName)
    {
        LogUtils.i(TAG, "loadFirmwareImage asset path " + mContext.getAssets().toString());
        // Create a buffer to hold the image data
        mImageData = new byte[FLASH_END_ADDRESS];
        // Mark the image as invalid initially.
        mValid = false;

        BufferedReader reader = null;
        try
        {
            // Open the firmware file and loop reading the lines
            reader = new BufferedReader(
                    new InputStreamReader(mContext.getAssets().open(aFileName)));
            String line;
            long address_upper = 0;
            while ((line = reader.readLine()) != null)
            {
                // Check we have a valid line
                if (line.length() < INTEL_HEX_HEADER_LENGTH || line.charAt(0) != INTEL_HEX_LINE_START)
                    return false;
                else
                {
                    // Extract the byte count, address and record type fields
                    int byte_count = readHexIntValue(line, INTEL_HEX_OFFSET_BYTE_COUNT, INTEL_HEX_BYTE_COUNT_CHARS);
                    int address_low = readHexIntValue(line, INTEL_HEX_OFFSET_ADDRESS, INTEL_HEX_ADDRESS_CHARS);
                    int record_type = readHexIntValue(line, INTEL_HEX_OFFSET_RECORD_TYPE, INTEL_HEX_RECORD_TYPE_CHARS);

                    // Handle based on the record data
                    switch (record_type)
                    {
                        case INTEL_HEX_DATA_RECORD_TYPE:
                            // This is firmware data - extract the data if valid.
                            long address_full = address_upper | address_low;
                            if (address_full + byte_count <= FLASH_END_ADDRESS)
                            {
                                extractHexData(line, INTEL_HEX_OFFSET_DATA_START, byte_count, mImageData,
                                        (int)address_full);
                            }
                            break;
                        case INTEL_HEX_EXTENDED_ADDRESS_RECORD_TYPE:
                            // This provides a new value for the upper 16 bits of the address
                            address_upper = readHexIntValue(line, INTEL_HEX_OFFSET_DATA_START, INTEL_HEX_ADDRESS_CHARS);
                            address_upper <<= 16;
                            break;
                        case INTEL_HEX_END_OF_FILE_RECORD_TYPE:
                            break;
                        default:
                            LogUtils.e(TAG, "Unknown record type " + record_type);
                            break;
                    }
                }
            }
        }
        catch (IOException e)
        {
            LogUtils.e(TAG, "Failed to read hex file");
        }
        finally
        {
            FileHelper.closeQuietly(reader);
        }

        // Now try to read the version information from the image.
        // First, we must read the row number of the last boot row from the metadata.
        int last_boot_row = getWord(mImageData, LAST_BOOT_META_POS);
        // If this is not before the start of Firmware1 image 1, it is invalid, to fail.
        if (last_boot_row >= FIRMWARE_2_START_ROW)
        {
            LogUtils.e(TAG, "Invalid last boot row " + last_boot_row);
            return false;
        }
        // Determine the offset of the version information from the read last boot row.
        int version_offset = (last_boot_row + 1)*FLASH_ROW_SIZE + VERSION_OFFSET;

        // Read out the 3 fields - these are all defined by the Cypress SDK
        mRevision = mImageData[version_offset];
        mMajorVer = (short)((mImageData[version_offset + 1] >> 4) & 0xf);
        mMinorVer = (short)(mImageData[version_offset + 1] & 0xf);

        LogUtils.i(TAG, "Bundled Hub Firmware Version " + mMajorVer + "." + mMinorVer + "." + mRevision);
        mValid = true;
        return true;
    }


    /**
     * Writes a firmware image to a connected Uscan Hub
     *
     * @param aFirmware Identifier of which firmware location to write to
     * @param aDeviceConnection The connected device to write the image to
     *
     * @return Whether writing the image was successful.
     */
    public boolean writeFirmwareImage(Firmware aFirmware, UsbDeviceConnection aDeviceConnection)
    {
        // The loaded firmware contains, the bootloader, firmware1 and firmware2. We only write a single
        // firmware image which is defined by the rows it impacts - the actual firmware image, and the
        // metdata for it.
        int start_row = aFirmware == Firmware.FIRMWARE1 ? FIRMWARE_1_START_ROW : FIRMWARE_2_START_ROW;
        int last_row = aFirmware == Firmware.FIRMWARE1 ? FIRMWARE_2_START_ROW - 1 : FIRMWARE_2_END_ROW;
        int meta_row = aFirmware == Firmware.FIRMWARE1 ? FIRMWARE_1_METADATA_ROW : FIRMWARE_2_METADATA_ROW;

        // Loop through and write all the rows of the image itself.
        for (int i = start_row; i <= last_row; i++)
        {
            int offset = i*FLASH_ROW_SIZE;
            if (aDeviceConnection.controlTransfer(KLINK_REQUEST_TYPE_OUT, KLINK_REQUEST, KLINK_CMD_FLASH_ROW, i,
                    mImageData, offset, FLASH_ROW_SIZE, KLINK_TRANSACTION_TIMEOUT_MS) < 0)
            {
                LogUtils.e(TAG, "Failed to flash row " + i);
                return false;
            }
        }

        // Finally flash the metadata row
        int offset = meta_row*FLASH_ROW_SIZE;
        if (aDeviceConnection.controlTransfer(KLINK_REQUEST_TYPE_OUT, KLINK_REQUEST, KLINK_CMD_FLASH_ROW, meta_row,
                mImageData, offset, FLASH_ROW_SIZE, KLINK_TRANSACTION_TIMEOUT_MS) < 0)
        {
            LogUtils.e(TAG, "Failed to flash metadata row " + meta_row);
            return false;
        }

        return true;
    }

    /**
     * Reads a 16-bit (little endian) word of data from a firmware image
     *
     * @param aBuffer The buffer to read the data from
     * @param aIndex The offset into the buffer to start
     *
     * @return The read 16-bit (unsigned) value
     */
    public static int getWord(byte[] aBuffer, int aIndex)
    {

        int temp = (((int)aBuffer[aIndex]) & 0xff) | ((((int)aBuffer[aIndex + 1]) & 0xff) << 8);
        return temp;
    }

    /**
     * Reads a signed 16-bit (little endian) word of data from a firmware image
     *
     * @param aBuffer The buffer to read the data from
     * @param aIndex The offset into the buffer to start
     *
     * @return The read 16-bit (unsigned) value
     */
    public static short getInt16(byte[] aBuffer, int aIndex)
    {
        return (short)(((aBuffer[aIndex + 1] & 0xff) << 8) | (aBuffer[aIndex] & 0xff));
    }

    /**
     * Reads an 8-bit unsigned value from a byte buffer
     *
     * @param aBuffer The buffer to read the data from
     * @param aIndex The offset into the buffer to start
     *
     * @return Unsigned 8-bit value held in a short
     */
    public static short getUint8(byte[] aBuffer, int aIndex)
    {
        return (short)(aBuffer[aIndex] & 0xff);
    }

    //==========================================================================
    // Private Section
    //==========================================================================

    //==========================================================================
    // Helper Functions Section

    /**
     * Extracts data from a hex formatted string into a byte array
     *
     * @param aString The string to extract the data from
     * @param aStrOffset The position in the string to start from
     * @param aBytes The number of bytes to be read
     * @param aDest The destination buffer to write the binary data to
     * @param aDestOffset The offset in the destination buffer to write the data to
     */
    private void extractHexData(String aString, int aStrOffset, int aBytes, byte[] aDest, int aDestOffset)
    {
        int str_index = aStrOffset;
        for (int i = 0; i < aBytes; i++)
        {
            int high_nibble = Character.getNumericValue(aString.charAt(str_index++));
            int low_nibble = Character.getNumericValue(aString.charAt(str_index++));
            int full = (high_nibble << 4) | low_nibble;
            aDest[aDestOffset++] = (byte)full;
        }
    }

    /**
     * Reads a hex format integer string
     *
     * @param aString The string to read from
     * @param aOffset The offset in the string to start reading from
     * @param aLength The number of hex chars to read
     *
     * @return The read value as an integer.
     */
    private int readHexIntValue(String aString, int aOffset, int aLength)
    {
        return Integer.parseInt(aString.substring(aOffset, aOffset + aLength), HEX_RADIX);
    }


    //==========================================================================
    // Members

    /** Context used to access resources */
    private Context mContext = null;

    /** Buffer to hold the full firmware image of a firmware file */
    private byte[] mImageData = null;

    /** The major version read from the firmware file */
    private short mMajorVer = 0;
    /** The minor version read from the firmware file */
    private short mMinorVer = 0;
    /** The revision read from the firmware file */
    private short mRevision = 0;

    /** Flag that indicates if a valid firmware file has been read */
    private boolean mValid = false;

    //==========================================================================
    // Constants

    /** Logging tag */
    private static final String TAG = LogAreas.KLINK;

    /** The size of a flash row in the hub in bytes */
    private static final int FLASH_ROW_SIZE = 128;

    /** The Flash end address in the hub */
    private static final int FLASH_END_ADDRESS = 0x20000;

    /** The number of flash rows in the hub flash */
    private static final int FLASH_ROWS = FLASH_END_ADDRESS/FLASH_ROW_SIZE;

    /** The start address of the Firmware1 image */
    private static final int FIRMWARE_1_START_ADDRESS = 0x1800;
    /** The start row of the Firmware1 image */
    private static final int FIRMWARE_1_START_ROW = FIRMWARE_1_START_ADDRESS/FLASH_ROW_SIZE;
    /** The metadata row of the Firmware1 image */
    private static final int FIRMWARE_1_METADATA_ROW = FLASH_ROWS - 1;

    /** The start address of the Firmware2 image */
    private static final int FIRMWARE_2_START_ADDRESS = 0x10A80;
    /** The start row of the Firmware2 image */
    private static final int FIRMWARE_2_START_ROW = FIRMWARE_2_START_ADDRESS/FLASH_ROW_SIZE;
    /** The last row of the Firmware2 image */
    private static final int FIRMWARE_2_END_ROW = FLASH_ROWS - 7;
    /** The metadata row of the Firmware21 image */
    private static final int FIRMWARE_2_METADATA_ROW = FLASH_ROWS - 2;

    /** The radix to use for parting hex values */
    private static final int HEX_RADIX = 16;

    /** Character that defines the start of a line in an INTEL HEX format file */
    private static final char INTEL_HEX_LINE_START = ':';
    /** The offset in a line of an INTEL hex file that contains the data byte count of the line */
    private static final int INTEL_HEX_OFFSET_BYTE_COUNT = 1;
    /** The offset in a line of an INTEL hex file that contains the address for the data in a line */
    private static final int INTEL_HEX_OFFSET_ADDRESS = 3;
    /** The offset in a line of an INTEL hex file that contains the record type for the data in a line */
    private static final int INTEL_HEX_OFFSET_RECORD_TYPE = 7;
    /** The offset in a line of an INTEL hex file that contains the data in a line */
    private static final int INTEL_HEX_OFFSET_DATA_START = 9;
    /** The minimum length of a line of an INTEL hex file */
    private static final int INTEL_HEX_HEADER_LENGTH = 11;

    /** The number of hex characters in the byte count field of a line of an INTEL hex file */
    private static final int INTEL_HEX_BYTE_COUNT_CHARS = 2;
    /** The number of hex characters in the address field of a line of an INTEL hex file */
    private static final int INTEL_HEX_ADDRESS_CHARS = 4;
    /** The number of hex characters in the record type field of a line of an INTEL hex file */
    private static final int INTEL_HEX_RECORD_TYPE_CHARS = 2;

    /** The record type of data in an INTEL hex format firmware file */
    private static final int INTEL_HEX_DATA_RECORD_TYPE = 0x00;
    /** The record type of an extended address in an INTEL hex format firmware file */
    private static final int INTEL_HEX_EXTENDED_ADDRESS_RECORD_TYPE = 0x4;
    /** The record type of end of file in an INTEL hex format firmware file */
    private static final int INTEL_HEX_END_OF_FILE_RECORD_TYPE = 0x1;

    /** The offset in bytes into the metadata row that contains the row number of the last boot row */
    private static final int LAST_BOOT_ROW_OFFSET = 69;

    /**
     * The offset in bytes into the firmware file where the metadata for the row number of the last boot
     * is located for the Firmware1 image
     */
    private static final int LAST_BOOT_META_POS = FLASH_ROW_SIZE*FIRMWARE_1_METADATA_ROW + LAST_BOOT_ROW_OFFSET;

    /** The offset in the last boot row where the version information begins */
    private static final int VERSION_OFFSET = 0xE0 + 6;
}
