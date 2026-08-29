## @file    nmake_env_helpers.cmake
## @brief   Helpers to run commands through VsDevCmd for NMake workflows.

function(create_vsdevcmd_wrapper WRAPPER_PATH COMMAND_LINE)
  file(WRITE "${WRAPPER_PATH}" "@echo off\n")
  file(APPEND "${WRAPPER_PATH}" "call \"${VS_DEVCMD}\" -arch=x64 >nul\n")
  file(APPEND "${WRAPPER_PATH}" "if errorlevel 1 exit /b %errorlevel%\n")
  file(APPEND "${WRAPPER_PATH}" "set VCPkgLocalAppDataDisabled=1\n")
  file(APPEND "${WRAPPER_PATH}" "${COMMAND_LINE}\n")
  file(APPEND "${WRAPPER_PATH}" "exit /b %errorlevel%\n")
endfunction()

function(create_vsdevcmd_wrapper_from_list WRAPPER_PATH COMMAND_LIST_VAR)
  set(_first TRUE)
  set(_cmd "")
  foreach(_token IN LISTS ${COMMAND_LIST_VAR})
    if(_token STREQUAL "")
      continue()
    endif()
    if(_first)
      set(_cmd "\"${_token}\"")
      set(_first FALSE)
    elseif(_token MATCHES " ")
      string(APPEND _cmd " \"${_token}\"")
    else()
      string(APPEND _cmd " ${_token}")
    endif()
  endforeach()
  create_vsdevcmd_wrapper("${WRAPPER_PATH}" "${_cmd}")
endfunction()

function(run_with_optional_vsdevcmd_wrapper
         USE_WRAPPER
         WRAPPER_PATH
         COMMAND_VAR_NAME
         WORKING_DIR
         RESULT_VAR
         OUTPUT_VAR
         ERROR_VAR)
  if(USE_WRAPPER)
    set(_command cmd /c "${WRAPPER_PATH}")
  else()
    set(_command ${${COMMAND_VAR_NAME}})
  endif()

  execute_process(
    COMMAND ${_command}
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _output
    ERROR_VARIABLE _error
    WORKING_DIRECTORY "${WORKING_DIR}"
  )

  set(${RESULT_VAR} "${_result}" PARENT_SCOPE)
  set(${OUTPUT_VAR} "${_output}" PARENT_SCOPE)
  set(${ERROR_VAR} "${_error}" PARENT_SCOPE)
endfunction()
