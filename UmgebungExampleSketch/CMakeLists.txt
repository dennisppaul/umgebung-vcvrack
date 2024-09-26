cmake_minimum_required(VERSION 3.15)
project(UmgebungSketch)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify the source file for the library
set(SOURCES
    UmgebungExampleSketch.cpp
)

# Set the output properties for the dynamic library
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

# Create the dynamic library
add_library(UmgebungSketch SHARED ${SOURCES})

# Set properties specific to macOS for the dynamic library
set_target_properties(UmgebungSketch PROPERTIES
    OUTPUT_NAME "UmgebungSketch"
    SUFFIX ".dylib"
)

# Optionally, set compiler flags for debugging
target_compile_definitions(UmgebungSketch PRIVATE KLST_VCV_DEBUG)