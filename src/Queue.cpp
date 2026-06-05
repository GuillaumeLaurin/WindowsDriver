/**
 * @file Queue.cpp
 * @brief I/O queue setup and IOCTL/read/write dispatch implementation
 *        (KMDF queue callbacks and WDM dispatch routines).
 */

#include "Queue.h"

#ifdef USE_KMDF

/**
 * @brief Initializes the KMDF I/O queue (sequential by default).
 */
NTSTATUS
QueueInitialize(
    _In_ WDFDEVICE Device
)
{
    NTSTATUS            status;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDFQUEUE            queue;

    LOG_TRACE("QueueInitialize");

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &queueConfig,
        WdfIoQueueDispatchSequential
    );

    queueConfig.EvtIoDeviceControl  = EvtIoDeviceControl;
    queueConfig.EvtIoRead           = EvtIoRead;
    queueConfig.EvtIoWrite          = EvtIoWrite;

    status = WdfIoQueueCreate(Device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);

    NT_CHECK_RETURN(status);

    return status;
}

/**
 * @brief IOCTL callback — dispatches device control requests.
 */
VOID 
EvtIoDeviceControl(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     OutputBufferLength,
    _In_ size_t     InputBufferLength,
    _In_ ULONG      IoControlCode
)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t   bytesOut = 0;

    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    LOG_TRACE("EvtIoDeviceControl - IOCTL=0x%08X", IoControlCode);

    switch (IoControlCode)
    {
        case IOCTL_TEMPLATE_GET_VERSION:
        {
            PULONG pVersion;
            status = WdfRequestRetrieveOutputBuffer(
                Request, sizeof(ULONG), (PVOID*)&pVersion, NULL
            );

            if (NT_SUCCESS(status))
            {
                *pVersion = (KMDF_VERSION_MAJOR << 16) | KMDF_VERSION_MINOR;
                bytesOut  = sizeof(ULONG);
            }

            break;
        }
        case IOCTL_TEMPLATE_DO_OPERATION:
        {
            /// @todo : implement the operation
            LOG_INFO("IOCTL_TEMPLATE_DO_OPERATION received");
            status   = STATUS_SUCCESS;
            bytesOut = 0;
            break;
        }
        default:
        {
            LOG_WARN("Unknown IOCTL: 0x%08X", IoControlCode);
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    WdfRequestCompleteWithInformation(Request, status, bytesOut);
}

/**
 * @brief Read callback.
 */
VOID
EvtIoRead(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     Length
)
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(Length);

    LOG_TRACE("EvtIoRead - %Iu bytes request", Length);
    /// @todo : fill the read buffer
    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, 0);
}

/**
 * @brief Write callback.
 */
VOID
EvtIoWrite(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     Length
)
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(Length);

    LOG_TRACE("EvtIoWrite - %Iu bytes request", Length);
    /// @todo : process the written data
    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, 0);
}

#else // !USE_KMDF

/**
 * @brief IRP_MJ_DEVICE_CONTROL handler (WDM).
 */
NTSTATUS
DispatchDeviceControl(
    _In_    PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP           Irp
)
{
    PIO_STACK_LOCATION stack     = IoGetCurrentIrpStackLocation(Irp);
    ULONG              ioctlCode = stack->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS           status    = STATUS_SUCCESS;
    ULONG_PTR          bytesOut  = 0;

    UNREFERENCED_PARAMETER(DeviceObject);

    LOG_TRACE("DispatchDeviceControl - IOCTL=0x%08X", ioctlCode);

    switch (ioctlCode)
    {
        case IOCTL_TEMPLATE_GET_VERSION:
        {
            ULONG outLen = stack->Parameters.DeviceIoControl.OutputBufferLength;
            if (outLen < sizeof(ULONG))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PULONG pVer = (PULONG)Irp->AssociatedIrp.SystemBuffer;
            *pVer       = 0x00010000; // placeholder
            bytesOut    = sizeof(PULONG);
            break;
        }
        case IOCTL_TEMPLATE_DO_OPERATION:
        {   
            LOG_INFO("IOCTL_MYDRIVER_DO_OPERATION received (WDM)");
            status   = STATUS_SUCCESS;
            bytesOut = 0;
            break;
        }
        default:
        {
            LOG_WARN("Unknown IOCTL: 0x%08X", ioctlCode);
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = bytesOut;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

/**
 * @brief IRP_MJ_READ handler (WDM).
 */
NTSTATUS
DispatchRead(
    _In_    PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP           Irp
)
{   
    UNREFERENCED_PARAMETER(DeviceObject);

    LOG_TRACE("DispatchRead");
    /// @todo : copy data into the system buffer
    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

/**
 * @brief IRP_MJ_WRITE handler (WDM).
 */
NTSTATUS
DispatchWrite(
    _In_    PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP           Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    LOG_TRACE("DispatchWrite");
    /// @todo : read data from the system buffer
    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

#endif // USE_KMDF