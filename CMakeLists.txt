cmake_minimum_required(VERSION 3.5)

project(Jackal)

set(CMAKE_CXX_STANDARD 11)

include_directories(${PROJECT_SOURCE_DIR}/dependency/includes)
include_directories(includes)

IF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    link_directories(${PROJECT_SOURCE_DIR}/dependency/libs)
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(SDL2 REQUIRED)
    if (SDL2_FOUND)
        message ("SDL2 found")
    else ()
        message (FATAL_ERROR "Cannot find SDL2")
    endif ()
ENDIF ()

file(GLOB_RECURSE SRCS src/*.cpp)
file(GLOB_RECURSE HEADERS includes/*.h)
source_group("Header Files" FILES ${HEADERS})

add_library(${PROJECT_NAME} ${SRCS} ${HEADERS})
add_library(Jackal::renderer ALIAS ${PROJECT_NAME})

target_link_libraries( ${PROJECT_NAME}
    PUBLIC
        SDL2
        SDL2main
        tbb
        tbb12
        assimp
)

#add_subdirectory(display/directional_lighting)
add_subdirectory(examples/example1_point_lighting)
add_subdirectory(examples/example2_spot_lighting)
add_subdirectory(examples/example3_directional_lighting)
add_subdirectory(examples/example4_alpha_blending)
add_subdirectory(examples/example5_alpha_to_coverage)
add_subdirectory(examples/example6_diablo3_pose)
add_subdirectory(examples/example7_normal_mapping)
add_subdirectory(examples/example8_complicated_scene)