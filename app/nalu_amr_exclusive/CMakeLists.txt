target_sources(nalu_amr_wind_exclusive PRIVATE
  nalu_amr.cpp)

target_link_libraries(nalu_amr_wind_exclusive PUBLIC
  exwsim
  yaml-cpp)
target_include_directories(nalu_amr_wind_exclusive SYSTEM PUBLIC ${YAML_CPP_INCLUDE_DIR})
