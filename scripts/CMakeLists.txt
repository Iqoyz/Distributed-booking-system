cmake_minimum_required(VERSION 3.30)
project(booking_system_server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SERVER_DIR "${CMAKE_SOURCE_DIR}/../server")
set(SRC_DIR "${SERVER_DIR}/Src")
set(INC_DIR "${SERVER_DIR}/Inc")
set(TEST_DIR "${CMAKE_SOURCE_DIR}/../tests")

set(BOOST_PATH "C:/Users/user/Documents/C++/boost_1_87_0") #change it to your own folder

# Boost Configuration
set(BOOST_ROOT "${BOOST_PATH}")
set(BOOST_INCLUDEDIR "${BOOST_ROOT}")
set(BOOST_LIBRARYDIR "${BOOST_ROOT}/stage/lib")

include_directories(${INC_DIR} ${BOOST_INCLUDEDIR})
link_directories(${BOOST_LIBRARYDIR})

# Collect source and test files
file(GLOB_RECURSE SRC_FILES "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE TEST_FILES "${TEST_DIR}/*.cpp")

# Step 1: Create Static Library for Shared Code
add_library(booking_system_lib STATIC ${SRC_FILES})
target_link_libraries(booking_system_lib boost_system-mgw12-mt-x64-1_87 ws2_32)

# Step 2: Main Server Executable (Linked to the Library)
add_executable(booking_system_server ${SRC_FILES})
target_link_libraries(booking_system_server booking_system_lib)

# Step 3: Test Executable (Linked to the Same Library)
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
