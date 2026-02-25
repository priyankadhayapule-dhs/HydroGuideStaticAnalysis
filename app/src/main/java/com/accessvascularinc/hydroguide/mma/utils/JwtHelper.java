package com.accessvascularinc.hydroguide.mma.utils;

import android.util.Base64;
import org.json.JSONObject;

/**
 * Helper class for JWT token validation
 */
public class JwtHelper {
    
    /**
     * Check if JWT token is expired
     * @param token JWT token string
     * @return true if token is expired or invalid, false if still valid
     */
    public static boolean isTokenExpired(String token) {
        if (token == null || token.isEmpty()) {
            return true;
        }
        
        try {
            // JWT format: header.payload.signature
            String[] parts = token.split("\\.");
            if (parts.length < 2) {
                return true;
            }
            
            // Decode payload (second part)
            String payload = parts[1];
            byte[] decodedBytes = Base64.decode(payload, Base64.URL_SAFE | Base64.NO_WRAP);
            String decodedPayload = new String(decodedBytes);
            
            JSONObject jsonPayload = new JSONObject(decodedPayload);
            
            // Check if 'exp' claim exists
            if (jsonPayload.has("exp")) {
                long expiration = jsonPayload.getLong("exp");
                long currentTime = System.currentTimeMillis() / 1000; // Current time in seconds
                
                // Token is expired if current time is past expiration
                return currentTime >= expiration;
            }
            
            // If no 'exp' claim, consider token invalid
            return true;
        } catch (Exception e) {
            MedDevLog.error("JwtHelper", "Error parsing JWT token", e);
            return true;
        }
    }
    
    /**
     * Check if token expiry timestamp is still valid
     * @param tokenExpiry Expiry timestamp in milliseconds
     * @return true if expired, false if still valid
     */
    public static boolean isTokenExpiryPassed(Long tokenExpiry) {
        if (tokenExpiry == null) {
            return true;
        }
        
        long currentTime = System.currentTimeMillis();
        return currentTime >= tokenExpiry;
    }
    
    /**
     * Extract expiration time from JWT token
     * @param token JWT token string
     * @return Expiration timestamp in milliseconds, or null if cannot extract
     */
    public static Long extractTokenExpiry(String token) {
        if (token == null || token.isEmpty()) {
            return null;
        }
        
        try {
            String[] parts = token.split("\\.");
            if (parts.length < 2) {
                return null;
            }
            
            String payload = parts[1];
            byte[] decodedBytes = Base64.decode(payload, Base64.URL_SAFE | Base64.NO_WRAP);
            String decodedPayload = new String(decodedBytes);
            
            JSONObject jsonPayload = new JSONObject(decodedPayload);
            
            if (jsonPayload.has("exp")) {
                long expiration = jsonPayload.getLong("exp");
                // Convert from seconds to milliseconds
                return expiration * 1000;
            }
            
            return null;
        } catch (Exception e) {
            MedDevLog.error("JwtHelper", "Error extracting token expiry", e);
            return null;
        }
    }
}
