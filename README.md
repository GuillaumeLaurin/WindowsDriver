# Template

A Windows kernel driver template (KMDF/WDM) built with CMake.

## Requirements

| Tool | Minimum version | Link |
|------|----------------|------|
| Windows 11 / 10 (dev machine) | 22H2+ | — |
| Visual Studio | 2022 (17.x) | [Download](https://visualstudio.microsoft.com/) |
| VS workload | Desktop development with C++ | Via VS Installer |
| CMake | 3.20+ | [Download](https://cmake.org/download/) |
| Windows Driver Kit (WDK) | 10.0.22621.0+ | [Download](https://learn.microsoft.com/windows-hardware/drivers/download-the-wdk) |

> The WDK version must match the Windows SDK version installed alongside Visual Studio.

---

## Project structure

```
Template/
├── cmake/
│   ├── FindWdk.cmake          # Automatic WDK detection module
│   └── SignDriver.cmake       # Post-build signing script (cmake -P)
├── include/
│   ├── Common.h               # Kernel includes, logging macros, shared structs
│   ├── Device.h               # Device object interface
│   └── Queue.h                # I/O queue and IOCTL interface
├── src/
│   ├── Driver.cpp               # DriverEntry / DriverUnload
│   ├── Device.cpp               # Device object creation and management
│   └── Queue.cpp                # I/O request queue and IOCTL dispatch
├── inf/
│   └── Template.inf.in  # INF template (expanded by CMake)
├── scripts/
│   └── Export-SigningCert.ps1 # One-shot helper: export cert for CI secrets
├── .github/
│   ├── workflows/
│   │   └── build.yml          # GitHub Actions CI pipeline
│   └── RELEASE_NOTES.md       # Notes for the latest tagged release
├── CMakeLists.txt
├── CMakePresets.json
└── .gitignore
```

---

## Build

### Quick start (CMake Presets)

```bat
REM Debug
cmake --preset debug
cmake --build --preset debug

REM Release
cmake --preset release
cmake --build --preset release

REM WDM Debug
cmake --preset wdm-debug
cmake --build --preset wdm-debug

REM WDM Release
cmake --preset wdm-release
cmake --build --preset wdm-release
```

### Manual (non-standard WDK path)

```bat
cmake -G "Visual Studio 17 2022" -A x64 ^
      -DWDK_ROOT="D:/WDK/10" ^
      -DWDK_VERSION="10.0.22621.0" ^
      -B build/custom
cmake --build build/custom --config Debug
```

### Available CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `USE_KMDF` | `ON` | Use KMDF (recommended) |
| `USE_WDM` | `OFF` | Use low-level WDM |
| `DRIVER_SIGN` | `OFF` | Sign the `.sys` after build |
| `WDK_ROOT` | Auto | WDK root path |
| `WDK_VERSION` | Auto | WDK version string (`10.0.XXXXX.0`) |
| `KMDF_VERSION` | Auto | KMDF version string (`1.XX`) |

---

## Driver signing

Signing is required for the driver to load on a target machine in test mode. A self-signed certificate is sufficient for development and testing.

### 1. Create the self-signed certificate (once per machine)

```bat
cmake --build build/debug --target create_test_cert
```

This creates `CN=Template` in `Cert:\CurrentUser\My` using `New-SelfSignedCertificate` (valid 10 years).

### 2. Build with signing enabled

```bat
cmake --preset release -DDRIVER_SIGN=ON
cmake --build --preset release
```

The `release-signed` preset has `DRIVER_SIGN=ON` pre-configured:

```bat
cmake --preset release-signed
cmake --build --preset release-signed
```

Signing is done via `cmake/SignDriver.cmake` which calls `signtool.exe` using the `CN=Template` certificate from `Cert:\CurrentUser\My`.

---

## Deployment on a test machine

### 1. Enable Test Signing Mode

On the test machine (as administrator):

```bat
bcdedit /set testsigning on
REM Reboot the machine
```

> ⚠️ Never enable this on a production machine.

### 2. Install the certificate

Copy the exported `.cer` file to the target machine, then run as Administrator:

```bat
certutil -addstore Root Template.cer
certutil -addstore TrustedPublisher Template.cer
```

### 3. Install the driver

```bat
cmake --build build/debug --target install_driver
```

Or manually with `devcon`:

```bat
devcon install build\debug\Template.inf Root\Template
```

### 4. Uninstall

```bat
cmake --build build/debug --target uninstall_driver
REM or
devcon remove Root\Template
```

---

## Debugging

### WinDbg (kernel debugging)

On the target machine, enable kernel debugging:

```bat
bcdedit /debug on
bcdedit /dbgsettings net hostip:<DEV_MACHINE_IP> port:50000 key:1.2.3.4
```

Then in WinDbg on the development machine:

```
File > Attach to Kernel > Net
Port: 50000 | Key: 1.2.3.4
```

### Debug messages (DbgPrint)

The `LOG_*` macros in `Common.h` emit messages visible in:
- **DebugView** (Sysinternals) — without a debugger attached
- **WinDbg** — with a kernel debugger attached

To enable kernel messages in DebugView: `Capture > Capture Kernel`

---

## Adding custom IOCTLs

1. Declare the code in `include/Queue.h`:

```c
#define IOCTL_MYDRIVER_MY_COMMAND \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
```

2. Add the `case` in `src/Queue.cpp` inside `EvtIoDeviceControl` (KMDF) or `DispatchDeviceControl` (WDM).

---

## CI/CD

The GitHub Actions pipeline (`.github/workflows/build.yml`) builds all four configurations (KMDF debug/release + WDM debug/release) on every push and pull request.

### What the pipeline does

| Step | Detail |
|------|--------|
| Install WDK | Silent install, cached between runs |
| Import certificate | From `DRIVER_CERT_PFX_B64` secret — skipped on PRs from forks |
| Configure | `DRIVER_SIGN=ON` for release builds when cert is present, `OFF` otherwise |
| Build | All four CMake presets |
| Export `.cer` | Public certificate included in the release package |
| Upload artifacts | One `.zip` per configuration, retained 30 days |
| GitHub Release | Created automatically on `v*` tags, includes all zips + install instructions |

### Setting up signing secrets (one-time)

**1.** Create the certificate locally (if not done):
```bat
cmake --build build/debug --target create_test_cert
```

**2.** Export it for GitHub Secrets:
```powershell
.\scripts\Export-SigningCert.ps1
```

This prints the base64-encoded PFX directly to the console (nothing is written to disk).

**3.** Add the two secrets in **GitHub > Settings > Secrets and variables > Actions**:

| Secret name | Value |
|-------------|-------|
| `DRIVER_CERT_PFX_B64` | Base64 string printed by the script |
| `CERT_PASSWORD` | Password entered during export (can be empty) |

**4.** Trigger a release by pushing a version tag:
```bash
git tag v1.0.0
git push origin v1.0.0
```

> If you rename the project (`project(...)` in `CMakeLists.txt`), also update `PROJECT_NAME` in `.github/workflows/build.yml` to match.

### Troubleshooting the cert export

| Symptom | Cause | Fix |
|---------|-------|-----|
| `bash: .scriptsExport-SigningCert.ps1: command not found` | Running the `.\` PowerShell path syntax from Git Bash — backslashes get swallowed | Invoke through PowerShell explicitly: `powershell -File scripts/Export-SigningCert.ps1` |
| `running scripts is disabled on this system` (`UnauthorizedAccess`) | Local PowerShell execution policy blocks unsigned `.ps1` scripts | Bypass for this run only (no permanent system change): `powershell -ExecutionPolicy Bypass -File scripts/Export-SigningCert.ps1` |
| `Certificate 'CN=Template' not found in Cert:\CurrentUser\My` | `create_test_cert` was never run, or was run for a different `PROJECT_NAME` | Run `cmake --build build/debug --target create_test_cert` first, and make sure the CMake project name matches `-CertSubject` |
| CI still fails signing with `No certificates were found that met all the given criteria` after adding secrets | The PFX's certificate subject doesn't match `CERT_SUBJECT` (`PROJECT_NAME`) used by the workflow, the private key wasn't included in the export, or the cert has expired | Confirm the cert exported has `CN=<PROJECT_NAME>` exactly, was exported with `Export-PfxCertificate` (not `Export-Certificate`, which drops the private key), and hasn't passed its `NotAfter` date |
| Signing works locally but not in CI | `DRIVER_CERT_PFX_B64` / `CERT_PASSWORD` secrets exist on a *different* repo (e.g. copied from another project) — secrets are per-repository, not shared | Re-export and add the secrets directly on this repository, under this exact `PROJECT_NAME` |
| Signing silently attempted even though no secret was ever added | The `Import-PfxCertificate` step failed non-terminating (bad password, corrupt PFX) but didn't stop the job, so the later signing step still ran with a stale/missing cert | Check the "Import signing certificate" step log for errors, not just its green checkmark — a `success` conclusion doesn't guarantee the cert actually imported |

---

## Resources

- [WDK Documentation](https://learn.microsoft.com/windows-hardware/drivers/)
- [Windows driver samples](https://github.com/microsoft/Windows-driver-samples)
- [WDF API reference](https://learn.microsoft.com/windows-hardware/drivers/ddi/_wdf/)
- [Static Driver Verifier](https://learn.microsoft.com/windows-hardware/drivers/devtest/static-driver-verifier)
- [Driver Verifier](https://learn.microsoft.com/windows-hardware/drivers/devtest/driver-verifier)