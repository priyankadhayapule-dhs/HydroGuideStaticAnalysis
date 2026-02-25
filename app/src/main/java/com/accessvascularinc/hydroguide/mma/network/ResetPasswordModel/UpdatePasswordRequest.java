package com.accessvascularinc.hydroguide.mma.network.ResetPasswordModel;

public class UpdatePasswordRequest {
    private String Email;
    private String NewPassword;
    private String currentPassword;
    private String oldUserName;
    private String newUserName;


    public UpdatePasswordRequest(String email, String newPassword, String currentPassword, String oldUserName, String newUserName) {
        this.Email = email;
        this.NewPassword = newPassword;
        this.currentPassword = currentPassword;
        this.oldUserName = oldUserName;
        this.newUserName = newUserName;
    }

    public String getEmail() {
        return Email;
    }

    public String getNewPassword() {
        return NewPassword;
    }

    public String getCurrentPassword() {
        return currentPassword;
    }
}
