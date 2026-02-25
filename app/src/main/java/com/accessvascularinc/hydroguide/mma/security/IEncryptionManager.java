package com.accessvascularinc.hydroguide.mma.security;

/**
 * Interface for password encryption and decryption operations.
 * Provides a contract for implementing secure password management strategies.
 * 
 * Current implementation uses AndroidKeyStore for secure key management,
 * but can be replaced with alternative encryption providers as needed.
 */
public interface IEncryptionManager {

    /**
     * Encrypts a plain text password using AndroidKeyStore.
     * The encryption is performed at OS level, keys never enter app memory.
     * 
     * @param plainPassword the plain text password to encrypt
     * @return the encrypted password as a Base64 encoded string
     * @throws Exception if encryption fails
     */
    String encryptPassword(String plainPassword) throws Exception;

    /**
     * Decrypts an encrypted password using AndroidKeyStore.
     * The decryption is performed at OS level using the stored key.
     * 
     * @param encryptedPassword the encrypted password (Base64 encoded)
     * @return the decrypted plain text password
     * @throws Exception if decryption fails or key is not available
     */
    String decryptPassword(String encryptedPassword) throws Exception;

    /**
     * Verifies that a plain password matches an encrypted password.
     * This is the primary method used during login validation.
     * 
     * @param plainPassword     the plain text password provided by user
     * @param encryptedPassword the encrypted password stored in database
     * @return true if passwords match, false otherwise
     */
    boolean verifyPassword(String plainPassword, String encryptedPassword);
}
