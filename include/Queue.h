#pragma once

#include "Common.h"

#define IOCTL_TEMPLATE_GET_VERSION \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TEMPLATE_DO_OPERATION \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_DATA)

#ifdef USE_KMDF

/**
 * @brief Initializes the KMDF I/O request queue
 * 
 * @param Device WDF device handle
 * @return NTSTATUS
 */
NTSTATUS
QueueInitialize(
    _In_ WDFDEVICE Device
);

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL  EvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_READ            EvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE           EvtIoWrite;

#else

DRIVER_DISPATCH DispatchDeviceControl;
DRIVER_DISPATCH DispatchRead;
DRIVER_DISPATCH DispatchWrite;

#endif


