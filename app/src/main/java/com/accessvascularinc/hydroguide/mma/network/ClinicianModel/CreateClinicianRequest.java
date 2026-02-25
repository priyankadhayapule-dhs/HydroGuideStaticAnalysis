package com.accessvascularinc.hydroguide.mma.network.ClinicianModel;

public class CreateClinicianRequest {
    private String Email;
    private String Name;
    private String Password;
    private String RoleName;
    private String OrganizationId;
    private String UserName;

    public CreateClinicianRequest(String email, String name, String password, String roleName, String organizationId,
            String userName) {
        this.Email = email;
        this.Name = name;
        this.Password = password;
        this.RoleName = roleName;
        this.OrganizationId = organizationId;
        this.UserName = userName;
    }

    public String getEmail() {
        return Email;
    }

    public String getName() {
        return Name;
    }

    public String getPassword() {
        return Password;
    }

    public String getRoleName() {
        return RoleName;
    }

    public String getOrganizationId() {
        return OrganizationId;
    }

    public String getUserName() {
        return UserName;
    }

    public void setEmail(String email) {
        this.Email = email;
    }

    public void setName(String name) {
        this.Name = name;
    }

    public void setPassword(String password) {
        this.Password = password;
    }

    public void setRoleName(String roleName) {
        this.RoleName = roleName;
    }

    public void setOrganizationId(String organizationId) {
        this.OrganizationId = organizationId;
    }

    public void setUserName(String userName) {
        this.UserName = userName;
    }
}
