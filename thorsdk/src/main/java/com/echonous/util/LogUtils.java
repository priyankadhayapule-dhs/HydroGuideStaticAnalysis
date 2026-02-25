package com.echonous.util;

import java.util.logging.Level;
import java.util.logging.Logger;

public class LogUtils {

    // Method for verbose logging
    public static void v(String tag, String message) {
        getLogger(tag).log(Level.FINE, message);
    }

    // Method for debug logging
    public static void d(String tag, String message) {
        getLogger(tag).log(Level.FINE, message);
    }

    // Method for info logging
    public static void i(String tag, String message) {
        getLogger(tag).log(Level.INFO, message);
    }

    // Method for warning logging
    public static void w(String tag, String message) {
        getLogger(tag).log(Level.WARNING, message);
    }

    // Method for error logging
    public static void e(String tag, String message) {
        getLogger(tag).log(Level.SEVERE, message);
    }

    // Method for error logging with throwable
    public static void e(String tag, String message, Throwable throwable) {
        getLogger(tag).log(Level.SEVERE, message, throwable);
    }

    private static Logger getLogger(String tag) {
        return Logger.getLogger(tag);
    }
}
