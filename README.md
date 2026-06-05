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
│   └── FindWDK.cmake          # Automatic WDK detection module
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
├── .github/
│   └── workflows/
│       └── build.yml          # GitHub Actions CI pipeline
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

## Deployment on a test machine

### 1. Enable Test Signing Mode

On the test machine (as administrator):

```bat
bcdedit /set testsigning on
REM Reboot the machine
```

> ⚠️ Never enable this on a production machine.

### 2. Create a self-signed test certificate

```bat
cmake --build build/debug --target create_test_cert
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

The GitHub Actions pipeline (`.github/workflows/build.yml`):
- Automatically installs the WDK (with caching)
- Builds Debug and Release configurations
- Publishes `.sys`, `.pdb`, and `.inf` as build artifacts

---

## Resources

- [WDK Documentation](https://learn.microsoft.com/windows-hardware/drivers/)
- [Windows driver samples](https://github.com/microsoft/Windows-driver-samples)
- [WDF API reference](https://learn.microsoft.com/windows-hardware/drivers/ddi/_wdf/)
- [Static Driver Verifier](https://learn.microsoft.com/windows-hardware/drivers/devtest/static-driver-verifier)
- [Driver Verifier](https://learn.microsoft.com/windows-hardware/drivers/devtest/driver-verifier)