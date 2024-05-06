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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# Add your source files
set(SOURCES 
    src/generator.hpp
    src/large_unsigned_integer.hpp
    src/large_unsigned_integer.cpp
    src/main.cpp
    src/spsc_queue.hpp
    src/square_root.hpp
    src/square_root.cpp
    src/utility.hpp
    src/test/generator_test.cpp
    src/test/large_unsigned_integer_test.cpp
    src/test/spsc_queue_test.cpp
    src/test/square_root_test.cpp
)

# Create an executable from the source files
add_executable(${EXECUTABLE_NAME} ${SOURCES})

# Specify the location of the vcpkg-installed Catch2 library
find_package(Catch2 CONFIG REQUIRED)

# Link Catch2 to your executable
target_link_libraries(${EXECUTABLE_NAME} PRIVATE Catch2::Catch2)
