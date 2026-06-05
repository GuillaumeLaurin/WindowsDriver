# ==============================================================================
#  FindWDK.cmake
#  Detects the Windows Driver Kit (WDK) and exposes all paths and variables
#  needed to build a kernel-mode driver.
#
#  Usage:
#    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
#    find_package(WDK REQUIRED)
#
#  Input variables (all optional — auto-detected if not set):
#    WDK_ROOT        Root of the WDK installation
#    WDK_VERSION     WDK/SDK version string  (e.g. 10.0.26100.0)
#    KMDF_VERSION    KMDF version string     (e.g. 1.35)
#
#  Output variables set after find_package(WDK):
#    WDK_FOUND
#    WDK_ROOT
#    WDK_VERSION
#    WDK_INC_KM          kernel-mode include dir
#    WDK_INC_SHARED      shared include dir
#    WDK_LIB_KM          kernel-mode lib dir (x64)
#    WDK_KMDF_FOUND
#    WDK_KMDF_VERSION
#    WDK_KMDF_VERSION_MAJOR
#    WDK_KMDF_VERSION_MINOR
#    WDK_INC_KMDF        KMDF include dir
#    WDK_LIB_KMDF        KMDF lib dir (x64)
#
#  Helper function exposed after find_package(WDK):
#    wdk_configure_target(<target> [KMDF] [WDM])
#      Applies all compile definitions, flags, includes, and libs to <target>.
# ==============================================================================

# ------------------------------------------------------------------------------
# 1. Locate WDK root
# ------------------------------------------------------------------------------
if(NOT WDK_ROOT)
    # Try the standard registry key written by the WDK installer
    if(CMAKE_HOST_WIN32)
        cmake_host_system_information(
            RESULT _reg_root
            QUERY WINDOWS_REGISTRY
                "HKLM/SOFTWARE/Microsoft/Windows Kits/Installed Roots"
            VALUE "KitsRoot10"
        )
        if(_reg_root)
            set(WDK_ROOT "${_reg_root}")
        endif()
    endif()

    # Fall back to the default install path
    if(NOT WDK_ROOT)
        set(WDK_ROOT "C:/Program Files (x86)/Windows Kits/10")
    endif()
endif()

file(TO_CMAKE_PATH "${WDK_ROOT}" WDK_ROOT)

if(NOT EXISTS "${WDK_ROOT}")
    if(WDK_FIND_REQUIRED)
        message(FATAL_ERROR
            "[FindWDK] WDK not found at: '${WDK_ROOT}'\n"
            "Install it from https://learn.microsoft.com/windows-hardware/drivers/download-the-wdk\n"
            "or pass -DWDK_ROOT=<path> to CMake.")
    else()
        set(WDK_FOUND FALSE)
        return()
    endif()
endif()

# ------------------------------------------------------------------------------
# 2. Detect WDK version — scan Include/* for numeric version folders only
#    (avoids accidentally picking up the "wdf" subfolder)
# ------------------------------------------------------------------------------
if(NOT WDK_VERSION)
    file(GLOB _wdk_dirs "${WDK_ROOT}/Include/*")
    set(_wdk_versions "")
    foreach(_d ${_wdk_dirs})
        get_filename_component(_name "${_d}" NAME)
        if(_name MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$")
            list(APPEND _wdk_versions "${_name}")
        endif()
    endforeach()

    if(NOT _wdk_versions)
        message(FATAL_ERROR "[FindWDK] No WDK version found under '${WDK_ROOT}/Include/'")
    endif()

    list(SORT _wdk_versions COMPARE NATURAL ORDER DESCENDING)
    list(GET  _wdk_versions 0 WDK_VERSION)
    message(STATUS "[FindWDK] WDK_VERSION auto-detected: ${WDK_VERSION}")
endif()

# ------------------------------------------------------------------------------
# 3. Kernel-mode paths
# ------------------------------------------------------------------------------
set(WDK_INC_KM     "${WDK_ROOT}/Include/${WDK_VERSION}/km")
set(WDK_INC_SHARED "${WDK_ROOT}/Include/${WDK_VERSION}/shared")
set(WDK_LIB_KM     "${WDK_ROOT}/Lib/${WDK_VERSION}/km/x64")

foreach(_p WDK_INC_KM WDK_INC_SHARED WDK_LIB_KM)
    if(NOT EXISTS "${${_p}}")
        message(WARNING "[FindWDK] Path not found: ${${_p}}")
    endif()
endforeach()

# ------------------------------------------------------------------------------
# 4. Detect KMDF version
#    Strategy 1: scan Include/wdf/kmdf/*
#    Strategy 2: fallback to Lib/wdf/kmdf/x64/*
# ------------------------------------------------------------------------------
set(WDK_KMDF_FOUND FALSE)

if(NOT KMDF_VERSION)
    file(GLOB _kmdf_dirs "${WDK_ROOT}/Include/wdf/kmdf/*")
    set(_kmdf_versions "")
    foreach(_d ${_kmdf_dirs})
        get_filename_component(_name "${_d}" NAME)
        if(_name MATCHES "^[0-9]+\\.[0-9]+$")
            list(APPEND _kmdf_versions "${_name}")
        endif()
    endforeach()

    if(NOT _kmdf_versions)
        file(GLOB _kmdf_dirs "${WDK_ROOT}/Lib/wdf/kmdf/x64/*")
        foreach(_d ${_kmdf_dirs})
            get_filename_component(_name "${_d}" NAME)
            if(_name MATCHES "^[0-9]+\\.[0-9]+$")
                list(APPEND _kmdf_versions "${_name}")
            endif()
        endforeach()
    endif()

    if(_kmdf_versions)
        list(SORT _kmdf_versions COMPARE NATURAL ORDER DESCENDING)
        list(GET  _kmdf_versions 0 KMDF_VERSION)
        message(STATUS "[FindWDK] KMDF_VERSION auto-detected: ${KMDF_VERSION}")
    endif()
endif()

if(KMDF_VERSION)
    set(_kmdf_inc_check "${WDK_ROOT}/Include/wdf/kmdf/${KMDF_VERSION}")
    set(_kmdf_lib_check "${WDK_ROOT}/Lib/wdf/kmdf/x64/${KMDF_VERSION}")

    if(NOT EXISTS "${_kmdf_inc_check}")
        message(FATAL_ERROR "[FindWDK] KMDF include directory not found: ${_kmdf_inc_check}")
    endif()
    if(NOT EXISTS "${_kmdf_lib_check}")
        message(FATAL_ERROR "[FindWDK] KMDF lib directory not found: ${_kmdf_lib_check}")
    endif()

    string(REPLACE "." ";" _kmdf_parts "${KMDF_VERSION}")
    list(GET _kmdf_parts 0 WDK_KMDF_VERSION_MAJOR)
    list(GET _kmdf_parts 1 WDK_KMDF_VERSION_MINOR)

    set(WDK_KMDF_VERSION   "${KMDF_VERSION}")
    set(WDK_INC_KMDF       "${_kmdf_inc_check}")
    set(WDK_LIB_KMDF       "${_kmdf_lib_check}")
    set(WDK_KMDF_FOUND     TRUE)
endif()

# ------------------------------------------------------------------------------
# 5. Result
# ------------------------------------------------------------------------------
set(WDK_FOUND TRUE)
mark_as_advanced(WDK_ROOT WDK_VERSION KMDF_VERSION)

message(STATUS "[FindWDK] WDK_ROOT    = ${WDK_ROOT}")
message(STATUS "[FindWDK] WDK_VERSION = ${WDK_VERSION}")
if(WDK_KMDF_FOUND)
    message(STATUS "[FindWDK] KMDF        = ${WDK_KMDF_VERSION} (${WDK_INC_KMDF})")
else()
    message(STATUS "[FindWDK] KMDF        = not found (WDM only)")
endif()

# ------------------------------------------------------------------------------
# 6. Helper: wdk_configure_target(<target> KMDF|WDM)
#
#  Applies to <target>:
#    - Include directories (km, shared, kmdf if requested)
#    - Compile definitions
#    - Compiler flags  (/kernel, /GS-, /EHs-c-, etc.)
#    - Linker flags    (/DRIVER, /SUBSYSTEM:NATIVE, etc.)
#    - Kernel libraries
# ------------------------------------------------------------------------------
function(wdk_configure_target TARGET_NAME)
    cmake_parse_arguments(_ARG "KMDF;WDM" "" "" ${ARGN})

    if(_ARG_KMDF AND _ARG_WDM)
        message(FATAL_ERROR "wdk_configure_target: KMDF and WDM are mutually exclusive")
    endif()
    if(_ARG_KMDF AND NOT WDK_KMDF_FOUND)
        message(FATAL_ERROR "wdk_configure_target: KMDF requested but not found in WDK")
    endif()

    # --- Includes ---
    target_include_directories(${TARGET_NAME} PRIVATE
        "${WDK_INC_KM}"
        "${WDK_INC_SHARED}"
    )
    if(_ARG_KMDF)
        target_include_directories(${TARGET_NAME} PRIVATE "${WDK_INC_KMDF}")
    endif()

    # --- Compile definitions ---
    # NOTE: _KERNEL_MODE must NOT be defined manually — /kernel sets it automatically
    target_compile_definitions(${TARGET_NAME} PRIVATE
        KERNEL_MODE
        _AMD64_
        POOL_TAG=0x4B445257
        _NO_CRT_STDIO_INLINE
        NO_STDLIB_INIT
        NTDDI_VERSION=0x0A000007
        _WIN32_WINNT=0x0A00
        WINVER=0x0A00
        WIN32_LEAN_AND_MEAN
        $<$<CONFIG:Debug>:DBG>
        $<$<CONFIG:Debug>:DRIVER_DBG>
    )
    if(_ARG_KMDF)
        target_compile_definitions(${TARGET_NAME} PRIVATE
            KMDF_VERSION_MAJOR=${WDK_KMDF_VERSION_MAJOR}
            KMDF_VERSION_MINOR=${WDK_KMDF_VERSION_MINOR}
        )
    endif()

    # --- Compiler flags ---
    # Remove /EHsc injected by MSBuild before adding our flags (avoids D9025)
    string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/EHsc" "" CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}"   PARENT_SCOPE)

    target_compile_options(${TARGET_NAME} PRIVATE
        /kernel
        /GS-
        /EHs-c-
        /GR-
        /Gz
        /Zp8
        /W4
        /WX
        /wd4100
        /wd4201
        $<$<CONFIG:Release>:/O2 /Oi>
        $<$<CONFIG:Debug>:/Od /Zi>
    )

    # --- Linker flags ---
    set(_entry "DriverEntry")
    if(_ARG_KMDF)
        set(_entry "FxDriverEntry")
    endif()

    target_link_options(${TARGET_NAME} PRIVATE
        /DRIVER
        /SUBSYSTEM:NATIVE
        /ENTRY:${_entry}
        /NODEFAULTLIB
        /MANIFEST:NO
        /ALIGN:4096
        /INCREMENTAL:NO
        $<$<CONFIG:Debug>:/DEBUG>
    )

    # --- Kernel libraries ---
    target_link_libraries(${TARGET_NAME} PRIVATE
        "${WDK_LIB_KM}/ntoskrnl.lib"
        "${WDK_LIB_KM}/hal.lib"
        "${WDK_LIB_KM}/wdm.lib"
        "${WDK_LIB_KM}/wdmsec.lib"
        "${WDK_LIB_KM}/BufferOverflowFastFailK.lib"
    )
    if(_ARG_KMDF)
        target_link_libraries(${TARGET_NAME} PRIVATE
            "${WDK_LIB_KMDF}/WdfDriverEntry.lib"
            "${WDK_LIB_KMDF}/WdfLdr.lib"
        )
    endif()
endfunction()