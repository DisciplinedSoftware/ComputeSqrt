cmake_minimum_required(VERSION 3.20)

# Set the variable for the executable name
set(EXECUTABLE_NAME ComputeSqrtOf42)

project(${EXECUTABLE_NAME} VERSION 1.0)

# Include vcpkg toolchain file
include("$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

# Set C++ standard to the latest available
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add your source files
set(SOURCES 
    src/main.cpp
    src/large_integer.hpp
    src/large_integer.cpp
    src/square_root.hpp
    src/square_root.cpp
    src/test/square_root_test.cpp
    src/test/large_integer_test.cpp
)

# Create an executable from the source files
add_executable(${EXECUTABLE_NAME} ${SOURCES})

# Specify the location of the vcpkg-installed Catch2 library
find_package(Catch2 CONFIG REQUIRED)

# Link Catch2 to your executable
target_link_libraries(${EXECUTABLE_NAME} PRIVATE Catch2::Catch2)
