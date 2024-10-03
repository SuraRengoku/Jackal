cmake_minimum_required(VERSION 3.5)
project(display)

set(CMAKE_CXX_STANDARD 11)

include_directories(../includes)
include_directories(../Dependency/includes)

set(publish_bin_debug   ${CMAKE_BINARY_DIR}/$<$<CONFIG:Debug>:Debug>>)
set(publish_bin_release ${CMAKE_BINARY_DIR}/$<$<CONFIG:Release>:Release>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG    ${publish_bin_debug})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE  ${publish_bin_release})

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
    Jackal::renderer
)