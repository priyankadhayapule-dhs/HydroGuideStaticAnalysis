package com.accessvascularinc.hydroguide.mma.network.VerifyOtpModel;

public class VerifyOtpRequest {
    private String email;
    private String verificationcode;

    public VerifyOtpRequest(String email, String verificationcode) {
        this.email = email;
        this.verificationcode = verificationcode;
    }

    public String getEmail() {
        return email;
    }

    public void setEmail(String email) {
        this.email = email;
    }

    public String getVerificationcode() {
        return verificationcode;
    }

    public void setVerificationcode(String verificationcode) {
        this.verificationcode = verificationcode;
    }
}
