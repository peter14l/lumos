# Lumos Certificate Generator
# This script creates a self-signed certificate for MSIX signing and prepares it for GitHub Secrets

Write-Host "=== Lumos Certificate Generator ===" -ForegroundColor Cyan
Write-Host ""

# Configuration
$certSubject = "CN=Lumos"
$certFriendlyName = "Lumos Certificate"
$pfxFileName = "LumosCert.pfx"
$cerFileName = "LumosCert.cer"
$base64FileName = "LumosCert_base64.txt"

# Prompt for password
Write-Host "Enter a password for the certificate (used for signing):" -ForegroundColor Yellow
$passwordSecure = Read-Host -AsSecureString
$passwordPlain = [Runtime.InteropServices.Marshal]::PtrToStringAuto([Runtime.InteropServices.Marshal]::SecureStringToBSTR($passwordSecure))

if ([string]::IsNullOrWhiteSpace($passwordPlain)) {
    Write-Host "Error: Password cannot be empty" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Creating self-signed certificate..." -ForegroundColor Green

# Create the certificate
try {
    $cert = New-SelfSignedCertificate `
        -Type Custom `
        -Subject $certSubject `
        -KeyUsage DigitalSignature `
        -FriendlyName $certFriendlyName `
        -CertStoreLocation "Cert:\CurrentUser\My" `
        -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}")
    
    Write-Host "✓ Certificate created successfully" -ForegroundColor Green
    Write-Host "  Thumbprint: $($cert.Thumbprint)" -ForegroundColor Gray
} catch {
    Write-Host "Error creating certificate: $_" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Exporting certificate to PFX (with private key)..." -ForegroundColor Green

# Export to PFX
try {
    $password = ConvertTo-SecureString -String $passwordPlain -Force -AsPlainText
    Export-PfxCertificate -Cert $cert -FilePath $pfxFileName -Password $password | Out-Null
    Write-Host "✓ Exported to: $pfxFileName" -ForegroundColor Green
} catch {
    Write-Host "Error exporting PFX: $_" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Exporting public certificate (for users)..." -ForegroundColor Green

# Export public certificate
try {
    Export-Certificate -Cert $cert -FilePath $cerFileName | Out-Null
    Write-Host "✓ Exported to: $cerFileName" -ForegroundColor Green
} catch {
    Write-Host "Error exporting CER: $_" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Converting to Base64 for GitHub Secrets..." -ForegroundColor Green

# Convert to Base64
try {
    $pfxBytes = [System.IO.File]::ReadAllBytes($pfxFileName)
    $base64 = [System.Convert]::ToBase64String($pfxBytes)
    $base64 | Out-File $base64FileName -Encoding ASCII
    Write-Host "✓ Base64 saved to: $base64FileName" -ForegroundColor Green
} catch {
    Write-Host "Error converting to Base64: $_" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Copying Base64 to clipboard..." -ForegroundColor Green

# Copy to clipboard
try {
    $base64 | Set-Clipboard
    Write-Host "✓ Base64 copied to clipboard!" -ForegroundColor Green
} catch {
    Write-Host "Warning: Could not copy to clipboard. Please copy manually from $base64FileName" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== Certificate Generation Complete ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Files created:" -ForegroundColor White
Write-Host "  1. $pfxFileName - Certificate with private key (DO NOT COMMIT)" -ForegroundColor Gray
Write-Host "  2. $cerFileName - Public certificate (distribute to users)" -ForegroundColor Gray
Write-Host "  3. $base64FileName - Base64 for GitHub Secrets (DO NOT COMMIT)" -ForegroundColor Gray
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Go to GitHub: Settings → Secrets and variables → Actions" -ForegroundColor White
Write-Host "  2. Create secret 'SIGNING_CERTIFICATE' and paste from clipboard (or $base64FileName)" -ForegroundColor White
Write-Host "  3. Create secret 'CERTIFICATE_PASSWORD' with value: $passwordPlain" -ForegroundColor White
Write-Host "  4. Add $cerFileName to your releases for users to install" -ForegroundColor White
Write-Host ""
Write-Host "Security reminders:" -ForegroundColor Red
Write-Host "  • DO NOT commit $pfxFileName or $base64FileName to git" -ForegroundColor White
Write-Host "  • Store the password securely" -ForegroundColor White
Write-Host "  • Only distribute $cerFileName (public certificate)" -ForegroundColor White
Write-Host ""

# Create .gitignore entry if it doesn't exist
$gitignorePath = ".gitignore"
if (Test-Path $gitignorePath) {
    $gitignoreContent = Get-Content $gitignorePath -Raw
    if ($gitignoreContent -notmatch "LumosCert") {
        Write-Host "Adding certificate files to .gitignore..." -ForegroundColor Green
        Add-Content $gitignorePath "`n# Certificate files (DO NOT COMMIT)`nLumosCert.pfx`nLumosCert_base64.txt"
        Write-Host "✓ Updated .gitignore" -ForegroundColor Green
    }
}

Write-Host "Press any key to exit..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
