<#
.SYNOPSIS
    Exports the driver self-signed certificate as a base64-encoded PFX
    ready to be stored as a GitHub Actions secret.

.DESCRIPTION
    Run this once locally after creating the certificate with:
        cmake --build build/debug --target create_test_cert

    The script exports the private key (PFX) and prints the base64 string
    to add to GitHub Secrets as DRIVER_CERT_PFX_B64.

.PARAMETER CertSubject
    Subject of the certificate to export (default: CN=Template).

.EXAMPLE
    .\scripts\Export-SigningCert.ps1
    .\scripts\Export-SigningCert.ps1 -CertSubject "CN=MyDriver"
#>
param(
    [string]$CertSubject = "CN=Template"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ---- Find the certificate -----------------------------------------------
$cert = Get-ChildItem Cert:\CurrentUser\My |
    Where-Object { $_.Subject -eq $CertSubject } |
    Select-Object -First 1

if (-not $cert) {
    Write-Error @"
Certificate '$CertSubject' not found in Cert:\CurrentUser\My.
Create it first:
    cmake --build build\debug --target create_test_cert
"@
    exit 1
}

Write-Host "Found: $($cert.Subject)  [Thumbprint: $($cert.Thumbprint)]"

# ---- Ask for a PFX password ----------------------------------------------
Write-Host ""
Write-Host "Enter a password for the PFX export (press Enter to leave empty):" -ForegroundColor Cyan
$securePass = Read-Host -AsSecureString

# ---- Export to a temp file -----------------------------------------------
$tmpPfx = Join-Path $env:TEMP "driver-export-$([System.IO.Path]::GetRandomFileName()).pfx"

try {
    Export-PfxCertificate -Cert $cert -FilePath $tmpPfx -Password $securePass | Out-Null

    # ---- Encode as base64 ------------------------------------------------
    $b64 = [Convert]::ToBase64String([IO.File]::ReadAllBytes($tmpPfx))

    # ---- Extract the plain-text password for display ---------------------
    $bstr     = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($securePass)
    $password = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($bstr)
    [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr)
}
finally {
    Remove-Item $tmpPfx -Force -ErrorAction SilentlyContinue
}

# ---- Print instructions --------------------------------------------------
Write-Host ""
Write-Host "================================================================" -ForegroundColor Green
Write-Host " Add these two secrets to your GitHub repository:" -ForegroundColor Green
Write-Host " Settings > Secrets and variables > Actions > New repository secret" -ForegroundColor Green
Write-Host "================================================================" -ForegroundColor Green
Write-Host ""
Write-Host "Secret name : DRIVER_CERT_PFX_B64" -ForegroundColor Yellow
Write-Host "Secret value:" -ForegroundColor Yellow
Write-Host $b64
Write-Host ""
Write-Host "Secret name : CERT_PASSWORD" -ForegroundColor Yellow
Write-Host "Secret value: $password" -ForegroundColor Yellow
Write-Host ""
Write-Host "IMPORTANT: do NOT save the PFX or the base64 string to any file" -ForegroundColor Red
Write-Host "           that might be committed to the repository." -ForegroundColor Red
