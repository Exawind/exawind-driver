#=============================================================================
# Exawind Testing
#=============================================================================

# Check if mesh submodule is populated
file(GLOB RESULT ${CMAKE_SOURCE_DIR}/test/meshes/*)
list(LENGTH RESULT RES_LEN)
if(RES_LEN EQUAL 0)
  message(WARNING
"Testing is enabled, but the mesh submodule is not initialized.\
 You should 'git submodule init' and 'git submodule update'\
 before running the tests.")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.cmake ${CMAKE_BINARY_DIR}/CTestCustom.cmake)

#=============================================================================
# Functions for adding tests / Categories of tests
#=============================================================================

macro(setup_test)
    set(CURRENT_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test_files/${TEST_NAME})
    set(CURRENT_TEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/test_files/${TEST_NAME})
    set(PLOT_GOLD ${GOLD_FILES_DIRECTORY}/${TEST_NAME}/plt00010)
    set(PLOT_TEST ${CURRENT_TEST_BINARY_DIR}/plt00010)
    file(MAKE_DIRECTORY ${CURRENT_TEST_BINARY_DIR})
    file(GLOB TEST_FILES "${CURRENT_TEST_SOURCE_DIR}/*")
    file(COPY ${TEST_FILES} DESTINATION "${CURRENT_TEST_BINARY_DIR}/")
    set(TEST_NP 2)
    set(MPI_COMMANDS "${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${TEST_NP} ${MPIEXEC_PREFLAGS}")
    if(EXAWIND_SAVE_GOLDS)
      file(MAKE_DIRECTORY ${SAVED_GOLDS_DIRECTORY}/${TEST_NAME})
      set(SAVE_GOLDS_COMMAND "&& cp -R ${PLOT_TEST} ${SAVED_GOLDS_DIRECTORY}/${TEST_NAME}/")
    endif()
    if(EXAWIND_TEST_WITH_FCOMPARE)
      set(FCOMPARE_COMMAND "&& ${MPI_COMMANDS} ${FCOMPARE_EXE} ${FCOMPARE_TOLERANCE} ${PLOT_GOLD} ${PLOT_TEST}")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
                    ${CMAKE_CURRENT_SOURCE_DIR}/meshes
                    ${CURRENT_TEST_BINARY_DIR}/meshes)
endmacro(setup_test)

# Standard regression test
function(add_test_r TEST_NAME)
    setup_test()
    add_test(${TEST_NAME} sh -c "${MPI_COMMANDS} ${CMAKE_BINARY_DIR}/${EXAWIND_EXE_NAME} ${MPIEXEC_POSTFLAGS} ${CURRENT_TEST_BINARY_DIR}/${TEST_NAME}.yaml > ${TEST_NAME}.log ${SAVE_GOLDS_COMMAND} ${FCOMPARE_COMMAND}")
    # Set properties for test
    set_tests_properties(${TEST_NAME} PROPERTIES
                         TIMEOUT 5400
                         PROCESSORS ${TEST_NP}
                         WORKING_DIRECTORY "${CURRENT_TEST_BINARY_DIR}/"
                         LABELS "regression"
                         ATTACHED_FILES_ON_FAIL "${CURRENT_TEST_BINARY_DIR}/${TEST_NAME}.log")
endfunction(add_test_r)

#=============================================================================
# Regression tests
#=============================================================================
add_test_r(sphere)
add_test_r(nalu-nalu-cylinder)
add_test_r(amr-nalu-cylinder)
add_test_r(nalu-nalu-cylinder-motion)
add_test_r(amr-nalu-cylinder-motion)
add_test_r(zalesak)
add_test_r(hybrid-multi-cylinder)

