cmake_minimum_required(VERSION 3.25)
project(dds LANGUAGES CXX)

add_library(dds ${CMAKE_CURRENT_SOURCE_DIR}/src/dds/DDSLoader.cpp)

target_include_directories(dds PUBLIC 
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(dds PUBLIC cxx_std_20)