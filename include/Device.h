#pragma once

/**
 * @file Device.h
 * @brief Device object creation/deletion and dispatch routine declarations
 *        (KMDF and WDM variants).
 */

#include "Common.h"

#ifdef USE_KMDF

EVT_WDF_DRIVER_DEVICE_ADD EvtDriverDeviceAdd;

EVT_WDF_DEVICE_CONTEXT_CLEANUP EvtDeviceContextCleanup;

/**
 * @brief Creates and configures the WDF device objects
 * 
 * @param DeviceInit Initialization structure provided by the framework
 * @return NTSTATUS
 */
NTSTATUS
DeviceCreate(
    _Inout_ PWDFDEVICE_INIT DeviceInit
);

#else

/**
 * @brief Creates the device object and symbolic link (WDM).
 * 
 * @param DriverObject Pointer to the driver object
 * @param RegistryPath Driver registry path
 * @return NTSTATUS
 */
NTSTATUS
DeviceCreate(
    _In_ PDRIVER_OBJECT     DriverObject,
    _In_ PUNICODE_STRING    RegistryPath
);

/**
 * @brief Deletes the device object and symbolic link (WDM).
 * 
 * @param DeviceObject Pointer to the DEVICE_OBJECT to remove
 */
VOID
DeviceDelete(
    _In_ PDEVICE_OBJECT DeviceObject
);

DRIVER_DISPATCH DispatchCreate;
DRIVER_DISPATCH DispatchClose;
DRIVER_DISPATCH DispatchCleanup;

#endif
