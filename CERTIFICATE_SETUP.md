# Certificate Setup for GitHub Actions

The Lumos MSIX package is signed with a consistent certificate stored in GitHub Secrets. This ensures users only need to trust the certificate once.

## Setting Up the Certificate

### 1. Generate a Self-Signed Certificate (One Time Only)

Run this PowerShell script **once** to create a certificate:

```powershell
# Create a self-signed certificate
$cert = New-SelfSignedCertificate `
    -Type Custom `
    -Subject "CN=Lumos" `
    -KeyUsage DigitalSignature `
    -FriendlyName "Lumos Certificate" `
    -CertStoreLocation "Cert:\CurrentUser\My" `
    -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}")

# Set a password for the certificate
$password = ConvertTo-SecureString -String "YourSecurePassword123!" -Force -AsPlainText

# Export the certificate to a PFX file
Export-PfxCertificate -Cert $cert -FilePath "LumosCert.pfx" -Password $password

Write-Host "Certificate created: LumosCert.pfx"
Write-Host "Password: YourSecurePassword123!"
```

**Important:** Save the password securely - you'll need it for GitHub Secrets.

### 2. Convert Certificate to Base64

Convert the PFX file to base64 for storage in GitHub Secrets:

```powershell
# Read the PFX file and convert to base64
$pfxBytes = [System.IO.File]::ReadAllBytes("LumosCert.pfx")
$base64 = [System.Convert]::ToBase64String($pfxBytes)

# Save to a text file for easy copying
$base64 | Out-File "LumosCert_base64.txt"

Write-Host "Base64 certificate saved to: LumosCert_base64.txt"
Write-Host "Copy this value to GitHub Secrets"
```

### 3. Add Secrets to GitHub Repository

1. Go to your GitHub repository
2. Navigate to **Settings** → **Secrets and variables** → **Actions**
3. Click **New repository secret**
4. Add two secrets:

   **Secret 1:**
   - Name: `SIGNING_CERTIFICATE`
   - Value: Paste the entire contents of `LumosCert_base64.txt`

   **Secret 2:**
   - Name: `CERTIFICATE_PASSWORD`
   - Value: The password you used (e.g., `YourSecurePassword123!`)

### 4. Export Certificate for Users

Users need to install the certificate to trust your MSIX packages:

```powershell
# Export the public certificate (without private key)
$cert = Get-ChildItem -Path "Cert:\CurrentUser\My" | Where-Object { $_.Subject -eq "CN=Lumos" }
Export-Certificate -Cert $cert -FilePath "LumosCert.cer"

Write-Host "Public certificate exported: LumosCert.cer"
```

**Include `LumosCert.cer` in your releases** so users can install it.

## User Installation Instructions

Users must install the certificate before installing the MSIX package:

### Option 1: Double-Click Installation
1. Double-click `LumosCert.cer`
2. Click **Install Certificate**
3. Select **Local Machine** (requires admin)
4. Choose **Place all certificates in the following store**
5. Click **Browse** → Select **Trusted People**
6. Click **OK** → **Next** → **Finish**

### Option 2: PowerShell Installation (Admin)
```powershell
Import-Certificate -FilePath "LumosCert.cer" -CertStoreLocation "Cert:\LocalMachine\TrustedPeople"
```

After installing the certificate, users can install the MSIX package normally.

## Workflow Integration

The GitHub Actions workflow now:
1. Decodes the certificate from `SIGNING_CERTIFICATE` secret
2. Writes it to `LumosCert.pfx`
3. Signs the MSIX package using the password from `CERTIFICATE_PASSWORD` secret
4. Uploads both the signed MSIX and the certificate for users

## Security Notes

- ⚠️ **Never commit the PFX file to the repository**
- ⚠️ **Keep the password secure**
- ✅ The base64 certificate in GitHub Secrets is encrypted
- ✅ Only the public certificate (`.cer`) should be distributed to users
- ✅ The same certificate is used for all builds, so users trust it once

## Troubleshooting

**If the build fails with "certificate not found":**
- Verify `SIGNING_CERTIFICATE` secret is set correctly
- Check that the base64 string has no extra whitespace

**If signing fails:**
- Verify `CERTIFICATE_PASSWORD` secret matches the password used when creating the certificate
- Ensure the password doesn't contain special characters that need escaping

**If users can't install the MSIX:**
- Ensure they installed `LumosCert.cer` to **Trusted People** store
- Verify they used **Local Machine** (not Current User)
- Check that they have administrator privileges
