cmake_minimum_required(VERSION 3.5)

project(Jackal)

set(CMAKE_CXX_STANDARD 11)

include_directories(src/includes)
include_directories(Dependency/includes)

IF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    link_directories(${PROJECT_SOURCE_DIR}/external/libs)
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(SDL2 REQUIRED)
    if (SDL2_FOUND)
        message ("SDL2 found")
    else ()
        message (FATAL_ERROR "Cannot find SDL2")
    endif ()
ENDIF ()

file(GLOB_RECURSE SRCS ./src/*.cpp)
file(GLOB_RECURSE HEADERS ./src/includes/*.h)
source_group("Header Files" FILES ${HEADERS})

add_library(${PROJECT_NAME} ${SRCS} ${HEADERS}
        main.cpp
        src/includes/JShadingState.h
        src/includes/JTexture2D.h
        src/JTexture2D.cpp
        src/JTextureContainer.cpp
        src/includes/JShadingPipeline.h
        src/JShadingPipeline.cpp
        src/includes/JDrawableMesh.h
        src/JDrawableMesh.cpp
        src/includes/JRenderer.h
        src/JRenderer.cpp
        src/includes/JShaderProgram.h
        src/JShaderProgram.cpp
        src/includes/JSceneParser.h
        src/JSceneParser.cpp
        src/includes/JWindowsApp.h
        src/JWindowsApp.cpp)
add_library(Jackal::renderer ALIAS ${PROJECT_NAME})

target_link_libraries( ${PROJECT_NAME}
    PUBLIC
        SDL2
        SDL2main
        tbb
        tbb12
        assimp
)