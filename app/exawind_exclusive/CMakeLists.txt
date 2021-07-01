target_sources(${EXAWIND_EXE_NAME}_exclusive PRIVATE
  exawind.cpp)

target_link_libraries(${EXAWIND_EXE_NAME}_exclusive PUBLIC
  ${EXAWIND_LIB_NAME}
  yaml-cpp)

target_include_directories(${EXAWIND_EXE_NAME}_exclusive SYSTEM PUBLIC ${YAML_CPP_INCLUDE_DIR})
