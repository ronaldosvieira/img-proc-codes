cmake_minimum_required(VERSION 2.8)
project(ImgProc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(PNG REQUIRED)
find_package(GLUI REQUIRED)

# GLUI can be founded at
# - http://packages.ubuntu.com/precise/amd64/libglui2c2/download
# - http://packages.ubuntu.com/precise/amd64/libglui-dev/download

foreach(i RANGE 1 8)

	file(GLOB_RECURSE TRABALHO_${i}
	    "trabalho$${i}/*.hpp"
	    "trabalho${i}/*.h"
	    "trabalho${i}/*.cpp"
	    "trabalho${i}/*.c"
	)

	add_library(trabalho${i}_lib STATIC "${CMAKE_SOURCE_DIR}/libs/pixellab/src/pixelLab.cpp")

	add_executable(trabalho${i} ${TRABALHO_${i}})

	set_property(TARGET trabalho${i} APPEND PROPERTY INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/trabalho${i}/include" "${CMAKE_SOURCE_DIR}/libs/pixellab/src")

	target_link_libraries(trabalho${i} trabalho${i}_lib ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${PNG_LIBRARIES} ${GLUI_LIBRARY})

endforeach(i)
