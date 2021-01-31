target_sources(nalu_amr_wind PRIVATE
  nalu_amr.cpp)

target_link_libraries(nalu_amr_wind PUBLIC
  exwsim
  yaml-cpp)
target_include_directories(nalu_amr_wind SYSTEM PUBLIC ${YAML_CPP_INCLUDE_DIR})
