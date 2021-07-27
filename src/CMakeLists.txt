target_sources(${EXAWIND_LIB_NAME} PRIVATE
  AMRTiogaIface.cpp
  AMRTiogaIface.h
  AMRWind.cpp
  AMRWind.h
  ExawindSolver.h
  MPIUtilities.h
  NaluWind.cpp
  NaluWind.h
  OversetSimulation.cpp
  OversetSimulation.h
  ParallelPrinter.h
  MemUsage.cpp)

target_include_directories(${EXAWIND_LIB_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${EXAWIND_LIB_NAME} PUBLIC
  AMR-Wind::amrwind_api
  Nalu-Wind::nalu
  std::filesystem)
