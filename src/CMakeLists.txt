target_sources(${EXAWIND_LIB_NAME} PRIVATE
  AMRTiogaIface.cpp
  AMRTiogaIface.h
  AMRWind.cpp
  AMRWind.h
  ExawindSolver.h
  ExawindSolver.cpp
  MPIUtilities.h
  NaluWind.cpp
  NaluWind.h
  OversetSimulation.cpp
  OversetSimulation.h
  ParallelPrinter.h
  MemoryUsage.h
  MemoryUsage.cpp)

target_include_directories(${EXAWIND_LIB_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${EXAWIND_LIB_NAME} PUBLIC
  AMR-Wind::amrwind_api
  AMR-Wind::buildInfoamrwind_obj
  AMReX::amrex
  Nalu-Wind::nalu)

if(AMReX_ASCENT)
  target_link_libraries(${EXAWIND_LIB_NAME} PUBLIC OpenMP::OpenMP_CXX)
endif()
