target_sources(exwsim PRIVATE
  AMRTiogaIface.cpp
  AMRWind.cpp
  NaluWind.cpp)

target_include_directories(exwsim PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(exwsim PUBLIC
  AMR-Wind::amrwind_api
  Nalu-Wind::nalu
  std::filesystem)
