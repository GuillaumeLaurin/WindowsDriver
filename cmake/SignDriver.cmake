# Invoked by the POST_BUILD custom command.
# Required variables (passed via -D): SIGNTOOL_EXE, DRIVER_FILE, CERT_SUBJECT, CERT_STORE
if(NOT EXISTS "${DRIVER_FILE}")
    message(FATAL_ERROR "Driver file not found: ${DRIVER_FILE}")
endif()

# signtool requires backslashes
string(REPLACE "/" "\\" DRIVER_FILE_WIN "${DRIVER_FILE}")
string(REPLACE "/" "\\" SIGNTOOL_WIN "${SIGNTOOL_EXE}")

if(NOT CERT_STORE)
    set(CERT_STORE "My")
endif()

execute_process(
    COMMAND
        "${SIGNTOOL_WIN}"
        sign
        /v
        /fd
        sha256
        /s
        "${CERT_STORE}"
        /n
        "${CERT_SUBJECT}"
        /t
        http://timestamp.digicert.com
        "${DRIVER_FILE_WIN}"
    RESULT_VARIABLE _result
)
if(NOT _result EQUAL 0)
    message(FATAL_ERROR "SignTool failed with exit code: ${_result}")
endif()
