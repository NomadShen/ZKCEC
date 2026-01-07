cmake_minimum_required(VERSION 3.10)

project(ZK-CEC)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_INSTALL_RPATH "/usr/local/lib")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_CXX_FLAGS "-march=native -flto -O3 -pthread -w ")

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

find_package(emp-tool REQUIRED)
include_directories(${EMP-TOOL_INCLUDE_DIRS})
find_package(emp-ot REQUIRED)
include_directories(${EMP-OT_INCLUDE_DIRS})
find_package(emp-zk REQUIRED)
include_directories(${EMP-ZK_INCLUDE_DIRS})

find_package(OpenSSL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

set(NTL_DIR $(brew --prefix ntl)) # replace $(brew --prefix ntl) with the path to NTL
find_path(NTL_INCLUDE_DIR NAMES NTL/ZZ.h HINTS $(NTL_DIR)/include)
find_library(NTL_LIBRARY NAMES ntl HINTS $(NTL_DIR)/lib)

if(NTL_INCLUDE_DIR AND NTL_LIBRARY)
    add_library(NTL::ntl UNKNOWN IMPORTED)
    set_target_properties(NTL::ntl PROPERTIES
        IMPORTED_LOCATION "${NTL_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${NTL_INCLUDE_DIR}")
    message(STATUS "Manually created NTL::ntl target.")
else()
    message(FATAL_ERROR "Could not find NTL library via manual search.")
endif()

add_executable(test main.cpp polynomial.cpp clause.cpp clauseRAM.cpp clauseCheck.hpp utils.cpp)
target_link_libraries(test PUBLIC emp-tool emp-zk OpenSSL::SSL OpenSSL::Crypto NTL::ntl sodium gmp)
