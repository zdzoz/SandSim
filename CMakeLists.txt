cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
project(SandSim LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if (APPLE)
    enable_language(OBJC)
endif()

add_subdirectory(thirdparty)

add_compile_options(-Wall -Werror -Wextra -Wpedantic)
add_compile_options(-fsanitize=address,undefined)
add_link_options(-fsanitize=address,undefined)

add_subdirectory(src)

# add_subdirectory(tests)

# copy shaders dir
# add_custom_command(
#     OUTPUT ${SHADER_OUTPUT_DIR}
#     COMMAND ${CMAKE_COMMAND} -E copy_directory ${SHADER_DIR} ${SHADER_OUTPUT_DIR}
#     DEPENDS ${SHADER_DIR}
#     COMMENT "Copying shaders to bin directory"
# )
#
# add_custom_target(copy_resources ALL DEPENDS ${SHADER_OUTPUT_DIR})
# add_dependencies(${PROJECT_NAME} copy_resources)
