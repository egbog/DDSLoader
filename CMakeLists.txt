cmake_minimum_required(VERSION 3.25)
project(dds LANGUAGES CXX)

add_library(dds INTERFACE)

target_include_directories(dds INTERFACE 
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(dds INTERFACE cxx_std_20)