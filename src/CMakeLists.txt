add_executable(${PROJECT_NAME}
  main.c
)

target_link_libraries(${PROJECT_NAME} PRIVATE sokol cimgui cglm)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/shaders)
target_compile_definitions(${PROJECT_NAME} PRIVATE CIMGUI_DEFINE_ENUMS_AND_STRUCTS)

if (APPLE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE SOKOL_METAL)
endif()
