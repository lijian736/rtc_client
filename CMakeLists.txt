#cmake min version
cmake_minimum_required (VERSION 2.8.12)

#project name
project(rtc_client)

option (SYS_BIG_ENDIAN "The system compile byte-order option" OFF)
if (SYS_BIG_ENDIAN)
    add_definitions(-DSYS_BIG_ENDIAN)
endif(SYS_BIG_ENDIAN)

# c++11 thread support
set(CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

#add subdirectory
add_subdirectory(src)

#output dir
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)