cmake_minimum_required(VERSION 3.14.0)

project(exwsim CXX C Fortran)

set(CMAKE_CXX_EXTENSIONS OFF)

find_package(MPI REQUIRED)
find_package(Trilinos REQUIRED)
find_package(TIOGA REQUIRED)
find_package(Nalu-Wind REQUIRED)
find_package(AMR-Wind REQUIRED)
if (NALU_USES_OPENFAST)
  find_package(OpenFAST REQUIRED)
endif()
find_package(YAML-CPP 0.6.2 REQUIRED)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

add_library(exwsim OBJECT)
add_subdirectory(src)

add_executable(nalu_amr_wind)
add_subdirectory(app/nalu_amr)
