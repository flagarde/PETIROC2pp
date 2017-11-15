########################################################
# cmake file for building Marlino
# @author Lagarde François, IPNL
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
########################################################

# project name
project(Petiroc2)

# which compilers to use for C and C++
#set(CMAKE_C_FLAGS -m32)
#set(CMAKE_CXX_FLAGS -m32)

# project version
SET( Petiroc2_VERSION_MAJOR 0 )
SET( Petiroc2_VERSION_MINOR 1 )
SET( Petiroc2_VERSION_PATCH 0 )

include(CheckIncludeFiles)
include(CheckCXXCompilerFlag)
set (CMAKE_CXX_STANDARD 14)


get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)

if (${LIB64} STREQUAL "TRUE")
    set(LIBSUFFIX 64)
else()
    set(LIBSUFFIX 64)
endif()


set(PETIROCLIB_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/Modules/lib${LIBSUFFIX} CACHE PATH "Installation directory for libraries") 
find_library( LIB_I_NEED libftd2xx.so.0 HINTS ${PETIROCLIB_PATH})

if(${LIB_I_NEED} STREQUAL "LIB_I_NEED-NOTFOUND")
    message(FATAL_ERROR "Couldn't find the libftd2xx.so.0 library at ${PETIROCLIB_PATH}")
endif()

message(STATUS "Found libftd2xx.so.0 at: ${LIB_I_NEED}")
set(PROJECT_INCLUDE_DIR2 "${CMAKE_CURRENT_SOURCE_DIR}/Compiled/include")
set(LIBRARY_OUTPUT_PATH2 "${CMAKE_CURRENT_SOURCE_DIR}/Compiled/lib")
set(EXECUTABLE_OUTPUT_PATH2 "${CMAKE_CURRENT_SOURCE_DIR}/Compiled/bin")
set(DOCUMENTATION_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Compiled/doc")
set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(BUILD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/LALUsb/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/Petiroc/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/rang/include/)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -ansi -fno-strict-aliasing -lpthread -ldl -lm -fPIC" )

SET(CMAKE_INSTALL_RPATH ${LIBRARY_OUTPUT_PATH2})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Modules/)
list( APPEND CMAKE_MODULE_PATH $ENV{ROOTSYS}/etc/cmake/)
find_package(ROOT REQUIRED)
include_directories(${ROOT_INCLUDE_DIR})
configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/Modules/Version.h.in" "${PROJECT_INCLUDE_DIR2}/Version.h")
ADD_SUBDIRECTORY( src/LALUsb)
ADD_SUBDIRECTORY( src/Petiroc)
#ADD_SUBDIRECTORY(src/doc)
#ADD_SUBDIRECTORY( src/fmt )
#find_library( LIB_I_NEED libftd2xx.so HINTS ${PETIROCLIB_PATH})
#find_library( LIB_I_NEED2 libPetiroc.so HINTS ${LIBRARY_OUTPUT_PATH2})
ADD_EXECUTABLE( Main ./main.cpp)
TARGET_LINK_LIBRARIES( Main ${LIB_I_NEED} Petiroc )
INSTALL(TARGETS Main DESTINATION ${EXECUTABLE_OUTPUT_PATH2} )  

add_custom_command(TARGET Main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PETIROCLIB_PATH}
        ${LIBRARY_OUTPUT_PATH2})
#ADD_SUBDIRECTORY(src/doc)
