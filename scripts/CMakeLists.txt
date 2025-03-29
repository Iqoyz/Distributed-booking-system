cmake_minimum_required(VERSION 3.16)
project(booking_system_server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Directories
set(SERVER_DIR "${CMAKE_SOURCE_DIR}/../server")
set(SRC_DIR "${SERVER_DIR}/Src")
set(INC_DIR "${SERVER_DIR}/Inc")
set(TEST_DIR "${CMAKE_SOURCE_DIR}/../tests")

include_directories(${INC_DIR})

# --- Boost Configuration ---
if(WIN32)
    # Your custom Boost path on Windows
    set(BOOST_PATH "C:/Users/user/Documents/C++/boost_1_87_0")
    set(BOOST_INCLUDEDIR "${BOOST_PATH}")
    set(BOOST_LIBRARYDIR "${BOOST_PATH}/stage/lib")
    include_directories(${BOOST_INCLUDEDIR})
    link_directories(${BOOST_LIBRARYDIR})
    set(BOOST_LIB_NAME boost_system-mgw12-mt-x64-1_87)  # Adjust to your Boost lib name
else()
    # Use system-installed Boost on macOS/Linux
    find_package(Boost 1.70 REQUIRED COMPONENTS system)
    if(Boost_FOUND)
        include_directories(${Boost_INCLUDE_DIRS})
        set(BOOST_LIB_NAME Boost::system)
    else()
        message(FATAL_ERROR "Boost not found. Please install Boost.")
    endif()
endif()

# Collect source and test files
file(GLOB_RECURSE SRC_FILES "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE TEST_FILES "${TEST_DIR}/*.cpp")

# --- Static Library ---
add_library(booking_system_lib STATIC ${SRC_FILES})
if(WIN32)
    target_link_libraries(booking_system_lib ${BOOST_LIB_NAME} ws2_32)
else()
    target_link_libraries(booking_system_lib ${BOOST_LIB_NAME})
endif()

# --- Server Executable ---
add_executable(booking_system_server ${SRC_FILES})
target_link_libraries(booking_system_server booking_system_lib)

# --- Test Executable ---
add_executable(server_test ${TEST_FILES})
target_link_libraries(server_test booking_system_lib)

# Enable Testing
enable_testing()
add_test(NAME TestServer COMMAND server_test)

# Optional: clang-format (only runs if found)
find_program(CLANG_FORMAT NAMES clang-format)
if (CLANG_FORMAT)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -style=file -i ${SRC_DIR}/*.cpp ${INC_DIR}/*.h ${TEST_DIR}/*.cpp
        COMMENT "Formatting source code with clang-format"
    )
else()
    message(WARNING "clang-format not found, skipping formatting step.")
endif()
