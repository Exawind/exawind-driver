if(NOT "${HOST_NAME}" STREQUAL "")
  message("Hostname is ${HOST_NAME}")
else()
  message(FATAL_ERROR "You need to set the HOST_NAME variable. CMake will exit." )
endif()

if(NOT "${CTEST_SOURCE_DIR}" STREQUAL "")
  message("CTEST_SOURCE_DIR is ${CTEST_SOURCE_DIR}")
else()
  message(FATAL_ERROR "You need to set the CTEST_SOURCE_DIR variable. CMake will exit." )
endif()

if(NOT "${CTEST_BUILD_DIR}" STREQUAL "")
  message("CTEST_BUILD_DIR is ${CTEST_BUILD_DIR}")
else()
  message(FATAL_ERROR "You need to set the CTEST_BUILD_DIR variable. CMake will exit." )
endif()

if("${NP}" STREQUAL "")
  set(NP 1)
endif()

message(STATUS "\nNumber of processes to use: ${NP}")

set(CTEST_SITE "${HOST_NAME}")
set(CTEST_BUILD_NAME "${CMAKE_SYSTEM_NAME}${EXTRA_BUILD_NAME}")
find_program(CTEST_GIT_COMMAND NAMES git)
find_program(MAKE NAMES make)

set(CTEST_UPDATE_COMMAND "${CTEST_GIT_COMMAND}")
set(CTEST_CONFIGURE_COMMAND "cmake ${CMAKE_CONFIGURE_ARGS} ${CTEST_SOURCE_DIR}")
set(CTEST_BUILD_COMMAND "${MAKE} -j${NP}")

message("\n -- Start dashboard - ${CTEST_BUILD_NAME} --")
ctest_start("Nightly" TRACK "Nightly")

message("\n -- Update - ${CTEST_BUILD_NAME} --")
ctest_update(SOURCE "${CTEST_SOURCE_DIR}" RETURN_VALUE result)
message(" -- Update exit code = ${result} --")

if(result GREATER -1)
  message("\n -- Configure - ${CTEST_BUILD_NAME} --")
  ctest_configure(BUILD "${CTEST_BUILD_DIR}" RETURN_VALUE result)
  message(" -- Configure exit code = ${result} --")
  if(result EQUAL 0)
    message("\n -- Build - ${CTEST_BUILD_NAME} --")
    ctest_read_custom_files("${CTEST_BUILD_DIR}")
    ctest_build(BUILD "${CTEST_BUILD_DIR}" RETURN_VALUE result)
    message(" -- Build exit code = ${result} --")
    if(result EQUAL 0)
      # Need to have TMPDIR set to disk on certain NREL machines for building so builds
      # do not run out of space but unset when running to stop OpenMPI from complaining
      if(UNSET_TMPDIR_VAR)
        message("Clearing TMPDIR variable...")
        unset(ENV{TMPDIR})
      endif()
      message("\n -- Test - ${CTEST_BUILD_NAME} --")
      ctest_test(BUILD "${CTEST_BUILD_DIR}"
                 PARALLEL_LEVEL ${NP}
                 RETURN_VALUE result)
      message(" -- Test exit code = ${result} --")
    endif()
  endif()
endif()

message("\n -- Submit - ${CTEST_BUILD_NAME} --")
#set(CTEST_NOTES_FILES "${TEST_LOG}")
#if(HAVE_STATIC_ANALYSIS_OUTPUT)
#  set(CTEST_NOTES_FILES ${CTEST_NOTES_FILES} "${STATIC_ANALYSIS_LOG}")
#endif()
ctest_submit(RETRY_COUNT 20
             RETRY_DELAY 20
             RETURN_VALUE result)
message(" -- Submit exit code = ${result} --")

message("\n -- Finished - ${CTEST_BUILD_NAME} --")
