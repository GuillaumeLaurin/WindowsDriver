#pragma once

/**
 * @file Common.h
 * @brief Common driver-wide includes, naming constants, logging macros,
 *        error-handling helpers, pool allocation helpers and the driver/
 *        device context definitions.
 */

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

/**
 * @brief Logs and returns the status from the current function if it is a failure code.
 *
 * @param status Expression evaluating to an NTSTATUS to check
 * @note Performs a `return` out of the calling function on failure.
 */
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

/**
 * @brief Logs and jumps to the given cleanup label if the status is a failure code.
 *
 * @param status Expression evaluating to an NTSTATUS to check
 * @param label  Label to jump to (via `goto`) on failure
 */
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

/**
 * @brief Allocates non-paged pool memory with the driver pool tag.
 *
 * @param size Number of bytes to allocate
 * @return Pointer to the allocated memory, or nullptr on failure
 * @note Non-paged memory is safe to access at IRQL >= DISPATCH_LEVEL.
 */
#define ALLOC_NONPAGED(size) \
    ExAllocatePool2(POOL_FLAG_NON_PAGED, (size), POOL_TAG)

/**
 * @brief Allocates paged pool memory with the driver pool tag.
 *
 * @param size Number of bytes to allocate
 * @return Pointer to the allocated memory, or nullptr on failure
 * @note Only safe to access at IRQL < DISPATCH_LEVEL.
 */
#define ALLOC_PAGED(size) \
    ExAllocatePool2(POOL_FLAG_PAGED, (size), POOL_TAG)

/**
 * @brief Frees pool memory allocated with the driver pool tag and nulls the pointer.
 *
 * @param ptr Pointer variable to free; left unchanged if already nullptr, and set to NULL after freeing
 */
#define SAFE_FREE(ptr)                              \
    do {                                            \
        if ((ptr) != NULL) {                        \
            ExFreePoolWithTag((ptr), POOL_TAG);     \
            (ptr) = NULL;                           \
        }                                           \
    } while (0)

/**
 * @brief Computes the number of elements in a fixed-size array.
 *
 * @param arr Array (not a pointer) whose element count is computed
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/**
 * @brief Driver-wide context — tracks initialization state and the list of
 *        devices owned by this driver.
 */
typedef struct _DRIVER_CONTEXT {
    BOOLEAN     Initialized; /**< TRUE once driver-wide state has been set up */
    KSPIN_LOCK  Lock;        /**< Protects concurrent access to DeviceList */
    LIST_ENTRY  DeviceList;  /**< Intrusive list of devices owned by the driver */
} DRIVER_CONTEXT, *PDRIVER_CONTEXT;

/**
 * @brief Per-device context attached to the device object (WDF context or
 *        WDM DeviceExtension).
 */
typedef struct _DEVICE_CONTEXT {
    ULONG           DeviceId;   /**< Identifier of the device instance */
    BOOLEAN         IsOpen;     /**< TRUE while a client handle is open */
    UNICODE_STRING  DeviceName; /**< Name assigned to the device object */
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

#ifdef USE_KMDF
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)
#endif
