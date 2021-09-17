target_sources(${EXAWIND_EXE_NAME} PRIVATE
  exawind.cpp)

target_link_libraries(${EXAWIND_EXE_NAME} PRIVATE
  ${EXAWIND_LIB_NAME}
  yaml-cpp)

target_include_directories(${EXAWIND_EXE_NAME} SYSTEM PRIVATE ${YAML_CPP_INCLUDE_DIR})
