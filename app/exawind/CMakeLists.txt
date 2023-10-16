target_sources(${EXAWIND_EXE_NAME} PRIVATE
  exawind.cpp)

target_link_libraries(${EXAWIND_EXE_NAME} PRIVATE
  ${EXAWIND_LIB_NAME}
  yaml-cpp)

target_include_directories(${EXAWIND_EXE_NAME} SYSTEM PRIVATE ${YAML_CPP_INCLUDE_DIR})

if(EXAWIND_ENABLE_ROCM)
  target_compile_definitions(${EXAWIND_EXE_NAME} PRIVATE EXAWIND_ENABLE_ROCM)
endif()

if(CXX_FILESYSTEM_HAVE_FS)
  target_compile_definitions(${EXAWIND_EXE_NAME} PRIVATE EXAWIND_HAS_STD_FILESYSTEM)
  target_link_libraries(${EXAWIND_EXE_NAME} PRIVATE
    std::filesystem)
endif()
