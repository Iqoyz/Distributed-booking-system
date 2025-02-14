cmake_minimum_required(VERSION 3.30)
project(udp_ws_server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/Src")
set(INC_DIR "${CMAKE_SOURCE_DIR}/Inc")

set(BOOST_PATH "C:/Users/user/Documents/C++/boost_1_87_0")

# Manually set Boost paths
set(BOOST_ROOT "${BOOST_PATH}")
set(BOOST_INCLUDEDIR "${BOOST_ROOT}")
set(BOOST_LIBRARYDIR "${BOOST_ROOT}/stage/lib")

include_directories(${INC_DIR} ${BOOST_INCLUDEDIR})


link_directories(${BOOST_LIBRARYDIR})

file(GLOB_RECURSE SRC_FILES "${SRC_DIR}/*.cpp")

add_executable(udp_ws_server ${SRC_FILES})

# Manually link Boost libraries
target_link_libraries(udp_ws_server boost_system-mgw12-mt-x64-1_87 ws2_32)


