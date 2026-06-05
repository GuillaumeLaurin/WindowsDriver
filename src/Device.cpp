#include "Device.h"
#include "Queue.h"

#ifdef USE_KMDF

/**
 * @brief Creates and configures the WDF device objects
 * 
 * @param DeviceInit Initialization structure provided by the framework
 * @return NTSTATUS
 */
NTSTATUS
DeviceCreate(
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
{
    NTSTATUS                status;
    WDFDEVICE               device;
    WDF_OBJECT_ATTRIBUTES   deviceAttributes;
    PDEVICE_CONTEXT         deviceContext;
    UNICODE_STRING          deviceName;
    UNICODE_STRING          symLink;

    LOG_TRACE("DeviceCreate (KMDF)");

    RtlInitUnicodeString(&deviceName, DRIVER_DEVICE_NAME);
    status = WdfDeviceInitAssignName(DeviceInit, &deviceName);
    NT_CHECK_RETURN(status);

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);
    WdfDeviceInitSetExclusive(DeviceInit, FALSE);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);
    deviceAttributes.EvtCleanupCallback = EvtDeviceContextCleanup;

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    NT_CHECK_RETURN(status);

    // Initialize device content
    deviceContext           = DeviceGetContext(device);
    deviceContext->IsOpen   = FALSE;
    deviceContext->DeviceId = 0;

    // Create user-mode accessible symbolic link
    RtlInitUnicodeString(&symLink, DRIVER_SYMLINK_NAME);
    status = WdfDeviceCreateSymbolicLink(device, &symLink);
    NT_CHECK_RETURN(status);

    // Initialize the I/O queue
    status = QueueInitialize(device);
    NT_CHECK_RETURN(status);

    LOG_INFO("DeviceCreate, device created successfully");
    return STATUS_SUCCESS;
}

/**
 * @brief KMDF cleanup callback — invoked before the device context is freed.
 */
VOID
EvtDeviceContextCleanup(
    _In_ WDFOBJECT DeviceObject
)
{
    PDEVICE_CONTEXT ctx = DeviceGetContext(DeviceObject);
    UNREFERENCED_PARAMETER(ctx);

    LOG_TRACE("EvtDeviceContextCleanup");

    // Release any resources allocated in the device context here !
}

#else // !USE_KMDF

/**
 * @brief Creates the device object and symbolic link (WDM).
 * 
 * @param DriverObject Pointer to the driver object
 * @param RegistryPath Driver registry path
 * @return NTSTATUS
 */
NTSTATUS
DeviceCreate(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS        status;
    PDEVICE_OBJECT  deviceObject    = NULL;
    UNICODE_STRING  deviceName;
    UNICODE_STRING  symLink;
    BOOLEAN         symLinkCreated  = FALSE;

    UNREFERENCED_PARAMETER(RegistryPath);

    LOG_TRACE("DeviceCreate (WDM)");

    RtlInitUnicodeString(&deviceName, DRIVER_DEVICE_NAME);

    status = IoCreateDevice(
        DriverObject,
        sizeof(DEVICE_CONTEXT),
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &deviceObject
    );
    NT_CHECK_GOTO(status, Cleanup);

    PDEVICE_CONTEXT ctx = (PDEVICE_CONTEXT)deviceObject->DeviceExtension;
    ctx->IsOpen     = FALSE;
    ctx->DeviceId   = 0;

    deviceObject->Flags |= DO_BUFFERED_IO;
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    RtlInitUnicodeString(&symLink, DRIVER_SYMLINK_NAME);
    status = IoCreateSymbolicLink(&symLink, &deviceName);
    NT_CHECK_GOTO(status, Cleanup);

    symLinkCreated = TRUE;
    
    LOG_INFO("DeviceCreated, success (%wZ)", &deviceName);
    return STATUS_SUCCESS;

Cleanup:
    if (symLinkCreated) {
        IoDeleteSymbolicLink(&symLink);
    }
    if (deviceObject != NULL) {
        IoDeleteDevice(deviceObject);
    }
    return status;
}

/**
 * @brief Deletes the device object and symbolic link (WDM).
 */
VOID
DeviceDelete(
    _In_ PDEVICE_OBJECT DeviceObject
)
{
    UNICODE_STRING symLink;

    LOG_TRACE("DeviceDelete");

    RtlInitUnicodeString(&symLink, DRIVER_SYMLINK_NAME);
    IoDeleteSymbolicLink(&symLink);

    if (DeviceObject != NULL) {
        IoDeleteDevice(DeviceObject);
    }
}

NTSTATUS
DispatchCreate(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP        Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    LOG_TRACE("DispatchCreate");
    Irp->IoStatus.Status        = STATUS_SUCCESS;
    Irp->IoStatus.Information   = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
DispatchClose(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP        Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    LOG_TRACE("DispatchClose");
    Irp->IoStatus.Status        = STATUS_SUCCESS;
    Irp->IoStatus.Information   = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
DispatchCleanup(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP        Irp 
) 
{
    UNREFERENCED_PARAMETER(DeviceObject);
    LOG_TRACE("DispatchCleanup");
    Irp->IoStatus.Status        = STATUS_SUCCESS;
    Irp->IoStatus.Information   = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

#endif // USE_KMDF