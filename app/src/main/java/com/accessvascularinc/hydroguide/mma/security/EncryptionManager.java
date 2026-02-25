package com.accessvascularinc.hydroguide.mma.security;

import android.content.Context;
import android.os.Build;
import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyProperties;
import android.util.Base64;
import android.util.Log;

import androidx.annotation.RequiresApi;

import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;
import java.security.KeyStore;
import java.security.SecureRandom;

/**
 * Concrete implementation of IEncryptionManager using Android KeyStore.
 * 
 * Security Features:
 * 1. Keys are generated and stored in AndroidKeyStore - never accessible to app
 * 2. Keys are specific to this package - cannot be used by other apps
 * 3. On app uninstall, keys are automatically deleted by Android system
 * 4. Encryption/Decryption happens at OS level - keys never enter app memory
 * 5. Even on rooted devices, extracted encrypted passwords cannot be decrypted
 * without the app's keys (which are deleted on uninstall)
 * 
 * Supports Android API 23+ (M and above)
 */
public class EncryptionManager implements IEncryptionManager {
    private static final String TAG = "EncryptionManager";
    private static final String KEY_STORE_PROVIDER = "AndroidKeyStore";
    private static final String KEY_ALIAS = "HydroGuidePasswordKey_v2";
    private static final String TRANSFORMATION = "AES/CBC/PKCS7Padding";
    private static final int KEY_SIZE = 256;
    private static final int IV_LENGTH = 16; // bytes

    private final Context context;
    private KeyStore keyStore;

    public EncryptionManager(Context context) {
        this.context = context;
        Log.d(TAG, "EncryptionManager initializing...");
        try {
            initializeKeyStore();
            Log.d(TAG, "EncryptionManager initialized successfully");
        } catch (Exception e) {
            MedDevLog.error(TAG, "Failed to initialize EncryptionManager", e);
            throw e;
        }
    }

    /**
     * Initialize AndroidKeyStore and create encryption key if it doesn't exist
     */
    @RequiresApi(api = Build.VERSION_CODES.M)
    private void initializeKeyStore() {
        try {
            keyStore = KeyStore.getInstance(KEY_STORE_PROVIDER);
            keyStore.load(null);
            // Only create the key if it does NOT already exist.
            // Deleting and recreating the key would make all previously stored
            // encrypted passwords impossible to decrypt, causing "Invalid password"
            // for existing offline users.
            if (!keyStore.containsAlias(KEY_ALIAS)) {
                Log.d(TAG, "Encryption key not found. Creating new key...");
                createKey();
            } else {
                Log.d(TAG, "Existing encryption key found. Reusing existing key.");
            }
        } catch (Exception e) {
            MedDevLog.error(TAG, "Error initializing KeyStore", e);
            throw new RuntimeException("Failed to initialize encryption", e);
        }
    }

    /**
     * Create a new encryption key in AndroidKeyStore
     * Key is:
     * - 256-bit AES key
     * - Specific to this package
     * - Not extractable (stays in KeyStore)
     * - Uses CBC mode with PKCS5 padding
     */
    @RequiresApi(api = Build.VERSION_CODES.M)
    private void createKey() {
        try {
            KeyGenerator keyGenerator = KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, KEY_STORE_PROVIDER);

            KeyGenParameterSpec spec = new KeyGenParameterSpec.Builder(
                    KEY_ALIAS,
                    KeyProperties.PURPOSE_ENCRYPT | KeyProperties.PURPOSE_DECRYPT)
                    .setKeySize(KEY_SIZE)
                    .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_PKCS7)
                    .setBlockModes(KeyProperties.BLOCK_MODE_CBC)
                    .build();

            keyGenerator.init(spec);
            keyGenerator.generateKey();
            Log.d(TAG, "Encryption key created successfully");
        } catch (Exception e) {
            MedDevLog.error(TAG, "Error creating encryption key", e);
            throw new RuntimeException("Failed to create encryption key", e);
        }
    }

    /**
     * Encrypts a plain text password
     * Returns Base64 encoded string of: IV + encrypted password
     * IV (Initialization Vector) is needed for decryption
     */
    @Override
    @RequiresApi(api = Build.VERSION_CODES.M)
    public String encryptPassword(String plainPassword) throws Exception {
        try {
            Log.d(TAG, "Starting password encryption...");

            if (plainPassword == null || plainPassword.isEmpty()) {
                throw new IllegalArgumentException("Password cannot be empty");
            }

            Log.d(TAG, "Getting Cipher instance for transformation: " + TRANSFORMATION);
            // Create a fresh cipher instance for this operation
            Cipher cipher = Cipher.getInstance(TRANSFORMATION);
            Log.d(TAG, "Cipher instance created successfully");

            Log.d(TAG, "Retrieving secret key with alias: " + KEY_ALIAS);
            SecretKey secretKey = (SecretKey) keyStore.getKey(KEY_ALIAS, null);
            if (secretKey == null) {
                MedDevLog.error(TAG, "Secret key is null!");
                throw new Exception("Encryption key not found in KeyStore");
            }
            Log.d(TAG, "Secret key retrieved successfully");

            // Initialize cipher in ENCRYPT_MODE (generates random IV)
            Log.d(TAG, "Initializing cipher in ENCRYPT_MODE...");
            cipher.init(Cipher.ENCRYPT_MODE, secretKey);
            Log.d(TAG, "Cipher initialized successfully");

            // Encrypt the password
            Log.d(TAG, "Encrypting password...");
            byte[] encryptedPassword = cipher.doFinal(plainPassword.getBytes());
            Log.d(TAG, "Password encrypted successfully, encrypted length: " + encryptedPassword.length);

            // Get the IV used for encryption (needed for decryption)
            byte[] iv = cipher.getIV();
            if (iv == null) {
                MedDevLog.error(TAG, "IV is null after encryption!");
                throw new Exception("IV generation failed");
            }
            Log.d(TAG, "IV obtained successfully, IV length: " + iv.length);

            // Combine IV + encrypted password for storage
            byte[] combined = new byte[IV_LENGTH + encryptedPassword.length];
            System.arraycopy(iv, 0, combined, 0, IV_LENGTH);
            System.arraycopy(encryptedPassword, 0, combined, IV_LENGTH, encryptedPassword.length);
            Log.d(TAG, "IV and encrypted password combined, total length: " + combined.length);

            // Return Base64 encoded string
            String encoded = Base64.encodeToString(combined, Base64.DEFAULT);
            Log.d(TAG, "Password encrypted successfully, encoded length: " + encoded.length());
            return encoded;
        } catch (Exception e) {
            MedDevLog.error(TAG, "Error encrypting password: " + e.getMessage(), e);
            throw e;
        }
    }

    /**
     * Decrypts an encrypted password
     * Expects Base64 encoded string of: IV + encrypted password
     */
    @Override
    @RequiresApi(api = Build.VERSION_CODES.M)
    public String decryptPassword(String encryptedPassword) throws Exception {
        try {
            Log.d(TAG, "Starting password decryption...");

            if (encryptedPassword == null || encryptedPassword.isEmpty()) {
                throw new IllegalArgumentException("Encrypted password cannot be empty");
            }

            // Create a fresh cipher instance for this operation
            Cipher cipher = Cipher.getInstance(TRANSFORMATION);
            Log.d(TAG, "Cipher instance created for decryption");

            // Decode from Base64
            byte[] decodedData = Base64.decode(encryptedPassword, Base64.DEFAULT);
            Log.d(TAG, "Base64 decoded, length: " + decodedData.length);

            // Extract IV and encrypted password
            byte[] iv = new byte[IV_LENGTH];
            byte[] encrypted = new byte[decodedData.length - IV_LENGTH];

            System.arraycopy(decodedData, 0, iv, 0, IV_LENGTH);
            System.arraycopy(decodedData, IV_LENGTH, encrypted, 0, encrypted.length);
            Log.d(TAG, "IV extracted (length: " + iv.length + "), encrypted data length: " + encrypted.length);

            // Get the secret key
            SecretKey secretKey = (SecretKey) keyStore.getKey(KEY_ALIAS, null);
            if (secretKey == null) {
                MedDevLog.error(TAG, "Secret key is null during decryption!");
                throw new Exception("Encryption key not found in KeyStore");
            }
            Log.d(TAG, "Secret key retrieved for decryption");

            // Initialize cipher in DECRYPT_MODE with the extracted IV
            cipher.init(Cipher.DECRYPT_MODE, secretKey, new IvParameterSpec(iv));
            Log.d(TAG, "Cipher initialized in DECRYPT_MODE");

            // Decrypt the password
            byte[] decryptedPassword = cipher.doFinal(encrypted);
            Log.d(TAG, "Password decrypted successfully");

            return new String(decryptedPassword);
        } catch (Exception e) {
            MedDevLog.error(TAG, "Error decrypting password: " + e.getMessage(), e);
            throw e;
        }
    }

    /**
     * Verifies if a plain password matches an encrypted password
     * Decrypts the stored password and compares with provided password
     */
    @Override
    @RequiresApi(api = Build.VERSION_CODES.M)
    public boolean verifyPassword(String plainPassword, String encryptedPassword) {
        try {
            Log.d(TAG, "Starting password verification...");

            if (plainPassword == null || plainPassword.isEmpty()) {
                MedDevLog.warn(TAG, "Plain password is null or empty");
                return false;
            }

            if (encryptedPassword == null || encryptedPassword.isEmpty()) {
                MedDevLog.warn(TAG, "Encrypted password is null or empty");
                return false;
            }

            Log.d(TAG, "Decrypting password for verification...");
            String decrypted = decryptPassword(encryptedPassword);
            Log.d(TAG, "Password decrypted, comparing with provided password");

            boolean matches = plainPassword.equals(decrypted);
            Log.d(TAG, "Password verification result: " + (matches ? "MATCH" : "NO MATCH"));

            return matches;
        } catch (Exception e) {
            MedDevLog.error(TAG, "Error verifying password: " + e.getMessage(), e);
            return false;
        }
    }
}
