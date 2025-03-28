cmake_minimum_required(VERSION 3.30)
project(booking_system_server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Directories
set(SERVER_DIR "${CMAKE_SOURCE_DIR}/../server")
set(SRC_DIR "${SERVER_DIR}/Src")
set(INC_DIR "${SERVER_DIR}/Inc")
set(TEST_DIR "${CMAKE_SOURCE_DIR}/../tests")

# Boost configuration using find_package for cross-platform support
find_package(Boost 1.87 REQUIRED COMPONENTS system)
if(Boost_FOUND)
    message(STATUS "Found Boost: ${Boost_VERSION} at ${Boost_INCLUDE_DIRS}")
    include_directories(${Boost_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Boost not found. Please install Boost.")
endif()

include_directories(${INC_DIR})

# Collect source and test files
file(GLOB_RECURSE SRC_FILES "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE TEST_FILES "${TEST_DIR}/*.cpp")

# Create Static Library for Shared Code
add_library(booking_system_lib STATIC ${SRC_FILES})
if(WIN32)
    target_link_libraries(booking_system_lib Boost::system ws2_32)
else()
    target_link_libraries(booking_system_lib Boost::system)
endif()

# Main Server Executable (Linked to the Library)
add_executable(booking_system_server ${SRC_FILES})
target_link_libraries(booking_system_server booking_system_lib)

# Test Executable (Linked to the Same Library)
add_executable(server_test ${TEST_FILES})
target_link_libraries(server_test booking_system_lib)

# Enable Testing
enable_testing()
add_test(NAME TestServer COMMAND server_test)

# Clang-format Custom Target (Including Test Folder) , (need install Clang in your device to use it)
find_program(CLANG_FORMAT NAMES clang-format)
if (CLANG_FORMAT)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -style=file -i ${SRC_DIR}/*.cpp ${INC_DIR}/*.h ${TEST_DIR}/*.cpp
        COMMENT "Formatting source code with clang-format (including tests)"
    )
    add_dependencies(booking_system_server format)
else()
    message(WARNING "clang-format not found! Skipping code formatting.")
endif()