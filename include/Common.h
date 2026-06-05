#pragma once

#include <ntddk.h>
#include <wdm.h>

#ifdef USE_KMDF
#include <wdf.h>
#endif

#define DRIVER_NAME_A       "Template"

#define DRIVER_NAME         L"Template"
#define DRIVER_DEVICE_NAME  L"\\Device\\Template"
#define DRIVER_SYMLINK_NAME L"\\DosDevices\\Template"

#ifndef POOL_TAG
#define POOL_TAG 'VRDM'
#endif

#if DBG
    #define LOG_INFO(fmt, ...)  DbgPrint("[" DRIVER_NAME_A "][INFO]  " fmt "\n", ##__VA_ARGS__)
    #define LOG_WARN(fmt, ...)  DbgPrint("[" DRIVER_NAME_A "][WARN]  " fmt "\n", ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...) DbgPrint("[" DRIVER_NAME_A "][ERROR]  " fmt "\n", ##__VA_ARGS__)
    #define LOG_TRACE(fmt, ...) DbgPrint("[" DRIVER_NAME_A "][TRACE]  " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_INFO(fmt, ...)  ((void)0)
    #define LOG_WARN(fmt, ...)  ((void)0)
    #define LOG_ERROR(fmt, ...) ((void)0)
    #define LOG_TRACE(fmt, ...) ((void)0)
#endif

#define NT_CHECK_RETURN(status)                     \
    do {                                            \
        NTSTATUS _s = (status);                     \
        if (!NT_SUCCESS(_s)) {                      \
            LOG_ERROR("NT_CHECK_RETURN failed: "    \
                      "0x%08X at %s:%d", _s,        \
                      __FILE__, __LINE__);          \
            return _s;                              \
        }                                           \
    } while (0)

#define NT_CHECK_GOTO(status, label)                \
    do {                                            \
        NTSTATUS _s = (status);                     \
        if (!NT_SUCCESS(_s)) {                      \
            LOG_ERROR("NT_CHECK_GOTO failed: "      \
                      "0x%08X at %s:%d", _s,        \
                      __FILE__, __LINE__);          \
            goto label;                             \
        }                                           \
    } while (0)

#define ALLOC_NONPAGED(size) \
    ExAllocatePool2(POOL_FLAG_NON_PAGED, (size), POOL_TAG)

#define ALLOC_PAGED(size) \
    ExAllocatePool2(POOL_FLAG_PAGED, (size), POOL_TAG)

#define SAFE_FREE(ptr)                              \
    do {                                            \
        if ((ptr) != NULL) {                        \
            ExFreePoolWithTag((ptr), POOL_TAG);     \
            (ptr) = NULL;                           \
        }                                           \
    } while (0)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef struct _DRIVER_CONTEXT {
    BOOLEAN     Initialized;
    KSPIN_LOCK  Lock;
    LIST_ENTRY  DeviceList;
} DRIVER_CONTEXT, *PDRIVER_CONTEXT;

typedef struct _DEVICE_CONTEXT {
    ULONG           DeviceId;
    BOOLEAN         IsOpen;
    UNICODE_STRING  DeviceName;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

#ifdef USE_KMDF
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)
#endif