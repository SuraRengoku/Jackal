cmake_minimum_required (VERSION 3.5)

project(example5_alpha_to_coverage)

# C++ 11 is required
set(CMAKE_CXX_STANDARD 11)

include_directories(../../includes)
include_directories(../../Dependency/includes)

# 指定可执行程序输出目录
set(publish_bin_debug 			${CMAKE_BINARY_DIR}/$<$<CONFIG:Debug>:Debug>)
set(publish_bin_release 		${CMAKE_BINARY_DIR}/$<$<CONFIG:Release>:Release>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG 	${publish_bin_debug})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE 	${publish_bin_release})

# Create the executable
add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
    Jackal::renderer
)

#file(GLOB_RECURSE DLLS ../../external/dlls/*.dll)

#add_custom_command(TARGET ${PROJECT_NAME} 
#   POST_BUILD 
#   COMMAND ${CMAKE_COMMAND} -E 
#       copy_if_different  
#        "${DLLS}"  
#        "${CMAKE_BINARY_DIR}/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/"
#)
