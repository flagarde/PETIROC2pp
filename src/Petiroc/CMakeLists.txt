cmake_minimum_required(VERSION 2.8)

project(Petiroc)


find_library( LIB_I_NEED libftd2xx.so.0 HINTS ${PETIROCLIB_PATH})

if(${LIB_I_NEED} STREQUAL "LIB_I_NEED-NOTFOUND")
    message(FATAL_ERROR "Couldn't find the libftd2xx.so.0 library at ${PETIROCLIB_PATH}")
endif()

message(STATUS "Found libftd2xx.so.0 at: ${LIB_I_NEED}")

# Find source files
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# Include header files
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library (Petiroc STATIC ${SOURCES})
target_link_libraries (Petiroc LALUsb)

# Install library
install(TARGETS Petiroc DESTINATION ${LIBRARY_OUTPUT_PATH2})

# Install library headers
file(GLOB HEADERS include/*.h)
install(FILES ${HEADERS} DESTINATION ${PROJECT_INCLUDE_DIR2})
