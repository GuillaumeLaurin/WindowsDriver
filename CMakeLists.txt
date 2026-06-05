cmake_minimum_required(VERSION 3.20)

project(Template
    VERSION 1.0.0
    DESCRIPTION "TO REMOVE IN A REAL PROJECT"
    LANGUAGES C CXX
)

# ------------------------------------------------------------------------------
# Options
# ------------------------------------------------------------------------------
option(USE_KMDF    "Use Kernel-Mode Driver Framework (KMDF)"  ON)
option(USE_WDM     "Use low-level WDM model"                  OFF)
option(DRIVER_SIGN "Sign the driver after build"              OFF)

if(USE_KMDF AND USE_WDM)
    message(FATAL_ERROR "USE_KMDF and USE_WDM cannot be used at the same time")
endif()

# ------------------------------------------------------------------------------
# WDK detection
# ------------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(WDK REQUIRED)

# ------------------------------------------------------------------------------
# Target
# ------------------------------------------------------------------------------

# Sanatize 
string(REGEX REPLACE "/RTC[^ ]*" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
string(REGEX REPLACE "/RTC[^ ]*" "" CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG}")

add_library(${PROJECT_NAME} SHARED
    src/Driver.cpp
    src/Device.cpp
    src/Queue.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".sys"
    PREFIX ""
    OUTPUT_NAME "${PROJECT_NAME}"
)

# Apply all WDK compile flags, includes, definitions, and libs
if(USE_KMDF)
    wdk_configure_target(${PROJECT_NAME} KMDF)
else()
    wdk_configure_target(${PROJECT_NAME} WDM)
endif()

# Project-specific headers
target_include_directories(${PROJECT_NAME} PRIVATE include/)

# ------------------------------------------------------------------------------
# INF file
# ------------------------------------------------------------------------------
configure_file(
    "${CMAKE_SOURCE_DIR}/inf/${PROJECT_NAME}.inf.in"
    "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.inf"
    @ONLY
)

# ------------------------------------------------------------------------------
# Driver signing (optional)
# ------------------------------------------------------------------------------
if(DRIVER_SIGN)
    find_program(SIGNTOOL signtool
        HINTS "${WDK_ROOT}/bin/${WDK_VERSION}/x64"
    )
    if(SIGNTOOL)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${CMAKE_COMMAND}"
                "-DSIGNTOOL_EXE=${SIGNTOOL}"
                "-DDRIVER_FILE=$<TARGET_FILE:${PROJECT_NAME}>"
                "-DCERT_SUBJECT=${PROJECT_NAME}"
                "-DCERT_STORE=My"
                -P "${CMAKE_SOURCE_DIR}/cmake/SignDriver.cmake"
            COMMENT "Signing driver with SignTool"
            VERBATIM
        )
    else()
        message(WARNING "signtool.exe not found — signing skipped.")
    endif()
endif()

# ------------------------------------------------------------------------------
# Utility: create self-signed test certificate (requires PowerShell)
# ------------------------------------------------------------------------------
add_custom_target(create_test_cert
    COMMAND powershell.exe -NoProfile -Command
        "if (-not (Get-ChildItem Cert:\\CurrentUser\\My | Where-Object { $_.Subject -eq 'CN=${PROJECT_NAME}' })) { New-SelfSignedCertificate -Type CodeSigningCert -Subject 'CN=${PROJECT_NAME}' -CertStoreLocation 'Cert:\\CurrentUser\\My' -FriendlyName '${PROJECT_NAME} Driver Test Certificate' -NotAfter (Get-Date).AddYears(10) | Out-Null; Write-Host 'Certificate created: CN=${PROJECT_NAME}' } else { Write-Host 'Certificate already exists: CN=${PROJECT_NAME}' }"
    COMMENT "Creating self-signed test certificate for driver signing"
)

# ------------------------------------------------------------------------------
# Utility: deploy / remove driver via devcon
# ------------------------------------------------------------------------------
find_program(DEVCON devcon
    HINTS "${WDK_ROOT}/tools/x64"
)
if(DEVCON)
    add_custom_target(install_driver
        COMMAND "${DEVCON}" install
            "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.inf"
            "Root\\${PROJECT_NAME}"
        COMMENT "Installing driver via devcon"
        DEPENDS ${PROJECT_NAME}
    )
    add_custom_target(uninstall_driver
        COMMAND "${DEVCON}" remove "Root\\${PROJECT_NAME}"
        COMMENT "Removing driver via devcon"
    )
endif()

# ------------------------------------------------------------------------------
# Configuration summary
# ------------------------------------------------------------------------------
message(STATUS "")
message(STATUS "=== Configuration ${PROJECT_NAME} ===")
message(STATUS "  Version   : ${PROJECT_VERSION}")
message(STATUS "  Mode      : ${CMAKE_BUILD_TYPE}")
if(USE_KMDF)
    message(STATUS "  Framework : KMDF ${WDK_KMDF_VERSION}")
else()
    message(STATUS "  Framework : WDM")
endif()
message(STATUS "  WDK       : ${WDK_ROOT} (${WDK_VERSION})")
message(STATUS "  Signing   : ${DRIVER_SIGN}")
message(STATUS "=================================")
message(STATUS "")