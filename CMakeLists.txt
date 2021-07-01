cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)

project(exawind CXX C Fortran)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(EXAWIND_TEST_WITH_FCOMPARE "Check AMR-Wind test plots against gold files" OFF)
option(EXAWIND_SAVE_GOLDS "Provide a directory in which to save golds during testing" OFF)

find_package(MPI REQUIRED)
find_package(Trilinos REQUIRED)
find_package(TIOGA REQUIRED)
find_package(Nalu-Wind REQUIRED)
find_package(AMR-Wind REQUIRED)
find_package(YAML-CPP 0.6.2 REQUIRED)
find_package(Filesystem REQUIRED)
if (NALU_USES_OPENFAST)
  find_package(OpenFAST REQUIRED)
endif()

# General information about machine, compiler, and build type
message(STATUS "Exawind information:")
message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER_VERSION = ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

set(EXAWIND_LIB_NAME "exwsim")
add_library(${EXAWIND_LIB_NAME} OBJECT)
add_subdirectory(src)

set(EXAWIND_EXE_NAME "exawind")
add_executable(${EXAWIND_EXE_NAME})
add_subdirectory(app/exawind)
add_executable(${EXAWIND_EXE_NAME}_exclusive)
add_subdirectory(app/exawind_exclusive)

add_subdirectory(test)

install(TARGETS ${EXAWIND_EXE_NAME})
install(TARGETS ${EXAWIND_EXE_NAME}_exclusive)
