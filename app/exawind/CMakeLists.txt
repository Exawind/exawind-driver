target_sources(${EXAWIND_EXE_NAME} PRIVATE
  exawind.cpp)

target_link_libraries(${EXAWIND_EXE_NAME} PUBLIC
  ${EXAWIND_LIB_NAME}
  yaml-cpp)

target_include_directories(${EXAWIND_EXE_NAME} SYSTEM PUBLIC ${YAML_CPP_INCLUDE_DIR})
