package com.echonous.util;

import java.io.Closeable;
import java.io.IOException;

/**
 * FileHelper class provides convenient functions for file operations.
 */
public class FileHelper {

    //==========================================================================
    // Constants

    /** The block size for copying file */
    private static final int COPY_BLOCK_SIZE_BYTES = 1048576;

    /** Tag for logging */
    private static final String TAG = LogAreas.FILE_HELPER;

    /**
     * Close a closable object catching any exceptions that might occur.
     *
     * @param aClosable The Closable object to attempt to close.
     *
     * @return true if the Closeable is closed successfully. false if the
     *         the Closeable is null or the closing is failed
     */
    public static boolean closeQuietly(Closeable aClosable)
    {
        if (aClosable == null)
        {
            // Nothing to close
            return false;
        }

        boolean success = false;

        try
        {
            aClosable.close();
            success = true;
        }
        catch (IOException e)
        {
            // Ignore it....
        }

        return success;
    }
}
